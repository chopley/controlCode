#ifndef GCP_RECEIVER_BOLOMETERCONSUMER_H
#define GCP_RECEIVER_BOLOMETERCONSUMER_H

/**
 * @file Bolometerconsumer.h
 * 
 * Started: Fri Jan  9 14:42:51 PST 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/receiver/specific/DioConsumer.h"

namespace gcp {

  namespace mediator {
    class Scanner;
  }

  namespace receiver {
    
    class BolometerConsumer : public DioConsumer {

    public:
      
      // The DIO client object managed by this thread will have to
      // contact the DIO server as well as the HardwareManager daemon
      // for hardware configuration information.

      BolometerConsumer(gcp::mediator::Scanner* parent,
			  std::string dioHost, unsigned short dioPort, 
			  std::string hwHost,  unsigned short hwPort,
			  unsigned npix=0);
      
      BolometerConsumer::~BolometerConsumer();


      gcp::util::RegMapDataFrameManager* grabReadFrame();

      void releaseReadFrame();

      void packFrame();

      // Process a message specific to this task.

      void processMsg(gcp::mediator::DioMsg* msg);

    }; // End class BolometerConsumer
    
  } // End namespace receiver
} // End namespace gcp

#endif // End #ifndef 

