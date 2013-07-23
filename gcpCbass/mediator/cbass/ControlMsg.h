#ifndef CONTROLMSG_H
#define CONTROLMSG_H

/**
 * @file ControlMsg.h
 * 
 * Tagged: Thu Nov 13 16:53:57 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/util/common/NetMsg.h"

#include "gcp/mediator/specific/AntennaControlMsg.h"
#include "gcp/mediator/specific/GrabberControlMsg.h"
#include "gcp/mediator/specific/ReceiverControlMsg.h"
#include "gcp/mediator/specific/WxControlMsg.h"

namespace gcp {
namespace mediator {
    
    class ControlMsg :
      public gcp::util::GenericTaskMsg {
      
      public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	
	// Messages for this task
	
	CONNECT,      // A message to connect to the control port.
	INIT,         // A greeting message from the control port.
	NETMSG,       // A network message intended for the controller.

	// Messages for tasks spawned by this one.
	
	ANTENNA_CONTROL_MSG, // A message for our Antenna Control task

	GRABBER_CONTROL_MSG, // A message for the task controlling the
			     // delay engine and lobe rotator

	WX_CONTROL_MSG,      // A message for the task controlling the
			     // weather station

	RECEIVER_CONTROL_MSG,// A message the the task running scripts
			     // to control the receiver
	
      };
      
      /**
       * A type for this message
       */
      MsgType type;

      /**
       * The contents of the message.
       */
      union {

	struct {
	  bool start;
	} init;

	gcp::util::NetMsg networkMsg;

	/**
	 * A message for the antenna control thread.
	 */
	AntennaControlMsg antennaControlMsg;
	
	/**
	 * A message for the grabber control thread.
	 */
	GrabberControlMsg grabberControlMsg;

	/**
	 * A message for the receiver control thread.
	 */
	ReceiverControlMsg receiverControlMsg;

	/**
	 * A message for the wx control thread.
	 */
	WxControlMsg wxControlMsg;

      } body;
      
      //------------------------------------------------------------
      // Methods for returning pointers to messages intended for other
      // tasks
      //------------------------------------------------------------

      inline AntennaControlMsg* getAntennaControlMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = ANTENNA_CONTROL_MSG;

	  return &body.antennaControlMsg;
	}
      
      inline GrabberControlMsg* getGrabberControlMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = GRABBER_CONTROL_MSG;

	  return &body.grabberControlMsg;
	}
      
      inline ReceiverControlMsg* getReceiverControlMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = RECEIVER_CONTROL_MSG;

	  return &body.receiverControlMsg;
	}
      
      inline WxControlMsg* getWxControlMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = WX_CONTROL_MSG;

	  return &body.wxControlMsg;
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

      /**
       * Method to pack a message to send a data frame.
       */
      inline void packInitMsg(bool start) {
	
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = INIT;

	body.init.start = start;
      }

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
      
    }; // End class ControlMsg
    
} // End namespace mediator
} // End namespace gcp

#endif // End #ifndef 
