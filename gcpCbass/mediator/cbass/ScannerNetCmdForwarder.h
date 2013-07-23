#ifndef GCP_ASSEMBLER_SCANNERNETCMDFORWARDER_H
#define GCP_ASSEMBLER_SCANNERNETCMDFORWARDER_H

/**
 * @file ScannerNetCmdForwarder.h
 * 
 * Tagged: Thu Jul  8 17:49:19 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/specific/ScannerNetCmdForwarder.h"

#include "gcp/mediator/specific/MasterMsg.h"

namespace gcp {
namespace mediator {

    class Control;
    class MasterMsg;

    class ScannerNetCmdForwarder : public gcp::util::ScannerNetCmdForwarder {
    public:
      
      /**
       * Constructor.
       */
      ScannerNetCmdForwarder(Control* parent);
      
      /**
       * Destructor.
       */
      virtual ~ScannerNetCmdForwarder();

      /**
       * Forward a network command for the delay subsystem.
       */
      void forwardNetCmd(gcp::util::NetCmd* netCmd);

    private:

      Control* parent_;

      /**
       * Forward a message to the control task.
       */
      void forwardScannerMsg(MasterMsg* msg);

    }; // End class ScannerNetCmdForwarder
    
} // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_SCANNERNETCMDFORWARDER_H
