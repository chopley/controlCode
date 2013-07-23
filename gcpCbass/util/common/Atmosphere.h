#ifndef GCP_UTIL_ATMOSPHERE_H
#define GCP_UTIL_ATMOSPHERE_H

/**
 * @file Atmosphere.h
 * 
 * Tagged: Wed Dec  1 11:18:25 PST 2004
 * 
 * @author GCP data acquisition
 */
/*
 * Set the temperature lapse rate, termination precision, and optical
 * and radio wavelengths required by the slalib refraction function
 * (slaRefco()). Note that for wavelengths longward of 100 microns
 * slaRefco() uses a model that is currently independent of wavelength,
 * so the accuracy of the chosen radio wavelength is not pertinent.
 */

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Frequency.h"
#include "gcp/util/common/Length.h"
#include "gcp/util/common/Percent.h"
#include "gcp/util/common/Pressure.h"
#include "gcp/util/common/Temperature.h"
#include "gcp/util/common/Wavelength.h"

namespace gcp {
  namespace util {
    
    class Atmosphere {
    public:
      
      // As suggested by slalib documentation (deg K/meter)
      
      static const double tropoLapseRate_;
      
      // Refraction calculation will be done to 10 milli-arcseconds accuracy 
      
      static Angle refracAccuracy_;
      
      // Near the center of the visible spectrum
      
      static Wavelength opticalWavelength_;
      
      /**
       * Constructor.
       */
      Atmosphere();
      
      /**
       * Destructor.
       */
      virtual ~Atmosphere();
      
      struct RefractionCoefficients {
	double a;
	double b;
      };
      
      /**
       * Fully-specified method for computing refraction coefficients
       */
      RefractionCoefficients
	refractionCoefficients(Length altitude,   
			       Temperature airTemp,
			       Pressure pressure, 
			       Percent humidity,
			       Wavelength wavelength,
			       Angle latitude,
			       double tropoLapseRate,
			       Angle accuracy);
      
      // Default methods using internal members
      
      RefractionCoefficients
	refractionCoefficients();
      
      RefractionCoefficients
	opticalRefractionCoefficients();
      
      // Set parameters we need for the refraction calc
      
      enum {
	NONE       =  0x0,
	TEMP       =  0x1,
	PRESSURE   =  0x2,
	HUMIDITY   =  0x4,
	ALTITUDE   =  0x8,
	LATITUDE   = 0x10,
	WAVE       = 0x20,
	ALL        = TEMP|PRESSURE|HUMIDITY|ALTITUDE|LATITUDE|WAVE,
	ALLOPTICAL = TEMP|PRESSURE|HUMIDITY|ALTITUDE|LATITUDE,
      };
      
      void setAirTemperature(Temperature airTemp);
      
      void setPressure(Pressure pressure);
      
      void setHumidity(Percent humidity);
      
      void setFrequency(Frequency frequency);
      
      void setAltitude(Length altitude);
      
      void setLatitude(Angle latitude);
      
      void setWavelength(Wavelength wavelength);
      
      bool canComputeRefraction();
      
      bool canComputeOpticalRefraction();
      
    protected:
      
      Temperature airTemperature_;
      Length altitude_;
      Angle latitude_;
      Wavelength wavelength_;
      Percent humidity_;
      Pressure pressure_;
      
      unsigned lacking_;
      
      void initialize();
      
    }; // End class Atmosphere
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_ATMOSPHERE_H
