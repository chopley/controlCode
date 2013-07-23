// $Id: Profiler.h,v 1.1.1.1 2009/07/06 23:57:26 eml Exp $

#ifndef GCP_UTIL_PROFILER_H
#define GCP_UTIL_PROFILER_H

/**
 * @file Profiler.h
 * 
 * Tagged: Sun Jan 14 19:48:39 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author Erik Leitch
 */

#include "gcp/util/common/TimeVal.h"

#include <iostream>

namespace gcp {
  namespace util {

    class Profiler {
    public:

      /**
       * Constructor.
       */
      Profiler() {};

      /**
       * Destructor.
       */
      virtual ~Profiler() {};

      void start() {
	start_.setToCurrentTime();
      }

      void stop() {
	stop_.setToCurrentTime();
      }

      TimeVal& diff() {
	diff_ = stop_ - start_;
	return diff_;
      }

    private:

      TimeVal start_, stop_, diff_;

    }; // End class Profiler

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_PROFILER_H
