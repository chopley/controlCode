#ifndef GCP_ASSEMBLER_GRABBERNETCMDFORWARDER_H
#define GCP_ASSEMBLER_GRABBERNETCMDFORWARDER_H

/**
 * @file GrabberNetCmdForwarder.h
 * 
 * Tagged: Thu Jul  8 18:20:25 UTC 2004
 * 
 * @author 
 */
#include "gcp/util/specific/GrabberNetCmdForwarder.h"

namespace gcp {
  namespace mediator {
    
    class Control;
    
    class GrabberNetCmdForwarder : 
      public gcp::util::GrabberNetCmdForwarder {
      public:
      
      /**
       * Constructor.
       */
      GrabberNetCmdForwarder(Control* parent);
      
      /**
       * Destructor.
       */
      virtual ~GrabberNetCmdForwarder();
      
      private:
      
      /**
       * Forward a network command intended for the frame grabber
       */
      void forwardNetCmd(gcp::util::NetCmd* netCmd);
      
      private:
      
      Control* parent_;
      
    }; // End class GrabberNetCmdForwarder
    
  } // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_GRABBERNETCMDFORWARDER_H
