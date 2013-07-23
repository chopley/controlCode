#ifndef GCP_UTIL_WXSERVERSA_H
#define GCP_UTIL_WXSERVERSA_H

/**
 * @file WxServerSA.h
 * 
 * Tagged: 2013 May 15
 * 
 * @author Stephen Muchovej
 */
#include "gcp/util/common/Ports.h"
#include "gcp/util/common/SerialClient.h"
#include "gcp/util/common/Server.h"
#include "gcp/util/common/WxReaderSA.h"

namespace gcp {
  namespace util {
    
    class WxServerSA : public Server, public WxReaderSA {
    public:
      
      /**
       * Constructor.
       */
      WxServerSA(bool spawnThread, int listenPort=Ports::wxPort("cbass"));
      
      /**
       * Destructor.
       */
      virtual ~WxServerSA();
      
    private:

      void initPort();
      void acceptClientAction();
      void timeOutAction();

    }; // End class WxServerSA
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_WXSERVERSA_H
