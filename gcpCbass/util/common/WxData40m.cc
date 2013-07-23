#include "gcp/util/common/WxData40m.h"

#include <iostream>
#include <iomanip>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
WxData40m::WxData40m() 
{
  year_ = 0;
  day_  = 0;
  hour_ = 0;
  min_  = 0;
  
  airTemperatureC_      = 0;
  internalTemperatureC_ = 0;
  windDirectionDegrees_ = 0;
  
  pressure_         = 0;
  relativeHumidity_ = 0;
  windSpeed_        = 0;
  batteryVoltage_   = 0;
  
  power_            = 0;
  received_         = false;

  COUT("Here 0");
  NETSTRUCT_UINT(year_);
  NETSTRUCT_UINT(day_);
  NETSTRUCT_UINT(hour_);
  NETSTRUCT_UINT(min_);

  NETSTRUCT_DOUBLE(airTemperatureC_);
  NETSTRUCT_DOUBLE(internalTemperatureC_);
  NETSTRUCT_DOUBLE(windDirectionDegrees_);

  NETSTRUCT_DOUBLE(pressure_);
  NETSTRUCT_DOUBLE(relativeHumidity_);
  NETSTRUCT_DOUBLE(windSpeed_);
  NETSTRUCT_DOUBLE(batteryVoltage_);

  NETSTRUCT_USHORT(power_);

  COUT("Here 1");
  // Read data

  NETSTRUCT_UCHAR_ARR(mtSampleTimeUchar_, 25);
  NETSTRUCT_DOUBLE(mtWindSpeed_);
  NETSTRUCT_DOUBLE(mtAdjWindDir_);
  NETSTRUCT_DOUBLE(mt3SecRollAvgWindSpeed_);
  NETSTRUCT_DOUBLE(mt3SecRollAvgWindDir_);
  NETSTRUCT_DOUBLE(mt2MinRollAvgWindSpeed_);
  NETSTRUCT_DOUBLE(mt2MinRollAvgWindDir_);
  NETSTRUCT_DOUBLE(mt10MinRollAvgWindSpeed_);
  NETSTRUCT_DOUBLE(mt10MinRollAvgWindDir_);
  NETSTRUCT_DOUBLE(mt10MinWindGustDir_);
  NETSTRUCT_DOUBLE(mt10MinWindGustSpeed_);

  NETSTRUCT_DOUBLE(mt60MinWindGustDir_);
  NETSTRUCT_DOUBLE(mt60MinWindGustSpeed_);

  NETSTRUCT_DOUBLE(mtTemp1_);
  NETSTRUCT_DOUBLE(mtRelHumidity_);
  NETSTRUCT_DOUBLE(mtDewPoint_);
  NETSTRUCT_DOUBLE(mtRawBaromPress_);
  NETSTRUCT_DOUBLE(mtAdjBaromPress_);
  NETSTRUCT_DOUBLE(mtVaporPressure_);
  NETSTRUCT_DOUBLE(mtRainToday_);
  NETSTRUCT_DOUBLE(mtRainRate_);

  COUT("Here 2");
  NETSTRUCT_BOOL(received_);
  COUT("Here 3");
}

/**.......................................................................
 * Destructor.
 */
