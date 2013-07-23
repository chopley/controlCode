// $Id: TimeOut.h,v 1.1.1.1 2009/07/06 23:57:27 eml Exp $

#ifndef GCP_UTIL_TIMEOUT_H
#define GCP_UTIL_TIMEOUT_H

/**
 * @file TimeOut.h
 * 
 * Tagged: Tue May  2 16:31:46 PDT 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:27 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/TimeVal.h"

namespace gcp {
  namespace util {

    class TimeOut {
    public:

      // Default interval 

      static const unsigned int defaultInterval_ = 5*60;

      // Public methods

      TimeOut();

      void setIntervalInSeconds(unsigned int seconds);

      void activate(bool active);

      void reset();

      struct timeval* tVal();

      TimeVal timeOut_; // The timeval managed by this class

    private:

      bool active_;
      bool resetPending_;
      
    }; // End class TimeOut

  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_TIMEOUT_H
