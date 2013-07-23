#ifndef GCP_ASSEMBLER_ANTENNACONSUMERNOTIFICATION_H
#define GCP_ASSEMBLER_ANTENNACONSUMERNOTIFICATION_H

/**
 * @file AntennaConsumerNotification.h
 * 
 * Tagged: Mon Apr 26 12:57:58 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/specific/Directives.h"
#include "gcp/mediator/specific/AntennaConsumer.h"

namespace gcp {
namespace mediator {
    
    class Scanner;

    class AntennaConsumerNotification :

      public AntennaConsumer {

    public:
      
      /**
       * Constructor attaches to the specified notification channel.
       */
      AntennaConsumerNotification(Scanner* parent,
				  std::string notifyChannel);
      
      /**
       * Destructor.
       */
      virtual ~AntennaConsumerNotification();
      
      /**
       * Override the base-class run methods
       */
      void run();
      
    private:
      
      /**
       * The name of the notification channel
       */
      std::string notifyChannel_;
      
    }; // End class AntennaConsumerNotification
    
} // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_ANTENNACONSUMERNOTIFICATION_H
