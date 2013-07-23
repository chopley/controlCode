// $Id: Correlation.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_CORRELATION_H
#define GCP_UTIL_CORRELATION_H

/**
 * @file Correlation.h
 * 
 * Tagged: Wed Jan 10 06:41:14 CST 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */

#include <iostream>

#include "gcp/util/common/Dft.h"

namespace gcp {
  namespace util {

    class Correlation {
    public:

      /**
       * Constructor.
       */
      Correlation(int n, bool optimize);

      /**
       * Const Assignment Operator.
       */
      void operator=(const Correlation& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(Correlation& objToBeAssigned);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, Correlation& obj);

      /**
       * Destructor.
       */
      virtual ~Correlation();

      /**
       * Compute the transform
       */
      virtual void computeTransform();

      /**
       * Return a pointer to the transformed data
       */
      virtual fftw_complex* getTransform();

      /**
       * Return true if the input array is full
       */
      virtual bool isReadyForTransform();

      /**
       * Return the size of the transform
       */
      unsigned transformSize();

      /**
       * Return the squared transform |F* x F|
       */
      double* abs2();

    protected:

      Dft dft1_;

    private:

      bool absCalculated_;

      /**
       * Calculate the squared transform
       */
      void calcAbs2();

    }; // End class Correlation

  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_CORRELATION_H
