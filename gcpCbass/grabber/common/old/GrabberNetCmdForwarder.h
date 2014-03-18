#ifndef GCP_GRABBER_GRABBERNETCMDFORWARDER_H
#define GCP_GRABBER_GRABBERNETCMDFORWARDER_H

/**
 * @file GrabberNetCmdForwarder.h
 * 
 * Tagged: Thu Jul  8 18:21:37 UTC 2004
 * 
 * @author 
 */
#include "gcp/util/common/NetCmdForwarder.h"

namespace gcp {
  namespace grabber {
    
    class Master;
    class MasterMsg;
    
    class GrabberNetCmdForwarder : public gcp::util::NetCmdForwarder {
    public:
      
      /**
       * Constructor.
       */
      GrabberNetCmdForwarder(Master* parent);
      
      /**
       * Destructor.
       */
      virtual ~GrabberNetCmdForwarder();
      
      //------------------------------------------------------------
      // Overwrite the base-class method by which all rtc commands are
      // processed
      //------------------------------------------------------------
      
      /**
       * A virtual method to forward a command received from the ACC.
       * Make this virtual so that inheritors can completely redefine
       * what happens with a received command, if they wish.
       */
      virtual void forwardNetCmd(gcp::util::NetCmd* netCmd);
      
    private:
      
      Master* parent_;
      
      /**
       * Forward a message to the scanner task.
       */
      void forwardScannerMsg(MasterMsg* msg);
      
    }; // End class GrabberNetCmdForwarder
    
  } // End namespace grabber
}; // End namespace gcp




#endif // End #ifndef GCP_GRABBER_GRABBERNETCMDFORWARDER_H
