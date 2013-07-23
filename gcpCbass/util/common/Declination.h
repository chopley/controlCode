// $Id: Declination.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_DECLINATION_H
#define GCP_UTIL_DECLINATION_H

/**
 * @file Declination.h
 * 
 * Tagged: Fri Jun 15 16:44:12 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author username: Command not found.
 */
#include "gcp/util/common/DecAngle.h"

namespace gcp {
  namespace util {

    class Declination : public DecAngle {
    public:

      /**
       * Constructor.
       */
      Declination();

      /**
       * Destructor.
       */
      virtual ~Declination();

      void addRadians(double radians);

      /** 
       * Add an angle to this declination
       */
      Declination operator+(Angle& angle) {
	Declination sum;
	sum.setRadians(radians_);
	sum.addRadians(angle.radians());
	return sum;
      }

      /** 
       * Subtract an angle from this declination
       */
      Declination operator-(Angle& angle) {
	Declination sum;
	sum.setRadians(radians_);
	sum.addRadians(-angle.radians());
	return sum;
      }

    private:
    }; // End class Declination

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_DECLINATION_H
