// $Id: Timer.h,v 1.1 2009/08/17 17:19:31 eml Exp $

#ifndef GCP_UTIL_TIMER_H
#define GCP_UTIL_TIMER_H

/**
 * @file Timer.h
 * 
 * Tagged: Fri Aug 14 10:54:50 PDT 2009
 * 
 * @version: $Revision: 1.1 $, $Date: 2009/08/17 17:19:31 $
 * 
 * @author tcsh: username: Command not found.
 */
#include "gcp/util/common/TimeVal.h"

namespace gcp {
  namespace util {

    class Timer {
    public:

      /**
       * Constructor.
       */
      Timer();

      /**
       * Destructor.
       */
      virtual ~Timer();

      void start();
      void stop();
      double deltaInSeconds();

    private:

      TimeVal start_;
      TimeVal stop_;
      TimeVal diff_;

    }; // End class Timer

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_TIMER_H
