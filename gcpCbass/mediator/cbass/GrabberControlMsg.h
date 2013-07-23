#ifndef GCP_ASSEMBLER_GRABBERCONTROLMSG_H
#define GCP_ASSEMBLER_GRABBERCONTROLMSG_H

/**
 * @file GrabberControlMsg.h
 * 
 * Tagged: Thu Jul  8 18:28:01 UTC 2004
 * 
 * @author 
 */
#include "gcp/util/common/GenericTaskMsg.h"

// Do not reshuffle these.  GCC3.2.2 is easily confused

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"
#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

namespace gcp {
  namespace mediator {
    
    class GrabberControlMsg :
      public gcp::util::GenericTaskMsg {
      public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	
	// A net command to be sent directly to the grabber socket.
	
	NETCMD,
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
      /**
       * The contents of the message.
       */
      union {
	
	/**
	 * A network command forwarded unmodified from the ACC
	 */
	struct {
	  gcp::control::NetCmdId opcode;
	  gcp::control::RtcNetCmd cmd;
	} netCmd;
	
      } body;
      
      //------------------------------------------------------------
      // Methods for packing messages to the GrabberControl
      // task
      //------------------------------------------------------------
      
      /**
       * A method for packing a net command.
       */
      inline void packRtcNetCmdMsg(gcp::control::RtcNetCmd* cmd, 
				   gcp::control::NetCmdId opcode) {
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = NETCMD;
	
	body.netCmd.cmd = *cmd;
	body.netCmd.opcode = opcode;
      }
      
    }; // End class GrabberControlMsg
    
  } // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_GRABBERCONTROLMSG_H
