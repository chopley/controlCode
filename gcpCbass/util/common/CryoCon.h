#ifndef GCP_UTIL_CRYOCON_H
#define GCP_UTIL_CRYOCON_H
/**
 * @file CryoCon.h
 * 
 * Tagged: Thu Oct 18 15:49:12 PDT 2007
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/GpibUsbDevice.h"
#include <string>
namespace gcp {
  namespace util {
    
    // Class to communciate with the CryoCon Temperature Controller

    class CryoCon : public GpibUsbDevice {
    public:
      
      /**
       * Constructor.
       */
      CryoCon(bool doSpawn=false);
      CryoCon(std::string port, bool doSpawn=false);
      CryoCon(GpibUsbController& controller);

      /**
       * Destructor.
       */
      virtual ~CryoCon();

      /**
       * Sets up the control Loop Parameters 
       */
      void setUpLoop(int loopNum=1);
      void setUpLoop(int loopNum, std::vector<float>& values);

      /**
       * Heat up the sensor
       */
      void heatUpSensor(int loopNum=1);

      /**
       * Resume cooling with PID
       */
      void resumeCooling(int loopNum=1);

      /**
       * Query the temperature 
       */
      float queryChannelTemperature(int val=0);

      /**
       * Query the Heater current
       */
      float queryHeaterCurrent();

      //------------------------------------------------------------
      // Device commands  *see pages 135-216 of manual.
      //------------------------------------------------------------
      /**
       * Set Input Units to Kelvin
       */
      void setInputUnits();

      /**
       * Clear Status
       */
      void clearStatus();

      /**
       * Reset Module
       */
      void resetModule();

      /**
       * Stop the control loop
       */
      void stopControlLoop();

      /**
       * Engage Control Loop
       */
      void engageControlLoop();

      /**
       * Set Sky temperature
       */
      void setSkyTemp(int loopNum=1, float val=6.5);

      /**
       * Set Source Channel
       */
      void setSourceChannel(int loopNum=1, int val=1);

      /**
       * Set Loop Range
       */
      void setLoopRange(int loopNum=1, int val=1);

      /**
       * Set P Gain factor in PID.
       */
      void setPGain(int loopNum=1, float val=0.5);

      /**
       * Set I Gain factor in PID.
       */
      void setIGain(int loopNum=1, float val=1);

      /**
       * Set D Gain factor in PID.
       */
      void setDGain(int loopNum=1, float val=0);

      /**
       * Set Control Loop Manual Power Output Setting
       */
      void setPowerOutput(int loopNum=1, float val=50);

      /**
       * Set Heater Load Resistance
       */
      void setHeaterLoad(int loopNum=1, float val=50);

      /**
       * Set Control Loop Type
       */
      void setControlLoopType(int loopNum=1, int val=1);


    private:

    }; // End class CryoCon
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_CRYOCON_H
