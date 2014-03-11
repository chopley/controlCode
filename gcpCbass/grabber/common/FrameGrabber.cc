/**.......................................................................
 * FrameGrabber Device class definition.
 */
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <string.h>

#include <string>
#include <sstream>
#include <vector>

#include <errno.h>

#include "gcp/util/common/Exception.h"

#include "gcp/grabber/common/FrameGrabber.h"

using namespace gcp::grabber;
using namespace gcp::util;
using namespace std;

string FrameGrabber::defaultDevFileName_("/dev/video0");

/**.......................................................................
 * Default constructor
 */
FrameGrabber::FrameGrabber(FrameGrabber::Standard stand)
{
#if HAVE_VIDEO
  initialize(stand);
#endif
}

/**.......................................................................
 * Destructor
 */
FrameGrabber::~FrameGrabber() 
{
  unmapCaptureBuffer();

  if(fd_ > 0) {
    close(fd_);
    fd_ = 0;
  }
}

/**.......................................................................
 * Install a frame grabber device
 */
void FrameGrabber::setDeviceFileName(string devFileName)
{
  openFrameGrabber(devFileName);
}

/**.......................................................................
 * Initialize internal members
 */
void FrameGrabber::initialize(FrameGrabber::Standard stand)
{
  fd_ = -1;
  channel_ = -1;
  initializeCaptureBuffer();

  setDeviceFileName(defaultDevFileName_);

  queryProperties();
  setChannel(); 
  setStandard(stand);
  setDepth();
  setBrightness();
  setContrast();
  setImageSize();
}

/**.......................................................................
 * Open the frame grabber device
 */
void FrameGrabber::openFrameGrabber(string devFileName)
{
#if HAVE_VIDEO
  devFileName_ = devFileName;
  fd_   = open(devFileName_.c_str(), O_RDWR);
  
  if (fd_ == -1 ) {
    ThrowSysError("Unable to open frame grabber device file: " 
		  << devFileName_);
  } else {
    COUT("Successfully opened frame grabber");
  }
#endif
}

/**.......................................................................
 * Set the effective image size of the captured image
 */
void FrameGrabber::setWindow(unsigned short width, unsigned short height)
{
#if HAVE_VIDEO
#ifdef V4L2
  enum v4l2_priority prio;
  ioctlThrow(VIDIOC_G_PRIORITY, &prio, "Unable to query priority");

  struct v4l2_format fmt;
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ioctlThrow(VIDIOC_G_FMT, &fmt, "Unable to get image format");

  fmt.fmt.pix.width  = width;
  fmt.fmt.pix.height = height;

  ioctlThrow(VIDIOC_S_FMT, &fmt, "Unable to set image resolution");
#else
  vw_.x         = vw_.y   = 0;
  vw_.width     = width;
  vw_.height    = height;
  vw_.chromakey = 0;
  vw_.flags     = 0;
  vw_.clips     = NULL;
  vw_.clipcount = 0;
  
  ioctlThrow(VIDIOCSWIN, &vw_, "Unable to set window");
#endif
#endif

  //------------------------------------------------------------
  // And store the image size internally
  //------------------------------------------------------------

  width_  = width;
  height_ = height;
}

unsigned FrameGrabber::getStandard(FrameGrabber::Standard stand)
{
#if HAVE_VIDEO
#ifdef V4L2

  switch(stand) {
  case STAND_NTSC:
    return V4L2_STD_NTSC;
    break;
  case STAND_PAL:
    return V4L2_STD_PAL;
    break;
  default:
    return V4L2_STD_SECAM;
    break;
  }

#else

  switch(stand) {
  case STAND_NTSC:
    return VIDEO_MODE_NTSC;
    break;
  case STAND_PAL:
    return VIDEO_MODE_PAL;
    break;
  default:
    return VIDEO_MODE_SECAM;
    break;
  }

#endif
#endif

  return 0;
}

/**.......................................................................
 * Set the video standard
 */
void FrameGrabber::setStandard(Standard stand)
{
#if HAVE_VIDEO
#ifdef V4L2

  v4l2_std_id std = getStandard(stand);
  ioctlThrow(VIDIOC_S_STD, &std, "Unable to set standard");

#else

  ioctlThrow(VIDIOCGCHAN, &cp_, "Unable to get channel/standard");
  cp_.norm = getStandard(stand);
  ioctlThrow(VIDIOCSCHAN, &cp_, "Unable to set channel/standard");

#endif
#endif
}


