#ifndef GCP_RECEIVER_XMLCONSUMER_H
#define GCP_RECEIVER_XMLCONSUMER_H

/**
 * @file XMLConsumer.h
 * 
 * Started: Fri Jan  9 14:42:51 PST 2004
 * 
 * @author Ken Aird
 */

#include "Utilities/HardwareManagerClient.h"

#include "gcp/util/common/GenericTask.h"

#include "gcp/mediator/specific/ScannerMsg.h"

#include "gcp/receiver/specific/XMLDataFrameManager.h"

namespace gcp {

  namespace mediator {
    class Scanner;
  }


  namespace receiver {

    class XMLConsumer : 
      public gcp::util::GenericTask<gcp::mediator::ScannerMsg> {
      
    public:
      
      XMLConsumer(gcp::receiver::XMLDataFrameManager* dataFrame,
                  gcp::mediator::Scanner* scanner, 
                  std::string regMapName,
                  std::string host="localhost", 
                  unsigned short port=5207,
                  bool send_connect_request=true);
          
      /**
       * Destructor.
       */
      virtual ~XMLConsumer();

      void sendDispatchDataFrameMsg();
      
      /**
       * We declare Scanner a friend so that it alone
       * can instantiate this class.
       */
      friend class gcp::mediator::Scanner; 
      
      static const unsigned nConnAttemptBeforeReport_ = 30;

    protected:

      // Process a message received on our message queue

      virtual void processMsg(gcp::mediator::ScannerMsg* msg);
      
      // Get the next set of data from the server

      virtual bool getData();

      // Check that we have a valid client connection

      virtual bool checkClient();

      // Register a communciation error

      void registerError();

      // Report a connection error

      virtual void reportError();
      virtual void reportSuccess();

      // Generic method to send a command to the HWM

      bool commandPending_;

      unsigned nConnAttempt_;

      gcp::mediator::Scanner* parent_;
      
      std::string regMapName_;
      
      std::string host_;  // Data server host name
      
      unsigned short port_;  // Data server TCP port
      
      bool send_connect_request_;
      bool connected_;
      int clientFd_;

      gcp::receiver::XMLDataFrameManager* dataFrame_;

      MuxReadout::HardwareManagerClient* client_;

    }; // End class XMLConsumer
    
  } // End namespace receiver
} // End namespace gcp

#endif // End #ifndef 
