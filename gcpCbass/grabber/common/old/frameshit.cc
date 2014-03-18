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
  fd = -1;
  io = IO_METHOD_MMAP;
  frame_count = 1;

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

#if(0)
  setDeviceFileName(defaultDevFileName_);

  queryProperties();

  setChannel(); 
  setDepth();
  setBrightness();
  setContrast();
  setWindow();
  createCaptureBuffer();
  setTunerToPal(); // -- not necessary. problem was with the driver
  printProperties();

#endif
  open_device();
  init_device();

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
  COUT("WIDTH_: " << width_);
  COUT("height_: " << height_);
  COUT("WIDTH: " << width);
  COUT("height: " << height);

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
    COUT("cp_1.channel: " << channel);

    /* despite what cp_ seems, norm is what you want to set as NTSC or
       PAL */
    cp_.norm = VIDEO_MODE_PAL;

    if (ioctl( fd_,  VIDIOCSCHAN, &cp_) == -1) {
      ThrowSysError("ioctl()");
    }

    struct timespec ts;
    ts.tv_sec  = 10;
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


  fd_set fds;
  struct timeval tv;
  int r;
  
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  
  /* Timeout. */
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  
  r = select(fd + 1, &fds, NULL, NULL, &tv);
  
  if (-1 == r) {
    if (EINTR == errno)
    errno_exit("select");
  }
  
  if (0 == r) {
    fprintf(stderr, "select timeout\n");
    exit(EXIT_FAILURE);
  }

  int out;
  //  COUT("ABOUT TO CALL READ_FRAME");
  //  out = read_frame();
  


  void *rawBuffer=0;
  vb_.frame  = 0;
  vb_.format = VIDEO_PALETTE_GREY;
  vb_.width  = width_;
  vb_.height = height_;

  int imageSize = width_*height_*((15)>>3);

  COUT("here1a");
  //  nanosleep(&ts, NULL);
  char* image = (char*)malloc (imageSize);
  if(image==0){
    COUT("MALLOC FAILED");
  }

  int bytesReceived = (read (fd_, image, imageSize));
  COUT("BYTES REC: " << bytesReceived);
  COUT("IMGSIZE:  "  << imageSize);
  if ( bytesReceived != imageSize){
  } else {
    COUT("IT READ SOMETHING!!");
  }

  returnImage.resize(vw_.width * vw_.height); // works for char vector

  rawBuffer = &returnImage[0];
  
  memcpy(rawBuffer, image, vw_.width*vw_.height);

  free(image);

#if(0)    
  
  if (ioctl(fd_, VIDIOC_QBUF, &vb_) == -1) {
    ThrowSysError("ioctl()");
  }
  COUT("here1b");  

  if (ioctl(fd_, VIDIOCMCAPTURE, &v4b_) == -1) {
    ThrowSysError("ioctl()");
  }
  COUT("here1b");  

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

void FrameGrabber::errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

int FrameGrabber::xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

void FrameGrabber::process_image(const void *p, int size)
{
#if 1

  std::vector<float> data(768*480);

  if(size != 768 * 480) {
    COUT("Size is " << size << " expected " << 768*480);
  }

  char* cptr = (char*)p;
  for(unsigned i=0; i < data.size(); i++) {
    data[i] = (float) ((int)(cptr[i]));
  }
  int crap;
  crap = PgUtil::v_grey2((int) data.size(), &data[0], 768, 480);
#endif
        if (out_buf)
                fwrite(p, size, 1, stdout);

        fflush(stderr);
        fprintf(stderr, ".");
        fflush(stdout);
}

