#ifndef GCP_UTIL_WXCLIENT_H
#define GCP_UTIL_WXCLIENT_H

/**
 * @file WxClient.h
 * 
 * Tagged: Wed 01-Feb-06 15:46:35
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Client.h"
#include "gcp/util/common/Ports.h"
#include "gcp/util/common/WxData.h"

#include <vector>
#include <string>

namespace gcp {
  namespace util {
    
    class WxClient : public Client {
    public:
      
      /**
       * Constructor.
       */
      WxClient(bool spawnThread, std::string host="sptcontrol", 
	       unsigned port=Ports::wxPort("spt"));
      
      /**
       * Destructor.
       */
      virtual ~WxClient();
      
    protected:
      
      WxData wxData_;
      std::vector<unsigned char> bytes_;
      virtual void readServerData(NetHandler& handler);
      virtual void processServerData() {};

    }; // End class WxClient
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_WXCLIENT_H
