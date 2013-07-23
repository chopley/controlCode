#ifndef GCP_UTIL_WXCLIENT40M_H
#define GCP_UTIL_WXCLIENT40M_H

/**
 * @file WxClient40m.h
 * 
 * Tagged: Wed 01-Feb-06 15:46:35
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Client.h"
#include "gcp/util/common/Ports.h"
#include "gcp/util/common/WxData40m.h"

#include <vector>
#include <string>

namespace gcp {
  namespace util {
    
    class WxClient40m : public Client {
    public:
      
      /**
       * Constructor.
       */
      WxClient40m(bool spawnThread, std::string host="cbassdaq.cm.pvt", 
	       unsigned port=Ports::wxPort("cbass"));
      
      /**
       * Destructor.
       */
      virtual ~WxClient40m();
      
    protected:
      
      WxData40m wxData_;
      std::vector<unsigned char> bytes_;
      virtual void readServerData(NetHandler& handler);
      virtual void processServerData() {};

    }; // End class WxClient40m
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_WXCLIENT40M_H
