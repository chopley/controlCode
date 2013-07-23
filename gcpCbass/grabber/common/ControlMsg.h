#ifndef GCP_GRABBER_CONTROLMSG_H
#define GCP_GRABBER_CONTROLMSG_H

/**
 * @file ControlMsg.h
 * 
 * Tagged: Thu Nov 13 16:53:57 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/util/common/NetMsg.h"

namespace gcp {
  namespace grabber {
    
    class ControlMsg :
      public gcp::util::GenericTaskMsg {
      
      public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	
	// Messages for this task
	
	CONNECT,      // A message to connect to the control port.
	NETMSG,       // A network message to be sent back to the
		      // control program
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
      /**
       * The contents of the message.
       */
      union {
	gcp::util::NetMsg networkMsg;
      } body;
      
      /**
       * Method to get a pointer to the NetMsg
       */
      inline gcp::util::NetMsg* getNetMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = NETMSG;
	  
	  return &body.networkMsg;
	}
      
      //------------------------------------------------------------
      // Methods for packing messages intended for the
      // Control task
      //------------------------------------------------------------
      
      /**
       * Method to pack a message to send a data frame.
       */
      inline void packConnectMsg() {
	
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = CONNECT;
      }
      
    }; // End class ControlMsg
    
  }; // End namespace grabber
}; // End namespace gcp


#endif // End #ifndef GCP_GRABBER_CONTROL_H
