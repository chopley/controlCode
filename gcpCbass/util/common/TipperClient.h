#ifndef GCP_UTIL_TIPPERCLIENT_H
#define GCP_UTIL_TIPPERCLIENT_H

/**
 * @file TipperClient.h
 * 
 * Tagged: Wed 01-Feb-06 15:46:35
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Client.h"
#include "gcp/util/common/Ports.h"
#include "gcp/util/common/TipperData.h"

#include <vector>
#include <string>

namespace gcp {
  namespace util {
    
    class TipperClient : public Client {
    public:
      
      /**
       * Constructor.
       */
      TipperClient(bool spawnThread, std::string host, 
		   unsigned port=TIPPER_SERVER_PORT);
      
      /**
       * Destructor.
       */
      virtual ~TipperClient();
      
    protected:
      
      TipperData tipperData_;
      std::vector<unsigned char> bytes_;
      void readServerData(NetHandler& handler);

    }; // End class TipperClient
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_TIPPERCLIENT_H
