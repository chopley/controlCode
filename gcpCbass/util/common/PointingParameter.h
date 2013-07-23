#ifndef GCP_UTIL_POINTINGPARAMETER_H
#define GCP_UTIL_POINTINGPARAMETER_H

/**
 * @file PointingParameter.h
 * 
 * Started: Wed Dec 17 20:12:42 UTC 2003
 * 
 * @author Erik Leitch
 */
namespace gcp {
  namespace util {
    
    class PointingParameter {
      
    public:
      
      enum Parameter {
	NONE        = 0,
	
	// The longitude,latitude and altitude of the site 
	
	SITE        = 1,
	
	// The atmospheric refraction parameters 
	
	ATMOSPHERE  = 2,
	
	// The value of UT1-UTC 
	
	UT1UTC      = 4, 
	
	// The value of the equation of the equinoxes 
	
	EQNEQX      = 8,
	
	// The number of counts per turn of the encoders 
	
	ENCODERS    = 0x10,
	
	// Axis tilts 
	
	TILTS       = 0x20,
	
	// Collimation terms 
	
	COLLIMATION = 0x40,
	
	// Encoder limits 
	
	LIMITS      = 0x80,
	
	// The gravitational flexure of the mount 
	
	FLEXURE     = 0x100,
	
	// The zero points of the encoders 
	
	ZEROS       = 0x200,
	
	// The antenna-specific offsets
	
	LOCATION    = 0x400,
	
	// The GPS time
	
	TIME        = 0x800,
	
	// The following should be the bitwise union of all of the above
	
	ALL         =0xFFF
      };
      
    }; // End class PointingParameter
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef 
