// $Id: AutoCorrelation.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_AUTOCORRELATION_H
#define GCP_UTIL_AUTOCORRELATION_H

/**
 * @file AutoCorrelation.h
 * 
 * Tagged: Wed Jan 10 06:33:03 CST 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */

#include <iostream>

#include "gcp/util/common/Dft.h"
#include "gcp/util/common/Correlation.h"

namespace gcp {
  namespace util {

    class AutoCorrelation : public Correlation {
    public:

      /**
       * Constructor.
       */
      AutoCorrelation(int n, bool optimize);

      /**
       * Const Assignment Operator.
       */
      void operator=(const AutoCorrelation& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(AutoCorrelation& objToBeAssigned);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, AutoCorrelation& obj);

      /**
       * Destructor.
       */
      virtual ~AutoCorrelation();

      /**
       * Push the next sample onto the input array
       */
      void pushSample(double sample);

    }; // End class AutoCorrelation

  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_AUTOCORRELATION_H
