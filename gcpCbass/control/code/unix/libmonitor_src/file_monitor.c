#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include "monitor.h"
#include "lprintf.h"
#include "genericregs.h"
#include "arraytemplate.h"
#include "monitor_stream.h"
#include "netbuf.h"
#include "pathname.h"
#include "archive.h"
#include "arcfile.h"
#include "const.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "gcp/util/common/DirList.h"

using namespace gcp::control;

static int monitor_file_cmp(const void *va, const void *vb);

/*
 * Set the initial size and allocation increment for the array of
 * files.
 */
#define NUM_MONITOR_FILE 100

/**.......................................................................
 * Define a struct that will encapsulate a single monitor file
 */
struct MonitorFile {
  ArcTimeStamp ts_;
  std::string name_;
};

/**.......................................................................
 * A function for passing to std::sort() to sort a list of MonitorFile
 * objects
 */
static bool compMonitorFile(MonitorFile f1, MonitorFile f2) 
{
  if(f1.ts_.mjd < f2.ts_.mjd)
    return true;
  else if(f1.ts_.mjd > f2.ts_.mjd)
    return false;
  else if(f1.ts_.sec < f2.ts_.sec)
    return true;
  else if(f1.ts_.sec > f2.ts_.sec)
    return false;
}

struct FileMonitor {
  char *dir;          /* The directory that contains the files */
  unsigned dirNum;    /* Sequential number of next archive file directory */
  char *path;         /* A path name construction buffer initialized with */
                      /*  the directory name + directory separator */
  char *name;         /* A pointer to the name part of path[], following */
                      /*  the directory directory separator */
  
  std::vector<MonitorFile>* files;

  int nfiles;         /* The number of files in files[] */
  int first;          /* The index of the first file in files[] that */
                      /*  contains data between ta and tb. */
  int next;           /* The index of the next file in files[] */
  double ta, tb;      /* The range of UTC MJDs for which to retrieve data */
  NetReadStr *nrs;    /* The file input stream */
  int nrs_size;       /* The size argument with which nrs was created */
  int fd;             /* The file-descriptor of the current archive file */
  ArrayMap *arraymap; /* The register map of the control program */
  RegMap *regmap;     /* The register map of the control program */
  struct {
    int utc;          /* The frame offset of the 2-element register that */
                      /*  records the UTC MJD in days and milliseconds */
  } reg;
  int range_ended;    /* True once the available data has been exhausted */
  int file_exhausted; /* True between hitting the end of one archive file */
                      /*  and opening the one that follows it. */
  int rewound;        /* True if nothing has been read since the stream */
                      /*  was last opened or rewound. */
  int rewind_pending; /* True if the stream should be rewound on the next */
                      /*  call to fm_read_frame(). */
  bool singleFile;
};

static FileMonitor *new_FileMonitor(char *dir, double utc_a, double utc_b);
static FileMonitor *new_FileMonitor(char *dir);
static FileMonitor *del_FileMonitor(FileMonitor *fm);

static MS_DESTRUCTOR(fm_destructor);
static MS_READ_FRAME(fm_read_frame);
static MS_SELECT_FD(fm_select_fd);
static MS_QUEUE_REWIND(fm_queue_rewind);
static MS_ARRAYMAP(fm_arraymap);

static int open_FileMonitorStream(MonitorStream *ms, char *dir, double ta,
				  double tb);
static int open_FileMonitorStream(MonitorStream *ms, char* dir);

static int fm_get_file_list(FileMonitor *fm);

static int bad_file_list(DIR *dir, struct dirent *dent);

static MsReadState fm_read_msg(FileMonitor *fm);
static int fm_read_size(FileMonitor *fm);
static ArrayMap *fm_read_ArrayMap(FileMonitor *fm);

static MsReadState fm_stage_next_file(FileMonitor *fm);
static MsReadState fm_open_next_file(FileMonitor *fm);

static MsReadState fm_open_file(FileMonitor *fm);

static void fm_close_file(FileMonitor *fm);
static MsReadState fm_next_frame(FileMonitor *fm, RegRawData *raw);

/*.......................................................................
 * Connect a given MonitorStream iterator to a sequential list of
 * archive data files.
 *
 * Input:
 *  ms      MonitorStream *  The stream iterator to be connected.
 *  dir              char *  The directory in which to search for
 *                           archive files.
 *  ta, tb         double    The desired range of UTC's expressed
 *                           as Modified Julian Dates.
 *                           The earliest available time will be
 *                            substituted for ta if ta <= 0.
 *                           The latest available time will be
 *                            substituted for tb if tb <= 0.
 * Output:
 *  return            int    0 - OK.
 *                           1 - Error.
 */
