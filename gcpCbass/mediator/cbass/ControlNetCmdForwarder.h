#ifndef GCP_ASSEMBLER_CONTROLNETCMDFORWARDER_H
#define GCP_ASSEMBLER_CONTROLNETCMDFORWARDER_H

/**
 * @file ControlNetCmdForwarder.h
 * 
 * Tagged: Sun May 16 13:26:53 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/specific/ControlNetCmdForwarder.h"

namespace gcp {
namespace mediator {
    
    class Control;
    class ControlMsg;

    class ControlNetCmdForwarder : public gcp::util::ControlNetCmdForwarder {
    public:
      
      /**
       * Constructor.
       */
      ControlNetCmdForwarder(Control* parent);
      
      /**
       * Destructor.
       */
      virtual ~ControlNetCmdForwarder();
      
      /**
       * Forward a network command for the delay subsystem.
       */
      void forwardNetCmd(gcp::util::NetCmd* netCmd);

    private:

      Control* parent_;

      /**
       * Forward a message to the control task.
       */
      void forwardControlMsg(ControlMsg* msg);

    }; // End class ControlNetCmdForwarder
    
} // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_CONTROLNETCMDFORWARDER_H
