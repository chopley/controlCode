// $Id: SolidAngle.h,v 1.1.1.1 2009/07/06 23:57:26 eml Exp $

#ifndef GCP_UTIL_SOLIDANGLE_H
#define GCP_UTIL_SOLIDANGLE_H

/**
 * @file SolidAngle.h
 * 
 * Tagged: Wed Sep 14 17:52:22 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author Erik Leitch
 */
#include <iostream>

#include "gcp/util/common/ConformableQuantity.h"

namespace gcp {
  namespace util {

    class SolidAngle : public ConformableQuantity {
    public:

      class Steradians {};

      /**
       * Constructor.
       */
      SolidAngle();
      SolidAngle(const Steradians& units, double sr);

      /**
       * Destructor.
       */
      virtual ~SolidAngle();

      void initialize();

      void setSr(double sr);

      inline double sr() {
	return sr_;
      }

    private:

      double sr_;

    }; // End class SolidAngle

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_SOLIDANGLE_H
