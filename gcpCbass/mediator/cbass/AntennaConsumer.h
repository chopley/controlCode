#ifndef GCP_ASSEMBLER_ANTENNACONSUMER_H
#define GCP_ASSEMBLER_ANTENNACONSUMER_H

/**
 * @file AntennaConsumer.h
 * 
 * Tagged: Mon Apr  5 15:10:53 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTask.h"
#include "gcp/mediator/specific/ScannerMsg.h"

namespace gcp {
namespace mediator {
    
    class Scanner;

    class AntennaConsumer :
      public gcp::util::GenericTask<ScannerMsg> {
    public:
      
      /**
       * Constructor.
       */
      AntennaConsumer(Scanner* parent);
      
      /**
       * Destructor.
       */
      virtual ~AntennaConsumer();
      
      protected:
      
      /**
       * Needed because Scanner will access private thread_
       * member.
       */    
      friend class Scanner;
      
      /**
       * Needed to call Scanner::packFrame() on receipt of an
       * event
       */
      Scanner* parent_; 
      
    }; // End class AntennaConsumer
    
} // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_ANTENNACONSUMER_H
