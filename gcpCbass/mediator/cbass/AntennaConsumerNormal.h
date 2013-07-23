#ifndef GCP_ASSEMBLER_ANTENNACONSUMERNORMAL_H
#define GCP_ASSEMBLER_ANTENNACONSUMERNORMAL_H

/**
 * @file AntennaConsumerNormal.h
 * 
 * Tagged: Mon Apr  5 14:01:23 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetCommHandler.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/mediator/specific/AntennaConsumer.h"

namespace gcp {
  
  namespace util {
    class TcpListener;
    class NetAntennaDataFrameHandler;
  }
  
namespace mediator {
    
    class Scanner;
    
    class AntennaConsumerNormal :
      public AntennaConsumer {
      public:
      
      /**
       * Constructor.
       */
      AntennaConsumerNormal(Scanner* parent);
      
      /**
       * Destructor.
       */
      virtual ~AntennaConsumerNormal();
      
      private:
      
      /**
       * Control will call our sendTaskMsg() method.
       */
      friend class Scanner;
      
      /**
       * Keep a pointer to the parent which instantiated this object.
       */
      Scanner* parent_;
      
      /**
       * Static pointer for use in static functions
       */
      static AntennaConsumerNormal* consumer_;

      /**
       * The number of antennas we know about.
       */
      int nAntenna_;
      
      /**
       * The server socket on which to listen for connection
       * requests from the antenna computers.
       */
      gcp::util::TcpListener* listener_;
      
      /**
       * A network buffers associated with antennas whose connections
       * we have accepted, but for which a greeting-response cycle is
       * not yet complete.
       */
      gcp::util::NetCommHandler temporaryHandler_; 
      
      /**
       * A vector of network buffers associated with established
       * antenna connections.
       */
      std::vector<gcp::util::NetAntennaDataFrameHandler*> connectedHandlers_; 
      
      /**
       * Time structs we will use for handling timeouts in select.
       */
      gcp::util::TimeVal startTime_;
      gcp::util::TimeVal timer_;
      struct timeval* timeOut_;

      /**
       * Set up for TCP/IP communications
       */
      void connectTcpIp();
      
      /**
       * Service our message queue.
       */
      void serviceMsgQ();
      
      /**
       * Call this function to start or stop listening on our server socket
       */
      void listen(bool listenVar);
      
      /**
       * This function is called to accept or reject a connection
       * request from an antenna.
       */
      void initializeConnection();

      /**
       * This function is called to terminate an established antenna
       * connection.
       */
      void terminateConnection(gcp::util::NetHandler* str);

      /**
       * This function is called to terminate an unestablished antenna
       * connection.
       */
      void terminateConnection(gcp::util::NetCommHandler* str);

      /**
       * This function is called when an ID message has been received on an
       * unestablished antenna connection.
       */
      void finalizeConnection();
      
      /**
       * Send a greeting message to an antenna
       */
      void sendGreetingMsg();

      /**
       * Check if we timed out waiting for a response from a newly connected
       * antenna
       */
      bool timedOut();
      
      /**
       * Act upon receipt of a network message from an antenna with
       * which we are in the process of establishing a connection.
       */
      static NET_READ_HANDLER(netMsgReadHandler);

      /**
       * Act upon completion of sending a betwork message to an
       * antenna with which we are in the process of establishing a
       * connection.
       */
      static NET_SEND_HANDLER(netMsgSentHandler);

      /**
       * A handler to be called when an error occurs reading or
       * sending a message to an antenna.
       */
      static NET_ERROR_HANDLER(netMsgErrorHandler);

      /**
       * Read or continue reading a partially read data frame
       */
      static NET_READ_HANDLER(netAntennaDataFrameReadHandler);

      /**
       * A handler to be called when an error occurs reading a data
       * frame from an antenna.
       */
      static NET_ERROR_HANDLER(netAntennaDataFrameErrorHandler);

    }; // End class AntennaConsumerNormal
    
} // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_ASSEMBLER_ANTENNACONSUMERNORMAL_H
