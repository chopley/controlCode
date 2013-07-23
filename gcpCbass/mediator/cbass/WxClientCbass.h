#ifndef GCP_MEDIATOR_WXCLIENTCBASS_H
#define GCP_MEDIATOR_WXCLIENTCBASS_H

/**
 * @file WxClientCbass.h
 * 
 * Tagged: Mon Jul 19 14:47:35 PDT 2004
 * 
 * @author Erik Leitch
 */
#include <list>
#include <string>
#include <sstream>

#include "gcp/util/common/Ports.h"
#include "gcp/util/common/WxClientSA.h"

namespace gcp {
  namespace mediator {
    
    class WxControl;
    
    class WxClientCbass : public gcp::util::WxClientSA {
    public:
      
      /**
       * Constructor.
       */
      WxClientCbass(WxControl* parent=0, std::string host="localhost", 
		  unsigned port=gcp::util::Ports::wxPort("cbass"));
      
      /**
       * Destructor.
       */
      virtual ~WxClientCbass();
      
    private:
      
      // WxControl may call some of our private methods
      
      friend class WxControl;
      
      WxControl* parent_;

      void processServerData();

    }; // End class WxClientCbass
    
  } // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_MEDIATOR_WXCLIENTCBASS_H
