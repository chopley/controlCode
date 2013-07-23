#ifndef GCP_ASSEMBLER_WXCONTROLMSG_H
#define GCP_ASSEMBLER_WXCONTROLMSG_H

/**
 * @file WxControlMsg.h
 * 
 * Tagged: Tue Nov 30 14:19:33 PST 2004
 * 
 * @author SZA data acquisition
 */
#include "gcp/util/common/GenericTaskMsg.h"

namespace gcp {
namespace mediator {
    
    class WxControlMsg :
      public gcp::util::GenericTaskMsg {
      
      public:
      
      /**
       * Enumerate supported WxControl messages
       */
      enum MsgType {
	READ,  // Read data from the weather station
      };
      
      /**
       * The type of this message
       */
      MsgType type;
      
      /**
       * Define a Message container
       */
      union {
      } body;
      
      //------------------------------------------------------------
      // Methods for packing messages to the wxs
      //------------------------------------------------------------
      
      inline void packReadMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = READ;
	}
      
    }; // End class WxControlMsg
    
} // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_WXCONTROLMSG_H