static int open_FileMonitorStream(MonitorStream *ms, char *dir, double ta,
				  double tb)
{
  FileMonitor* fm=0;   /* The file context of the iterator */
  
  // Check the arguments.

  if(!ms) {
    lprintf(stderr, "open_FileMonitorStream: NULL argument(s).\n");
    return 1;
  };
  
  // Attempt to open the file stream.

  fm = new_FileMonitor(dir, ta, tb);

  if(!fm)
    return 1;

  fm->dirNum = 2;
  
  // Assign the channel to the generic monitor-stream iterator.

  return open_MonitorStream(ms, (void *) fm, fm_destructor,
			    fm_read_frame, 0, 0, 0, fm_queue_rewind,
			    fm_select_fd, fm_arraymap, true);
}

/*.......................................................................
 * Connect a given MonitorStream iterator to a named archive data file
 *
 * Input:
 *  ms      MonitorStream *  The stream iterator to be connected.
 *  dir              char *  The fully qualified file name
 *
 * Output:
 *  return            int    0 - OK.
 *                           1 - Error.
 */
static int open_FileMonitorStream(MonitorStream *ms, char *dir)
{
  FileMonitor *fm;   /* The file context of the iterator */
  
  // Check the arguments.

  if(!ms) {
    lprintf(stderr, "open_FileMonitorStream: NULL argument(s).\n");
    return 1;
  };
  
  // Attempt to open the file stream.

  fm = new_FileMonitor(dir);
  if(!fm)
    return 1;
  fm->dirNum = 2;
  
  // Assign the channel to the generic monitor-stream iterator.

  return open_MonitorStream(ms, (void *) fm, fm_destructor,
			    fm_read_frame, 0, 0, 0, fm_queue_rewind,
			    fm_select_fd, fm_arraymap, true);
}

/*.......................................................................
 * Return a new monitor stream wrapper around a list of archive data files.
 *
 * Input:
 *  dir              char *  The directory in which to search for
 *                           archive files.
 *  ta, tb         double    The desired range of UTC's expressed
 *                           as Modified Julian Dates.
 *                           The earliest available time will be
 *                            substituted for ta if ta <= 0.
 *                           The latest available time will be
 *                            substituted for tb if tb <= 0.
 * Output:
 *  return MonitorStream *  The connected monitor stream iterator, or
 *                          NULL on error.
 */
MonitorStream *new_FileMonitorStream(char *dir, double ta, double tb)
{
  MonitorStream* ms = new_MonitorStream(true);

  if(!ms || open_FileMonitorStream(ms, dir, ta, tb))
    return del_MonitorStream(ms);

  return ms;
}

/*.......................................................................
 * Return a new monitor stream wrapper around an archive data file
 *
 * Input:
 *  dir              char *  The archive file
 *
 * Output:
 *  return MonitorStream *  The connected monitor stream iterator, or
 *                          NULL on error.
 */
MonitorStream *new_FileMonitorStream(char *dir)
{
  MonitorStream *ms = new_MonitorStream(true);
  if(!ms || open_FileMonitorStream(ms, dir))
    return del_MonitorStream(ms);
  return ms;
}

/*.......................................................................
 * This is a wrapper around del_FileMonitor() which deletes a FileMonitor
 * object via the (void *) alias used by MonitorStream objects.
 *
 * Input:
 *  context          void *  The FileMonitor object to be deleted, cast
 *                           to (void *).
 * Output:
 *  return           void *  The deleted context (always NULL).
 */
static MS_DESTRUCTOR(fm_destructor)
{
  return (void *) del_FileMonitor((FileMonitor *)context);
}

/*.......................................................................
 * Create the context of a file monitor-stream iterator. This is
 * designed for use in retrieving archived data from data files.
 *
 * Input:
 *  dir            char *  The directory in which to search for
 *                         archive files.
 *  ta, tb       double    The desired range of UTC's expressed
 *                         as Modified Julian Dates.
 *                         The earliest available time will be
 *                          substituted for ta if ta <= 0.
 *                         The latest available time will be
 *                          substituted for tb if tb <= 0.
 * Output:
 *  return  FileMonitor *  The new file monitor object, or NULL on
 *                         error.
 */
