#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#include "lprintf.h"
#include "genericcontrol.h"
#include "const.h"
#include "genericregs.h"
#include "netbuf.h"
#include "arcfile.h"
#include "optcam.h"
#include "fitsio.h"
#include "grabber.h"
#include "archiver.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/ImageHandler.h"

#include "gcp/grabber/common/Flatfield.h"

using namespace gcp::control;

/**.......................................................................
 * Image buffers will be maintained in objects of the following type.
 * Callers must acquire the guard mutex before attempting to access
 * the image buffer.
 */
typedef struct {
  pthread_mutex_t guard;       /* The mutual exlusion guard of the image */
  int guard_ok;                /* True after initializing 'guard' */
  pthread_cond_t start;        /* When the grabberr has finished saving */
                               /*  the latest image, it will set */
                               /*  the 'save' flag to 1 and signal */
                               /*  the 'start' condition variable */
  int start_ok;                /* True after initializing 'start' */
  int save;
  double dx;                    /* The x-increment of a pixel */
  double dy;                    /* The y-increment of a pixel */
  double xa;                    /* The x-coordinate of the blc of the image */
  double ya;                    /* The y-coordinate of the blc of the image */
  double xpeak;                 /* The coordinate of the peak of the image */
  double ypeak;                 /* The y-coordinate of the peak of the image */
  double max;                   /* The max pixel value */
  double snr;                   /* signal to noise of the peak pixel */
  unsigned short image[GRABBER_IM_SIZE];
  unsigned int utc[2];
  double actual[3];
  unsigned seq;
} ImageBuffer;

static int grabber_image_error(ImageBuffer *im);

/**.......................................................................
 * An object of the following type is used to hold the state of
 * the grabber thread.
 */
struct Grabber {
  ControlProg *cp;             /* The state-object of the control program */
  gcp::util::Pipe *pipe;       /* An externally allocated control-input pipe */
  int archive;                 /* True if we are archiving images. */
  char *dir;                   /* The directory in which to place new images */
  char *path;                  /* The pathname of the current archive file */
  FILE *fp;                    /* The file-pointer attached to 'path' */
  ImageBuffer im;              /* The image buffer */
  NetBuf *net;                 /* The network buffer of the output image */
  double* actual_;

  std::vector<gcp::util::ImageHandler>* imageHandlers_;

  gcp::util::ImageHandler* currentImage_;
};

static int chdir_grabber(Grabber *grabber, char *dir);
static int open_grabfile(Grabber *grabber, char *dir, ArcTimeStamp *time);
static int grabber_write_image(Grabber *grabber);
static void flush_grabfile(Grabber *grabber);
static void close_grabfile(Grabber *grabber);

static int 
sendOpticalCameraFov(ControlProg* cp, gcp::util::Angle& fov,
		     unsigned chanMask = gcp::grabber::Channel::ALL);

static int 
sendOpticalCameraXImDir(ControlProg* cp, ImDir dir,
			unsigned chanMask = gcp::grabber::Channel::ALL);

static int 
sendOpticalCameraYImDir(ControlProg* cp, ImDir dir,
			unsigned chanMask = gcp::grabber::Channel::ALL);

static int 
sendDeckAngleRotationSense(ControlProg* cp, RotationSense sense, 
			   unsigned chanMask = gcp::grabber::Channel::ALL);

static int 
sendOpticalCameraAspectRatio(ControlProg* cp, double aspect,
			     unsigned chanMask = gcp::grabber::Channel::ALL);

static int 
sendOpticalCameraRotationAngle(ControlProg* cp, gcp::util::Angle& collimation,
			       unsigned chanMask = gcp::grabber::Channel::ALL);

static int 
sendGrabberCombine(ControlProg* cp, unsigned combine, 
		   unsigned chanMask = gcp::grabber::Channel::ALL);

static int 
sendGrabberFlatfieldType(ControlProg* cp, unsigned flatfield, 
			 unsigned chanMask = gcp::grabber::Channel::ALL);

// Send a search box command to connected clients

int sendFrameGrabberSearchBox(ControlProg* cp, 
			      unsigned ixmin, unsigned ixmax, 
			      unsigned iymin, unsigned iymax, 
			      bool inc,
			      unsigned chanMask);

/**.......................................................................
 * Create the state object of a grabber thread.
 *
 * Input:
 *  cp     ControlProg *   The state object of the control program.
 *  pipe          Pipe *   The pipe on which to listen for control messages.
 * Output:
 *  return        void *   The new Grabber object, or NULL on error.
 */
