#ifndef DIOMSG_H
#define DIOMSG_H

/**
 * @file DioMsg.h
 * 
 * Tagged: Thu Nov 13 16:54:00 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTaskMsg.h"

namespace gcp {
  namespace mediator {

    class DioMsg :
      public gcp::util::GenericTaskMsg {
      
      public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	SETFILTER
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
      union body {

	struct {
	  unsigned mask;
	  double freqHz;
	  unsigned ntaps;
	} setFilter;

      } body;

      //------------------------------------------------------------
      // Methods for packing messages intended for the
      // Dio task
      //------------------------------------------------------------
      
      /**
       * Method to pack a message to send a data frame.
       */
      inline void packSetFilterMsg(unsigned mask, 
				   double freqHz, unsigned ntaps) {
	
	genericMsgType_ = 
	  gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	
	body.setFilter.mask   = mask;
	body.setFilter.freqHz = freqHz;
	body.setFilter.ntaps  = ntaps;
	
	type = SETFILTER;
      }
      
    }; // End class DioMsg
    
  } // End namespace mediator
} // End namespace gcp
  
#endif // End #ifndef 