static FileMonitor *new_FileMonitor(char *dir, double ta, double tb)
{
  FileMonitor *fm;   /* The object to be returned */
  size_t dir_len;    /* The length of the directory name */
  
  // Check the arguments.

  if(!dir) {
    lprintf(stderr, "new_FileMonitor: NULL directory argument.\n");
    return NULL;
  };
  
  // Swap the times if needed to make ta <= tb (unless either of the
  // numbers is -1).

  if(tb > 0 && tb < ta) {
    double tmp = ta;
    ta = tb;
    tb = tmp;
  };
  
  // Allocate the container.

  fm = (FileMonitor *) malloc(sizeof(FileMonitor));
  if(!fm) {
    lprintf(stderr, "new_FileMonitor: Insufficient memory.\n");
    return NULL;
  };
  
  // Initialize the container at least up to the point at which it is
  // safe to call del_FileMonitor().

  fm->dir = NULL;
  fm->path = NULL;
  fm->name = NULL;
  fm->files = NULL;
  fm->nfiles = 0;
  fm->first = 0;
  fm->next = 0;
  fm->ta = ta;
  fm->tb = tb;
  fm->nrs = NULL;
  fm->nrs_size = 0;
  fm->fd = -1;
  fm->arraymap = NULL;
  fm->regmap = NULL;
  fm->reg.utc = -1;
  fm->range_ended = 0;
  fm->file_exhausted = 0;
  fm->rewound = 1;
  fm->rewind_pending = 0;

  fm->singleFile = false;
  
  // Allocate a copy of the directory name, after expanding any ~ or
  // ~user/ prefixes and appending a '/' if necessary.

  fm->dir = new_pathname(dir, NULL);
  if(!fm->dir)
    return del_FileMonitor(fm);
  
  // Get the length of the expanded directory name.

  dir_len = strlen(fm->dir);
  
  // Allocate a string long enough to contain the directory name, an
  // archive file name and a string terminator, '\0'.

  fm->path = (char* )malloc(dir_len + arc_file_name_length() + 1);
  if(!fm->path) {
    lprintf(stderr, "Insufficient memory for archive file name.\n");
    return del_FileMonitor(fm);
  };
  
  // Copy the directory to the start of the path buffer.

  strcpy(fm->path, fm->dir);
  
  // Record the position in the path buffer at which file names should
  // be appended.

  fm->name = fm->path + dir_len;
  
  // Allocate a temporary file input stream to use solely to read the
  // required sizes of the final input and output streams.

  fm->nrs = new_NetReadStr(fm->fd, 100);
  if(!fm->nrs)
    return del_FileMonitor(fm);

  fm->files = new std::vector<MonitorFile>();
  if(!fm->files) {
    return del_FileMonitor(fm);
  }
    
  // Create a sorted list of archive data files from the specified
  // directory.

  if(fm_get_file_list(fm)) {
    return del_FileMonitor(fm);
  }
  
  // Open the first file that lies in the requested time range.  Note
  // that this also replaces fm->nrs with an input stream of an
  // appropriate size, and reads the file's register map into
  // fm->regmap.

  if(fm_open_next_file(fm) != MS_READ_REGMAP) {
    return del_FileMonitor(fm);
  }

  return fm;
}

/*.......................................................................
 * Create the context of a file monitor-stream iterator. This is
 * designed for use in retrieving archived data from data files.
 *
 * Input:
 *  dir            char *  The directory in which to search for
 *                         archive files.
 *  ta, tb       double    The desired range of UTC's expressed
 *                         as Modified Julian Dates.
 *                         The earliest available time will be
 *                          substituted for ta if ta <= 0.
 *                         The latest available time will be
 *                          substituted for tb if tb <= 0.
 * Output:
 *  return  FileMonitor *  The new file monitor object, or NULL on
 *                         error.
 */
static FileMonitor *new_FileMonitor(char *filename)
{
  FileMonitor *fm;   /* The object to be returned */
  size_t dir_len;    /* The length of the directory name */
/*
 * Check the arguments.
 */
  if(!filename) {
    lprintf(stderr, "new_FileMonitor: NULL directory argument.\n");
    return NULL;
  };

/*
 * Allocate the container.
 */
  fm = (FileMonitor *) malloc(sizeof(FileMonitor));
  if(!fm) {
    lprintf(stderr, "new_FileMonitor: Insufficient memory.\n");
    return NULL;
  };
/*
 * Initialize the container at least up to the point at which it is
 * safe to call del_FileMonitor().
 */
  fm->dir = NULL;
  fm->path = NULL;
  fm->name = NULL;
  fm->files = NULL;
  fm->nfiles = 0;
  fm->first = 0;
  fm->next = 0;
  fm->ta = 0;
  fm->tb = 0;
  fm->nrs = NULL;
  fm->nrs_size = 0;
  fm->fd = -1;
  fm->arraymap = NULL;
  fm->regmap = NULL;
  fm->reg.utc = -1;
  fm->range_ended = 0;
  fm->file_exhausted = 0;
  fm->rewound = 1;
  fm->rewind_pending = 0;

  fm->singleFile = true;
/*
 * Allocate a copy of the directory name, after expanding
 * any ~ or ~user/ prefixes and appending a '/' if necessary.
 */
  fm->dir = new_pathname(filename, NULL);
  if(!fm->dir)
    return del_FileMonitor(fm);
/*
 * Get the length of the expanded directory name.
 */
  dir_len = strlen(fm->dir);
/*
 * Allocate a string long enough to contain the directory name,
 * an archive file name and a string terminator, '\0'.
 */
  fm->path = (char* )malloc(dir_len + 1);
  if(!fm->path) {
    lprintf(stderr, "Insufficient memory for archive file name.\n");
    return del_FileMonitor(fm);
  };
/*
 * Copy the directory to the start of the path buffer.
 */
  strcpy(fm->path, filename);
/*
 * Record the position in the path buffer at which file names should be
 * appended.
 */
  fm->name = fm->path;
/*
 * Allocate a temporary file input stream to use solely to read
 * the required sizes of the final input and output streams.
 */
  fm->nrs = new_NetReadStr(fm->fd, 100);
  if(!fm->nrs)
    return del_FileMonitor(fm);
/*
 * Open the first file that lies in the requested time range.
 * Note that this also replaces fm->nrs with an input stream
 * of an appropriate size, and reads the file's register
 * map into fm->regmap.
 */
  if(fm_open_file(fm) != MS_READ_REGMAP)
    return del_FileMonitor(fm);
  return fm;
}

