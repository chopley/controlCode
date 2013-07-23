#ifndef GCP_UTIL_WXDATA40M_H
#define GCP_UTIL_WXDATA40M_H

/**
 * @file WxData40m.h
 * 
 * Tagged: Wed 01-Feb-06 15:41:19
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetStruct.h"

namespace gcp {
  namespace util {
    
    class WxData40m : public NetStruct {
    public:

      /**
       * Constructor.
       */
      WxData40m();
      
      /**
       * Destructor.
       */
      virtual ~WxData40m();
      
      unsigned year_;
      unsigned day_;
      unsigned hour_;
      unsigned min_;

      double airTemperatureC_;
      double internalTemperatureC_;
      double windDirectionDegrees_;

      double pressure_;
      double relativeHumidity_;
      double windSpeed_;
      double batteryVoltage_;

      unsigned short power_;


      std::string mtSampleTime_;
      unsigned char mtSampleTimeUchar_[25];

      double mtWindSpeed_;
      double mtAdjWindDir_;
      double mt3SecRollAvgWindSpeed_;
      double mt3SecRollAvgWindDir_;
      double mt2MinRollAvgWindSpeed_;
      double mt2MinRollAvgWindDir_;
      double mt10MinRollAvgWindSpeed_;
      double mt10MinRollAvgWindDir_;
      double mt10MinWindGustDir_;
      double mt10MinWindGustSpeed_;
      std::string mt10MinWindGustTime_;
      double mt60MinWindGustDir_;
      double mt60MinWindGustSpeed_;
      std::string mt60MinWindGustTime_;
      double mtTemp1_;
      double mtRelHumidity_;
      double mtDewPoint_;
      double mtRawBaromPress_;
      double mtAdjBaromPress_;
      double mtVaporPressure_;
      double mtRainToday_;
      double mtRainRate_;

      bool received_;

      // Write the contents of this object to an ostream

      friend std::ostream& 
	gcp::util::operator<<(std::ostream& os, WxData40m& data);

      std::string header();

    }; // End class WxData40m

    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_WXDATA40M_H