CP_NEW_FN(new_Grabber)
{
  Grabber *grabber;     /* The object to be returned */
  int status;        /* The status return value of a pthread function */
  double xa,xb,ya,yb;
  
  // Allocate the container.
  
  grabber = (Grabber* )malloc(sizeof(Grabber));
  
  if(!grabber) {
    lprintf(stderr, "new_Grabber: Insufficient memory.\n");
    return NULL;
  };
  
  // Before attempting any operation that might fail, initialize the
  // container at least up to the point at which it can be safely
  // passed to del_Grabber().
  
  grabber->cp             = cp;
  grabber->pipe           = pipe;
  grabber->dir            = NULL;
  grabber->path           = NULL;
  grabber->fp             = NULL;
  grabber->archive        = 0;
  grabber->im.guard_ok    = 0;
  grabber->im.start_ok    = 0;
  grabber->im.save        = 1;
  grabber->actual_        = 0;
  grabber->imageHandlers_ = 0;
  
  // Initialize parameters.  All positions will be computed in pixels.
  // In these units, the first pixel of the image is centered at -N/2 + 0.5.
  
  xa = -(double)(GRABBER_XNPIX)/2 + 0.5;
  ya = -(double)(GRABBER_YNPIX)/2 + 0.5;
  xb =  (double)(GRABBER_XNPIX)/2 - 0.5;
  yb =  (double)(GRABBER_YNPIX)/2 - 0.5;
  
  // Store the start pixel values, and the image deltas, in degrees
  
  grabber->im.xa = xa;
  grabber->im.ya = ya;
  grabber->im.dx = 1.0;
  grabber->im.dy = 1.0;
  
  grabber->net = NULL;
  
  // Create the mutex that protects access to the image buffer buffer.
  
  status = pthread_mutex_init(&grabber->im.guard, NULL);
  if(status) {
    lprintf(stderr, "pthread_mutex_init: %s\n", strerror(status));
    return del_Grabber(grabber);
  };
  grabber->im.guard_ok = 1;
  
  // Create the condition variable that the grabber uses to signal
  // that it has finished copying the latest image from its image
  // buffer into its network buffer.
  
  status = pthread_cond_init(&grabber->im.start, NULL);
  if(status) {
    lprintf(stderr, "pthread_cond_init: %s\n", strerror(status));
    return del_Grabber(grabber);
  };
  grabber->im.start_ok = 1;
  
  // Allocate a buffer in which to compose image records.
  
  grabber->net = new_NetBuf(NET_PREFIX_LEN + FITS_HEADER_SIZE + 
			    GRABBER_IM_SIZE*2);
  if(!grabber->net)
    return del_Grabber(grabber);

  // Initialize the array of image handlers 

  grabber->imageHandlers_ = new std::vector<gcp::util::ImageHandler>;

  if(!grabber->imageHandlers_)
    return del_Grabber(grabber);

  grabber->imageHandlers_->resize(gcp::grabber::Channel::nChan_);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++)
    grabber->imageHandlers_->at(iChan).setChannel(gcp::grabber::Channel::intToChannel(iChan));

  // And return the initialized object

  return grabber;
}

/*.......................................................................
 * Delete the state-object of an grabber thread.
 *
 * Input:
 *  obj         void *  The Grabber object to be deleted.
 * Output:
 *  return      void *  The deleted Grabber object (always NULL).
 */
CP_DEL_FN(del_Grabber)
{
  Grabber *grabber = (Grabber* )obj;
  if(grabber) {

    if(grabber->dir) {
      free(grabber->dir);
      grabber->dir = NULL;
    };

    close_grabfile(grabber);

    if(grabber->im.guard_ok)
      pthread_mutex_destroy(&grabber->im.guard);

    if(grabber->im.start_ok)
      pthread_cond_destroy(&grabber->im.start);

    grabber->net = del_NetBuf(grabber->net);

    if(grabber->imageHandlers_)
      delete grabber->imageHandlers_;

    free(grabber);
  };

  return NULL;
}

/**.......................................................................
 * Attempt to send a message to a grabber thread.
 *
 * Input:
 *  cp      ControlProg *  The state-object of the control program.
 *  msg GrabberMessage *  The message to be sent. This must have been
 *                         filled by one of the pack_grabber_<type>()
 *                         functions.
 *  timeout        int    The max number of milliseconds to wait for the
 *                         message to be sent, or -1 to wait indefinitely.
 * Output:
 *  return    PipeState    The status of the transaction:
 *                           PIPE_OK    - The message was read successfully.
 *                           PIPE_BUSY  - The send couldn't be accomplished
 *                                        without blocking (only returned
 *                                        when timeout=PIPE_NOWAIT).
 *                           PIPE_ERROR - An error occurred.
 */
PipeState send_GrabberMessage(ControlProg *cp, GrabberMessage *msg,
			      int timeout)
{
  return cp_Grabber(cp)->pipe->write(msg, sizeof(*msg), timeout);
}

/*.......................................................................
 * Send a shutdown message to the grabber thread using non-blocking I/O.
 * Return 0 if the message was sent, 0 otherwise.
 *
 * Input:
 *  cp      ControlProg *  The control-program resource object.
 * Output:
 *  return          int    0 - Message sent ok.
 *                         1 - Unable to send message.
 */
CP_STOP_FN(stop_Grabber)
{
  GrabberMessage msg;   /* The message to be sent */
  return pack_grabber_shutdown(&msg) ||
    send_GrabberMessage(cp, &msg, PIPE_NOWAIT) != PIPE_OK;
}

/*.......................................................................
 * Prepare a shutdown message for subsequent transmission to the
 * grabber thread.
 *
 * Input:
 *  msg  GrabberMessage *  The message object to be prepared.
 * Output:
 * return       int     0 - OK.
 *                      1 - Error.
 */