/*.......................................................................
 * Delete a file monitor object.
 *
 * Input:
 *  fm      FileMonitor *  The object to be deleted.
 * Output:
 *  return  FileMonitor *  The deleted object (always NULL).
 */
static FileMonitor *del_FileMonitor(FileMonitor *fm)
{
  if(fm) {

    fm_close_file(fm);

    if(fm->dir)
      free(fm->dir);

    if(fm->path)
      free(fm->path);

    fm->name = NULL;  /* fm->name is part of fm->path so already free'd */

    if(fm->files) {
      delete fm->files;
      fm->files = 0;
    }
	

    fm->nrs      = del_NetReadStr(fm->nrs);

    fm->regmap = del_RegMap(fm->regmap);

    free(fm);
  };

  return NULL;
}

/*.......................................................................
 * Delete the contents of a file monitor object and replace it with
 * the contents of a new file monitor object, then free the memory
 * pointed to by newFm.
 *
 * Input:
 *  fm      FileMonitor *  The object to be replaced
 *  newfm   FileMonitor *  The object to be copied into fm
 */
static void replace_FileMonitor(FileMonitor *fm, FileMonitor *newFm)
{
  if(fm) {
    fm_close_file(fm);
    if(fm->dir)
      free(fm->dir);
    if(fm->path)
      free(fm->path);
    fm->nrs = del_NetReadStr(fm->nrs);

    fm->arraymap = del_ArrayMap(fm->arraymap);
    fm->regmap   = del_RegMap(fm->regmap);
  };
  *fm = *newFm;
  free(newFm);
}

/*.......................................................................
 * Read the initial buffer-size requirements message from a newly opened
 * file. If the size hasn't been set before, fm->nrs_size will be
 * initialized with the extracted size. Otherwise if the new size differs
 * from fm->nrs_size an error will be generated.
 *
 * Input:
 *  fm    FileMonitor *  The file monitor object.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static int fm_read_size(FileMonitor *fm)
{
  int nrs_size; /* The new size extracted from the file */
  int opcode;    /* The type of message that has been read */
  NetBuf *net;   /* The network input buffer */
/*
 * Get an alias to the input buffer.
 */
  net = fm->nrs->net;
/*
 * Read the next message from the current file.
 */
  switch(fm_read_msg(fm)) {
  case MS_READ_DONE:
    break;
  default:
    lprintf(stderr, "Error reading frame size from %s.\n", fm->name);
    return 1;
    break;
  };
/*
 * Unpack the message.
 */
  if(net_start_get(net, &opcode) ||
     opcode != ARC_SIZE_RECORD ||
     net_get_int(net, 1, (unsigned int* )&nrs_size) ||
     net_end_get(net)) {
    lprintf(stderr, "Error reading %s.\n", fm->name); 
    return 1;
  };
/*
 * If the size requirements have changed or haven't been set before,
 * replace the current network buffer with a new one of the required
 * size.
 */
  if(fm->nrs_size != nrs_size) {
    fm->nrs_size = nrs_size;
    fm->nrs = del_NetReadStr(fm->nrs);
    fm->nrs = new_NetReadStr(fm->fd, fm->nrs_size);
    if(!fm->nrs)
      return 1;
  };
  return 0;
}

/*.......................................................................
 * This is a private function of new_FileMonitor() and fm_next_frame().
 * It reads the second record of an archive file, which contains a
 * template from which a register map can be constructed. It uses this
 * to allocate a new register map object and assigns this to fm->arraymap.
 *
 * Input:
 *  fm    FileMonitor *  The file monitor object.
 * Output:
 *  return     ArrayMap *  The new register map, or NULL on error.
 */
static ArrayMap *fm_read_ArrayMap(FileMonitor *fm)
{
  int opcode;                 /* The type of message that has been read */
  NetBuf* net=0;                /* The network input buffer */
  ArrayMap* arraymap=NULL;        /* The new register map */
  
  // Read the next message from the control program.

  switch(fm_read_msg(fm)) {
  case MS_READ_DONE:
    break;
  default:
    lprintf(stderr, "Error reading array map from %s.\n", fm->name);
    return NULL;
    break;
  };
  
  // Get an alias to the input buffer.

  net = fm->nrs->net;
  
  // Unpack the array template from the input buffer.

  if(net_start_get(net, &opcode) ||
     opcode != ARC_ARRAYMAP_RECORD ||
     (arraymap = net_get_ArrayMap(net)) == NULL ||
     net_end_get(net)) {
    lprintf(stderr, "The array-map in file %s is corrupt.\n", fm->name);
    return del_ArrayMap(arraymap);
  };
  
  // Return the new array map.

  return arraymap;
}

