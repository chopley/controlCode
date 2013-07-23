#define __FILEPATH__ "grabber/FrameGrabber.cc"

/**.......................................................................
 * FrameGrabber Device class definition.
 *
 * <dl><dt><b>Author </b></dt><dd>Colby Gutierrez-Kraybill</dl>
 * $Revision: 1.2 $
 * $Date: 2010/03/04 21:17:26 $
 * $Id: FrameGrabber.cc,v 1.2 2010/03/04 21:17:26 sjcm Exp $
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

#if HAVE_VIDEO
string FrameGrabber::defaultDevFileName_("/dev/video0");
#endif

/**.......................................................................
 * Default constructor
 */
FrameGrabber::FrameGrabber()
{
#if HAVE_VIDEO
  initialize();
#endif
}

/**.......................................................................
 * Destructor
 */
FrameGrabber::~FrameGrabber() {}

#if HAVE_VIDEO
/**.......................................................................
 * Install a frame grabber device
 */
void FrameGrabber::setDeviceFileName(string devFileName)
{
  openFrameGrabber(devFileName);
}

void FrameGrabber::initialize()
{
  setDeviceFileName(defaultDevFileName_);

  //  queryProperties();

  setChannel(); 
#if(1)
  setDepth();
  setBrightness();
  setContrast();
  setWindow();
  createCaptureBuffer();
#endif
  setTunerToPal();//  -- not necessary. problem was with the driver
  printProperties();
}

/**.......................................................................
 * Open the frame grabber device
 */
void FrameGrabber::openFrameGrabber(string devFileName)
{
  devFileName_ = devFileName;
  fd_   = open(devFileName_.c_str(), O_RDWR);
  
  if (fd_ == -1 ) {
    ThrowSysError("Unable to open frame grabber device file: " 
		  << devFileName_);
  } else {
    COUT("Successfully opened frame grabber");
  }
}

/**.......................................................................
 * Set the effective image size of the captured image
 */
void FrameGrabber::setWindow(unsigned short width, unsigned short height)
{
#if(0)
  COUT("WIDTH_: " << width_);
  COUT("height_: " << height_);
  COUT("WIDTH: " << width);
  COUT("height: " << height);
#endif

  vw_.x      = vw_.y   = 0;
  vw_.width  = width_  = width;
  vw_.height = height_ = height;
  vw_.chromakey = 0;
  vw_.flags     = 0;
  vw_.clips     = NULL;
  vw_.clipcount = 0;
  
  errno = 0;
  if (ioctl( fd_, VIDIOCSWIN, &vw_) == -1) {
    ThrowSysError("ioctl()");
  }
}

#endif
/**.......................................................................
 * Set the channel
 */
void FrameGrabber::setChannel(unsigned short channel)
{
#if HAVE_VIDEO

  if(cp_.channel != channel) {

    cp_.channel = channel;
    COUT("cp_.channel: " << channel);

    /* despite what cp_ seems, norm is what you want to set as NTSC or
       PAL */
    cp_.norm = VIDEO_MODE_PAL;

    if (ioctl( fd_,  VIDIOCSCHAN, &cp_) == -1) {
      ThrowSysError("ioctl()");
    }
    
    struct timespec ts;
    ts.tv_sec  = 5;
    ts.tv_nsec = 0;
    
    nanosleep(&ts, NULL);
  }
#endif
}

#if HAVE_VIDEO
/**.......................................................................
 * Create an mmapped area where captured images will be stored.
 */
void FrameGrabber::createCaptureBuffer ()
{
  imageBuffer_ = NULL;
  
  errno=0;
  if(ioctl( fd_, VIDIOCGMBUF, &vm_ ) == -1) {
    ThrowSysError("ioctl()");
  }
  
  errno=0;
  imageBuffer_ = mmap(0, vm_.size, PROT_READ|PROT_WRITE, MAP_SHARED, 
		      fd_, 0);
  
  if (reinterpret_cast<long int>(imageBuffer_) == -1) {
    ThrowSysError("mmap()");
  }
}
#endif

/**.......................................................................
 * Digitize and return the next image from the frame grabber
 */
