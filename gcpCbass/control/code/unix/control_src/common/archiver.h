#ifndef archiver_h
#define archiver_h

#include <pthread.h>

#include "genericcontrol.h"
#include "genericregs.h"
#include "list.h"
#include "netbuf.h"
#include "regset.h"

#include "gcp/util/common/RegDescription.h"

namespace gcp {
  namespace control {
    class ArchiverWriter;
  }
}

/*
 * Register frames will be maintained in objects of the following type.
 * Callers must acquire the guard mutex before attempting to access
 * the frame buffer.
 */
typedef struct {
  pthread_mutex_t guard;       /* The mutual exlusion guard of the frame */
  int guard_ok;                /* True after initializing 'guard' */
  pthread_cond_t start;        /* When the archiver has finished saving */
                               /*  the latest integration, it will set */
                               /*  the 'integrate' flag to 1 and signal */
                               /*  the 'start' condition variable */
  int start_ok;                /* True after initializing 'start' */
  int integrate;               /* See the documentation of 'start' */
  unsigned nframe;             /* The number of frames to accumulate per */
                               /*  integration. */
  unsigned count;              /* The number of frames accumulated during */
                               /*  an ongoing integration */
  bool newFrame;               /* True if we should begin a new frame
				  on the next half-second boundary */
  unsigned currFrameSeq;       /* The sequence number associated with
				  the current frame */
  unsigned lastFrameSeq;       /* The sequence number associated with
				  the current frame */
  ArrayMap *arraymap;          /* The array map of this experiment */
  RegRawData *frame;           /* The frame being integrated or copied */
  unsigned int* features;      /* A pointer to the frame.features register */
                               /*  in frame[] */
  unsigned int* markSeq;       /* A pointer to the frame.mark_seq register */
                               /*  in frame[] */
  unsigned int lastSeq;        /* The value of *mark_seq when the last */
                               /*  frame was saved to disk. */
} FrameBuffer;

/*
 * An object of the following type is used to hold the state of
 * the archiver thread.
 */
struct Archiver {
  ControlProg *cp;             /* The state-object of the control program */
  gcp::util::Pipe *pipe;                  /* An externally allocated control-input pipe */
  ListMemory *list_mem;        /* Memory for allocating lists and their nodes */
  List *clients;               /* The list of connected clients */
  CcPipeMsg ccmsg;             /* A control client message container to use */
                               /*  for sending status messages to clients in */
                               /*  the clients list. */
  char *dir;                   /* The directory in which to place new archive */
                               /*  files. */
  char *path;                  /* The pathname of the current archive file */
  FILE *fp;                    /* The file-pointer attached to 'path' */
  FrameBuffer fb;              /* The integration frame buffer */
  gcp::control::NetBuf *net;   /* The network buffer of the output frame */
  int filter;                  /* If true only record frames that contain */
                               /*  feature markers. */
  int file_size;               /* The number of frames to record before */
                               /*  starting a new archive file, or 0 if */
                               /*  no limit is desired. */
  int nrecorded;               /* The number of frames recorded in the */
                               /*  current file. */
  struct {
    int pending;
    unsigned seq;
  } tv_offset;

  gcp::control::ArchiverWriter* writer_;
};

Archiver *cp_Archiver(ControlProg *cp);

int get_reg_info(Archiver *arc, short iregmap, short board, short block, 
		 short index, unsigned int *val);

unsigned int* arc_find_reg(Archiver *arc, char* regmap, char* board, 
			   char* name);

double* arcFindActualReg(Archiver *arc, char* regmap);

int chdir_archiver(Archiver *arc, char *dir);
int open_arcfile(Archiver *arc, char *dir);
void flush_arcfile(Archiver *arc);
void close_arcfile(Archiver *arc);
int arc_send_ccmsg(Archiver *arc, gcp::util::Pipe *client, const char *fmt, ...);

double getRegVal(Archiver *arc, gcp::util::RegDescription& regDesc);

#endif