int pack_grabber_shutdown(GrabberMessage *msg)
{
  /*
   * Check arguments.
   */
  if(!msg) {
    lprintf(stderr, "pack_grabber_shutdown: NULL argument.\n");
    return 1;
  };
  msg->type = GRAB_SHUTDOWN;
  return 0;
}

/*.......................................................................
 * Prepare a change-default-directory message for subsequent transmission
 * to the grabber thread.
 *
 * Input:
 *  msg   GrabberMessage *  The message object to be packed for subsequent
 *                         transmission.
 *  dir            char *  The directory in which to open subsequent log
 *                         files.
 * Output:
 * return           int    0 - OK.
 *                         1 - Error.
 */
int pack_grabber_chdir(GrabberMessage *msg, char *dir)
{
  // Check arguments.

  if(!msg || !dir) {
    lprintf(stderr, "pack_grabber_chdir: NULL argument(s).\n");
    return 1;
  };
 
  msg->type = GRAB_CHDIR;
  strncpy(msg->body.chdir.dir, dir, CP_FILENAME_MAX);
  msg->body.chdir.dir[CP_FILENAME_MAX] = '\0';

  return 0;
}

/*.......................................................................
 * Prepare an open-data-file message for subsequent transmission to the
 * grabber thread.
 *
 * Input:
 *  msg  GrabberMessage *  The message object to be prepared.
 *  dir             char *   The directory in which to open the file.
 * Output:
 * return            int     0 - OK.
 *                           1 - Error.
 */
int pack_grabber_open(GrabberMessage *msg, char *dir)
{
  
  // Check arguments.

  if(!msg || !dir) {
    lprintf(stderr, "pack_grabber_open: NULL argument(s).\n");
    return 1;
  };

  msg->type = GRAB_OPEN;
  strncpy(msg->body.open.dir, dir, CP_FILENAME_MAX);
  msg->body.open.dir[CP_FILENAME_MAX] = '\0';

  return 0;
}

/*.......................................................................
 * Prepare a flush-data-file message for subsequent transmission to the
 * grabber thread.
 *
 * Input:
 *  msg  GrabberMessage *  The message object to be prepared.
 * Output:
 * return       int     0 - OK.
 *                      1 - Error.
 */
int pack_grabber_flush(GrabberMessage *msg)
{
  
  // Check arguments.

  if(!msg) {
    lprintf(stderr, "pack_grabber_flush: NULL argument.\n");
    return 1;
  };

  msg->type = GRAB_FLUSH;

  return 0;
}

/*.......................................................................
 * Prepare a close-data-file message for subsequent transmission to the
 * grabber thread.
 *
 * Input:
 *  msg  GrabberMessage *  The message object to be prepared.
 * Output:
 * return       int     0 - OK.
 *                      1 - Error.
 */
int pack_grabber_close(GrabberMessage *msg)
{
  
  // Check arguments.

  if(!msg) {
    lprintf(stderr, "pack_grabber_close: NULL argument.\n");
    return 1;
  };

  msg->type = GRAB_CLOSE;

  return 0;
}

/*.......................................................................
 * Prepare a save-grabber-image message for subsequent transmission to
 * the grabber thread.
 *
 * Input:
 *  msg  GrabberMessage *  The message object to be prepared.
 * Output:
 *  return       int    0 - OK.
 *                      1 - Error.
 */
int pack_grabber_image(GrabberMessage *msg)
{
  
  // Check arguments.

  if(!msg) {
    lprintf(stderr, "pack_grabber_image: NULL argument.\n");
    return 1;
  };
  
  // Compose the message.

  msg->type = GRAB_IMAGE;

  return 0;
}

/**.......................................................................
 * This is the error return function of grabber_integrate_image(). It
 * releases exclusive access to the image buffer and returns the error
 * code of grabber_integrate_image().
 *
 * Input:
 *  im     ImageBuffer *  The locked image buffer.
 * Output:
 *  return         int    1.
 */
static int grabber_image_error(ImageBuffer *im)
{
  pthread_mutex_unlock(&im->guard);
  return 1;
}

/**.......................................................................
 * This is the entry-point of the grabber thread.
 *
 * Input:
 *  arg          void *  A pointer to the Grabber state object pointer,
 *                       cast to (void *).
 * Output:
 *  return       void *  NULL.
 */
