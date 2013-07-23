#ifndef GCP_UTIL_POINTINGMODE_H
#define GCP_UTIL_POINTINGMODE_H

/**
 * @file PointingMode.h
 * 
 * Tagged: Thu Nov 13 16:53:47 UTC 2003
 * 
 * @author Erik Leitch
 */
namespace gcp {
  namespace util {
    
    
    /**
     * Define a class to specify a pointing mode
     */
    class PointingMode {
      
    public:
      
      /**
       * Enumerate possible pointing modes.
       */
      enum Type {
	OPTICAL,
	RADIO
      };
      
    }; // End class PointingMode
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef 
