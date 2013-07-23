#ifndef TCPCLIENT_H
#define TCPCLIENT_H

/**
 * @file TcpClient.h
 * 
 * Started: Sat Mar  6 14:44:17 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Port.h"

#include <string>

namespace gcp {
  namespace util {
    
    class TcpClient : public Port {
    public:
      
      /**
       * Constructor with no preset host or port.
       */
      TcpClient();
      
      /**
       * Constructor with host and port
       */
      TcpClient(std::string host, unsigned port);
      
      /**
       * Destructor.
       */
      virtual ~TcpClient();
      
      /**
       * Attempt to get a connection to the server.
       */
      int connectToServer(bool doWait);
      
      /**
       * Attempt to get a connection to the server.
       */
      int connectToServer(std::string host, unsigned port, bool doWait);
      
      /**
       * Configure the socket managed by this class for
       * blocking/non-blocking I/O
       */ 
      void setBlocking(bool doWait);
      
      /**
       * Set the host
       */
      void setHost(std::string host);
      
      /**
       * Set the port number.
       */
      void setPort(unsigned port);
      
      /**
       * Connect
       */
      int connect() {};
      
      /**
       * Disconnect 
       */
      void disconnect();
      
      /**
       * Return true if we are connected
       */
      bool isConnected();
      
      std::string getHost() {
	return host_;
      }

      unsigned int getPort() {
	return port_;
      }

    private:
      
      // True when connected to the server.
      
      bool connected_;
      
      // The host to connect to
      
      std::string host_;
      bool hostInit_;
      
      // The port to connect to
      
      unsigned port_;
      bool portInit_;
      
      /**
       * Connect to the server
       */
      int privateConnectToServer(bool doWait);
      
    }; // End class TcpClient
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef 