/**.......................................................................
 * Set the channel
 */
void FrameGrabber::setChannel(int channel)
{
  if(channel == channel_) 
    return;

#if HAVE_VIDEO
#ifdef V4L2
  ioctlThrow(VIDIOC_S_INPUT, &channel, "Unable to set channel");
#else
  ioctlThrow(VIDIOCGCHAN, &cp_, "Unable to get channel/standard");
  cp_.channel = channel;
  ioctlThrow(VIDIOCSCHAN, &cp_, "Unable to set channel/standard");
#endif
#endif

  channel_ = channel;
}

/**.......................................................................
 * Initialize image capture buffers
 */
void FrameGrabber::initializeCaptureBuffer()
{
  imageBuffer_ = 0;
  bufLen_ = 0;

  unsigned nBuf = 4;

  captureBuffers_.resize(nBuf);
  for(unsigned i=0; i < nBuf; i++)
    captureBuffers_[i] = 0;
}

/**.......................................................................
 * Create the buffer into which we will capture images
 */
void FrameGrabber::createCaptureBuffer()
{
#if HAVE_VIDEO
#ifdef V4L2
  createMmapBuffers(captureBuffers_, bufLen_);
#else
  imageBuffer_ = createMmapBuffer(bufLen_);
#endif
#endif
}

/**.......................................................................
 * Create the mmap'd buffer we will use to read back images from the
 * grabber
 */
void* FrameGrabber::createMmapBuffer(unsigned& bufLen)
{
  void* retval = NULL;

#if HAVE_VIDEO
#ifndef V4L2
  ioctlThrow(VIDIOCGMBUF, &vm_, "Unable to query image grab buffer");
  retval = mmap(0, vm_.size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, 0);
  bufLen = vm_.size;
#endif
  
  if(reinterpret_cast<long int>(retval) == -1) {
    ThrowSysError("mmap()");
  }
#endif

  return retval;
}

/**.......................................................................
 * Create the mmap'd buffer we will use to read back images from the
 * grabber
 */
void FrameGrabber::createMmapBuffers(std::vector<void*>& buffers, unsigned& bufLen)
{
#if HAVE_VIDEO
#ifdef V4L2
  v4l2_requestbuffers rb;
  rb.count  = buffers.size();
  rb.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  rb.memory = V4L2_MEMORY_MMAP;

  ioctlThrow(VIDIOC_REQBUFS, &rb, "Unable to request capture buffers: ");

  //------------------------------------------------------------
  // Now memory map the buffers
  //------------------------------------------------------------

  for(unsigned i=0; i < buffers.size(); i++) {

    struct v4l2_buffer b;

    b.index  = i;
    b.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    b.memory = V4L2_MEMORY_MMAP;
    
    ioctlThrow(VIDIOC_QUERYBUF, &b, "Unable to query capture buffer");
    
    buffers[i] = mmap(0, b.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, b.m.offset);

    bufLen = b.length;
  }
#endif
#endif
}

/**.......................................................................
 * Unmap any memory-mapped capture buffers
 */
void FrameGrabber::unmapCaptureBuffer()
{
#if HAVE_VIDEO
#ifdef V4L2
  for(unsigned i=0; i < captureBuffers_.size(); i++) {
    if(captureBuffers_[i]) {
      munmap(captureBuffers_[i], bufLen_);
      captureBuffers_[i] = 0;
    }
  }
#else
  if(imageBuffer_) {
    munmap(imageBuffer_, bufLen_);
    imageBuffer_ = 0;
  }
#endif
#endif
}

/**.......................................................................
 * Digitize and return the next image from the frame grabber
 */
void FrameGrabber::getImage(vector<char> &returnImage)
{
#if HAVE_VIDEO
  startCapture();
  unsigned index = syncFrame();
  stopCapture();

  //------------------------------------------------------------
  // Return the image into the supplied buffer  
  //------------------------------------------------------------

  returnImage.resize(width_ * height_); // works for char vector

#ifdef V4L2
  void* vptr = captureBuffers_[index];
#else
  void* vptr = imageBuffer_;
#endif

  //------------------------------------------------------------
  // Now memcpy into the resized buffer
  //------------------------------------------------------------

  memcpy(&returnImage[0], vptr, width_*height_);
#endif
}