void FrameGrabber::getImage( vector<char> &returnImage )
{
#if HAVE_VIDEO
  void *rawBuffer=0;
  vb_.frame  = 0;
  vb_.format = VIDEO_PALETTE_GREY;
  vb_.width  = width_;
  vb_.height = height_;

  int imageSize = width_*height_*((15)>>3);

  //  nanosleep(&ts, NULL);
  char* image = (char*)malloc (imageSize);
  if(image==0){
    COUT("MALLOC FAILED");
  }

  int bytesReceived = (read (fd_, image, imageSize));
  COUT("BYTES REC: " << bytesReceived);
  COUT("IMGSIZE:  "  << imageSize);
  if ( bytesReceived != imageSize){
    COUT("DID NOT READ CORRECT AMOUNT OF DATA");
  } else {
  }

  returnImage.resize(vw_.width * vw_.height); // works for char vector
  rawBuffer = &returnImage[0];
  
  memcpy(rawBuffer, image, vw_.width*vw_.height);

  free(image);

#if(0)
  errno = 0;
  if (ioctl(fd_, VIDIOCMCAPTURE, &vb_) == -1) {
    ThrowSysError("ioctl()");
  }
  errno = 0;
  if ( ioctl( fd_, VIDIOCSYNC, &vb_ ) == -1 ) {
    ThrowSysError("ioctl()");
  }
  
  returnImage.resize(vw_.width * vw_.height); // works for char vector
  
  rawBuffer = &returnImage[0];
  
  memcpy(rawBuffer, imageBuffer_, vw_.width*vw_.height);
#endif  
#endif
}

/**.......................................................................,
 * Set the image size
 */
void FrameGrabber::setImageSize(int width, int height)
{
#if HAVE_VIDEO
  setWindow(width, height);
#endif
}

#if HAVE_VIDEO
/**.......................................................................
 * Query the capabilities of the video card
 */
void FrameGrabber::queryCapabilities()
{
  errno = 0;
  if (ioctl( fd_, VIDIOCGCAP, &vc_ ) == -1 ) {
    ThrowSysError("ioctl()");
  }
}
void FrameGrabber::printCapabilities()
{
  printCapabilities(vc_);
}

void FrameGrabber::printCapabilities(struct video_capability& vc)
{
  std::ostringstream os;

  // Format capabilities

  os << "Name: " << vc.name << endl;
  os << "Channels (video/audio): " << vc.channels << "/" << vc.audios << endl;
  os << "Max Dimensions: " << vc.maxwidth << "x" << vc.maxheight << endl;
  os << "Min Dimensions: " << vc.minwidth << "x" << vc.minheight << endl;
  os << "Video Capture: " << (vc.type & VID_TYPE_CAPTURE ? "" : "Not ") 
     << "Supported." << endl;
  os << "Video Tuner: " << (vc.type & VID_TYPE_TUNER ? "" : "Not ") 
     << "Supported." << endl;
  os << "Teletext: " << (vc.type & VID_TYPE_TELETEXT ? "" : "Not ") 
     << "Supported." << endl;
  os << "Video Overlay: " << (vc.type & VID_TYPE_OVERLAY ? "" : "Not ") 
     << "Supported." << endl;
  os << "Chromakey Overlay: " << (vc.type & VID_TYPE_CHROMAKEY ? "" : "Not ") 
     << "Supported." << endl;
  os << "Video Clipping: " << (vc.type & VID_TYPE_CLIPPING ? "" : "Not ") 
     << "Supported." << endl;
  os << "Use Framebuffer: " << (vc.type & VID_TYPE_FRAMERAM ? "" : "Not ") 
     << "Supported." << endl;
  os << "Scaleable: " << (vc.type & VID_TYPE_SCALES ? "" : "Not ") 
     << "Supported." << endl;
  os << "Monochrome Only: " << (vc.type & VID_TYPE_MONOCHROME ? "Yes" : "No") 
     << endl;
  os << "Subarea Capture: " << (vc.type & VID_TYPE_SUBCAPTURE ? "" : "Not ")
     << "Supported." << endl;
  os << "Raw Type Bitfield: 0x" << hex << vc.type << dec << endl;
  os << endl;

  COUT(os.str());
}

/**.......................................................................
 * Query the capabilities of the video card
 */
void FrameGrabber::queryPicture()
{
  errno = 0;
  if (ioctl( fd_, VIDIOCGPICT, &vp_ ) == -1 ) {
    ThrowSysError("ioctl()");
  }
}

void FrameGrabber::printPicture()
{
  printPicture(vp_);
}

void FrameGrabber::printPicture(struct video_picture& vp)
{
  COUT("Picture properties: " << std::endl << std::endl
       << "Brightness/Contrast: " << vp.brightness << "/" 
       << vp.contrast << std::endl
       << "Depth:               " << vp.depth << " bits." << std::endl
       << "Pallette:            " << vp.palette << std::endl);
}

void FrameGrabber::setTunerToPal()
{
  queryTuner();
  vt_.flags = VIDEO_TUNER_PAL;
  vt_.mode = VIDEO_MODE_PAL;

  errno = 0;
  if (ioctl( fd_, VIDIOCSFREQ, &vt_ ) == -1 ) {
    ThrowSysError("ioctl()");
  }

}

/**.......................................................................
 * Query the capabilities of the video tuner
 */
void FrameGrabber::queryTuner()
{
  errno = 0;
  if (ioctl( fd_, VIDIOCGFREQ, &vt_ ) == -1 ) {
    ThrowSysError("ioctl()");
  }
}

