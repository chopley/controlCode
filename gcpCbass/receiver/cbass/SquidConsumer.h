#ifndef GCP_RECEIVER_SQUIDCONSUMER_H
#define GCP_RECEIVER_SQUIDCONSUMER_H

/**
 * @file SquidConsumer.h
 * 
 * Started: Fri Jan  9 14:42:51 PST 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/BoardDataFrameManager.h"

#include "gcp/receiver/specific/DioConsumer.h"

namespace gcp {

  namespace util {
    class  ArrayDataFrameManager;
  }

  namespace mediator {
    class Scanner;
  }
    

  namespace receiver {
    
    class SquidConsumer : public DioConsumer {

    public:
      
      // The DIO client object managed by this thread will have to
      // contact the DIO server as well as the HardwareManager daemon
      // for hardware configuration information.

      SquidConsumer(gcp::mediator::Scanner* parent,
		    std::string dioHost, unsigned short dioPort, 
		    std::string hwHost,  unsigned short hwPort);
      
      SquidConsumer::~SquidConsumer();

      gcp::util::BoardDataFrameManager& getFrame();

    }; // End class SquidConsumer

  } // End namespace receiver
} // End namespace gcp

#endif // End #ifndef 

