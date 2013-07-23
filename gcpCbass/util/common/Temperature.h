#ifndef GCP_UTIL_TEMPERATURE_H
#define GCP_UTIL_TEMPERATURE_H

/**
 * @file Temperature.h
 * 
 * Tagged: Wed Dec  1 11:48:14 PST 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/ConformableQuantity.h"
#include "gcp/util/common/Unit.h"

namespace gcp {
  namespace util {
    
    class Temperature : public ConformableQuantity {
    public:
      
      class Kelvin     : public Unit {
      public:
	void addNames();
      };

      class Centigrade{};
      class Celsius   {};
      class Fahrenheit{};

      /**
       * Constructor.
       */
      Temperature();
      Temperature(const Kelvin& units,     double kelvins);
      Temperature(const Centigrade& units, double centigrade);
      Temperature(const Celsius& units,    double celsius);
      Temperature(const Fahrenheit& units, double fahrenheit);
      
      /**
       * Destructor.
       */
      virtual ~Temperature();
      
      void setC(double centigrade);
      void setF(double fahrenheit);
      void setK(double kelvin);

      double C();
      double F();
      double K();

      static const double kelvinZeroPointInC_;

      void initialize();

      // Allows cout << Temperature

      friend std::ostream& operator<<(std::ostream& os, Temperature& temp);

    private:

      double centigrade_;

    }; // End class Temperature
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_TEMPERATURE_H
