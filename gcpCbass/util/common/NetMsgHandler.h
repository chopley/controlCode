#ifndef GCP_UTIL_NETMSGHANDLER_H
#define GCP_UTIL_NETMSGHANDLER_H

/**
 * @file NetMsgHandler.h
 * 
 * Tagged: Mon Mar 15 18:24:26 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetHandler.h"
#include "gcp/util/common/NetMsg.h"

namespace gcp {
  namespace util {
    
    class NetMsg;
    
    class NetMsgHandler : public NetHandler {
    public:
      
      /**
       * Constructor.
       */
      NetMsgHandler();
      
      /**
       * Destructor.
       */
      virtual ~NetMsgHandler();
      
      /**
       * Send a message to a socket described by a previously attached
       * fd.
       */
      gcp::util::NetSendStr::NetSendId 
	sendNetMsg(gcp::util::NetMsg* msg);
      
      /**
       * Send a message to a socket
       */
      gcp::util::NetSendStr::NetSendId 
	sendNetMsg(int fd, gcp::util::NetMsg* msg);
      
      /**
       * Pack a network message into the network buffer.
       */
      void packNetMsg(gcp::util::NetMsg* msg);
      
      /**
       * Pack a new-style network message into our send buffer.
       */
      bool packNewRtcNetMsg(NetMsg* msg);

      /**
       * Pack a greeting message
       */
      void packGreetingMsg(unsigned int antenna);
      
      /**
       * Pack an antenna id message
       */
      void packAntennaIdMsg(unsigned int antenna);
      
      /**
       * Read a net message out of the network buffer
       */
      void readNetMsg();
      
      /**
       * Return the last message read.
       */
      gcp::util::NetMsg* getLastReadNetMsg();
      gcp::util::NetMsg* getLastSentNetMsg();
      
      /**
       * Overload the base-class methods to install user-defined handlers
       */
      void installReadHandler(NET_READ_HANDLER(*handler), void* arg);
      void installSendHandler(NET_SEND_HANDLER(*handler), void* arg);
      void installErrorHandler(NET_ERROR_HANDLER(*handler), void* arg);
      
    private:
      
      /**
       * The GCP array map
       */
      ArrayMap* arraymap_;     
      
      /**
       * The last read message
       */
      gcp::util::NetMsg lastReadNetMsg_;
      gcp::util::NetMsg lastSentNetMsg_;
      
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
      
    }; // End class NetMsgHandler
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_NETMSGHANDLER_H
