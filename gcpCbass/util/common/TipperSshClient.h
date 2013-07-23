// $Id: TipperSshClient.h,v 1.1.1.1 2009/07/06 23:57:27 eml Exp $

#ifndef GCP_UTIL_TIPPERSSHCLIENT_H
#define GCP_UTIL_TIPPERSSHCLIENT_H

/**
 * @file TipperSshClient.h
 * 
 * Tagged: Fri Jan 26 20:58:26 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:27 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/SshClient.h"
#include "gcp/util/common/Ports.h"
#include "gcp/util/common/TipperData.h"

#include <iostream>

namespace gcp {
  namespace util {

    class TipperSshClient : public gcp::util::SshClient {
    public:

      /**
       * Constructors
       */
      TipperSshClient(bool spawnThread, 
		      std::string gateway="bicep",
		      std::string host="bicep3",
		      unsigned short port=Ports::tipperPort("bicep"));

      /**
       * Destructor.
       */
      virtual ~TipperSshClient();

      // Initialize network container sizes and start up this thread
      // if requested to

      void initialize(bool spawnThread);

    protected:
      
      TipperData tipperData_;
      std::vector<unsigned char> bytes_;
      virtual void readServerData(NetHandler& handler);
      virtual void processServerData() {};

    }; // End class TipperSshClient

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_TIPPERSSHCLIENT_H
