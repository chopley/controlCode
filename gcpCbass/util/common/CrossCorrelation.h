// $Id: CrossCorrelation.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_CROSSCORRELATION_H
#define GCP_UTIL_CROSSCORRELATION_H

/**
 * @file CrossCorrelation.h
 * 
 * Tagged: Wed Jan 10 06:34:52 CST 2007
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

    class CrossCorrelation : public Correlation {
    public:

      /**
       * Constructor.
       */
      CrossCorrelation(int n, bool optimize);

      /**
       * Const Assignment Operator.
       */
      void operator=(const CrossCorrelation& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(CrossCorrelation& objToBeAssigned);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, CrossCorrelation& obj);

      /**
       * Destructor.
       */
      virtual ~CrossCorrelation();

      /**
       * Compute the transform
       */
      void computeTransform();

      /**
       * Return a pointer to the transformed data
       */
      fftw_complex* getTransform();

      /**
       * Return true if the input array is full
       */
      bool isReadyForTransform();

      /**
       * Return a pointer to the transformed data
       */
      fftw_complex* calcCorrelation();

      /**
       * Push the next sample onto the input array
       */
      void pushSample(double sample1, double sample2);

    private:

      Dft dft2_;
      bool correlationCalculated_;

    }; // End class CrossCorrelation

  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_CROSSCORRELATION_H
