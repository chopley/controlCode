#ifndef GCP_UTIL_CCNETMSG_H
#define GCP_UTIL_CCNETMSG_H

/**
 * @file CcNetMsg.h
 * 
 * Tagged: Mon Mar 15 15:29:07 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/NetSendStr.h"

#include "gcp/control/code/unix/libunix_src/common/netobj.h"
#include "gcp/control/code/unix/libunix_src/common/control.h"

namespace gcp {
  namespace util {
    
    class CcNetMsg {
    public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	LOG   = gcp::control::CC_LOG_MSG,   // A message to be logged
	REPLY = gcp::control::CC_REPLY_MSG, // A reply to a CC_INPUT_CMD
	SCHED = gcp::control::CC_SCHED_MSG, // A message from the scheduler
	ARC   = gcp::control::CC_ARC_MSG,   // A message from the archiver
	PAGE  = gcp::control::CC_PAGE_MSG,  // A message regarding the pager
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
      /**
       * The contents of the message.
       */
      gcp::control::CcNetMsg body;
      
      //------------------------------------------------------------
      // Methods to pack Network messages
      //------------------------------------------------------------
      
    }; // End class CcNetMsg
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_CCNETMSG_H