/*.......................................................................
 * Read the next frame of registers from a sequence of archive files, into
 * ms_RegRawData(ms).
 *
 * Input:
 *  ms   MonitorStream *  The monitor-stream iterator.
 *  dowait         int    Since non-blocking I/O is not supported for
 *                        files, this parameter is ignored in this
 *                        stream module.
 * Output:
 *  return MsReadState    The status of the read, from:
 *                         MS_READ_ENDED - End of stream.
 *                         MS_READ_BREAK - The next call will read
 *                                         from a new file.
 *                         MS_READ_DONE  - A complete frame has been
 *                                         read and calibrated.
 */
static MS_READ_FRAME(fm_read_frame)
{
  FileMonitor *fm = (FileMonitor *) ms_SourceContext(ms);
  NetBuf *net;            /* The file read buffer */
  MsReadState status;     /* The return value of fm_next_frame() */
  double utc;             /* The timestamp UTC of the file as a MJD */
  gcp::util::RegDate date;
  RegRawData *raw;        /* The container of uncalibrated register data */
  unsigned skip;          /* The number of frames to skip per frame */
  
  // Did we previously hit the end of the time range.

  if(fm->range_ended)
    return MS_READ_ENDED;
  
  // Start the next file?

  if(fm->file_exhausted) {
    status = fm_open_next_file(fm);
    if(status != MS_READ_DONE)
      return status;
  };
  
  // Get an alias to the input buffer.

  net = fm->nrs->net;
  
  // Get the container in which the raw data will be deposited.
 
  raw = ms_RegRawData(ms);

  // Determine the number of frames that should be skipped between
  // frames.

  skip = ms_get_interval(ms);
  if(skip > 0)
    skip--;
  
  // Locate the next message to return. This includes skipping frames
  // when sub-sampling has been requested, and on startup finding the
  // first frame that lies within the requested time range.

  do {
    
    // Should we skip any intervening frames?

    if(skip && lseek(fm->fd, 
		     skip * (NET_PREFIX_LEN + net_RegRawData_size(raw)),
		     SEEK_CUR) < 0) {
      lprintf(stderr, "Error positioning %s (%s).\n", fm->name,
	      strerror(errno));
      return fm_stage_next_file(fm);
    };
    
    // Read the next frame from this or the next file.

    switch((status=fm_next_frame(fm, raw))) {
    case MS_READ_DONE:
      break;
    default:
      return status;
      break;
    };
    
    // Compute the UTC Modified Julian Date from the Modified Julian
    // Day number and the time-of-day (ms).

    raw->fm->readReg("array", "frame", "utc", date.data());

    utc = date.mjd();

    // Continue until the start time is reached or past, and until
    // ms_get_interval(ms)-1 frames have been ignored.

  } while(!fm->singleFile && utc < fm->ta);
  
  // If the time-stamp of the new frame is later than the end time of
  // requested time range, then we have reached the end of the stream.

  if(fm->tb > 0.0 && utc > fm->tb) {
    fm->range_ended = 1;
    return MS_READ_ENDED;
  };
  
  // Flag the fact that some data has been read since the stream was
  // created or last rewound.

  fm->rewound = 0;
  return MS_READ_DONE;
}

/*.......................................................................
 * Read the next message from an archive file. The message will be left
 * for subsequent unpacking in fm->nrs.
 *
 * Input:
 *  fm      FileMonitor *  The file monitor object.
 * Output:
 *  return MsReadState    The state of the transaction. One of:
 *                          MS_READ_ENDED - End of file reached.
 *                          MS_READ_DONE  - The message has been read.
 */
static MsReadState fm_read_msg(FileMonitor *fm)
{
/*
 * Read the next message.
 */
  for(;;) {
    switch(nrs_read_msg(fm->nrs)) {
    case NetReadStr::NET_READ_SIZE:
    case NetReadStr::NET_READ_DATA:
      break;
    case NetReadStr::NET_READ_DONE:
      return MS_READ_DONE;
      break;
    case NetReadStr::NET_READ_ERROR:
      lprintf(stderr, "Skipping the rest of archive file: %s\n", fm->name);
      return MS_READ_ENDED;
      break;
    case NetReadStr::NET_READ_CLOSED:
      return MS_READ_ENDED;
      break;
    };
  };
}

/*.......................................................................
 * Rewind the stream.
 *
 * Input:
 *  ms   MonitorStream *  The monitor-stream iterator.
 * Output:
 *  return        int     MS_SEND_ERROR - Abnormal completion.
 *                        MS_SEND_DONE  - Normal completion.
 */
