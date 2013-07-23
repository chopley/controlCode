#ifndef GCP_UTIL_DELAYLOCATION_H
#define GCP_UTIL_DELAYLOCATION_H

/**
 * @file DelayLocation.h
 * 
 * Tagged: Thu Aug  5 06:51:59 PDT 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Debug.h"

namespace gcp {
  namespace util {
    
    class Location;

    class DelayLocation {
    public:
      
      /**
       * Constructor.
       */
      DelayLocation();
      
      /**
       * Destructor.
       */
      virtual ~DelayLocation();
      
      /**
       * A function called whenever an outside caller changes a location
       */
      virtual void locationChanged(Location* loc);

    }; // End class DelayLocation
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_DELAYLOCATION_H
