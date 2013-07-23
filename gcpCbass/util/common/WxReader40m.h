// $Id: WxReader40m.h,v 1.2 2009/11/12 23:11:37 eml Exp $

#ifndef GCP_UTIL_WXREADER40M_H
#define GCP_UTIL_WXREADER40M_H

/**
 * @file WxReader40m.h
 * 
 * Tagged: Fri Aug 14 16:13:36 PDT 2009
 * 
 * @version: $Revision: 1.2 $, $Date: 2009/11/12 23:11:37 $
 * 
 * @author tcsh: username: Command not found.
 */
#include "gcp/util/common/CurlUtils.h"
#include "gcp/util/common/WxData40m.h"

namespace gcp {
  namespace util {

    class WxReader40m : public CurlUtils {
    public:

      /**
       * Constructor.
       */
      WxReader40m();

      /**
       * Destructor.
       */
      virtual ~WxReader40m();

      void getData();

      WxData40m data_;
      
    }; // End class WxReader40m

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_WXREADER40M_H
