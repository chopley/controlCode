#ifndef GCP_UTIL_CONTROLNETCMDFORWARDER_H
#define GCP_UTIL_CONTROLNETCMDFORWARDER_H

/**
 * @file ControlNetCmdForwarder.h
 * 
 * Tagged: Sun May 16 13:28:31 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetCmdForwarder.h"

namespace gcp {
  namespace util {
    
    class ControlNetCmdForwarder : public NetCmdForwarder {
    public:
      
      /**
       * Constructor.
       */
      ControlNetCmdForwarder();
      
      /**
       * Destructor.
       */
      virtual ~ControlNetCmdForwarder();
      
      //------------------------------------------------------------
      // Overwrite the base-class method by which all rtc commands for
      // the antennas are processed
      //------------------------------------------------------------
      
      /**
       * A virtual method to forward a command received from the ACC.
       * Make this virtual so that inheritors can completely redefine
       * what happens with a received command, if they wish.
       */
      virtual void forwardNetCmd(gcp::util::NetCmd* netCmd);
      
    private:
    }; // End class ControlNetCmdForwarder
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_CONTROLNETCMDFORWARDER_H
