#ifndef GCP_MEDIATOR_RECEIVERCONTROLMSG_H
#define GCP_MEDIATOR_RECEIVERCONTROLMSG_H

/**
 * @file ReceiverControlMsg.h
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
    
    class ReceiverControlMsg :
      public gcp::util::GenericTaskMsg {
      public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	
	// A script command to be issued
	
	COMMAND,

	// The directory in which to output files

	DIRECTORY
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
      /**
       * The contents of the message.
       */
      union {
	
	
	// A command to execute

	struct {

	  // The script to execute

	  char script[gcp::control::NET_LOG_MAX+1]; 

	  // A sequence number associated with this script

	  unsigned seq; 

	} command;

	// A directory specification

	struct {

	  // The script to execute

	  char dir[gcp::control::NET_LOG_MAX+1]; 

	} directory;

      } body;
      
      //------------------------------------------------------------
      // Methods for packing messages to the ReceiverControl
      // task
      //------------------------------------------------------------
      
      /**
       * A method for packing a net command.
       */
      inline void packCommandMsg(char* script, unsigned seq) {

	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = COMMAND;
	
	strcpy(body.command.script, script);

	body.command.seq = seq;
      }

      /**
       * A method for packing a directory command.
       */
      inline void packDirectoryMsg(char* dir) {

	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = DIRECTORY;
	
	strcpy(body.directory.dir, dir);
      }
      
    }; // End class ReceiverControlMsg
    
  } // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_MEDIATOR_RECEIVERCONTROLMSG_H
