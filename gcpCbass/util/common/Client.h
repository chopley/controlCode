#ifndef GCP_UTIL_CLIENT_H
#define GCP_UTIL_CLIENT_H

/**
 * @file Client.h
 * 
 * Tagged: Wed 01-Feb-06 10:43:27
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/NetDat.h"
#include "gcp/util/common/NetHandler.h"
#include "gcp/util/common/Runnable.h"
#include "gcp/util/common/TcpClient.h"
#include "gcp/util/common/TimeVal.h"

namespace gcp {
  namespace util {
    
    class Client : public Runnable {
    public:
      
      /**
       * Constructor.
       */
      Client(bool spawn, std::string host, unsigned connectPort, 
	     unsigned readBufSize=0, unsigned sendBufSize=0);
      
      /**
       * Destructor.
       */
      virtual ~Client();
      
      /**
       * Block in select
       */
      void run();

      void setReadBufSize(unsigned size);
      void setSendBufSize(unsigned size);

    protected:
      
      // Method called when data have been completely read from the server

      virtual void readServerData(NetHandler& handler) {}; 

      // Send data to the server

      void sendServerData(NetDat& dat);

      TimeVal timeOut_;
      struct timeval* timeOutPtr_;

    private:

      bool stop_;

      // The connection manager

      TcpClient tcp_;

      NetHandler handler_;

      FdSet fdSet_;

      void initMembers(std::string host, unsigned port, 
		      unsigned readBufSize, unsigned sendBufSize);

      // A run method to be called from pthread_start()

      static RUN_FN(runFn);

      static NET_READ_HANDLER(readHandler);
      static NET_SEND_HANDLER(sendHandler);
      static NET_ERROR_HANDLER(errHandler);

      void connect();
      void disconnect();

    }; // End class Client
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_CLIENT_H
