#ifndef GCP_UTIL_NETCOMMHANDLER_H
#define GCP_UTIL_NETCOMMHANDLER_H

/**
 * @file NetCommHandler.h
 * 
 * Tagged: Wed Mar 17 01:07:59 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/NetMsgHandler.h"
#include "gcp/util/common/NetCmdHandler.h"

namespace gcp {
  namespace util {
    
    class NetCommHandler {
    public:
      
      /**
       * Constructor.
       */
      NetCommHandler();
      
      /**
       * Destructor.
       */
      virtual ~NetCommHandler();
      
      /**
       * Attach the read network I/O streams to a socket.
       */
      void attachReadStream(int fd);
      
      /**
       * Attach the send network I/O streams to a socket.
       */
      void attachSendStream(int fd);
      
      /**
       * Attach all streams to an fd
       */
      void attach(int fd);
      
      /**
       * Pack a network message into our send buffer
       */
      void packNetMsg(NetMsg* msg);
      
      /**
       * Pack a network message into our send buffer
       */
      bool packNewRtcNetMsg(NetMsg* msg);

      /**
       * Pack a network command into our send buffer
       */
      void packNetCmd(NetCmd* cmd);
      
      /**
       * Pack a network command into our send buffer
       */
      void packRtcNetCmd(gcp::control::RtcNetCmd* cmd, 
			 gcp::control::NetCmdId opcode);
      
      /**
       * Read a message into our read buffer.
       */
      gcp::util::NetReadStr::NetReadId readNetMsg();
      
      /**
       * Send a message in our read buffer.
       */
      gcp::util::NetSendStr::NetSendId sendNetMsg();
      
      /**
       * Read a command into our read buffer.
       */
      gcp::util::NetReadStr::NetReadId readNetCmd();
      
      /**
       * Send a command in our send buffer.
       */
      gcp::util::NetSendStr::NetSendId sendNetCmd();
      
      /**
       * Return the last read command
       */
      gcp::util::NetCmd* getLastReadNetCmd();
      
      /**
       * Return the last sent command
       */
      gcp::util::NetCmd* getLastSentNetCmd();
      
      /**
       * Return the last read message
       */
      gcp::util::NetMsg* getLastReadNetMsg();
      
      /**
       * Return the last sent message
       */
      gcp::util::NetMsg* getLastSentNetMsg();
      
      /**
       * Return the fd that our read streams are attached to.
       */
      int getReadFd();
      
      /*
       * Return the fd that our send streams are attached to.
       */
      int getSendFd();
      
      /**
       * Return the single fd that both streams are attached to.
       */
      int getFd();
      
      /**
       * Return a pointer to our NetMsg handler.
       */
      NetMsgHandler* getNetMsgHandler();
      
      /**
       * Return a pointer to our NetCmd handler
       */
      NetCmdHandler* getNetCmdHandler();
      
      /**
       * Return the antenna id of this handler
       */
      inline AntNum::Id getId() {
	return antNum_.getId();
      }
      
      inline unsigned getIntId() {
	return antNum_.getIntId();
      }
      
      /**
       * Setthe antenna id of this handler
       */
      inline void setId(AntNum::Id id) {
	antNum_.setId(id);
      }
      
    private:
      
      int readFd_;
      int sendFd_;
      AntNum antNum_;
      NetMsgHandler netMsgHandler_;
      NetCmdHandler netCmdHandler_;
      
    }; // End class NetCommHandler
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_NETCOMMHANDLER_H
