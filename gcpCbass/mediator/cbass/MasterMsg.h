#ifndef MASTERMSG_H
#define MASTERMSG_H

/**
 * @file MasterMsg.h
 * 
 * Tagged: Thu Nov 13 16:53:58 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/GenericMasterTaskMsg.h"
#include "gcp/util/common/SignalTask.h"

#include "gcp/mediator/specific/ControlMsg.h"
#include "gcp/mediator/specific/ScannerMsg.h"
#include "gcp/mediator/specific/OptCamMsg.h"

namespace gcp {
namespace mediator {

    class MasterMsg :
      public gcp::util::GenericMasterTaskMsg {
      
      public:
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	CONTROL_CONNECTED, // A message that we are dis/connected from
			   // the control port
	
	OPTCAM_CONNECTED,  // A message that we are dis/connected from
			   // the scanner port
	SCANNER_CONNECTED, // A message that we are dis/connected from
			   // the scanner port

	PTEL_CONNECTED,    // A message that the pointing telescope is
			   // dis/connected from the ptel port

	DEICING_CONNECTED,    // A message that the deicing control computer is
			   // dis/connected from the ptel port

	SEND_HEARTBEAT,
	
	CONTROL_MSG,
	OPTCAM_MSG,
	SCANNER_MSG
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
      /**
       * A union of message bodies.
       */
      union {
	
	struct {
	  bool connected;
	} controlConnected;

	struct {
	  bool connected;
	} scannerConnected;

	struct {
	  bool connected;
	} optcamConnected;

	struct {
	  bool connected;
	} ptelConnected;

	struct {
	  bool connected;
	} deicingConnected;

	/**
	 * A message for the control thread.
	 */
	ControlMsg controlMsg;
	
	/**
	 * A message for the control thread.
	 */
	OptCamMsg optcamMsg;
	
	/**
	 * A message for the scanner thread.
	 */
	ScannerMsg scannerMsg;
	
      } body;
      
      //------------------------------------------------------------
      // Methods for returning pointers to messages for threads
      // managed by this task.
      //------------------------------------------------------------
      
      inline OptCamMsg* getOptCamMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = OPTCAM_MSG;
	  return &body.optcamMsg;
	}
      
      inline ScannerMsg* getScannerMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = SCANNER_MSG;
	  return &body.scannerMsg;
	}
      
      inline ControlMsg* getControlMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = CONTROL_MSG;
	  return &body.controlMsg;
	}
      
      
      //------------------------------------------------------------
      // Methods for packing messages.
      
      /**
       * Pack a message to send a heartbeat.
       */
      inline void packSendHeartBeatMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = SEND_HEARTBEAT;
	}
      
      /**
       * Pack a control connection status message
       */
      inline void packControlConnectedMsg(bool connected)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = CONTROL_CONNECTED;
	  
	  body.controlConnected.connected = connected;
	}

      /**
       * Pack a ptel control connection status message
       */
      inline void packPtelControlConnectedMsg(bool connected)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = PTEL_CONNECTED;
	  
	  body.ptelConnected.connected = connected;
	}

      /**
       * Pack a deicing control connection status message
       */
      inline void packDeicingControlConnectedMsg(bool connected)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = DEICING_CONNECTED;
	  
	  body.deicingConnected.connected = connected;
	}

      /**
       * Pack a scanner connection status message
       */
      inline void packScannerConnectedMsg(bool connected)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = SCANNER_CONNECTED;
	  
	  body.scannerConnected.connected = connected;
	}

      /**
       * Pack a optcam connection status message
       */
      inline void packOptCamConnectedMsg(bool connected)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = OPTCAM_CONNECTED;
	  
	  body.optcamConnected.connected = connected;
	}
      
    }; // End class MasterMsg
    
} // End namespace mediator
} // End namespace gcp
  
#endif // End #ifndef 