/**.......................................................................
 * Stop capture of a frame
 */
void FrameGrabber::stopCapture()
{
#if HAVE_VIDEO
#ifdef V4L2
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ioctlThrow(VIDIOC_STREAMOFF, &type, "Unable to turn the capture stream off");
#else
  return;
#endif
#endif
}

/**.......................................................................
 * Initiate capture of a single frame
 */
unsigned FrameGrabber::syncFrame()
{
  unsigned retval = 0;
#if HAVE_VIDEO
#ifdef V4L2

  waitForDevice();

  struct v4l2_buffer b;
  b.index = 0;
  b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  b.memory = V4L2_MEMORY_MMAP;

  ioctlThrow(VIDIOC_DQBUF, &b, "Unable to dequeue a buffer: ");

  retval = b.index;
#else
  void *rawBuffer=0;
  vb_.frame  = 0;
  vb_.format = VIDEO_PALETTE_GREY;
  vb_.width  = width_;
  vb_.height = height_;

  ioctlThrow(VIDIOCSYNC, &vb_, "Capture sync, " );
  retval = 0;
#endif
#endif
  return retval;
}

/**.......................................................................
 * Initiate capture of a single frame
 */
void FrameGrabber::startCapture()
{
#if HAVE_VIDEO
#ifdef V4L2
  for(unsigned i=0; i < captureBuffers_.size(); i++) {
    struct v4l2_buffer b;
    b.index  = i;
    b.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    b.memory = V4L2_MEMORY_MMAP;

    ioctlThrow(VIDIOC_QBUF, &b, "Unable to queue a buffer");
  }

  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ioctlThrow(VIDIOC_STREAMON, &type, "Unable to turn the capture stream on");
#else
  vb_.frame  = 0;
  vb_.format = VIDEO_PALETTE_GREY;
  vb_.width  = width_;
  vb_.height = height_;
  
  ioctlThrow(VIDIOCMCAPTURE, &vb_, "Unable to start capture");
#endif
#endif
}

/**.......................................................................,
 * Set the image size
 */
void FrameGrabber::setImageSize(int width, int height)
{
#if HAVE_VIDEO
  unmapCaptureBuffer();
  setWindow(width, height);
  createCaptureBuffer();
#endif
}

/**.......................................................................
 * Query the capabilities of the video card
 */
void FrameGrabber::queryCapabilities()
{
#if HAVE_VIDEO
#ifdef V4L2
  return;
#else
  ioctlThrow(VIDIOCGCAP, &vc_, "Unable to query capabilities");
#endif
#endif
}

void FrameGrabber::printCapabilities()
{
#if HAVE_VIDEO
#ifndef V4L2
  std::ostringstream os;

  // Format capabilities

  os << "Name: " << vc_.name << endl;
  os << "Channels (video/audio): " << vc_.channels << "/" << vc_.audios << endl;
  os << "Max Dimensions: " << vc_.maxwidth << "x" << vc_.maxheight << endl;
  os << "Min Dimensions: " << vc_.minwidth << "x" << vc_.minheight << endl;
  os << "Video Capture: " << (vc_.type & VID_TYPE_CAPTURE ? "" : "Not ") 
     << "Supported." << endl;
  os << "Video Tuner: " << (vc_.type & VID_TYPE_TUNER ? "" : "Not ") 
     << "Supported." << endl;
  os << "Teletext: " << (vc_.type & VID_TYPE_TELETEXT ? "" : "Not ") 
     << "Supported." << endl;
  os << "Video Overlay: " << (vc_.type & VID_TYPE_OVERLAY ? "" : "Not ") 
     << "Supported." << endl;
  os << "Chromakey Overlay: " << (vc_.type & VID_TYPE_CHROMAKEY ? "" : "Not ") 
     << "Supported." << endl;
  os << "Video Clipping: " << (vc_.type & VID_TYPE_CLIPPING ? "" : "Not ") 
     << "Supported." << endl;
  os << "Use Framebuffer: " << (vc_.type & VID_TYPE_FRAMERAM ? "" : "Not ") 
     << "Supported." << endl;
  os << "Scaleable: " << (vc_.type & VID_TYPE_SCALES ? "" : "Not ") 
     << "Supported." << endl;
  os << "Monochrome Only: " << (vc_.type & VID_TYPE_MONOCHROME ? "Yes" : "No") 
     << endl;
  os << "Subarea Capture: " << (vc_.type & VID_TYPE_SUBCAPTURE ? "" : "Not ")
     << "Supported." << endl;
  os << "Raw Type Bitfield: 0x" << hex << vc_.type << dec << endl;
  os << endl;

  COUT(os.str());
#endif
#endif
}

