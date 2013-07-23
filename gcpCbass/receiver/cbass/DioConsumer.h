#ifndef GCP_RECEIVER_DIOCONSUMER_H
#define GCP_RECEIVER_DIOCONSUMER_H

/**
 * @file DioConsumer.h
 * 
 * Started: Fri Jan  9 14:42:51 PST 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/BoardDataFrameManager.h"

#include "gcp/mediator/specific/DioMsg.h"

namespace gcp {

  namespace mediator {
    class Scanner;
  }

  namespace util {
    class RegMapDataFrameManager;
    class ArrayMapDataFrameManager;
  }

  namespace receiver {
    
    class DioClient;

    class DioConsumer : 
      public gcp::util::GenericTask<gcp::mediator::DioMsg> {

    public:

      virtual ~DioConsumer();

      // Service our message queue

      virtual void serviceMsgQ();

      // Process a message specific to this task.

      virtual void processMsg(gcp::mediator::DioMsg* msg) {};

    protected:
      
      // The DIO client object managed by this thread will have to
      // contact the DIO server as well as the HardwareManager daemon
      // for hardware configuration information.

      DioConsumer(gcp::mediator::Scanner* parent,
		  std::string dioHost, unsigned short dioPort, 
		  std::string hwHost,  unsigned short hwPort);
      
      // We declare Scanner a friend so that it alone can instantiate
      // this class.

      friend class gcp::mediator::Scanner; 
      
      // Needed to call Scanner::packFrame() on receipt of an event

      gcp::mediator::Scanner* parent_; 
      
      // Members pertaining to the data connection to the DIO server
 
      std::string dioHost_;     // Data server host name
      unsigned short dioPort_;  // Data server TCP port

      // Members pertaining to the connection to the hardware manager

      std::string hwHost_;     // Hardware manager server host name
      unsigned short hwPort_;  // Hardwarea manager TCP port
      
      // A timer that will be used as a timeout in select()

      gcp::util::TimeVal timer_;
      struct timeval* timeOut_;

      // Connects to data server, requests data and converts data stream into
      // map of vectors, one vector per data channel
      
      DioClient* dioClient_;

      // Set the timeout until the next connection attempt

      void resetTimeout();

      // Set the timeout until the next connection attempt

      void clearTimeout();

      // Pack a data frame

      virtual void packFrame() {};

      gcp::util::RegMapDataFrameManager* grabReadFrame();
      void releaseReadFrame();

      // Copy stored values of persistent items into the register
      // frame
      
      virtual void 
	copyPersistentRegs(gcp::util::ArrayMapDataFrameManager* frame);

      virtual gcp::util::BoardDataFrameManager& getFrame() {};

    public:

      // Check for from the DIO Client
      
      void checkForDioData();

    }; // End class DioConsumer

  } // End namespace receiver
} // End namespace gcp

#endif // End #ifndef 

