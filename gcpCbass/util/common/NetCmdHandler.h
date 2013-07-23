#ifndef GCP_UTIL_NETCMDHANDLER_H
#define GCP_UTIL_NETCMDHANDLER_H

/**
 * @file NetCmdHandler.h
 * 
 * Started: Thu Feb 26 16:21:06 UTC 2004
 * 
 * @author Erik Leitch
 */

// Shared control code includes

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

#include "gcp/util/common/NetHandler.h"
#include "gcp/util/common/NetCmd.h"

namespace gcp {
  namespace util {
    
    class NetCmdHandler : public NetHandler {
    public:
      
      /**
       * Constructor.
       */
      NetCmdHandler();
      
      /**
       * Destructor.
       */
      virtual ~NetCmdHandler();
      
      /**
       * Pack a network command into our send buffer.;
       */
      void packNetCmd(gcp::util::NetCmd* rtc);
      
      /**
       * Pack a command into our send buffer.
       */
      void packNetCmd(gcp::control::RtcNetCmd* rtc, 
		      gcp::control::NetCmdId opcode);
      
      /**
       * Return the network message we just read.
       */
      gcp::util::NetCmd* getLastReadNetCmd();
      
      /**
       * Of the one we just sent
       */
      gcp::util::NetCmd* getLastSentNetCmd();
      
      /**
       * Overload the base-class methods to install user-defined handlers
       */
      void installReadHandler(NET_READ_HANDLER(*handler), void* arg);
      void installSendHandler(NET_SEND_HANDLER(*handler), void* arg);
      void installErrorHandler(NET_ERROR_HANDLER(*handler), void* arg);
      
    private:
      
      /**
       * The last read command.
       */
      gcp::util::NetCmd lastReadNetCmd_;
      
      /**
       * The last sent command.
       */
      gcp::util::NetCmd lastSentNetCmd_;
      
      /**
       * Read a NetCmd out of the network buffer
       */
      void readNetCmd();
      
      /**
       * A handler to be called when a message has been completely read.
       */
      static NET_READ_HANDLER(readHandler);
      
      // A pointer to a user-defined handler
      
      NET_READ_HANDLER(*userReadHandler_);
      void* userReadArg_;
      
      /**
       * A handler to be called when a message has been completely sent.
       */
      static NET_SEND_HANDLER(sendHandler);
      
      // A pointer to a user-defined handler
      
      NET_SEND_HANDLER(*userSendHandler_);
      void* userSendArg_;
      
      /**
       * A handler to be called when an error has occurred
       */
      static NET_ERROR_HANDLER(errorHandler);
      
      // A pointer to a user-defined handler
      
      NET_ERROR_HANDLER(*userErrorHandler_);
      void* userErrorArg_;
      
    }; // End class NetCmdHandler
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef 


