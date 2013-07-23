// $Id: WxReaderSA.h,v 1.2 2009/11/12 23:11:37 sjcm Exp $

#ifndef GCP_UTIL_WXREADERSA_H
#define GCP_UTIL_WXREADERSA_H

/**
 * @file WxReaderSA.h
 * 
 * Tagged: Tue May 14 2013
 * 
 * @version: $Revision: 1.2 $, $Date: 2009/11/12 23:11:37 $
 * 
 * @author tcsh: username: Command not found.
 */
#include "gcp/util/common/CurlUtils.h"
#include "gcp/util/common/WxDataSA.h"
#include <fstream>
#include <string.h>
#include <cerrno>
#include <iostream>

namespace gcp {
  namespace util {

    class WxReaderSA : public CurlUtils {
    public:

      /**
       * Constructor.
       */
      WxReaderSA();

      /**
       * Destructor.
       */
      virtual ~WxReaderSA();

      void getData();

      std::string getFileContents(const char *filename);

      WxDataSA data_;

      const char* defaultFilename_;
      
    }; // End class WxReaderSA

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_WXREADERSA_H
