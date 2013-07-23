#ifndef SCANNERMSG_H
#define SCANNERMSG_H

/**
 * @file ScannerMsg.h
 * 
 * Tagged: Thu Nov 13 16:54:00 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTaskMsg.h"

#include "gcp/mediator/specific/DioMsg.h"

namespace gcp {
namespace mediator {

    class ScannerMsg :
      public gcp::util::GenericTaskMsg {
      
      public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	START_DATAFRAME,    // A message to start a new data frame.
	DISPATCH_DATAFRAME, // A message to dispatch a pending data frame.
	PACK_BOLO_DATAFRAME,// A mesage that a bolometer frame is
			    // ready to be packed
	CONNECT,            // A message to connect to the archiver port.
	FEATURE,

	DIO_MSG
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
      union body {

	struct {
	  unsigned seq;
	  unsigned mode;
	  unsigned mask;
	} feature;

	DioMsg dioMsg;

      } body;

      inline DioMsg* getDioMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = DIO_MSG;

	  return &body.dioMsg;
	}
      

      //------------------------------------------------------------
      // Methods for packing messages intended for the
      // Scanner task
      //------------------------------------------------------------
      
      /**
       * Method to pack a message to send a data frame.
       */
      inline void packStartDataFrameMsg() {
	
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = START_DATAFRAME;
      }
      
      /**
       * Method to pack a message to dispatch a data frame.
       */
      inline void packDispatchDataFrameMsg() {
	
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = DISPATCH_DATAFRAME;
      }

      /**
       * Method to pack a message to pack a bolometer data frame
       */
      inline void packPackBoloDataFrameMsg() {
	
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = PACK_BOLO_DATAFRAME;
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
       * Method to pack a message to set a feature bitmask
       */
      inline void packFeatureMsg(unsigned seq, 
				 unsigned mode,
				 unsigned mask) {
	
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	type = FEATURE;

	body.feature.seq  = seq;
	body.feature.mode = mode;
	body.feature.mask = mask;
      }


    }; // End class ScannerMsg
    
    
} // End namespace mediator
} // End namespace gcp
  
#endif // End #ifndef 