void FrameGrabber::printTuner()
{
  printTuner(vt_);
}

void FrameGrabber::printTuner(struct video_tuner& vt)
{
  COUT("Tuner properties: " << std::endl << std::endl
       << "Name:        " << vt.name      << std::endl
       << "Range Low:   " << vt.rangelow  << std::endl
       << "Range High:  " << vt.rangehigh << std::endl
       << "Flags:       " << vt.flags     << std::endl
       << "Flags & PAL: " << (vt.flags&VIDEO_TUNER_PAL)   << std::endl
       << "Flags & NTSC:" << (vt.flags&VIDEO_TUNER_NTSC)  << std::endl
       << "Mode:        " << vt.mode);

}

/**.......................................................................
 * Query the capabilities of the video card
 */
void FrameGrabber::queryWindow()
{
  errno = 0;
  if (ioctl( fd_, VIDIOCGWIN, &vw_ ) == -1 ) {
    ThrowSysError("ioctl()");
  }
}

void FrameGrabber::printWindow()
{
  printWindow(vw_);
}

void FrameGrabber::printWindow(struct video_window& vw)
{
  COUT(std::endl 
       << "Video window: " << std::endl << std::endl
       << "x:         " << vw.x << std::endl
       << "y:         " << vw.y << std::endl
       << "width:     " << vw.width << std::endl
       << "height:    " << vw.height << std::endl
       << "chromakey: " << vw.chromakey << std::endl
       << "flags:     " << vw.flags << std::endl
       << "clipcount: " << vw.clipcount << std::endl);

}

/**.......................................................................
 * Query the capabilities of the video card
 */
void FrameGrabber::queryChannel(unsigned short channel)
{
  errno = 0;
  if (ioctl( fd_, VIDIOCGCHAN, &cp_ ) == -1 ) {
    ThrowSysError("ioctl()");
  }
}

void FrameGrabber::printChannel(struct video_channel& cp)
{
  COUT(std::endl 
       << "Video channel: " << std::endl << std::endl
       << "channel: " << cp_.channel << std::endl
       << "name:    " << cp_.name << std::endl
       << "tuners:  " << cp_.tuners << std::endl
       << "flags:   " << cp_.flags << std::endl
       << "type:    " << cp_.type << std::endl
       << "flags:   " << cp_.flags << std::endl
       << "norm:    " << cp_.norm << std::endl);
}

void FrameGrabber::queryImageBuffer()
{
  errno = 0;

  if(!(ioctl(fd_, VIDIOCGMBUF, &vm_ ) > -1)) 
    ThrowSysError("ioctl()");
}

void FrameGrabber::printImageBuffer()
{
  printImageBuffer(vm_);
}

void FrameGrabber::printImageBuffer(struct video_mbuf& vm)
{
  std::ostringstream os;

  os << std::endl << "Video mbuf: " << std::endl << std::endl
     << "size:         " << vm.size << std::endl
     << "frames:       " << vm.frames << std::endl
     << "offsets:      " << std::endl;

    for(unsigned iFrame=0; iFrame < vm.frames; iFrame++)
      os << "              " << vm.offsets[iFrame] << std::endl;;

    os << std::endl;

    COUT(os.str());
}

void FrameGrabber::queryProperties()
{
  queryCapabilities();
  queryWindow();
  queryImageBuffer();
  queryPicture();
  queryTuner();
}

void FrameGrabber::printProperties()
{
  printCapabilities(vc_);
  printWindow(vw_);
  printImageBuffer(vm_);
  printPicture(vp_);
  printTuner(vt_);
}

void FrameGrabber::setDepth(int depth)
{
  if(!(ioctl( fd_, VIDIOCGPICT, &vp_) > -1))
    ThrowSysError("ioctl()");
        
  vp_.palette = VIDEO_PALETTE_GREY;
  vp_.depth = depth; 

  if(!(ioctl( fd_, VIDIOCSPICT, &vp_ ) > -1))
    ThrowSysError("ioctl()");
}

void FrameGrabber::setBrightness(unsigned short brightness)
{
  if(!(ioctl( fd_, VIDIOCGPICT, &vp_) > -1))
    ThrowError("ioctl()");
        
  vp_.brightness = brightness; // Change only the brightness

  if(!(ioctl( fd_, VIDIOCSPICT, &vp_) > -1))
    ThrowError("ioctl()");
}

void FrameGrabber::setContrast(unsigned short contrast)
{
  if(!(ioctl( fd_, VIDIOCGPICT, &vp_) > -1))
    ThrowError("ioctl()");
        
  vp_.contrast = contrast; // Change only the contrast

  if(!(ioctl( fd_, VIDIOCSPICT, &vp_) > -1))
    ThrowError("ioctl()");
}

#endif

