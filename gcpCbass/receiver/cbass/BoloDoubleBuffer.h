// $Id: BoloDoubleBuffer.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_RECEIVER_BOLODOUBLEBUFFER_H
#define GCP_RECEIVER_BOLODOUBLEBUFFER_H

/**
 * @file BoloDoubleBuffer.h
 * 
 * Tagged: Sun Jan 14 18:01:04 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author Erik Leitch
 */

#include <iostream>

#include "gcp/util/common/DoubleBuffer.h"

namespace gcp {
  namespace receiver {

    class BoloDoubleBuffer : public gcp::util::DoubleBuffer {
    public:

      /**
       * Constructor.
       */
      BoloDoubleBuffer();

      /**
       * Destructor.
       */
      virtual ~BoloDoubleBuffer();

    private:
    }; // End class BoloDoubleBuffer

  } // End namespace receiver
} // End namespace gcp



#endif // End #ifndef GCP_RECEIVER_BOLODOUBLEBUFFER_H
