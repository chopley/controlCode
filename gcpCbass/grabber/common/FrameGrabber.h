#ifndef GCP_GRABBER_FRAMEGRABBER_H
#define GCP_GRABBER_FRAMEGRABBER_H

/**
 * @file FrameGrabber.h
 * 
 * Tagged: Wed Jul 14 17:58:02 UTC 2004
 * 
 * @author Erik Leitch
 */
#include <string>
#include <vector>

//#define V4L2

#if HAVE_VIDEO

#include <linux/types.h>
#include <linux/videodev.h>

#ifdef V4L2
#include <linux/videodev2.h>
#endif

#endif

namespace gcp {
  namespace grabber {
    
    class FrameGrabber {
    public:
      
      enum Standard {
	STAND_UNKNOWN = 0x0,
	STAND_NTSC    = 0x1,
	STAND_PAL     = 0x2,
	STAND_SECAM   = 0x4,
      };

      /**
       * Constructors.
       */
      FrameGrabber(FrameGrabber::Standard stand=STAND_PAL);
      
      /**
       * Destructor.
       */
      virtual ~FrameGrabber();
      
      void setDeviceFileName(std::string devFileName);

      void queryProperties();
      void printProperties();

      void queryCapabilities();
      void printCapabilities();

      void queryWindow();
      void printWindow();

      void setStandard(Standard stand);

      void setChannel(int channel = defaultChannel_);
      int queryChannel();
      void printChannel();

      void setImageSize(int width = defaultWidth_, 
			int height = defaultHeight_);

      void getImage(std::vector<char> &theImage);

      void queryImageBuffer();
      void printImageBuffer();

      void queryPicture();
      void printPicture();

      void setDepth(int depth=defaultPixelDepth_);
      void setBrightness(unsigned short brightness = defaultBrightness_);
      void setContrast(unsigned short contrast = defaultContrast_);

      void createCaptureBuffer();

    private:
      
      static std::string defaultDevFileName_;
      static const unsigned short defaultWidth_      =   768;
      static const unsigned short defaultHeight_     =   480;
      static const int defaultPixelDepth_            =     8;
      static const unsigned short defaultBrightness_ = 32768;
      static const unsigned short defaultContrast_   = 27648;
      static const unsigned short defaultChannel_    =     1;

      int fd_;
      int width_;
      int height_;
      void* imageBuffer_;
      std::vector<void*> captureBuffers_;
      unsigned bufLen_;
      int channel_;

      std::string devFileName_;
      
#if HAVE_VIDEO
#ifndef V4L2
      struct video_capability vc_;
      struct video_channel    cp_;
      struct video_picture    vp_;
      struct video_window     vw_;
      struct video_mbuf       vm_;
      struct video_mmap       vb_;
#endif
#endif
      
      void openFrameGrabber(std::string devFileName);
      void initialize(Standard stand);
      unsigned getStandard(FrameGrabber::Standard stand);

      void setWindow(unsigned short width = defaultWidth_, 
		     unsigned short height = defaultHeight_);

      //------------------------------------------------------------
      // Private methods for handling mmap buffers
      //------------------------------------------------------------

      void initializeCaptureBuffer();
      void* createMmapBuffer(unsigned& bufLen);
      void  createMmapBuffers(std::vector<void*>& buffers, unsigned& bufLen);
      void unmapCaptureBuffer();

      //------------------------------------------------------------
      // Private methods for capturing a frame
      //------------------------------------------------------------

      void startCapture();
      unsigned syncFrame();
      void stopCapture();

      //------------------------------------------------------------
      // Utility method for issuing ioctl commands
      //------------------------------------------------------------

      void ioctlThrow(int request, void* argp, std::string message);
      void waitForDevice();

    }; // End class FrameGrabber
    
  } // End namespace grabber
}; // End namespace gcp

#endif // End #ifndef GCP_GRABBER_FRAMEGRABBER_H
