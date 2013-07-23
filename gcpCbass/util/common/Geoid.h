#ifndef GCP_UTIL_GEOID_H
#define GCP_UTIL_GEOID_H

/**
 * @file Geoid.h
 * 
 * Tagged: Wed Aug 25 02:57:01 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Ellipsoid.h"

namespace gcp {
  namespace util {
    
    class Geoid : public Ellipsoid {
    public:
      
      // The equatorial radius of the earth
      
      static const Length earthEqRadius_;
      
      // The IAU flattening of the earth, from Explanatory Supplement
      // p. 161
      
      static const double flattening_;
      
      /**
       * Constructor.
       */
      Geoid();
      
      /**
       * Destructor.
       */
      virtual ~Geoid();
      
      /**
       * Return the length of the radius vector from the center of the
       * earth to the surface at a given geocentric latitude.
       */
      Length geocentricRadius(Angle geocentricLatitude);
      
      /**
       * Return the length of the radius vector normal to the surface
       * at a given geodetic latitude
       */
      Length geodeticRadius(Angle geodeticLatitude);
      
      /**
       * Return the geocentric latitude corresponding to a given
       * geodetic latitude
       */
      Angle geocentricLatitude(Angle geodeticLatitude);
      
      /**
       * Return the flattening
       */
      inline double flattening() {
	return flattening_;
      }
      
      double eccentricitySquared();
      
    private:
    }; // End class Geoid
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_GEOID_H
