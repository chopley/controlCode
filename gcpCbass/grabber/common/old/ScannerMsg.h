#ifndef GCP_GRABBER_SCANNERMSG_H
#define GCP_GRABBER_SCANNERMSG_H

/**
 * @file ScannerMsg.h
 * 
 * Tagged: Thu Nov 13 16:54:00 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTaskMsg.h"

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

namespace gcp {
  namespace grabber {
    
    class ScannerMsg :
      public gcp::util::GenericTaskMsg {
      
      public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	CONNECT,    // A message to connect to the archiver port.
	GRAB_FRAME, // A message to start a new data frame.
	CONFIGURE,  // A message to configure the frame grabber
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
      union {
	
	struct {
	  unsigned mask;
	  unsigned channelMask;
	  unsigned nCombine;
	  unsigned flatfield;
	  unsigned seconds;
	} configure;
	
      } body;
      
      //------------------------------------------------------------
      // Methods for packing messages intended for the
      // Scanner task
      //------------------------------------------------------------
      
      /**
       * Method to pack a message to send a data frame.
       */
      inline void packGrabFrameMsg() {
	
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = GRAB_FRAME;
      }
      
      /**
       * Method to pack a message to send a data frame.
       */
      inline void packConnectMsg() {
	
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = CONNECT;
      }
      
      /**
       * Method to pack a message to integrate
       */
      inline void packConfigureMsg(gcp::control::FgOpt mask, 
				   unsigned channelMask, 
				   unsigned nCombine, 
				   unsigned flatfield,
				   unsigned seconds) {
	
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = CONFIGURE;
	
	body.configure.mask        = mask;
	body.configure.channelMask = channelMask;
	body.configure.nCombine    = nCombine;
	body.configure.flatfield   = flatfield;
	body.configure.seconds     = seconds;
      }
      
    }; // End class ScannerMsg
    
    
  }; // End namespace grabber
}; // End namespace gcp


#endif // End #ifndef SZA_UTIL_GRABBERMSG_H
