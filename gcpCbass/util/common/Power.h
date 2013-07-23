#ifndef GCP_UTIL_POWER_H
#define GCP_UTIL_POWER_H

/**
 * @file Power.h
 * 
 * Tagged: Thu Oct 18 17:42:37 PDT 2007
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/ConformableQuantity.h"

namespace gcp {
  namespace util {
    
    class Power : public ConformableQuantity {
    public:
      
      class dBm {};

      /**
       * Constructor.
       */
      Power();
      Power(const dBm& units, double dBmPow);

      /**
       * Destructor.
       */
      virtual ~Power();
      
      void setdBm(double dBmPow) {
	dBm_ = dBmPow;
      }

      double getdBm() {
	return dBm_;
      }

    private:

      void initialize();

      // The actual frequency, in dBm
      
      double dBm_;

    }; // End class Power
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_POWER_H