CP_THREAD_FN(grabber_thread)
{
  Grabber *grabber = (Grabber* )arg; /* The state-object of the
					current thread */
  GrabberMessage msg; /* An message reception container */
  
  // Enable logging of the scheduler's stdout and stderr streams.

  if(log_thread_stream(grabber->cp, stdout) ||
     log_thread_stream(grabber->cp, stderr)) {
    cp_report_exit(grabber->cp);
    return NULL;
  };
  
  // Wait for commands from other threads.

  gcp::util::FdSet fdSet;
  fdSet.registerReadFd(grabber->pipe->readFd());
  
  while(select(fdSet.size(), fdSet.readFdSet(), NULL, NULL, NULL) > 0) {
    
    if(grabber->pipe->read(&msg, sizeof(msg), PIPE_WAIT) != PIPE_OK)
      break;
    
    // Interpret the message.

    switch(msg.type) {
    case GRAB_IMAGE:
      (void) grabber_write_image(grabber);
      break;
    case GRAB_SHUTDOWN:
      close_grabfile(grabber);
      cp_report_exit(grabber->cp);
      return NULL;
      break;
    case GRAB_CHDIR:
      (void) chdir_grabber(grabber, msg.body.chdir.dir);
      break;
    case GRAB_OPEN: // Tell the grabber thread to save subsequent
		    // images to disk.
      grabber->archive = 1;
      (void) chdir_grabber(grabber, msg.body.open.dir); // Pass on to
							// chdir in
							// case a new
							// directory
							// was
							// specified
      break;
    case GRAB_FLUSH:
      flush_grabfile(grabber);
      break;
    case GRAB_CLOSE: // Tell the grabber thread not to save subsequent
		     // images to disk. 
      grabber->archive = 0;
      break;
    case GRAB_SEQ:   // Update the grabber transaction sequence number 
      grabber->im.seq = msg.body.seq;
      break;
    default:
      lprintf(stderr, "grabber_thread: Unknown command-type received.\n");
      break;
    };
  };

  fprintf(stderr, "Grabber thread exiting after pipe read error.\n");
  cp_report_exit(grabber->cp);

  return NULL;
}

/**.......................................................................
 * Flush unwritten data to the current grabber data-file.
 *
 * Input:
 *  grabber     Grabber *   The state-object of the grabber thread.
 */
static void flush_grabfile(Grabber *grabber)
{
  if(grabber->fp && fflush(grabber->fp)) {
    lprintf(stderr, "Error flushing grabber file: %s\n", grabber->path);
    close_grabfile(grabber);
  };
}

/**.......................................................................
 * Close the current grabber data-file. Until a new data file is opened,
 * subsequently integrated images will be discarded.
 *
 * Input:
 *  grabber     Grabber *   The state-object of the grabber thread.
 */
static void close_grabfile(Grabber *grabber)
{
  if(grabber->fp) {
    if(fclose(grabber->fp))
      lprintf(stderr, "Error closing grabber file: %s\n", grabber->path);
    else
      lprintf(stdout, "Closing grabber file: %s\n", grabber->path);
  };

  grabber->fp = NULL;

  if(grabber->path)
    free(grabber->path);

  grabber->path = NULL;
}

/**.......................................................................
 * Open a new grabber file in a given directory. If the file is opened
 * successfully, close the previous grabber file.
 *
 * Input:
 *  grabber   Grabber *   The state-object of the current thread.
 *  dir       char *   The name of the directory in which to create the
 *                     file or NULL or "" to use the last directory that
 *                     was specified.
 * Output:
 *  return     int     0 - OK.
 *                     1 - Error.
 */
static int open_grabfile(Grabber *grabber, char *dir, ArcTimeStamp *time)
{
  char *path;                  /* The path name of the file */
  FILE *fp;                    /* The file-pointer of the open file */
  
  // Record a new log file directory?

  if(dir && *dir!='\0')
    (void) chdir_grabber(grabber, dir);
  else
    dir = grabber->dir;
  
  // Compose the full pathname of the grabber file.  Once we include
  // passing an accurate time stamp with the image, we should pass
  // this as the second argument, instead of NULL, so that the time
  // when the image was taken will be incorporated into the file name.

  path = arc_path_name(dir, time, ARC_GRAB_FILE);
  if(!path)
    return 1;
  
  // Attempt to open the new data file.

  fp = fopen(path, "wb");
  if(!fp) {
    lprintf(stderr, "Unable to open grabber file: %s\n", path);
    free(path);
    return 1;
  };
  
  // Close the current grabber file if open.

  close_grabfile(grabber);
  
  // Install the new grabber file.

  grabber->path = path;
  grabber->fp = fp;
  
  // Report the successful opening of the file.

  lprintf(stdout, "Opening grabber file: %s\n", path);
  
  return 0;
}

/**.......................................................................
 * Change the directory in which subsequent grabber files will be written.
 *
 * Input:
 *  dir      char *   The directory to use when creating subsequent
 *                    grabber files.
 * Output:
 *  return    int     0 - OK.
 *                    1 - Error.
 */
static int chdir_grabber(Grabber *grabber, char *dir)
{
  
  // Make a copy of the name of the current grabber directory.

  if(dir != grabber->dir && *dir != '\0') {
    size_t bytes = strlen(dir)+1;
    char *tmp = (char* )(grabber->dir ? realloc(grabber->dir, bytes) : 
			 malloc(bytes));
    if(!tmp) {
      lprintf(stderr, "Unable to record new grabber directory.\n");
      return 1;
    } else {
      strcpy(tmp, dir);
      grabber->dir = tmp;
    };

  };

  return 0;
}

/**.......................................................................
 * Write the latest image to disk.
 */
