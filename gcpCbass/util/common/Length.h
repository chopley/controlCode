#ifndef GCP_UTIL_LENGTH_H
#define GCP_UTIL_LENGTH_H

/**
 * @file Length.h
 * 
 * Tagged: Wed Aug 25 02:58:04 PDT 2004
 * 
 * @author Erik Leitch
 */
#include <iostream>
#include <cmath>

#include "gcp/util/common/ConformableQuantity.h"

namespace gcp {
  namespace util {
    
    class Length : public ConformableQuantity {
    public:
      
      class Centimeters {};
      class Meters {};
      class Kilometers {};
      class Microns {};
      
      // Scale factors used by this class

      static const unsigned cmPerMeter_;
      static const unsigned cmPerKm_;
      static const unsigned micronsPerCm_;

      static const double metersPerMile_;
      static const double cmPerMile_;
      
      /**
       * Constructor.
       */
      Length();
      Length(const Centimeters& units, double cm);
      Length(const Meters& units, double m);
      
      /**
       * Copy constructor
       */
      Length(const Length& length);
      
      /**
       * Destructor.
       */
      virtual ~Length();
      
      /**
       * Set the length of this object
       */
      void setCentimeters(double cm) 
	{
	  cm_ = cm;
	  finite_ = finite(cm);
	}
      
      void setMeters(double m) 
	{
	  setCentimeters(m * cmPerMeter_);
	}
      
      void setKilometers(double km) 
	{
	  setCentimeters(km * cmPerMeter_);
	}
      
      /**
       * Get the length of this object
       */
      inline double centimeters() const {
	return cm_;
      }
      
      inline double meters() const {
	return cm_ / cmPerMeter_;
      }
      
      inline double kilometers() const {
	return cm_ / cmPerKm_;
      }
      
      // Operators
      
      /** 
       * Add two Lengths
       */
      Length operator+(Length& length);
      
      /** 
       * Subtract two Lengths
       */
      Length operator-(Length& length);
      
      /** 
       * Multiply a length by a constant
       */
      Length operator*(double multFac);
      
      /** 
       * Divide two Lengths
       */
      double operator/(Length& length);
      
      /**
       * Allows cout << Length
       */
      friend std::ostream& operator<<(std::ostream& os, Length& length);
      
      void initialize();
      
    protected:
      
      double cm_;
      
    }; // End class Length
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_LENGTH_H
