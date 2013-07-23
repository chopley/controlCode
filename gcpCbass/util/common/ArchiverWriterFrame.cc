#include "gcp/util/common/ArchiverWriterFrame.h"

#include "gcp/control/code/unix/libunix_src/common/arcfile.h"
#include "gcp/control/code/unix/libunix_src/common/archive.h"

#include "gcp/util/common/TimeVal.h"

#include <string.h>

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
ArchiverWriterFrame::ArchiverWriterFrame()
{
  net_       = 0;
  dir_       = 0;
  path_      = 0;
  fp_        = 0;
  nrecorded_ = 0;
  fileSize_  = 0;
  frame_     = 0;
  arrayMap_  = 0;

  // Allocate the array map, and the raw data frame
 
  arrayMap_  = new_ArrayMap();
  if(!arrayMap_)
    ThrowError("Unable to allocate array map");

  frame_     = new_RegRawData(arrayMap_, false);
  if(!frame_)
    ThrowError("Unable to allocate data frame");
  
  // Determine the sizes of each of the records that appear in archive
  // files.

  {
    int size1 = NET_INT_SIZE;                // Initial buffer-dimension record 
    int size2 = net_ArrayMap_size();         // Array map record 
    int size3 = net_RegRawData_size(frame_); // Frame record 
    
    // Determine the largest of the record sizes.

    int recsize = size1 > size2 ? size1 : size2;

    if(size3 > recsize)
      recsize = size3;
    
    // Allocate a buffer in which to compose archive records.

    net_ = new_NetBuf(NET_PREFIX_LEN + recsize);

    if(!net_)
      ThrowError("Unable to allocate network buffer");
  };
}

/**.......................................................................
 * Destructor.
 */
ArchiverWriterFrame::~ArchiverWriterFrame() 
{
  closeArcfile();

  if(net_ != 0) 
    net_ = del_NetBuf(net_);

  if(frame_ != 0)
    frame_ = del_RegRawData(frame_);

  if(arrayMap_ != 0)
    arrayMap_ = del_ArrayMap(arrayMap_);

  if(dir_ != 0) {
    free(dir_);
    dir_ = 0;
  }
}

int ArchiverWriterFrame::chdir(char *dir)
{
  
  // Make a copy of the name of the current archive directory.

  if(dir != dir_ && *dir != '\0') {
    size_t bytes = strlen(dir)+1;
    char *tmp = (char* )(dir_ ? realloc(dir_, bytes) : malloc(bytes));
    if(!tmp) {
      ReportError("Unable to record new archive directory");
      return 1;
    } else {
      strcpy(tmp, dir);
      dir_ = tmp;
    };
  };
  return 0;
}

int ArchiverWriterFrame::openArcfile(std::string dir)
{
  return openArcfile((char*)dir.c_str());
}

int ArchiverWriterFrame::openArcfile(char *dir)
{
  char* path=0;                // The path name of the file 
  FILE* fp=0;                  // The file-pointer of the open file 
  unsigned int size;           // The network buffer size 
  
  // Record a new log file directory?

  if(dir && *dir!='\0')
    (void) chdir(dir);
  else
    dir = dir_;
  
  // Compose the full pathname of the archive file.

  path = arc_path_name(dir, NULL, ARC_DAT_FILE);
  if(!path)
    return 1;
  
  // Attempt to open the new data file.

  fp = fopen(path, "wb");

  if(!fp) {
    ReportError("Unable to open archive file: " << path);
    free(path);
    return 1;
  };
  
  // Close the current archive file if open.

  closeArcfile();

  // Install the new archive file.

  path_ = path;
  fp_   = fp;
  
  // Report the successful opening of the file.

  ReportMessage("Starting new archive file: " << path);

  // Output the initial record that contains the minimum network buffer
  // size needed to read records from the file.

  size = net_->size;
  if(net_start_put(net_, ARC_SIZE_RECORD) ||
     net_put_int(net_, 1, &size) ||
     net_end_put(net_) ||
     (int)fwrite(net_->buf, sizeof(net_->buf[0]),
	    net_->nput, fp_) != net_->nput) {
    ReportSysError("Error writing archive-file size");
    closeArcfile();
    return 1;
  };
  
  // Output the record that contains the details of the array map.

  if(net_start_put(net_, ARC_ARRAYMAP_RECORD) ||
     net_put_ArrayMap(net_) ||
     net_end_put(net_) ||
     (int)fwrite(net_->buf, sizeof(net_->buf[0]),
	    net_->nput, fp_) != net_->nput) {
    ReportSysError("Error writing archive-file regmap");
    closeArcfile();
    return 1;
  };
  return 0;
}

void ArchiverWriterFrame::closeArcfile()
{
  if(fp_) {
    if(fclose(fp_)) {
      ReportError("Error closing archive file: " << path_);
    } else {
      ReportMessage("Closing archive file: " << path_);
    }
  };

  fp_ = NULL;

  if(path_)
    free(path_);

  path_ = NULL;
}

void ArchiverWriterFrame::flushArcfile()
{
  if(fp_ && fflush(fp_)) {
    ReportError("Error flushing archive file: " << path_);
    closeArcfile();
  };
}

int ArchiverWriterFrame::writeIntegration() 
{
  return (int)(fwrite(net_->buf, sizeof(net_->buf[0]), net_->nput, fp_) != net_->nput);
}

int ArchiverWriterFrame::saveIntegration()
{
  int status;        // The status return value of a pthread function 
  
  // Construct an output record of the integrated data.  Note that the
  // return status is ignored until exclusive access to the buffer has
  // been released further below.

  status = net_start_put(net_, ARC_FRAME_RECORD) ||
           net_put_RegRawData(net_, frame_) ||
	   net_end_put(net_);
  
  // abort if there was an error.

  if(status)
    return 1;

    // Do we have an archive file to save to, and should we save this
  // frame?

  static TimeVal curr, last, diff;
  if(isOpen()) {

    last.setToCurrentTime();

    if(writeIntegration()) {
      closeArcfile();
      return 1;
    };

    curr.setToCurrentTime();

    diff = curr-last;

    // If the current archive file has reached the maximum size
    // specified by the user, open a new one for the next record.

    if(fileSize_ > 0 && ++nrecorded_ >= fileSize_)
      return openArcfile(dir_);

  };

  return 0;
}

void ArchiverWriterFrame::setFileSize(unsigned fileSize)
{
  fileSize_ = fileSize;
}