static int grabber_write_image(Grabber *grabber)
{
  int status;        /* The status return value of a pthread function */
  
  // Get convenient aliases of the image buffer and output buffer.

  ImageBuffer *im = &grabber->im;
  NetBuf *net = grabber->net;
  ArcTimeStamp time;
  
  // Acquire exclusive access to the image buffer.

  if((status=pthread_mutex_lock(&im->guard))) {
    lprintf(stderr, "grabber_write_image (mutex_lock): %s.\n",
	    strerror(status));
    return 1;
  };
  
  // Construct an output record of the image.  Note that the return
  // status is ignored until exclusive access to the buffer has been
  // released further below.

  net->nget = net->nput = 0;
  status |= net_put_fitshead(net, im->utc);
  status |= net_put_short(net, GRABBER_IM_SIZE, im->image);
  
  // Set the date when this image was taken so that we can open a file
  // with the appropriate name.

  time.mjd = im->utc[0];
  time.sec = im->utc[1]/1000; /* Convert from milliseconds to seconds. */
  
  // Prepare the image buffer for the next image.

  im->save = 1;
  
  // Signal other threads that the image-buffer is now ready for a new
  // image.

  pthread_cond_signal(&im->start);
  
  // Relinquish exclusive access to the buffer.

  pthread_mutex_unlock(&im->guard);
  
  // Having safely released exclusive access to the image buffer,
  // abort if there was an error.

  if(status)
    return 1;
  
  // If archiving, attempt to open a new grabber file here.

  if(grabber->archive)
    if(open_grabfile(grabber, grabber->dir, &time))
      return 1;
  
  // Do we have an grabber file to save to, and should we save this
  // image?

  if(grabber->fp && grabber->archive) {
    if((int)fwrite(net->buf, sizeof(net->buf[0]), net->nput, grabber->fp) 
       != net->nput) {

      lprintf(stderr, "Error writing grabber data file (%s).\n",
	      strerror(errno));
      close_grabfile(grabber);
      return 1;
    };

    
    // And close the grabfile, since we will open a new one on the
    // next receipt of an image.

    if(grabber->fp)
      close_grabfile(grabber);

  };

  return 0;
}

/**.......................................................................
 * Copy the latest image from the control thread image buffer and tell
 * the grabber thread to save it to disk.
 */
int grabber_save_image(ControlProg *cp, unsigned short *image, 
		       unsigned int utc[2], unsigned short channel, 
		       signed actual[3]) 
{
  COUT("IUnside save_grabber_image()");
  Grabber *grabber = cp_Grabber(cp);
  ImageBuffer *im  = &grabber->im;
  int status;        /* The status return value of a pthread function */
  int ix,iy,indFrom, indTo,ind,ixmax=0,iymax=0;
  int first=1;
  int ixmid,iymid;
  unsigned short max=0;
  double fpix;

  grabber->currentImage_ = &grabber->imageHandlers_->at(channel);

  grabber->currentImage_->installNewImage(image); 
  grabber->currentImage_->getStats();

  // Gain exclusive access to the image buffer.
  
  if((status=pthread_mutex_lock(&im->guard))) {
    lprintf(stderr, "arc_integrate_frame (mutex_lock): %s.\n",
	    strerror(status));
    return 1;
  };
  
  // If a save isn't currently in progress wait for the grabber to
  // release the buffer.
  
  while(!im->save) {
    status = pthread_cond_wait(&im->start, &im->guard);
    if(status) {
      lprintf(stderr, "grabber_integrate_frame (cond_wait): %s.\n",
	      strerror(status));
      return grabber_image_error(im);
    };
  };
  
  // Now copy the control program image to the grabber image buffer.
  
  for(unsigned i=0; i < grabber->currentImage_->imageToArchive_.size(); i++)
    grabber->im.image[i] = grabber->currentImage_->imageToArchive_[i];
  
  // Copy the utc.
  
  grabber->im.utc[0] = utc[0];
  grabber->im.utc[1] = utc[1];
  
  // Read the actual position from the archiver thread
  
  if(grabber->actual_ == 0) {
    grabber->actual_ = arcFindActualReg(cp_Archiver(cp), 
					"antenna0");
    
    if(grabber->actual_ == 0)
      ThrowError("Unable to look up register antenna0.tracker.actual");
  }
  
  grabber->im.actual[0] = *(grabber->actual_+0);
  grabber->im.actual[1] = *(grabber->actual_+1);
  grabber->im.actual[2] = *(grabber->actual_+2);
  
  // And queue the latest image for archiving.
  
  {
    GrabberMessage msg;
    im->save = 1;
    if(pack_grabber_image(&msg) ||
       send_GrabberMessage(cp, &msg, PIPE_WAIT) != PIPE_OK)
      return grabber_image_error(im);
  };
  
  // And send the scheduler thread a command telling it that the new
  // image has been acquired.
  
  {
    SchedulerMessage msg;

    if(pack_scheduler_grab_done(&msg, grabber->im.seq) ||
       send_SchedulerMessage(cp, &msg, PIPE_WAIT) != PIPE_OK)
      return grabber_image_error(im);
  };
  
  // Relinquish exclusive access to the image buffer.
  
  pthread_mutex_unlock(&im->guard);

  return 0;
}

/**.......................................................................
 * Return the offset of the peak in x and y, converted to horizontal
 * and vertical offsets, in degrees
 */
