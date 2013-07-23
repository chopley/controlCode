#ifndef GCP_UTIL_WXDATASA_H
#define GCP_UTIL_WXDATASA_H

/**
 * @file WxDataSA.h
 * 
 * Tagged: May 15, 2013
 * 
 * @author Stephen Muchovej
 */
#include "gcp/util/common/NetStruct.h"

namespace gcp {
  namespace util {
    
    class WxDataSA : public NetStruct {
    public:

      /**
       * Constructor.
       */
      WxDataSA();
      
      /**
       * Destructor.
       */
      virtual ~WxDataSA();
      
      double airTemperatureC_;
      double windDirectionDegrees_;

      double pressure_;
      double relativeHumidity_;
      double windSpeed_;

      std::string mtSampleTime_;
      unsigned char mtSampleTimeUchar_[35];
      double mtWindSpeed_;
      double mtAdjWindDir_;
      double mtTemp1_;
      double mtRelHumidity_;
      double mtAdjBaromPress_;

      double mtDewPoint_;
      double mtVaporPressure_;

      bool received_;

      // Write the contents of this object to an ostream

      friend std::ostream& 
	gcp::util::operator<<(std::ostream& os, WxDataSA& data);

      std::string header();

    }; // End class WxDataSA

    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_WXDATASA_H
