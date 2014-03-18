#ifndef GCP_GRABBER_FRAMEGRABBER_H
#define GCP_GRABBER_FRAMEGRABBER_H

/**
 * @file FrameGrabber.h
 * 
 * Tagged: Wed Jul 14 17:58:02 UTC 2004
 * 
 * @author 
 */
#include <string>
#include <vector>

#if HAVE_VIDEO
#include <linux/types.h>
#include <linux/videodev.h>
#endif

// new ones
#include <iostream>
#include <iomanip>

#include <cmath>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/HourAngle.h"
#include "gcp/util/common/Declination.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include "gcp/pgutil/common/PgUtil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

//end of new ones

namespace gcp {
  namespace grabber {
    
    class FrameGrabber {
    public:
      
      /**
       * Constructors.
       */
      FrameGrabber();
      
      /**
       * Destructor.
       */
      virtual ~FrameGrabber();

      // new ones
      enum io_method {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
      };
      
      struct buffer {
        void   *start;
        size_t  length;
      };
      
      char            *dev_name;
      static enum io_method   io;
      static int              fd;
      struct buffer          *buffers;
      unsigned int     n_buffers;
      int              out_buf;
      int              force_format;
      static int              frame_count;
      // done with new vars
      // new functions calls

      void errno_exit(const char *s);
      int xioctl(int fh, int request, void *arg);
      void process_image(const void *p, int size);
      int read_frame(void);
      void init_read(unsigned int buffer_size);
      void stop_capturing(void);
      void start_capturing(void);		
      void uninit_device(void);
      void init_mmap(void);
      void init_userp(unsigned int buffer_size);
      void init_device(void);
      void close_device(void);
      void open_device(void);

#if HAVE_VIDEO
      void setDeviceFileName(std::string devFileName);
      void initialize();

      void queryProperties();
      void printProperties();

      void queryCapabilities();
      void printCapabilities();

      void queryWindow();
      void printWindow();
      void setWindow(unsigned short width = defaultWidth_, 
		     unsigned short height = defaultHeight_);

      void queryChannel(unsigned short channel);
      void printChannel(struct video_channel& cp);
      void setTunerToPal();

#endif

      void setChannel(unsigned short channel = defaultChannel_);
      void setImageSize(int width, int height);
      void getImage(std::vector<char> &theImage);

#if HAVE_VIDEO
      void queryImageBuffer();
      void printImageBuffer();

      void queryPicture();
      void printPicture();

      void setDepth(int depth=defaultPixelDepth_);
      void setBrightness(unsigned short brightness = defaultBrightness_);
      void setContrast(unsigned short contrast = defaultContrast_);

      void createCaptureBuffer();

      void queryTuner();
      void printTuner();

    private:
      
      static std::string defaultDevFileName_;
      static const unsigned short defaultWidth_      =   512;
      static const unsigned short defaultHeight_     =   480;
      static const int defaultPixelDepth_            =     8;
      static const unsigned short defaultBrightness_ = 32768;
      static const unsigned short defaultContrast_   = 27648;
#endif

      static const unsigned short defaultChannel_    =     1;

#if HAVE_VIDEO
      int fd_;
      int width_;
      int height_;
      void* imageBuffer_;
      
      std::string devFileName_;
      
      struct video_capability vc_;
      struct video_channel    cp_;
      struct video_picture    vp_;
      struct video_window     vw_;
      struct video_mbuf       vm_;
      struct video_mmap       vb_;
      struct video_tuner      vt_;
      struct v4l2_output     v4out_;

      void openFrameGrabber(std::string devFileName);
      
      void printImageBuffer(struct video_mbuf& vm);
      void printCapabilities(struct video_capability& vc);
      void printWindow(struct video_window& vw);
      void printPicture(struct video_picture& vp);
      void printTuner(struct video_tuner& vt);
#endif

    }; // End class FrameGrabber
    
  } // End namespace grabber
}; // End namespace gcp

#endif // End #ifndef GCP_GRABBER_FRAMEGRABBER_H
