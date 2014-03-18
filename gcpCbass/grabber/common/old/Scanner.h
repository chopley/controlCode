#ifndef GCP_GRABBER_SCANNER_H
#define GCP_GRABBER_SCANNER_H

/**
 * @file Scanner.h
 * 
 * Tagged: Mon Jul 12 05:46:38 UTC 2004
 * 
 * @author 
 */
#include <valarray>

#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/NetStr.h"
#include "gcp/util/common/TcpClient.h"
#include "gcp/util/common/TimeOut.h"

#include "gcp/grabber/common/Image.h"
#include "gcp/grabber/common/ScannerMsg.h"

namespace gcp {
  namespace grabber {
    
    class FrameGrabber;
    class Master;
    
    class Scanner :
      public gcp::util::GenericTask<ScannerMsg> {
      
      public:
      
      /**
       * Constructor.
       */
      Scanner(Master* parent, bool simulate);
      
      /**
       * Destructor.
       */
      virtual ~Scanner();
      
      private:
      
      // A vector of image objects maintained by this task

      std::vector<Image> images_;

      // Which image we are currently integrating

      unsigned count_;

      unsigned channel_;

      // An object that will handle timeouts for the Scanner task. 

      gcp::util::TimeOut timeOut_;

      // A mask of channels to sample on each interval

      unsigned intervalMask_;

      // When sendImages() is called, this mask will record which images
      // remain to be sent

      unsigned remainMask_;
      bool storeAsFlatfield_;

      //  Master will access private members of this class.

      friend class Master;
      
      // A private pointer to the parent class

      Master* parent_;
      
      /**
       * The frame grabber object
       */
      FrameGrabber* fg_;
      
      /**
       * If true, simulate the frame grabber hardware
       */
      bool simulate_;
      
      /**
       * A buffer where we will store the image just received from the
       * frame grabber
       */
      std::valarray<unsigned short> image_;
      
      /**
       * An object to manage our connection to the control program
       */
      gcp::util::TcpClient client_;
      
      /**
       * The TCP/IP network stream we will use to communicate with
       * the array control program.
       */
      gcp::util::NetStr* netStr_;       
      
      /**
       * Parse the greeting message.
       */
      void parseGreeting();
      
      /**
       * Connect to the master port
       */
      bool connect();        
      
      /**
       * Disconnect from the Grabber port
       */
      void disconnect();     
      
      /**
       * Attempt to connect to the host
       */
      void connectScanner(bool reEnable);
      
      /**
       * Disconnect from the host.
       */
      void disconnectScanner();
      
      /**
       * Process a message received on our message queue
       *
       * @throws Exception
       */
      void processMsg(ScannerMsg* taskMsg);
      
      /**
       * Service our message queue.
       */
      void serviceMsgQ();
      
      /**
       * A method to send a connection status message to the master.
       */
      void sendScannerConnectedMsg(bool connected);
      
      /**
       * A handler to be called when a message has been completely read
       */
      static NET_READ_HANDLER(netMsgReadHandler);
      
      /**.......................................................................
       * A handler to be called when a message has been completely sent
       */
      static NET_SEND_HANDLER(imageSentHandler);
      
      /**.......................................................................
       * A handler to be called when an error occurs in communication
       */
      static NET_ERROR_HANDLER(netErrorHandler);
      
      /**.......................................................................
       * Get an image from the frame grabber
       */
      void addGrabberImage(Image& image);

      /**
       * Pack an image into our network buffer.
       */
      void packImage(Image& image);
      
      // Send multiple sequential images

      void sendImages(unsigned chanMask, bool storeAsFlatfield);
      
      // Send an image

      void sendNextImage();
      void sendImage(Image& image, bool storeAsFlatfield);
      
    }; // End class Scanner
    
  } // End namespace grabber
}; // End namespace gcp




#endif // End #ifndef GCP_GRABBER_SCANNER_H
