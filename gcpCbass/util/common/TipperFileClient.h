// $Id: TipperFileClient.h,v 1.1.1.1 2009/07/06 23:57:27 eml Exp $

#ifndef GCP_UTIL_TIPPERFILECLIENT_H
#define GCP_UTIL_TIPPERFILECLIENT_H

/**
 * @file TipperFileClient.h
 * 
 * Tagged: Tue Feb 13 10:24:38 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:27 $
 * 
 * @author Erik Leitch
 */

#include <iostream>
#include <fstream>

#include "gcp/util/common/TipperSshClient.h"

namespace gcp {
  namespace util {

    class TipperFileClient : public TipperSshClient {
    public:

      /**
       * Constructor.
       */
      TipperFileClient(std::string fileName);

      /**
       * Destructor.
       */
      virtual ~TipperFileClient();

      // Process data received from the server

      void processServerData();

    private:

      std::ofstream of_;
      
    }; // End class TipperFileClient

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_TIPPERFILECLIENT_H
