#ifndef GCP_UTIL_TIPPERSERVER_H
#define GCP_UTIL_TIPPERSERVER_H

/**
 * @file TipperServer.h
 * 
 * Tagged: Wed 01-Feb-06 15:36:17
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Ports.h"
#include "gcp/util/common/Server.h"
#include "gcp/util/common/TipperData.h"
#include "gcp/util/common/TipperCommunicator.h"
#include "gcp/util/common/TipperReader.h"

#include <string>

namespace gcp {
  namespace util {
    
    class TipperServer : public TipperReader, 
      public Server {
    public:
      
      /**
       * Constructor.
       */
      TipperServer(std::string ftpHost, bool spawnThread);
      
      /**
       * Destructor.
       */
      virtual ~TipperServer();
      
    private:

      TipperCommunicator tipperComm_;
      unsigned timeOutCounter_;
      bool first_;

      void timeOutAction();
      void acceptClientAction();

    }; // End class TipperServer
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_TIPPERSERVER_H
