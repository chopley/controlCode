#ifndef GCP_UTIL_SPEED_H
#define GCP_UTIL_SPEED_H

/**
 * @file Speed.h
 * 
 * Tagged: Wed Dec  1 23:39:12 PST 2004
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/ConformableQuantity.h"

namespace gcp {
  namespace util {
    
    class Speed : public ConformableQuantity {
    public:
      
      class CentimetersPerSec {};
      class KilometersPerSec {};
      class MetersPerSec {};
      class MilesPerHour {};
      
      /**
       * Constructor.
       */
      Speed();
      Speed(const CentimetersPerSec& units, double cmPerSec);
      Speed(const MetersPerSec& units, double mPerSec);
      Speed(const MilesPerHour& units, double mPh);
      
      /**
       * Destructor.
       */
      virtual ~Speed();
      
      /**
       * Set a speed
       */
      void setCentimetersPerSec(double cmPerSec);
      void setMetersPerSec(double mPerSec);
      void setMilesPerHour(double mPh);
      
      /**
       * Get a speed
       */
      double centimetersPerSec();
      double metersPerSec();
      double milesPerHour();
      double mph();
      
      void initialize();
      
    private:
      
      double cmPerSec_;
      
    }; // End class Speed
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_SPEED_H
