#ifndef OPTCAM_H
#define OPTCAM_H

/**
 * @file OptCam.h
 * 
 * Tagged: Thu Nov 13 16:53:59 UTC 2003
 * 
 * @author Erik Leitch
 */
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>  // Needed for FIONBIO

#include "gcp/control/code/unix/libunix_src/common/optcam.h"
#include "gcp/control/code/unix/libunix_src/common/netbuf.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/NetReadStr.h"
#include "gcp/util/common/NetSendStr.h"

#include "gcp/mediator/specific/OptCamMsg.h"

namespace gcp {
namespace mediator {
    
    /**
     * Forward declaration of Master lets us use it
     * without defining it.
     */
    class Master;
    
    /**
     * A class to encapsulate the transmission of frames from the
     * optical camera from the AC to the ACC.
     */
    class OptCam :
      public gcp::util::GenericTask<OptCamMsg> {
      
      public:
      
      /**
       * Destructor.
       */
      ~OptCam();
      
      /**
       * Return true when connected to the ACC
       */
      bool isConnected();
      
      private:
      
      /**
       *  will access private members of this class.
       */
      friend class Master;
      
      /**
       * A pointer to the resources of the parent task.
       */
      Master* master_;
      
      /**
       * The connection to the control program optcam port (-1
       * when not connected).
       */
      int fd_;               
      
      /**
       * The set of fds to listen for input from
       */
      fd_set read_fds_;      
      
      /**
       * The set of fds to watch for writeability
       */
      fd_set write_fds_;     
      
      /**
       * The max fd + 1 in read_fds
       */
      int fd_set_size_;      
      
      /**
       * The IP address of the control host
       */
      std::string host_;          
      
      /**
       * true if connected to the control port
       */
      bool connected_;       
      
      /**
       * true if connected to the control port
       */
      bool ready_;           
      
      /**
       * The TCP/IP network input stream for the optical telescope
       */
      gcp::util::NetReadStr* nrs_;      
      
      /**
       * The TCP/IP network output stream for the optical
       * telescope
       */
      gcp::util::NetSendStr* nss_;      
      
      /**
       * A temporary buffer for storing the current tracker
       * position
       */
      signed actual_[3];     
      
      /**
       * A temporary buffer for storing the requested tracker
       * position
       */
      signed expected_[3];   
      
      unsigned int utc_[2];
      /**
       * An external network buffer for sending frame grabber
       * images to the unix control program
       */
      unsigned char* image_net_buffer_;
      
      /**
       * A work array where we will store the processed image
       * until ready to send.
       */
      unsigned short* image_; 
      
      /**
       * Private constructor insures this can only be instantiated
       * by .
       */
      OptCam(Master* master);
      
      /**
       * Private method to register a file descriptor to be
       * watched for input.
       */
      void registerReadFd(int fd);
      
      /**
       * Private method to register a file descriptor to be
       * watched for output availability.
       */
      void registerWriteFd(int fd);
      
      /**
       * Remove a file descriptor from the mask of descriptors to
       * be watched for readability.
       */
      void clearFromReadFd(int fd);
      
      /**
       * Remove a file descriptor from the mask of descriptors to
       * be watched for writeability.
       */
      void clearFromWriteFd(int fd);	  
      
      /**
       * Read all or part of a control-program command from the
       * network.  If the command is completely received,
       * interpret it and prepare for the next command. Otherwise
       * postpone interpretting the message until a future call to
       * this function receives its completion.
       *
       * @throws Exception
       */
      void readCommand();
      
      /**
       * Randomly generate a field of stars
       */
      void fillImage();
      
      /**
       * Send an image from the "frame grabber"
       *
       * @throws Exception
       */
      void sendImage();
      
      /**
       * Connect to the optical camera port of the control program.
       */
      bool connect();
      
      /**
       * Disconnect from the control program.
       */
      void disconnect();
      
      /**
       * OptCam main event loop
       */
      void run();
      
      void processMsg(OptCamMsg* taskMsg);
      
    }; // End class OptCam
    
    
} // End namespace mediator
} // End namespace gcp

#endif // End #ifndef 