static MS_QUEUE_REWIND(fm_queue_rewind)
{
  FileMonitor *fm = (FileMonitor *) ms_SourceContext(ms);
/*
 * If needed queue the rewind to be performed on the next call
 * to fm_read_frame(). We can't do it here because this will
 * change the register map and only fm_read_frame() can signal
 * such a change.
 */
  if(!fm->rewound) {
    fm->next = fm->first;
    fm->range_ended = 0;
    fm->rewound = 1;
    fm->file_exhausted = 1;
  };
  return MS_SEND_DONE;
}

/*.......................................................................
 * This is the monitor-stream method that returns the socket fd for use
 * in select(). See monitor_stream.h for its definition.
 */
static MS_SELECT_FD(fm_select_fd)
{
  FileMonitor *fm = (FileMonitor *) ms_SourceContext(ms);
  return fm->fd;
}

/*.......................................................................
 * This is the monitor-stream method that returns the current register
 * map of the stream. See monitor_stream.h for its definition.
 */
static MS_ARRAYMAP(fm_arraymap)
{
  FileMonitor *fm = (FileMonitor *) ms_SourceContext(ms);

  return fm->arraymap;
}

/*.......................................................................
 * Construct a sorted list of archive files from a given directory
 *
 * Input:
 *  fm    FileMonitor *  The stream container.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static int fm_get_file_list(FileMonitor *fm)
{
  char *errmsg;             /* An error message */
  int i;
  
  // The following container records the filename timestamp of the
  // latest archive-file that precedes the earliest time of interest.

  struct {
    double utc;        /* The timestamp as a Modified Julian Date */
    int mjd;           /* The Modified Julian Day number from the file name */
    int sec;           /* The time of day (seconds) from the file name */
  } latest_preceding = {0.0,0,0};
  
  // Attempt to open the named directory

  std::list<gcp::util::DirList::DirEnt> fileList;

  // List all files under the named directory, including recursive
  // searches into subdirectories.  We return the list of all files,
  // including files that are symlinks

  try {
    gcp::util::DirList dirList(fm->dir, true);
    fileList = dirList.getFiles(true);
  } catch(...) {
    return 1;
  }

  fm->files->resize(fileList.size());

  std::list<gcp::util::DirList::DirEnt>::iterator entry;
  for(entry = fileList.begin(); entry != fileList.end(); entry++) {
    ArcFileType type; /* The type of archive file */
    ArcTimeStamp ts;  /* The archive-filename timestamp */
    double utc;       /* The Modified Julian Date of the file timestamp */
    
    // Try to decode the file name as that of an archive file.

    if(arc_file_id((char*)entry->name_.c_str(), 0, &ts, &type) 
       || type != ARC_DAT_FILE)
      continue;
    
    // Combine the Modified Julian Day number and time of day into a
    // Modified Julian Date.

    utc = ts.mjd + (double) ts.sec/daysec;
    
    // If the timestamp is later than the latest time that we are
    // interested in skip it.

    if(fm->tb > 0.0 && utc > fm->tb)
      continue;
    
    // The latest timestamp that precedes the earliest time of
    // interest may refer to a file who's later entries are in the
    // range of interest.  latest_preceding refers to the timestamp of
    // the latest timestamp found so far that precedes fm->ta.

    if(fm->ta > 0.0 && utc <= fm->ta) {
      if(utc > latest_preceding.utc) {
	latest_preceding.utc = utc;
	latest_preceding.mjd = ts.mjd;
	latest_preceding.sec = ts.sec;
      } else {
	continue;
      };
    };
    
    // Check whether the file is a regular file and whether we have
    // read access to it.

    errmsg = test_pathname((char*)entry->fullName().c_str(), PATH_IS_REG, PATH_READ);
    if(errmsg) {
      lprintf(stderr, "Ignoring %s (%s).\n", entry->fullName().c_str(), errmsg);
      continue;
    };
    
    // Record the time-stamp of the new file.

    fm->files->at(fm->nfiles).ts_.mjd = ts.mjd;
    fm->files->at(fm->nfiles).ts_.sec = ts.sec;
    fm->files->at(fm->nfiles).name_   = entry->fullName();

    fm->nfiles++;
 };
  
  fm->files->resize(fm->nfiles);

  // Did we fail to find any readable archive files?

  if(fm->nfiles < 1) {
    lprintf(stderr,
	    "Directory %s doesn't contain any suitable archive files.\n",
	    fm->dir);
    return 1;
  };
  
  // Sort the list of time-stamps.

  std::sort(fm->files->begin(), fm->files->end(), compMonitorFile);
  
  // If no time-stamps preceded the specified time-stamp, then select
  // the earliest available file as the first file of the stream.

  if(latest_preceding.utc <= 0.0) {
    fm->first = 0;
  } else {
    
    // Find the file that supplied the value of latest_preceding.

    for(i=0; i < fm->nfiles && (fm->files->at(i).ts_.mjd != latest_preceding.mjd ||
				fm->files->at(i).ts_.sec != latest_preceding.sec);
	i++)
      ;
    
    // Record its fm->files[] index.

    fm->first = i;
    
    // The above loop shouldn't fail, but caution doesn't hurt.
    
    if(i >= fm->nfiles) {
      lprintf(stderr, "fm_get_file_list: Internal error (file lost).\n");
      return 1;
    };
  }
  
  // Initialize the stream to start with the earliest file.
  
  fm->next = fm->first;

  return 0;
}

