#include "gcp/util/common/Exception.h"
#include "gcp/util/common/String.h"
#include "gcp/util/common/WxReaderSA.h"

#include<curl/curl.h>

#include <string.h>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
WxReaderSA::WxReaderSA() {
  defaultFilename_ = "/home/cbassuser/cronjob_scripts/weatherData.txt";
}

/**.......................................................................
 * Destructor.
 */
WxReaderSA::~WxReaderSA() {}

void WxReaderSA::getData()
{
  std::string latestData;
  
  try{
    latestData = getFileContents(defaultFilename_);
  } catch (Exception& err) {
    COUT("ERROR READING FILE");
    COUT("Caught an exception: " << err.what());
  };

  String str;
  str = String(latestData);
  
  CTOUT("Read string: " << str);

  try {
#if(0)
    data_.weekDay_                 = str.findNextStringSeparatedByChars(" ").str();
    data_.month_                   = str.findNextStringSeparatedByChars(" ", " ").str();
    data_.day_                     = str.findNextStringSeparatedByChars(" ", " ").toInt();
    data_.mtSampleTime_            = str.findNextStringSeparatedByChars(" ", " ").str();
    data_.year2_                   = str.findNextStringSeparatedByChars(" ", " ").str();
    data_.year_                   = str.findNextStringSeparatedByChars(" ", "W").toInt();
#endif
    data_.mtSampleTime_            = str.findNextStringSeparatedByChars("-").str();
    data_.airTemperatureC_         = str.findNextInstanceOf("=", "degC").toDouble();
    data_.pressure_                = str.findNextInstanceOf(",", "mbar").toDouble();
    data_.relativeHumidity_        = str.findNextInstanceOf(",", "%").toDouble();
    data_.windSpeed_               = str.findNextInstanceOf(",", "m/s").toDouble();
    data_.windDirectionDegrees_    = str.findNextInstanceOf(",", "deg").toDouble();
    data_.mtVaporPressure_         = str.findNextInstanceOf(",", "mm").toDouble();
    data_.mtDewPoint_              = str.findNextInstanceOf(",", "degC").toDouble();

    if(data_.pressure_ == 0){
      data_.received_ = false;
    } else {
      data_.received_ = true;
    }
    data_.mtSampleTime_.append("P");
    strcpy((char*)data_.mtSampleTimeUchar_, data_.mtSampleTime_.c_str());
    //    strcpy((char*)data_.monthUchar_, data_.month_.c_str());
    //    strcpy((char*)data_.weekDayUchar_, data_.weekDay_.c_str());

    data_.mtTemp1_         = data_.airTemperatureC_*9/5+32;  // to convert to F for WxClientCbass
    data_.mtWindSpeed_     = data_.windSpeed_*3600/1600;// to convert to mph for WxClientCbass
    data_.mtAdjWindDir_    = data_.windDirectionDegrees_;
    data_.mtAdjBaromPress_ = data_.pressure_*0.750061683/25.4; // to convert from mbar to inches Hg
    data_.mtRelHumidity_   = data_.relativeHumidity_;

    


#if(1)
    //    COUT("Today is " << data_.weekDayUchar_ << ", the " << data_.day_ << "th of " << data_.monthUchar_ << " in the yaer of our lord " << data_.year_);
    COUT("time    : " << data_.mtSampleTimeUchar_);
    COUT("air temp: " << data_.airTemperatureC_);
    COUT("pressure: " << data_.pressure_);
    COUT("relHumid: " << data_.relativeHumidity_);
    COUT("windSpeed:" << data_.windSpeed_);
    COUT("windDir:  " << data_.windDirectionDegrees_);
    COUT("vapPress: " << data_.mtVaporPressure_);
    COUT("dewPoint: " << data_.mtDewPoint_);
#endif

  } catch(Exception& err) {

    COUT("Caught an exception: " << err.what());

    data_.received_ = false;
  }

  return;
}

std::string WxReaderSA::getFileContents(const char *filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
    {
      std::string contents;
      in.seekg(0, std::ios::end);
      contents.resize(in.tellg());
      in.seekg(0, std::ios::beg);
      in.read(&contents[0], contents.size());
      in.close();
      return(contents);
    }
  throw(errno);
}
