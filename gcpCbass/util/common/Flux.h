// $Id: Flux.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_FLUX_H
#define GCP_UTIL_FLUX_H

/**
 * @file Flux.h
 * 
 * Tagged: Wed Sep 14 17:14:39 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */
#include <iostream>

#include "gcp/util/common/ConformableQuantity.h"
#include "gcp/util/common/Frequency.h"
#include "gcp/util/common/Temperature.h"
#include "gcp/util/common/SolidAngle.h"

namespace gcp {
  namespace util {

    class Flux : public ConformableQuantity {
    public:

      class Jansky {};
      class MilliJansky {};
      class MegaJansky {};

      /**
       * Constructor.
       */
      Flux();
      Flux(const Jansky& units, double Jy);
      Flux(const MilliJansky& units, double mJy);
      Flux(const MegaJansky& units, double MJy);
      Flux(Frequency& freq, Temperature& temp, SolidAngle& omega);

      /**
       * Destructor.
       */
      virtual ~Flux();

      void setJy(double Jy);
      void setMilliJy(double mJy);
      void setMegaJy(double MJy);

      // Return the flux, in Jy

      inline double Jy() {
	return Jy_;
      }

      // Return the flux, in mJy

      inline double mJy() {
	return Jy_ * mJyPerJy_;
      }

      static const double mJyPerJy_;
      static const double JyPerMJy_;

      void initialize();

      friend std::ostream& gcp::util::operator<<(std::ostream& os, Flux& flux);

      bool operator>=(Flux& flux);
      bool operator<=(Flux& flux);

    private:

      double Jy_;

    }; // End class Flux

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_FLUX_H
