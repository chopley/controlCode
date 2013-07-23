#ifndef GCP_UTIL_SERVER_H
#define GCP_UTIL_SERVER_H

/**
 * @file Server.h
 * 
 * Tagged: Thu 02-Feb-06 17:13:21
 * 
 * @author Erik Leitch
 */
#include <list>
#include <string>
#include <stdio.h>

#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/NetDat.h"
#include "gcp/util/common/NetHandler.h"
#include "gcp/util/common/Runnable.h"
#include "gcp/util/common/SignalTaskMsg.h"
#include "gcp/util/common/TimeOut.h"

namespace gcp {
  namespace util {
    
    class Port;
    class SignalTask;
    class TcpListener;
    
    class Server : public Runnable {
      
    public:
      
      struct ServerConnection {
	NetHandler handler_;
	Server* parent_;

	ServerConnection(int fd, 
			 unsigned readBufSize, unsigned sendBufSize, 
			 Server* parent);

	~ServerConnection();
      };

      /**
       * Constructor for serial connections
       */
      Server(bool spawnThread, int listenPort, unsigned readBufSize=0, 
	     unsigned sendBufSize=0);
      
      /**
       * Destructor.
       */
      virtual ~Server();
      
      /**
       * Block in select
       */
      virtual void run();

    protected:

      unsigned readBufSize_;

      unsigned sendBufSize_;

      TimeOut timeOut_;

      // Method called when we time out in our select loop

      virtual void timeOutAction() {};

      // Method called when data have been completely read from a client

      virtual void readClientData(NetHandler& handler) {};

      // Method called immediately after a client has connected

      virtual void acceptClientAction() {};

      // Method to check any other fds that may be registered

      virtual void checkOtherFds() {};

      void sendClientData(NetDat& dat);

      void setReadBufSize(unsigned size);
      void setSendBufSize(unsigned size);

      void setTimeOutSeconds(unsigned int seconds);

    protected:

      /**
       * The set of sockets to be watched
       */
      FdSet fdSet_;
      
    private:
      
      unsigned int timeOutSeconds_;

      /**
       * A private thread which will manage signal handling
       */
      SignalTask* signalTask_;
      
      /**
       * True if we should stop
       */
      bool stop_;
      
      /**
       * Object for listening for socket connection requests
       */
      TcpListener* listener_;
      
      /**
       * A vector of network buffers
       */
      std::list<ServerConnection*> clients_;
      
      /**
       * Initialize members
       */
      void initMembers(int listenPort, unsigned readBufSize, unsigned sendBufSize);
      
      /**
       * Set the port number on which we should listen for connection
       * requests.  Also sets the queue length
       */
      void listen(unsigned port, unsigned nClients = 5);
      
      /**
       * Send a shutdown message to connected clients
       */
      void sendShutdownMsg();
      
      /**
       * Accept a client connection
       */
      void acceptConnection();
      
    private:

      /**
       * Check clients for data to be read
       */
      void checkClients();
      
      /**
       * Read data from a client socket
       */
      void readFromClient(NetHandler* client);
      
      /**
       * A shutdown method
       */
      static SIGNALTASK_HANDLER_FN(shutDown);
      
      /**
       * A run method to be called from pthread_start()
       */
      static RUN_FN(runFn);
      
      // Check clients for data to be read

      void checkClientsForReadableData();

      // Check clients for writability

      void checkClientsForWritability();

      static NET_READ_HANDLER(readHandler);
      static NET_SEND_HANDLER(sendHandler);
      static NET_ERROR_HANDLER(errHandler);

    }; // End class Server
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_SERVER_H
