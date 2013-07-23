// $Id: WxSshClient.h,v 1.2 2009/08/17 21:18:32 eml Exp $

#ifndef GCP_UTIL_WXSSHCLIENT_H
#define GCP_UTIL_WXSSHCLIENT_H

/**
 * @file WxSshClient.h
 * 
 * Tagged: Fri Jan 26 20:58:26 NZDT 2007
 * 
 * @version: $Revision: 1.2 $, $Date: 2009/08/17 21:18:32 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/SshClient.h"
#include "gcp/util/common/Ports.h"
#include "gcp/util/common/WxData40m.h"

#include <iostream>

namespace gcp {
  namespace util {

    class WxSshClient : public gcp::util::SshClient {
    public:

      /**
       * Constructors
       */
      WxSshClient(bool spawnThread, 
		      std::string gateway="data.ovro.caltech.edu",
		      std::string host="cbassdaq.cm.pvt",
		      unsigned short port=Ports::wxPort("cbass"));

      /**
       * Destructor.
       */
      virtual ~WxSshClient();

      // Initialize network container sizes and start up this thread
      // if requested to

      void initialize(bool spawnThread);

    protected:
      
      WxData40m wxData_;
      std::vector<unsigned char> bytes_;
      virtual void readServerData(NetHandler& handler);
      virtual void processServerData() {};

    }; // End class WxSshClient

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_WXSSHCLIENT_H
