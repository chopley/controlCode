#ifndef GCP_UTIL_HPDCPOWERSUPPLY_H
#define GCP_UTIL_HPDCPOWERSUPPLY_H

/**
 * @file HpDcPowerSupply.h
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
    
    class HpDcPowerSupply : public GpibUsbDevice {
    public:
      
      /**
       * Constructor.
       */
      HpDcPowerSupply(bool doSpawn=false);
      HpDcPowerSupply(std::string port, bool doSpawn=false);
      
      /**
       * Destructor.
       */
      virtual ~HpDcPowerSupply();

      //------------------------------------------------------------
      // Device commands
      //------------------------------------------------------------

      double setVoltage(double voltage);
      double getVoltage();

    private:
      
      static GPIB_RESPONSE_HANDLER(checkVoltage);

    }; // End class HpDcPowerSupply
    
  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_HPDCPOWERSUPPLY_H
