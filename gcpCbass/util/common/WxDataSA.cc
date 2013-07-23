#include "gcp/util/common/WxDataSA.h"

#include <iostream>
#include <iomanip>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
WxDataSA::WxDataSA() 
{
  airTemperatureC_      = 0;
  windDirectionDegrees_ = 0;
  
  pressure_         = 0;
  relativeHumidity_ = 0;
  windSpeed_        = 0;

  received_         = false;


  COUT("Here 0");
  NETSTRUCT_DOUBLE(airTemperatureC_);
  NETSTRUCT_DOUBLE(windDirectionDegrees_);

  NETSTRUCT_DOUBLE(pressure_);
  NETSTRUCT_DOUBLE(relativeHumidity_);
  NETSTRUCT_DOUBLE(windSpeed_);

  COUT("Here 1");
  // Read data

  NETSTRUCT_UCHAR_ARR(mtSampleTimeUchar_, 35);
  NETSTRUCT_DOUBLE(mtWindSpeed_);
  NETSTRUCT_DOUBLE(mtAdjWindDir_);
  NETSTRUCT_DOUBLE(mtTemp1_);
  NETSTRUCT_DOUBLE(mtRelHumidity_);
  NETSTRUCT_DOUBLE(mtAdjBaromPress_);
  NETSTRUCT_DOUBLE(mtDewPoint_);
  NETSTRUCT_DOUBLE(mtVaporPressure_);

  COUT("Here 2");
  NETSTRUCT_BOOL(received_);
  COUT("Here 3");
}

/**.......................................................................
 * Destructor.
 */
WxDataSA::~WxDataSA() {}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::util::operator<<(ostream& os, WxDataSA& data)
{
    os  << "airTemperatureC_ = " << setw(20) << setprecision(5) << data.airTemperatureC_ << std::endl
     << "windDirectionDegrees_ = " << setw(20) << setprecision(5) << data.windDirectionDegrees_ << std::endl

     << "pressure_ = " << setw(20) << setprecision(5) << data.pressure_ << std::endl
     << "relativeHumidity_ = " << setw(20) << setprecision(5) << data.relativeHumidity_ << std::endl
     << "windSpeed_ = " << setw(20) << setprecision(5) << data.windSpeed_ << std::endl
     << "received_ = " << setw(8) << setprecision(5) << data.received_ << std::endl

     << "mtSampleTimeUchar_ = " << setw(20) << setprecision(5) << data.mtSampleTimeUchar_ << std::endl
     << "mtWindSpeed_ = " << setw(20) << setprecision(5) << data.mtWindSpeed_ << std::endl
     << "mtAdjWindDir_ = " << setw(20) << setprecision(5) << data.mtAdjWindDir_ << std::endl
     << "mtTemp1_ = " << setw(20) << setprecision(5) << data.mtTemp1_ << std::endl
     << "mtRelHumidity_ = " << setw(20) << setprecision(5) << data.mtRelHumidity_ << std::endl
     << "mtAdjBaromPress_ = " << setw(20) << setprecision(5) << data.mtAdjBaromPress_ << std::endl
     << "mtDewPoint_ = " << setw(20) << setprecision(5) << data.mtDewPoint_ << std::endl
     << "mtVaporPressure_ = " << setw(20) << setprecision(5) << data.mtVaporPressure_;
   
  COUT("Here 0");
 
  return os;
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
std::string WxDataSA::header()
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
     << " " << setw(20) << "Wind Speed (m/s)";

  return os.str();
}
