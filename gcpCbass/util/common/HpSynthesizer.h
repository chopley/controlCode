#ifndef GCP_UTIL_HPSYNTHESIZER_H
#define GCP_UTIL_HPSYNTHESIZER_H

/**
 * @file HpSynthesizer.h
 * 
 * Tagged: Thu Oct 18 15:49:12 PDT 2007
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/Frequency.h"
#include "gcp/util/common/GpibUsbDevice.h"
#include "gcp/util/common/Power.h"

namespace gcp {
  namespace util {
    
    // Class to communciate with the GCP Agilent E4424B Signal
    // Generator (synthesizer)

    class HpSynthesizer : public GpibUsbDevice {
    public:
      
      /**
       * Constructor.
       */
      HpSynthesizer(bool doSpawn=false);
      HpSynthesizer(std::string port, bool doSpawn=false);
      HpSynthesizer(GpibUsbController& controller);
      
      /**
       * Destructor.
       */
      virtual ~HpSynthesizer();

      //------------------------------------------------------------
      // Device commands
      //------------------------------------------------------------

      Frequency setFrequency(Frequency freq);
      void setFrequencyNoResp(Frequency freq);
      Frequency getFrequency();

      Power setOutputPower(Power pow);
      Power getOutputPower();

      bool enableRfOutput(bool enable);
      bool queryRfOutputEnabled();

      bool enableOutputMod(bool enable);
      bool queryOutputModEnabled();

    private:
      
      static GPIB_RESPONSE_HANDLER(checkPower);
      static GPIB_RESPONSE_HANDLER(checkFrequency);
      static GPIB_RESPONSE_HANDLER(checkRfOutput);
      static GPIB_RESPONSE_HANDLER(checkOutputMod);

    }; // End class HpSynthesizer
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_HPSYNTHESIZER_H