WxData40m::~WxData40m() {}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::util::operator<<(ostream& os, WxData40m& data)
{
  os << "year_ = " << setw(8) << setprecision(5) << data.year_ << std::endl
     << "day_ = " << setw(8) << setprecision(5) << data.day_  << std::endl
     << "hour_ = " << setw(8) << setprecision(5) << data.hour_ << std::endl
     << "min_ = " << setw(8) << setprecision(5) << data.min_  << std::endl

     << "airTemperatureC_ = " << setw(20) << setprecision(5) << data.airTemperatureC_ << std::endl
     << "internalTemperatureC_ = " << setw(20) << setprecision(5) << data.internalTemperatureC_ << std::endl
     << "windDirectionDegrees_ = " << setw(20) << setprecision(5) << data.windDirectionDegrees_ << std::endl

     << "pressure_ = " << setw(20) << setprecision(5) << data.pressure_ << std::endl
     << "relativeHumidity_ = " << setw(20) << setprecision(5) << data.relativeHumidity_ << std::endl
     << "windSpeed_ = " << setw(20) << setprecision(5) << data.windSpeed_ << std::endl
     << "batteryVoltage_ = " << setw(20) << setprecision(5) << data.batteryVoltage_ << std::endl

     << "power_ = " << setw(8) << setprecision(5) << data.power_ << std::endl
     << "received_ = " << setw(8) << setprecision(5) << data.received_ << std::endl

     << "mtSampleTime_ = " << setw(20) << setprecision(5) << data.mtSampleTime_ << std::endl
     << "mtSampleTimeUchar_ = " << setw(20) << setprecision(5) << data.mtSampleTimeUchar_ << std::endl
     << "mtWindSpeed_ = " << setw(20) << setprecision(5) << data.mtWindSpeed_ << std::endl
     << "mtAdjWindDir_ = " << setw(20) << setprecision(5) << data.mtAdjWindDir_ << std::endl
     << "mt3SecRollAvgWindSpeed_ = " << setw(20) << setprecision(5) << data.mt3SecRollAvgWindSpeed_ << std::endl
     << "mt3SecRollAvgWindDir_ = " << setw(20) << setprecision(5) << data.mt3SecRollAvgWindDir_ << std::endl
     << "mt2MinRollAvgWindSpeed_ = " << setw(20) << setprecision(5) << data.mt2MinRollAvgWindSpeed_ << std::endl
     << "mt2MinRollAvgWindDir_ = " << setw(20) << setprecision(5) << data.mt2MinRollAvgWindDir_ << std::endl
     << "mt10MinRollAvgWindSpeed_ = " << setw(20) << setprecision(5) << data.mt10MinRollAvgWindSpeed_ << std::endl
     << "mt10MinRollAvgWindDir_ = " << setw(20) << setprecision(5) << data.mt10MinRollAvgWindDir_ << std::endl
     << "mt10MinWindGustDir_ = " << setw(20) << setprecision(5) << data.mt10MinWindGustDir_ << std::endl
     << "mt10MinWindGustSpeed_ = " << setw(20) << setprecision(5) << data.mt10MinWindGustSpeed_ << std::endl
     << "mt10MinWindGustTime_ = " << setw(20) << setprecision(5) << data.mt10MinWindGustTime_ << std::endl
     << "mt60MinWindGustDir_ = " << setw(20) << setprecision(5) << data.mt60MinWindGustDir_ << std::endl
     << "mt60MinWindGustSpeed_ = " << setw(20) << setprecision(5) << data.mt60MinWindGustSpeed_ << std::endl
     << "mt60MinWindGustTime_ = " << setw(20) << setprecision(5) << data.mt60MinWindGustTime_ << std::endl
     << "mtTemp1_ = " << setw(20) << setprecision(5) << data.mtTemp1_ << std::endl
     << "mtRelHumidity_ = " << setw(20) << setprecision(5) << data.mtRelHumidity_ << std::endl
     << "mtDewPoint_ = " << setw(20) << setprecision(5) << data.mtDewPoint_ << std::endl
     << "mtRawBaromPress_ = " << setw(20) << setprecision(5) << data.mtRawBaromPress_ << std::endl
     << "mtAdjBaromPress_ = " << setw(20) << setprecision(5) << data.mtAdjBaromPress_ << std::endl
     << "mtVaporPressure_ = " << setw(20) << setprecision(5) << data.mtVaporPressure_ << std::endl
     << "mtRainToday_ = " << setw(20) << setprecision(5) << data.mtRainToday_ << std::endl
     << "mtRainRate_ = " << setw(20) << setprecision(5) << data.mtRainRate_;
   
  COUT("Here 0");
 
  return os;
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
std::string WxData40m::header()
{
  ostringstream os;

  os << setw(8) << "Year"
     << " " << setw(8) << "Day"
     << " " << setw(8) << "Hour"
     << " " << setw(8) << "Min" 
    
     << " " << setw(20) << "Air Temp (C)"
     << " " << setw(20) << "Int Temp (C)"  
     << " " << setw(20) << "Wind Dir (deg)"
    
     << " " << setw(20) << "Pressure (mB)"
     << " " << setw(20) << "Rel Hum (%)"
     << " " << setw(20) << "Wind Speed (m/s)"
     << " " << setw(20) << "Batt. Voltage (V)"
    
     << " " << setw(8) << "Power"; 

  return os.str();
}
