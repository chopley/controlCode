#ifndef GCP_UTIL_FREQUENCY_H
#define GCP_UTIL_FREQUENCY_H

/**
 * @file Frequency.h
 * 
 * Tagged: Fri Aug 20 12:44:45 PDT 2004
 * 
 * @author Erik Leitch
 */
#include <iostream>

#include "gcp/util/common/ConformableQuantity.h"
#include "gcp/util/common/Speed.h"

namespace gcp {
  namespace util {
    
    class Wavelength;
    
    class Frequency : public ConformableQuantity {
    public:
      
      // A few useful conversions
      
      static const double HzPerGHz_;
      static const double HzPerMHz_;
      static Speed lightSpeed_;
      
      class MegaHz {};
      class GigaHz {};
      class Hertz {};
      
      /**
       * Constructor.
       */
      Frequency();
      Frequency(const Hertz&  units,  double Hz);
      Frequency(const MegaHz& units, double MHz);
      Frequency(const GigaHz& units, double GHz);
      Frequency(Wavelength& wavelength);
      
      /**
       * Destructor.
       */
      virtual ~Frequency();
      
      // Set the frequency, in GHz
      
      void setGHz(double GHz);
      
      // Set the frequency, in MHz
      
      void setMHz(double MHz);
      
      // Set the frequency, in MHz
      
      void setHz(double Hz);
      
      // Return the frequency, in GHz
      
      inline double GHz() {
	return Hz_ / HzPerGHz_;
      }
      
      // Return the frequency, in MHz
      
      inline double MHz() {
	return Hz_ / HzPerMHz_;
      }
      
      inline unsigned short yigUnits() {
	return (unsigned short)MHz();
      }
      
      // Return the frequency, in Hz
      
      inline double Hz() const {
	return Hz_;
      }
      
      /**
       * Allows cout << Length
       */
      friend std::ostream& operator<<(std::ostream& os, Frequency& frequency);
      
      Frequency operator-(Frequency& frequency);
      
      void initialize();
      
    protected:
      
      // The actual frequency, in Hz
      
      double Hz_;
      
    }; // End class Frequency
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_FREQUENCY_H