int FrameGrabber::read_frame(void)
{
  struct v4l2_buffer buf;
  unsigned int i;
  
  switch (io) {
  case IO_METHOD_READ:
    if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
      switch (errno) {
      case EAGAIN:
	return 0;
	
      case EIO:
	/* Could ignore EIO, see spec. */
	
	/* fall through */
	
      default:
	errno_exit("read");
      }
    }
    
    process_image(buffers[0].start, buffers[0].length);
    break;
    
  case IO_METHOD_MMAP:
    CLEAR(buf);
    
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    
    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
      switch (errno) {
      case EAGAIN:
	return 0;
	
      case EIO:
	/* Could ignore EIO, see spec. */
	
	/* fall through */

      default:
	errno_exit("VIDIOC_DQBUF");
      }
    }
    
    assert(buf.index < n_buffers);
    
    process_image(buffers[buf.index].start, buf.bytesused);
    
    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
      errno_exit("VIDIOC_QBUF");
    break;
    
  case IO_METHOD_USERPTR:
    CLEAR(buf);
    
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;
    
    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
      switch (errno) {
      case EAGAIN:
	return 0;
	
      case EIO:
	/* Could ignore EIO, see spec. */
	
	/* fall through */
	
      default:
	errno_exit("VIDIOC_DQBUF");
      }
    }
    
    for (i = 0; i < n_buffers; ++i)
      if (buf.m.userptr == (unsigned long)buffers[i].start
	  && buf.length == buffers[i].length)
	break;
    
    assert(i < n_buffers);
    
    process_image((void *)buf.m.userptr, buf.bytesused);
    
    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
      errno_exit("VIDIOC_QBUF");
    break;
  }
  
  return 1;
}

#if(0)
FrameGrabber:: void mainloop(void)
{
        unsigned int count;

        count = frame_count;

        while (count-- > 0) {
                for (;;) {
                        fd_set fds;
                        struct timeval tv;
                        int r;

                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 2;
                        tv.tv_usec = 0;

                        r = select(fd + 1, &fds, NULL, NULL, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;
                                errno_exit("select");
                        }

                        if (0 == r) {
                                fprintf(stderr, "select timeout\n");
                                exit(EXIT_FAILURE);
                        }

                        if (read_frame())
                                break;
                        /* EAGAIN - continue select loop. */
                }
        }
}
#endif

void FrameGrabber::stop_capturing(void)
{
        enum v4l2_buf_type type;

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
                        errno_exit("VIDIOC_STREAMOFF");
                break;
        }
}

void FrameGrabber::start_capturing(void)
{
        unsigned int i;
        enum v4l2_buf_type type;

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i) {
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i) {
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_USERPTR;
                        buf.index = i;
                        buf.m.userptr = (unsigned long)buffers[i].start;
                        buf.length = buffers[i].length;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;
        }
}

void FrameGrabber::uninit_device(void)
{
        unsigned int i;

        switch (io) {
        case IO_METHOD_READ:
                free(buffers[0].start);
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i)
                        if (-1 == munmap(buffers[i].start, buffers[i].length))
                                errno_exit("munmap");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i)
                        free(buffers[i].start);
                break;
        }

        free(buffers);
}

void FrameGrabber::init_read(unsigned int buffer_size)
{
  buffers = (struct buffer*)calloc(1, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        buffers[0].length = buffer_size;
        buffers[0].start = malloc(buffer_size);

        if (!buffers[0].start) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }
}

void FrameGrabber::init_mmap(void)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "memory mapping\n", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf(stderr, "Insufficient buffer memory on %s\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        buffers = (struct buffer*)calloc(req.count, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}

void FrameGrabber::init_userp(unsigned int buffer_size)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count  = 4;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "user pointer i/o\n", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        buffers = (struct buffer*)calloc(4, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;
                buffers[n_buffers].start = malloc(buffer_size);

                if (!buffers[n_buffers].start) {
                        fprintf(stderr, "Out of memory\n");
                        exit(EXIT_FAILURE);
                }
        }
}

