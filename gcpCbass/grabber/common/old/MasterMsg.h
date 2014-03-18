#ifndef GCP_GRABBER_MASTERMSG_H
#define GCP_GRABBER_MASTERMSG_H

/**
 * @file MasterMsg.h
 * 
 * Tagged: Thu Nov 13 16:53:58 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/GenericMasterTaskMsg.h"

#include "gcp/grabber/common/ControlMsg.h"
#include "gcp/grabber/common/ScannerMsg.h"

namespace gcp {
  namespace grabber {
    
    class MasterMsg :
      public gcp::util::GenericMasterTaskMsg {
      
      public:
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	CONTROL_CONNECTED, // A message that we are dis/connected from
			   // the control port
	SCANNER_CONNECTED, // A message that we are dis/connected from
			   // the scanner port
	CONTROL_MSG,       // A message for the control task
	SCANNER_MSG        // A message for the scanner task
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
	
	/**
	 * A message for the control thread.
	 */
	ControlMsg controlMsg;
	
	/**
	 * A message for the scanner thread.
	 */
	ScannerMsg scannerMsg;
	
      } body;
      
      //------------------------------------------------------------
      // Methods for returning pointers to messages for threads
      // managed by this task.
      //------------------------------------------------------------
      
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
       * Pack a scanner connection status message
       */
      inline void packScannerConnectedMsg(bool connected)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = SCANNER_CONNECTED;
	  
	  body.scannerConnected.connected = connected;
	}
      
    }; // End class MasterMsg
    
  }; // End namespace grabber
}; // End namespace gcp


#endif // End #ifndef GCP_GRABBER_MASTER_H
