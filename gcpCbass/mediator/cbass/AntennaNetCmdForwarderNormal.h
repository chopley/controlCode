#ifndef GCP_ASSEMBLER_ANTENNANETCMDFORWARDERNORMAL_H
#define GCP_ASSEMBLER_ANTENNANETCMDFORWARDERNORMAL_H

/**
 * @file AntennaNetCmdForwarderNormal.h
 * 
 * Tagged: Sun May 16 13:03:49 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/specific/AntennaNetCmdForwarder.h"

namespace gcp {
namespace mediator {
    
    class Control;

    class AntennaNetCmdForwarderNormal : 
      public gcp::util::AntennaNetCmdForwarder {
    public:
      
      /**
       * Constructor.
       */
      AntennaNetCmdForwarderNormal(Control* parent);
      
      /**
       * Destructor.
       */
      virtual ~AntennaNetCmdForwarderNormal();
      
      /**
       * Forward a network command intended for an antenna
       */
      void forwardNetCmd(gcp::util::NetCmd* netCmd);

      private:

      Control* parent_;

    }; // End class AntennaNetCmdForwarderNormal
    
} // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_ANTENNANETCMDFORWARDERNORMAL_H