void grabber_offset_info(ControlProg *cp, 
			 gcp::util::Angle& xoff, 
			 gcp::util::Angle& yoff, 
			 unsigned& ipeak, unsigned& jpeak,
			 unsigned& chanMask)
{
  Grabber *grabber = cp_Grabber(cp);
  gcp::util::ImageHandler* image = 0;

  if(chanMask==(unsigned)gcp::grabber::Channel::NONE) {
    image = grabber->currentImage_;
  } else {
    unsigned ichan = gcp::grabber::Channel::channelToInt((gcp::grabber::Channel::FgChannel)chanMask);
    image = &grabber->imageHandlers_->at(ichan);
  }

  image->getOffsetInfo(xoff, yoff, ipeak, jpeak);

  // Reset the channel mask, so that it contains the valid channel id
  // of the current image channel of Channel::NONE was passed on input

  chanMask = image->channel_;
}

/**.......................................................................
 * Return the requested statistic about the peak value of the frame
 * grabber image.
 */
void grabber_peak_info(ControlProg *cp, double& peak, double& snr,
		       unsigned& chanMask)
{
  Grabber *grabber = cp_Grabber(cp);
  gcp::util::ImageHandler* image = 0;

  if(chanMask==(unsigned)gcp::grabber::Channel::NONE) {
    image = grabber->currentImage_;
  } else {
    unsigned ichan = gcp::grabber::Channel::channelToInt((gcp::grabber::Channel::FgChannel)chanMask);
    image = &grabber->imageHandlers_->at(ichan);
  }

  gcp::util::ImageHandler::ImStat stat = image->getStats();

  peak = stat.max_;
  snr  = stat.snr_;

  // Reset the channel mask, so that it contains the valid channel id
  // of the current image channel of Channel::NONE was passed on input

  chanMask = image->channel_;
}

/**.......................................................................
 * Public method to set the sense of the deck angle rotation
 */
int setDeckAngleRotationSense(ControlProg* cp, RotationSense sense, 
			      unsigned chanMask)
{
  Grabber *grabber = cp_Grabber(cp);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) 
      ih->setDeckAngleRotationSense(sense);
  }

  return sendDeckAngleRotationSense(cp, sense, chanMask);
}

/**.......................................................................
 * Method to send the optical camera ximdir
 */
static int sendDeckAngleRotationSense(ControlProg* cp, RotationSense sense, 
				      unsigned chanMask)
{
  CcPipeMsg pmsg;
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask        = CFG_DKROTSENSE;
  pmsg.msg.grabber.dkRotSense  = sense;
  pmsg.msg.grabber.channelMask = chanMask;

  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to set the grabber COMBINE
 */
int setGrabberCombine(ControlProg* cp, unsigned combine, unsigned chanMask) 
{
  Grabber *grabber = cp_Grabber(cp);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) 
      ih->setNCombine(combine);
  }

  return sendGrabberCombine(cp, combine, chanMask);
}

/**.......................................................................
 * Public method to set the grabber COMBINE
 */
static int sendGrabberCombine(ControlProg* cp, unsigned combine, 
			      unsigned chanMask)
{
  CcPipeMsg pmsg;
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask        = CFG_COMBINE;
  pmsg.msg.grabber.nCombine    = combine;
  pmsg.msg.grabber.channelMask = chanMask;

  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to send the grabber FLATFIELD types
 */
int setGrabberFlatfieldType(ControlProg* cp, unsigned flatfield, 
			    unsigned chanMask)
{
  Grabber *grabber = cp_Grabber(cp);
  CcPipeMsg pmsg;
  
  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) 
      ih->setFlatfieldType(flatfield);
  }

  return sendGrabberFlatfieldType(cp, flatfield, chanMask);
}

/**.......................................................................
 * Public method to send the grabber FLATFIELD types
 */
static int sendGrabberFlatfieldType(ControlProg* cp, unsigned flatfield, 
				    unsigned chanMask)
{
  CcPipeMsg pmsg;
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask = CFG_FLATFIELD;
  pmsg.msg.grabber.flatfield   = flatfield;
  pmsg.msg.grabber.channelMask = chanMask;
  
  // Queue the command to be sent to connected control clients
  
  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to set the optical camera FOV
 */
int setOpticalCameraFov(ControlProg* cp, gcp::util::Angle& fov, 
			unsigned chanMask)
{
  Grabber *grabber = cp_Grabber(cp);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) 
      ih->setFov(fov);
  }

  return sendOpticalCameraFov(cp, fov, chanMask);
}

/**.......................................................................
 * Public method to reset the optical camera FOV
 */
int setOpticalCameraFov(ControlProg* cp, unsigned chanMask)
{
  gcp::util::Angle fov(gcp::util::ImageHandler::defaultFov_);
  return setOpticalCameraFov(cp, fov, chanMask);
}

/**.......................................................................
 * Public method to set the optical camera FOV
 */
