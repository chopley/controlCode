#include "gcp/util/common/WxData.h"

#include <iostream>
#include <iomanip>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
WxData::WxData() 
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

  NETSTRUCT_BOOL(received_);
}

/**.......................................................................
 * Destructor.
 */
WxData::~WxData() {}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::util::operator<<(ostream& os, WxData& data)
{
  os << " " << setw(8) << setprecision(5) << data.year_
     << " " << setw(8) << setprecision(5) << data.day_ 
     << " " << setw(8) << setprecision(5) << data.hour_
     << " " << setw(8) << setprecision(5) << data.min_ 

     << " " << setw(20) << setprecision(5) << data.airTemperatureC_
     << " " << setw(20) << setprecision(5) << data.internalTemperatureC_
     << " " << setw(20) << setprecision(5) << data.windDirectionDegrees_

     << " " << setw(20) << setprecision(5) << data.pressure_
     << " " << setw(20) << setprecision(5) << data.relativeHumidity_
     << " " << setw(20) << setprecision(5) << data.windSpeed_
     << " " << setw(20) << setprecision(5) << data.batteryVoltage_

     << " " << setw(8) << setprecision(5) << data.power_
     << " " << setw(8) << setprecision(5) << data.received_;
    
  return os;
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
std::string WxData::header()
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
