#ifndef GCP_UTIL_WXSERVER_H
#define GCP_UTIL_WXSERVER_H

/**
 * @file WxServer.h
 * 
 * Tagged: Wed 01-Feb-06 15:36:17
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Ports.h"
#include "gcp/util/common/SerialClient.h"
#include "gcp/util/common/Server.h"
#include "gcp/util/common/WxData.h"

namespace gcp {
  namespace util {
    
    class WxServer : public Server {
    public:
      
      /**
       * Constructor.
       */
      WxServer(bool spawnThread, std::string serialPort, int baudRate, 
	       int listenPort);
      
      /**
       * Destructor.
       */
      virtual ~WxServer();
      

    private:

      void initPort();
      void acceptClientAction();
      void timeOutAction();
      void checkOtherFds();
      void readRecord();

      SerialClient port_;
      std::stringstream os_;
      WxData data_;

    }; // End class WxServer
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_WXSERVER_H
