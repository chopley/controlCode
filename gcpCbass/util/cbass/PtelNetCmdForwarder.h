#ifndef SZA_UTIL_PTELNETCMDFORWARDER_H
#define SZA_UTIL_PTELNETCMDFORWARDER_H

/**
 * @file PtelNetCmdForwarder.h
 * 
 * Tagged: Thu Jul  8 18:21:37 UTC 2004
 * 
 * @author 
 */
#include "gcp/util/common/NetCmdForwarder.h"

namespace gcp {
  namespace util {
    
    class PtelNetCmdForwarder : public NetCmdForwarder {
    public:
      
      /**
       * Constructor.
       */
      PtelNetCmdForwarder() {};
      
      /**
       * Destructor.
       */
      virtual ~PtelNetCmdForwarder() {};
      
      //------------------------------------------------------------
      // Overwrite the base-class method by which all rtc commands are
      // processed
      //------------------------------------------------------------
      
      /**
       * A virtual method to forward a command received from the ACC.
       * Make this virtual so that inheritors can completely redefine
       * what happens with a received command, if they wish.
       */
      virtual void forwardNetCmd(gcp::util::NetCmd* netCmd) {};
      
    }; // End class PtelNetCmdForwarder
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef SZA_UTIL_PTELNETCMDFORWARDER_H