static int sendOpticalCameraFov(ControlProg* cp, gcp::util::Angle& fov, 
				unsigned chanMask)
{
  CcPipeMsg pmsg;
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask = CFG_FOV;
  pmsg.msg.grabber.fov  = fov.arcmin();
  pmsg.msg.grabber.channelMask = chanMask;

  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to set the optical camera FOV
 */
int setOpticalCameraAspectRatio(ControlProg* cp, double aspect, 
				unsigned chanMask)
{
  Grabber *grabber = cp_Grabber(cp);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) 
      ih->setAspectRatio(aspect);
  }

  return sendOpticalCameraAspectRatio(cp, aspect, chanMask);
}

/**.......................................................................
 * Public method to set the optical camera ASPECT
 */
static int sendOpticalCameraAspectRatio(ControlProg* cp, double aspect, 
					unsigned chanMask)
{
  CcPipeMsg pmsg;
  
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask = CFG_ASPECT;
  pmsg.msg.grabber.aspect = aspect;
  pmsg.msg.grabber.channelMask = chanMask;

  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to set the optical camera collimation
 */
int setOpticalCameraRotationAngle(ControlProg* cp, unsigned chanMask)
{
  gcp::util::Angle angle(gcp::util::ImageHandler::defaultRotationAngle_);
  return setOpticalCameraRotationAngle(cp, angle, chanMask);
}

/**.......................................................................
 * Public method to set the optical camera collimation
 */
int setOpticalCameraRotationAngle(ControlProg* cp, 
				  gcp::util::Angle& collimation, 
				  unsigned chanMask)
{
  Grabber *grabber = cp_Grabber(cp);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) 
      ih->setRotationAngle(collimation);
  }

  return sendOpticalCameraRotationAngle(cp, collimation, chanMask);
}

/**.......................................................................
 * Public method to set the optical camera COLLIMATION
 */
static int sendOpticalCameraRotationAngle(ControlProg* cp, 
					  gcp::util::Angle& collimation,
					  unsigned chanMask)
{
  CcPipeMsg pmsg;
  
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask = CFG_COLLIMATION;
  pmsg.msg.grabber.collimation = collimation.degrees();
  pmsg.msg.grabber.channelMask = chanMask;

  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to set the optical camera X-image orientation
 */
int setOpticalCameraXImDir(ControlProg* cp, ImDir dir, unsigned chanMask)
{
  Grabber *grabber = cp_Grabber(cp);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) 
      ih->setXImDir(dir);
  }

  return sendOpticalCameraXImDir(cp, dir, chanMask);
}

/**.......................................................................
 * Method to send the optical camera ximdir
 */
static int sendOpticalCameraXImDir(ControlProg* cp, ImDir dir, unsigned chanMask)
{
  CcPipeMsg pmsg;
  
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask   = CFG_XIMDIR;
  pmsg.msg.grabber.ximdir = dir;
  pmsg.msg.grabber.channelMask = chanMask;

  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to set the optical camera Y-image orientation
 */
int setOpticalCameraYImDir(ControlProg* cp, ImDir dir, unsigned chanMask)
{
  Grabber *grabber = cp_Grabber(cp);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) 
      ih->setYImDir(dir);
  }

  return sendOpticalCameraYImDir(cp, dir, chanMask);
}

/**.......................................................................
 * Method to send the optical camera yximdir
 */
static int sendOpticalCameraYImDir(ControlProg* cp, ImDir dir, 
				   unsigned chanMask)
{
  CcPipeMsg pmsg;
  
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask = CFG_YIMDIR;
  pmsg.msg.grabber.yimdir = dir;
  pmsg.msg.grabber.channelMask = chanMask;
  
  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Send the current grabber config to all connected clients.
 */
int sendCurrentGrabberConfiguration(Grabber* grabber)
{
  CcPipeMsg pmsg;
  pmsg.id                      = CC_GRABBER_MSG;

  int waserr=0;
  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {

    pmsg.msg.grabber.mask = (CFG_ALL & 

			     ~CFG_PEAK_OFFSETS &  // No defaults for this

			     ~CFG_DKROTSENSE &    // Not for SPT

			     ~CFG_CHAN_ASSIGN &   // Assign on a
						  // per-channel basis

			     ~CFG_ADD_SEARCH_BOX);// Have to send
						  // these separately
						  // on a per-channel
						  // basis

    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);
  
    pmsg.msg.grabber.aspect      = ih->aspectRatio_;
    pmsg.msg.grabber.nCombine    = ih->nCombine_;
    pmsg.msg.grabber.flatfield   = ih->flatfieldType_;
    pmsg.msg.grabber.fov         = ih->fov_.arcmin();
    pmsg.msg.grabber.collimation = ih->rotationAngle_.degrees();
    pmsg.msg.grabber.ximdir      = ih->xImDir_;
    pmsg.msg.grabber.yimdir      = ih->yImDir_;
    pmsg.msg.grabber.dkRotSense  = ih->deckAngleRotationSense_;
    pmsg.msg.grabber.channelMask = (unsigned)ih->channel_;

    // See if there exists an association with a pointing telescope

    try {

      pmsg.msg.grabber.ptelMask = 
	(unsigned)gcp::util::PointingTelescopes::getSinglePtel(ih->channel_);

      pmsg.msg.grabber.mask |= CFG_CHAN_ASSIGN;
    } catch(gcp::util::Exception& err) {
    }

    // Queue the command to be sent to connected control clients

    waserr |= sendToCcClients(grabber->cp, &pmsg);

    // Send any search boxes associated with this channel
    
    pmsg.msg.grabber.mask = CFG_ADD_SEARCH_BOX;

    for(unsigned iBox=0; iBox < ih->boxes_.size(); iBox++) {

      COUT("Sending: " 
	   << ih->boxes_[iBox].ixmin_	
	   << " " << ih->boxes_[iBox].ixmax_		   
	   << " " << ih->boxes_[iBox].iymin_	
	   << " " << ih->boxes_[iBox].iymax_	
	   << " " << ih->boxes_[iBox].inc_);


      pmsg.msg.grabber.ixmin = ih->boxes_[iBox].ixmin_;	
      pmsg.msg.grabber.ixmax = ih->boxes_[iBox].ixmax_;	
      pmsg.msg.grabber.iymin = ih->boxes_[iBox].iymin_;	
      pmsg.msg.grabber.iymax = ih->boxes_[iBox].iymax_;	
      pmsg.msg.grabber.inc   = ih->boxes_[iBox].inc_;   

      // Queue the command to be sent to connected control clients

      waserr |= sendToCcClients(grabber->cp, &pmsg);
    }
    
  }

  return waserr;
}

