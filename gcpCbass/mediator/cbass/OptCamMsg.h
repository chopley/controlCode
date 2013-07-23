#ifndef OPTCAMMSG_H
#define OPTCAMMSG_H

/**
 * @file OptCamMsg.h
 * 
 * Tagged: Thu Nov 13 16:53:59 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTaskMsg.h"

namespace gcp {
namespace mediator {

    class OptCamMsg :
      public gcp::util::GenericTaskMsg {
      
      public:
      
      /**
       * Enumerate supported message types.
       */
      enum MsgType {
	CONNECT       // A message to connect to the optcam port.
      };
      
      /**
       * A type for this message
       */
      MsgType type;
      
    }; // End class OptcamMsg
    
} // End namespace mediator
} // End namespace gcp
  
#endif // End #ifndef 
