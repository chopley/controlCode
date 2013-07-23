// $Id: TipperSshClient.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_MEDIATOR_TIPPERSSHCLIENT_H
#define GCP_MEDIATOR_TIPPERSSHCLIENT_H

/**
 * @file TipperSshClient.h
 * 
 * Tagged: Fri Jan 26 20:58:26 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/TipperSshClient.h"

#include "gcp/util/common/RegMapDataFrameManager.h"

#include "gcp/control/code/unix/libunix_src/common/regmap.h"

#include <iostream>

namespace gcp {
  namespace mediator {

    class TipperSshClient : public gcp::util::TipperSshClient {
    public:

      /**
       * Constructors
       */
      TipperSshClient(gcp::util::RegMapDataFrameManager* share,
		      std::string gateway="bicep",
		      std::string host="bicep3",
		      unsigned short port=gcp::util::Ports::tipperPort("bicep"));

      /**
       * Destructor.
       */
      virtual ~TipperSshClient();

      // Method called when data have been completely read from the
      // server

      void readServerData(gcp::util::NetHandler& handler);

      void reportError();

    private:

      gcp::util::RegMapDataFrameManager* share_;

      RegMapBlock* utcBlock_;
      RegMapBlock* tauBlock_;
      RegMapBlock* tAtmBlock_;

    }; // End class TipperSshClient

  } // End namespace mediator
} // End namespace gcp



#endif // End #ifndef GCP_MEDIATOR_TIPPERSSHCLIENT_H
