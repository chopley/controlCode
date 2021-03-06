#ifndef GCP_UTIL_NETCMDFORWARDER_H
#define GCP_UTIL_NETCMDFORWARDER_H

/**
 * @file NetCmdForwarder.h
 * 
 * Tagged: Sun May 16 12:37:32 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetCmd.h"

namespace gcp {
  namespace util {
    
    /**
     * A pure interface class for forwarding network commands received
     * from the control program
     */
    class NetCmdForwarder {
    public:
      
      /**
       * Constructor.
       */
      NetCmdForwarder();
      
      /**
       * Destructor.
       */
      virtual ~NetCmdForwarder();
      
      /**
       * A virtual method to forward a command received from the ACC.
       * Make this virtual so that inheritors can completely redefine
       * what happens with a received command, if they wish.
       */
      virtual void forwardNetCmd(gcp::util::NetCmd* netCmd) = 0;
      
    }; // End class NetCmdForwarder
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_NETCMDFORWARDER_H