/*.......................................................................
 * This is the private error-return function of fm_get_file_list().
 */
static int bad_file_list(DIR *dir, struct dirent *dent)
  {
  if(dir)
    closedir(dir);
  if(dent)
    free(dent);
  return 1;
}

/*.......................................................................
 * Close the last archive file and open file fm->files[fm->next++].
 *
 * Input:
 *  fm     FileMonitor *  The file stream supplier object.
 * Output:
 *  return MsReadState    The completion status of the transaction. One of:
 *                          MS_READ_ENDED  - The end of the file list was
 *                                           reached.
 *                          MS_READ_REGMAP - The succesfully opened file has
 *                                           a differing register map.
 *                          MS_READ_DONE   - The succesfully opened file has
 *                                           an equivalent register map.
 *                                           
 */
static MsReadState fm_open_next_file(FileMonitor *fm)
{
  MonitorFile* file=0;   // The file list entry 
  ArrayMap* arraymap=0;      // The register map of the new file 

  // If we are reading only a single file, return immediately

  if(fm->singleFile)
    return MS_READ_ENDED;

  // Open one file after another until one is opened successfully, or
  // end of file is reached.

  for(;;) {
    
    // Close any previously opened file.

    fm_close_file(fm);
    
    // Have we reached the end of the file list?

    if(fm->next >= fm->nfiles)
      return MS_READ_ENDED;
    
    // Get details of the file to be opened.

    file = &fm->files->at(fm->next++);
    
    // Open the file

    fm->fd = open((char*)file->name_.c_str(), O_RDONLY, 0);

    if(fm->fd < 0) {
      lprintf(stderr, "Error opening %s: %s\n", fm->name, strerror(errno));
      continue;  // Try the next file 
    };
    
    // Connect the input stream to the new file descriptor.

    attach_NetReadStr(fm->nrs, fm->fd);
    
    // Read the buffer size requirement of the new file and check that
    // it matches that of previous files (if any).

    if(fm_read_size(fm))
      continue;  // Try the next file 
    
    // Replace the current arraymap in fm->arraymap by the one contained
    // in the new file.

    arraymap = fm_read_ArrayMap(fm);

    if(!arraymap)
      continue;  // Try the next file 
    
    // Mark the file as containing frames to be read.

    
    // If the new register map is equivalent to the old one, keep
    // using the old one.

    if(equiv_ArrayMap(fm->arraymap, arraymap)) {
      arraymap = del_ArrayMap(arraymap);
      fm->file_exhausted = 0;
      return MS_READ_DONE;
    } else {
      RegMapBlock *utc_reg;   /* The time/date register */
      
      // Discard the old register map.

      del_ArrayMap(fm->arraymap);

      // Record the new one.

      fm->arraymap = arraymap;
      
      // Locate the register that contains the time and date of each
      // record.

      utc_reg = find_ArrayMapBlock(fm->arraymap, "array", "frame", "utc");

      if(!utc_reg) {
	lprintf(stderr, "Unable to find the array.frame.utc register in %s\n",
		fm->name);
	continue;			/* Try the next file */
      };
      
      // Record the frame offset of this register.

      fm->reg.utc = utc_reg->slot_;
      
      // The file is ready to have frames read from it.

      fm->file_exhausted = 0;
      
      // Report the change.

      return MS_READ_REGMAP;
    };
  };
}

/*.......................................................................
 * Close the last archive file and open a named file
 *
 * Input:
 *  fm     FileMonitor *  The file stream supplier object.
 * Output:
 *  return MsReadState    The completion status of the transaction. One of:
 *                          MS_READ_ENDED  - The end of the file list was
 *                                           reached.
 *                          MS_READ_REGMAP - The succesfully opened file has
 *                                           a differing register map.
 *                          MS_READ_DONE   - The succesfully opened file has
 *                                           an equivalent register map.
 *                                           
 */