/**.......................................................................
 * Query the capabilities of the video card
 */
void FrameGrabber::queryPicture()
{
#if HAVE_VIDEO
#ifndef V4L2
  ioctlThrow(VIDIOCGPICT, &vp_, "Unable to query picture");
#endif
#endif
}

void FrameGrabber::printPicture()
{
#if HAVE_VIDEO
#ifndef V4L2
  COUT("Picture properties: " << std::endl << std::endl
       << "Brightness/Contrast: " << vp_.brightness << "/" 
       << vp_.contrast << std::endl
       << "Depth:               " << vp_.depth << " bits." << std::endl
       << "Pallette:            " << vp_.palette << std::endl);
#endif
#endif
}

/**.......................................................................
 * Query the capabilities of the video card
 */
void FrameGrabber::queryWindow()
{
#if HAVE_VIDEO
#ifdef V4L2
  struct v4l2_cropcap cc;
  cc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ioctlThrow(VIDIOC_CROPCAP, &cc, "Unable to query cropping limits: ");
  COUT("Available cropping bounds are: " << cc.bounds.width << " x " << cc.bounds.height);
#else
  ioctlThrow(VIDIOCGWIN, &vw_, "Unable to query grab window");
#endif
#endif
}

void FrameGrabber::printWindow()
{
#if HAVE_VIDEO
#ifndef V4L2
  COUT(std::endl 
       << "Video window: " << std::endl << std::endl
       << "x:         " << vw_.x << std::endl
       << "y:         " << vw_.y << std::endl
       << "width:     " << vw_.width << std::endl
       << "height:    " << vw_.height << std::endl
       << "chromakey: " << vw_.chromakey << std::endl
       << "flags:     " << vw_.flags << std::endl
       << "clipcount: " << vw_.clipcount << std::endl);
#endif
#endif
}

/**.......................................................................
 * Query the capabilities of the video card
 */
int FrameGrabber::queryChannel()
{
#if HAVE_VIDEO
#ifdef V4L2
  int channel=0;
  ioctlThrow(VIDIOC_G_INPUT, &channel, "Unable to query channel");
  return channel;
#else
  ioctlThrow(VIDIOCGCHAN, &cp_, "Unable to query channel");
  return cp_.channel;
#endif
#endif
}

void FrameGrabber::printChannel()
{
#if HAVE_VIDEO
#ifndef V4L2
  COUT(std::endl 
       << "Video channel: " << std::endl << std::endl
       << "channel: " << cp_.channel << std::endl
       << "name:    " << cp_.name << std::endl
       << "tuners:  " << cp_.tuners << std::endl
       << "flags:   " << cp_.flags << std::endl
       << "type:    " << cp_.type << std::endl
       << "flags:   " << cp_.flags << std::endl
       << "norm:    " << cp_.norm << std::endl);
#endif
#endif
}

void FrameGrabber::queryImageBuffer()
{
#if HAVE_VIDEO
#ifndef V4L2
  ioctlThrow(VIDIOCGMBUF, &vm_, "Unable to query image buffer");
#endif
#endif
}

void FrameGrabber::printImageBuffer()
{
#if HAVE_VIDEO
#ifndef V4L2
  std::ostringstream os;

  os << std::endl << "Video mbuf: " << std::endl << std::endl
     << "size:         " << vm_.size << std::endl
     << "frames:       " << vm_.frames << std::endl
     << "offsets:      " << std::endl;

    for(unsigned iFrame=0; iFrame < vm_.frames; iFrame++)
      os << "              " << vm_.offsets[iFrame] << std::endl;;

    os << std::endl;

    COUT(os.str());
#endif
#endif
}

