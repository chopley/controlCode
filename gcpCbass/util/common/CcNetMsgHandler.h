#ifndef GCP_UTIL_CCNETMSGHANDLER_H
#define GCP_UTIL_CCNETMSGHANDLER_H

/**
 * @file CcCcNetMsgHandler.h
 * 
 * Tagged: Tue Mar 23 15:43:10 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/CcNetMsg.h"
#include "gcp/util/common/NetReadStr.h"
#include "gcp/util/common/NetSendStr.h"

namespace gcp {
  namespace util {
    
    class CcNetMsg;
    
    class CcNetMsgHandler {
    public:
      
      /**
       * Constructor.
       */
      CcNetMsgHandler();
      
      /**
       * Destructor.
       */
      virtual ~CcNetMsgHandler();
      
      /**
       * Attach the network I/O stream to a socket.
       */
      void attachReadStream(int fd);
      
      /**
       * Attach the network I/O stream to a socket.
       */
      void attachSendStream(int fd);
      
      /**
       * Send a message packed into our network buffer to a socket
       * described by a previously attached fd.
       */
      gcp::util::NetSendStr::NetSendId send();
      
      /**
       * Send a message to a socket described by a previously attached
       * fd.
       */
      gcp::util::NetSendStr::NetSendId 
	send(gcp::util::CcNetMsg* msg);
      
      /**
       * Send a message packed into our network buffer to the
       * specified socket.  
       */
      gcp::util::NetSendStr::NetSendId send(int fd);
      
      /**
       * Send a message to a socket
       */
      gcp::util::NetSendStr::NetSendId 
	send(int fd, gcp::util::CcNetMsg* msg);
      
      /**
       * Read a message into our network buffer from a socket
       * described by a previously attached fd.
       */
      gcp::util::NetReadStr::NetReadId read();
      
      /**
       * Read a message into our network buffer from the specified
       * socket.
       */
      gcp::util::NetReadStr::NetReadId read(int fd);
      
      /**
       * Pack a network message.
       */
      void packCcNetMsg(gcp::util::CcNetMsg* msg);
      
      /**
       * Return the last message read.
       */
      gcp::util::CcNetMsg* getCcNetMsg();
      
    private:
      
      gcp::util::CcNetMsg netMsg_;
      gcp::util::NetSendStr* nss_;
      gcp::util::NetReadStr* nrs_;
      
    }; // End class CcNetMsgHandler
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_CCNETMSGHANDLER_H
