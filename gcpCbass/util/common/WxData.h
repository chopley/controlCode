#ifndef GCP_UTIL_WXDATA_H
#define GCP_UTIL_WXDATA_H

/**
 * @file WxData.h
 * 
 * Tagged: Wed 01-Feb-06 15:41:19
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/NetStruct.h"

namespace gcp {
  namespace util {
    
    class WxData : public NetStruct {
    public:

      /**
       * Constructor.
       */
      WxData();
      
      /**
       * Destructor.
       */
      virtual ~WxData();
      
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

	bool received_;

	// Write the contents of this object to an ostream

	friend std::ostream& 
	  gcp::util::operator<<(std::ostream& os, WxData& data);

	std::string header();

    }; // End class WxData

    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_WXDATA_H