void FrameGrabber::queryProperties()
{
  queryCapabilities();
  queryWindow();
  queryImageBuffer();
}

void FrameGrabber::printProperties()
{
  printCapabilities();
  printWindow();
  printImageBuffer();
  printPicture();
}

void FrameGrabber::setDepth(int depth)
{
#if HAVE_VIDEO
#ifdef V4L2
  //------------------------------------------------------------
  // Under V4L2, the depth is implied by the format and is no longer
  // explicitly settable.  Palette has moved to the v4l2_pix_format
  // struct
  //------------------------------------------------------------

  struct v4l2_format fmt;
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ioctlThrow(VIDIOC_G_FMT, &fmt, "Unable to get image format");

  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
  fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

  ioctlThrow(VIDIOC_S_FMT, &fmt, "Unable to set image format");
#else

  ioctlThrow(VIDIOCGPICT, &vp_, "Unable to query image controls");
        
  vp_.palette = VIDEO_PALETTE_GREY;
  vp_.depth = depth; 

  ioctlThrow(VIDIOCSPICT, &vp_, "Unable to set depth/palette");
#endif
#endif
}

/**.......................................................................
 * Change the brightness settings on the device (if capable)
 */
void FrameGrabber::setBrightness(unsigned short brightness)
{
#if HAVE_VIDEO
#ifdef V4L2
  struct v4l2_queryctrl qCtl;
  qCtl.id = V4L2_CID_BRIGHTNESS;
  ioctlThrow( VIDIOC_QUERYCTRL, &qCtl, "Unable to query brightness control: " );
  
  if(brightness < qCtl.minimum || brightness > qCtl.maximum)
    ThrowError("Requested brightness is out of range for this device");

  struct v4l2_control ctl;
  ctl.id = V4L2_CID_BRIGHTNESS;
  ctl.value = brightness;
  ioctlThrow( VIDIOC_S_CTRL, &ctl, "Unable to set brightness: " );
#else
  ioctlThrow(VIDIOCGPICT, &vp_, "Unable to get brightness settings");
  vp_.brightness = brightness; // Change only the brightness
  ioctlThrow(VIDIOCSPICT, &vp_, "Unable to set brightness");
#endif
#endif
}

/**.......................................................................
 * Change the contrast settings on the device (if capable)
 */
void FrameGrabber::setContrast(unsigned short contrast)
{
#if HAVE_VIDEO
#ifdef V4L2
  struct v4l2_queryctrl qCtl;
  qCtl.id = V4L2_CID_CONTRAST;
  ioctlThrow(VIDIOC_QUERYCTRL, &qCtl, "Unable to query constrast control");
  
  if(contrast < qCtl.minimum || contrast > qCtl.maximum)
    ThrowError("Requested contrast is out of range for this device");

  struct v4l2_control ctl;
  ctl.id = V4L2_CID_CONTRAST;
  ctl.value = contrast;
  ioctlThrow(VIDIOC_S_CTRL, &ctl, "Unable to set contrast");
#else
  ioctlThrow(VIDIOCGPICT, &vp_, "Unable to get contrast settings");
  vp_.contrast = contrast; // Change only the contrast
  ioctlThrow(VIDIOCSPICT, &vp_, "Unable to set contrast");
#endif
#endif
}


void FrameGrabber::ioctlThrow(int request, void* argp, std::string message)
{
  errno = 0;
  if(ioctl(fd_, request, argp) == -1) {
    ThrowSysError(message << ": ioctl()");
  }
}

/**.......................................................................
 * Wait until the device is readable
 */
void FrameGrabber::waitForDevice() 
{
#if HAVE_VIDEO
#ifdef V4L2
  fd_set fdSet;
  FD_ZERO(&fdSet);
  FD_SET(fd_, &fdSet);
  
  //------------------------------------------------------------
  // Wait until the file descriptor becomes readable
  //------------------------------------------------------------
  
  struct timeval tv;
  tv.tv_sec  = 2;
  tv.tv_usec = 0;

  int nready = select(fd_+1, &fdSet, NULL, NULL, &tv);

  if(nready < 0) {
    ThrowError("Error waiting for device to return a capture buffer");
  }

  if(nready == 0) {
    ThrowError("Timed out waiting for device to return a capture buffer");
  }
#endif
#endif
}