void FrameGrabber::init_device(void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
        unsigned int min;

        if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is no video capture device\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        switch (io) {
        case IO_METHOD_READ:
                if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                        fprintf(stderr, "%s does not support read i/o\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                }
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                        fprintf(stderr, "%s does not support streaming i/o\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                }
                break;
        }


        /* Select video input, video standard and tune here. */


        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {
                /* Errors ignored. */
        }


        CLEAR(fmt);

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (force_format) {
                fmt.fmt.pix.width       = 640;
                fmt.fmt.pix.height      = 480;
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
                fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

                if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                        errno_exit("VIDIOC_S_FMT");

                /* Note VIDIOC_S_FMT may change width and height. */
        } else {
                /* Preserve original settings as set by v4l2-ctl for example */
                if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
                        errno_exit("VIDIOC_G_FMT");
        }

        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;

	COUT("Bytes perline = " << fmt.fmt.pix.bytesperline << " height = " << fmt.fmt.pix.height);

        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        switch (io) {
        case IO_METHOD_READ:
                init_read(fmt.fmt.pix.sizeimage);
                break;

        case IO_METHOD_MMAP:
                init_mmap();
                break;

        case IO_METHOD_USERPTR:
                init_userp(fmt.fmt.pix.sizeimage);
                break;
        }
}

void FrameGrabber::close_device(void)
{
        if (-1 == close(fd))
                errno_exit("close");

        fd = -1;
}

void FrameGrabber::open_device(void)
{
        struct stat st;

        if (-1 == stat(dev_name, &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no device\n", dev_name);
                exit(EXIT_FAILURE);
        }

        fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

#if(0)
static void usage(FILE *fp, int argc, char **argv)
{
        fprintf(fp,
                 "Usage: %s [options]\n\n"
                 "Version 1.3\n"
                 "Options:\n"
                 "-d | --device name   Video device name [%s]\n"
                 "-h | --help          Print this message\n"
                 "-m | --mmap          Use memory mapped buffers [default]\n"
                 "-r | --read          Use read() calls\n"
                 "-u | --userp         Use application allocated buffers\n"
                 "-o | --output        Outputs stream to stdout\n"
                 "-f | --format        Force format to 640x480 YUYV\n"
                 "-c | --count         Number of frames to grab [%i]\n"
                 "",
                 argv[0], dev_name, frame_count);
}

static const char short_options[] = "d:hmruofc:";

static const struct option
long_options[] = {
        { "device", required_argument, NULL, 'd' },
        { "help",   no_argument,       NULL, 'h' },
        { "mmap",   no_argument,       NULL, 'm' },
        { "read",   no_argument,       NULL, 'r' },
        { "userp",  no_argument,       NULL, 'u' },
        { "output", no_argument,       NULL, 'o' },
        { "format", no_argument,       NULL, 'f' },
        { "count",  required_argument, NULL, 'c' },
        { 0, 0, 0, 0 }
};

int Program::main()
{
        dev_name = "/dev/video0";

#if 0
        for (;;) {
                int idx;
                int c;

                c = getopt_long(argc, argv,
                                short_options, long_options, &idx);

                if (-1 == c)
                        break;

                switch (c) {
                case 0: /* getopt_long() flag */
                        break;

                case 'd':
                        dev_name = optarg;
                        break;

                case 'h':
                        usage(stdout, argc, argv);
                        exit(EXIT_SUCCESS);

                case 'm':
                        io = IO_METHOD_MMAP;
                        break;

                case 'r':
                        io = IO_METHOD_READ;
                        break;

                case 'u':
                        io = IO_METHOD_USERPTR;
                        break;

                case 'o':
                        out_buf++;
                        break;

                case 'f':
                        force_format++;
                        break;

                case 'c':
                        errno = 0;
                        frame_count = strtol(optarg, NULL, 0);
                        if (errno)
                                errno_exit(optarg);
                        break;

                default:
                        usage(stderr, argc, argv);
                        exit(EXIT_FAILURE);
                }
        }
#endif

        open_device();
        init_device();

#if 0
	//	stop_capturing();
	uninit_device();
#else
        start_capturing();
        mainloop();
        stop_capturing();
        uninit_device();
#endif
        close_device();
        fprintf(stderr, "\n");
        return 0;
}
#endif
