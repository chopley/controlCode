#ifndef GCP_UTIL_SITE_H
#define GCP_UTIL_SITE_H

/**
 * @file Site.h
 * 
 * Tagged: Wed Dec  1 17:08:41 PST 2004
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Length.h"

namespace gcp {
  namespace util {
    
    /**
     * Class for
     */
    class Site {
    public:
      
      /**
       * Constructor.
       */
      Site();
      
      Site(Angle& longitude, Angle& latitude, Length& altitude);
      
      /**
       * Destructor.
       */
      virtual ~Site();
      
      inline Length altitude() {
	return altitude_;
      }
      
      inline Angle latitude() {
	return latitude_;
      }
      
      inline Angle longitude() {
	return longitude_;
      }
      
      inline bool hasLongitude() {
	return longInit_;
      }
      
      inline bool hasLatitude() {
	return latInit_;
      }
      
      inline bool hasAltitude() {
	return altInit_;
      }
      
      /**
       * Set methods
       */
      void setLongitude(Angle& longitude);
      void setLatitude(Angle& latitude);
      void setAltitude(Length& altitude);
      
      void setTo(Angle& longitude, Angle& latitude, Length& altitude);
      
      void initialize();
      
    private:
      
      Length altitude_;
      bool altInit_;
      
      Angle longitude_;
      bool longInit_;
      
      Angle latitude_;
      bool latInit_;
      
    }; // End class Site
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_SITE_H