/**.......................................................................
 * Public method to send peak offsets to viewer clients
 */
int sendPeakOffsets(ControlProg* cp, unsigned ipeak, unsigned jpeak, 
		    unsigned chanMask)
{
  CcPipeMsg pmsg;
  
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask = CFG_PEAK_OFFSETS;
  pmsg.msg.grabber.ipeak = ipeak;
  pmsg.msg.grabber.jpeak = jpeak;
  pmsg.msg.grabber.channelMask = chanMask;
  
  // Queue the command to be sent to connected control clients

  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to send a frame grabber channel - pointing telescope
 * association
 */
int sendFgChannelAssignment(ControlProg* cp, 
			    gcp::grabber::Channel::FgChannel chan,
			    gcp::util::PointingTelescopes::Ptel ptel)
{
  CcPipeMsg pmsg;
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask        = CFG_CHAN_ASSIGN;
  pmsg.msg.grabber.channelMask = (unsigned)chan;
  pmsg.msg.grabber.ptelMask    = (unsigned)ptel;

  gcp::util::PointingTelescopes::assignFgChannel(ptel, chan);

  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to set search box
 */
int setFrameGrabberSearchBox(ControlProg* cp, 
			     unsigned ixmin, unsigned iymin, 
			     unsigned ixmax, unsigned iymax, 
			     bool inc,
			     unsigned chanMask)
{
  Grabber *grabber = cp_Grabber(cp);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) {
      if(inc) {
	ih->addIncludeBox(ixmin, iymin, ixmax, iymax);
      } else {
	ih->addExcludeBox(ixmin, iymin, ixmax, iymax);
      }

    }
  }

  return sendFrameGrabberSearchBox(cp, 
				   ixmin, ixmax, iymin, iymax, inc, chanMask);
}

/**.......................................................................
 * Send a search box command to connected clients
 */
int sendFrameGrabberSearchBox(ControlProg* cp, 
			      unsigned ixmin, unsigned ixmax, 
			      unsigned iymin, unsigned iymax, 
			      bool inc,
			      unsigned chanMask)
{
  CcPipeMsg pmsg;
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask        = CFG_ADD_SEARCH_BOX;
  pmsg.msg.grabber.channelMask = (unsigned)chanMask;
  pmsg.msg.grabber.ixmin = ixmin;
  pmsg.msg.grabber.ixmax = ixmax;
  pmsg.msg.grabber.iymin = iymin;
  pmsg.msg.grabber.iymax = iymax;
  pmsg.msg.grabber.inc = inc;

  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to delete a search box
 */
int remFrameGrabberSearchBox(ControlProg* cp, 
			     unsigned ix, unsigned iy, 
			     unsigned chanMask)
{
  Grabber *grabber = cp_Grabber(cp);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) {
      ih->deleteNearestBox(ix, iy);
    }
  }

  CcPipeMsg pmsg;
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask        = CFG_REM_SEARCH_BOX;
  pmsg.msg.grabber.channelMask = (unsigned)chanMask;
  pmsg.msg.grabber.ixmin = ix;
  pmsg.msg.grabber.iymin = iy;

  return sendToCcClients(cp, &pmsg);
}

/**.......................................................................
 * Public method to delete all search boxes
 */
int remAllFrameGrabberSearchBoxes(ControlProg* cp, unsigned chanMask)
{
  Grabber *grabber = cp_Grabber(cp);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    gcp::util::ImageHandler* ih = &grabber->imageHandlers_->at(iChan);

    if(chanMask & ih->channel_) {
      ih->deleteAllBoxes();
    }
  }

  CcPipeMsg pmsg;
  pmsg.id = CC_GRABBER_MSG;
  pmsg.msg.grabber.mask        = CFG_REM_ALL_SEARCH_BOX;
  pmsg.msg.grabber.channelMask = (unsigned)chanMask;

  return sendToCcClients(cp, &pmsg);
}