static MsReadState fm_open_file(FileMonitor *fm)
{
  ArrayMap* arraymap=0;      // The register map of the new file 
  
  // Open one file after another until one is opened successfully, or
  // end of file is reached.

  // Close any previously opened file.
  
  fm_close_file(fm);
  
  // Open the file (note that fm->name points to the name part of
  // fm->path).

  fm->fd = open(fm->path, O_RDONLY, 0);
  if(fm->fd < 0) {
    lprintf(stderr, "Error opening %s: %s\n", fm->name, strerror(errno));
    return MS_READ_ENDED;
  };
    
  // Connect the input stream to the new file descriptor.

  attach_NetReadStr(fm->nrs, fm->fd);
    
  // Read the buffer size requirement of the new file and check that
  // it matches that of previous files (if any).
  
  if(fm_read_size(fm))
    return MS_READ_ENDED;
  
  // Replace the current arraymap in fm->arraymap by the one contained
  // in the new file.
  
  arraymap = fm_read_ArrayMap(fm);


  if(!arraymap)
    return MS_READ_ENDED;
  
  // Mark the file as containing frames to be read.
  
  
  // If the new register map is equivalent to the old one, keep
  // using the old one.
  
  if(equiv_ArrayMap(fm->arraymap, arraymap)) {

    arraymap = del_ArrayMap(arraymap);

    fm->file_exhausted = 0;
    return MS_READ_DONE;
  } else {
    RegMapBlock *utc_reg;   /* The time/date register */
    
    // Discard the old register map.
    
    fm->arraymap = del_ArrayMap(fm->arraymap);
    
    // Record the new one.
    
    fm->arraymap = arraymap;
    
    // Locate the register that contains the time and date of each
    // record.
    
    utc_reg = find_ArrayMapBlock(fm->arraymap, "array", "frame", "utc");
    if(!utc_reg) {
      lprintf(stderr, "Unable to find the array.frame.utc register in %s\n",
	      fm->name);
      return MS_READ_ENDED;
    };
    
    // Record the frame offset of this register.
    
    fm->reg.utc = utc_reg->slot_;
    
    // The file is ready to have frames read from it.
    
    fm->file_exhausted = 0;
    
    // Report the change.
    
    return MS_READ_REGMAP;
  };
}

/*.......................................................................
 * Close the last monitor-input file.
 *
 * Input:
 *  fm    FileMonitor *  The file-stream supplier object.
 */
static void fm_close_file(FileMonitor *fm)
{
  if(fm->fd >= 0) {
    close(fm->fd);
    fm->fd = -1;
    attach_NetReadStr(fm->nrs, -1);
  };
}

/*.......................................................................
 * Read the next available archive from a spliced sequence of files.
 *
 * Input:
 *  fm      FileMonitor *  The file-input archive stream.
 * Input/Output:
 *  raw      RegRawData *  The output raw data container, dimensioned
 *                         according to the size of fm->regmap.
 * Output:
 *  return  MsReadState    The status of the read, from:
 *                          MS_READ_ENDED  - End of files reached.
 *                          MS_READ_DONE   - The frame has been read.
 *                          MS_READ_BREAK  - The next call to fm_read_frame()
 *                                           will read from a new file.
 */
static MsReadState fm_next_frame(FileMonitor *fm, RegRawData *raw)
{
  int opcode;             /* The received message type */
  NetBuf *net;            /* The input stream buffer */

  // Get an alias to the input stream buffer.

  net = fm->nrs->net;
  
  // Read until a valid message is found, an error occurs, or the end
  // of the list of files is reached.

  for(;;) {
    
    // Read the next message from the current file.

    switch(fm_read_msg(fm)) {
      
      // The new frame has been succesfully read into the buffer.
      // Unpack it into *raw.
      
    case MS_READ_DONE:
      if(net_start_get(net, &opcode) ||
	 opcode != ARC_FRAME_RECORD ||
	 net_get_RegRawData(net, raw) ||
	 net_end_get(net)) {
	return fm_stage_next_file(fm);
      };

      return MS_READ_DONE;
      break;
      
      // The end of the current archive file has been reached, or said
      // file is corrupt so stage the next file of the time-range to
      // be read on the next call to fm_read_frame().

    default:
      return fm_stage_next_file(fm);
      break;
    };
  };
}

/*.......................................................................
 * Mark the current file as completed and if there is a subsequent file
 * prepare to open it on the next call to fm_read_frame().
 *
 * Input:
 *  fm      FileMonitor *  The file-input archive stream.
 * Output:
 *  return  MsReadState    The status of the read, from:
 *                          MS_READ_ENDED - End of files reached.
 *                          MS_READ_BREAK - The next call to fm_read_frame()
 *                                          will read from a new file.
 */
static MsReadState fm_stage_next_file(FileMonitor *fm)
{
  // Mark the file as ended.

  fm->file_exhausted = 1;
  
  // Have we exhausted the list of files that lie within the requested
  // time range?

  if(fm->next >= fm->nfiles) {
    return MS_READ_ENDED;
  } else {
    MonitorFile* next = &fm->files->at(fm->next);

    if(fm->tb > 0.0 && next->ts_.mjd+(double)next->ts_.sec/daysec > fm->tb)
      return MS_READ_ENDED;
  };
  
  // Defer opening the next file to the next call to fm_read_frame(),
  // then warn the caller of the break in the stream.

  return MS_READ_BREAK;
}
