#ifndef GCP_UTIL_LSTHERMAL_H
#define GCP_UTIL_LSTHERMAL_H
/**
 * @file LsThermal.h
 * 
 * Tagged: Thu Oct 18 15:49:12 PDT 2007
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/GpibUsbDevice.h"

namespace gcp {
  namespace util {
    
    // Class to communciate with the LakeShore 218 

    class LsThermal : public GpibUsbDevice { 

    public:
      
      /**
       * Constructor.
       */
      LsThermal(bool doSpawn=false);
      LsThermal(std::string port, bool doSpawn=false);
      LsThermal(GpibUsbController& controller);
      
      /**
       * Destructor.
       */
      virtual ~LsThermal();

      //------------------------------------------------------------
      // Device commands  *see pages 6-14 to 6-30 of manual.
      //------------------------------------------------------------

      /**
       * Request Temperature Monitor
       */
      std::vector<float> requestMonitor(int monitorNumber=-1);

      std::string queryDataLog();

      /**
       * Request Analog Output
       */
      std::vector<float> requestAnalogOutput(int monitorNumber);

      /**
       * Reset Module
       */
      void resetModule();

    private:
      std::vector<float> parseRegularResponse(std::string responseString, int numValues=1);

    }; // End class LsThermal
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_LSTHERMAL_H
