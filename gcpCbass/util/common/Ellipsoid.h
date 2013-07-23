#ifndef GCP_UTIL_ELLIPSOID_H
#define GCP_UTIL_ELLIPSOID_H

/**
 * @file Ellipsoid.h
 * 
 * Tagged: Wed Aug 25 02:50:06 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Length.h"

namespace gcp {
  namespace util {
    
    class Ellipsoid {
    public:
      
      /**
       * Constructors.
       */
      Ellipsoid();
      Ellipsoid(Length majorAxis, Length minorAxis);
      
      /**
       * Destructor.
       */
      virtual ~Ellipsoid();
      
      /**
       * Return the length of the vector from the center of the
       * ellipsoid to the surface at a given ellipsoidal latitude
       */
      virtual double flattening();
      
      /**
       * Return the eccentricity e, and e^2
       */
      virtual double eccentricity();
      virtual double eccentricitySquared();
      
      /**
       * Return the length of the radius vector at a given ellipsoidal
       * latitude
       */
      Length radius(Angle latitude);
      
    private:
      
      // The semi-major and semi-minor axes of this ellipsoid
      
      Length a_;
      Length b_;
      
    }; // End class Ellipsoid
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_ELLIPSOID_H
