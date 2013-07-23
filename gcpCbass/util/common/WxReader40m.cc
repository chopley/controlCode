#include "gcp/util/common/Exception.h"
#include "gcp/util/common/String.h"
#include "gcp/util/common/WxReader40m.h"

#include<curl/curl.h>

#include <string.h>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
WxReader40m::WxReader40m() {}

/**.......................................................................
 * Destructor.
 */
WxReader40m::~WxReader40m() {}

void WxReader40m::getData()
{
  getUrl("http://wx.cm.pvt/latestsampledata.xml", true);

  String str = lastRead_.str();

  CTOUT("Read string: " << str);

  try {
    data_.mtSampleTime_            = str.findNextInstanceOf("\"mtSampTime\">", "</meas>").str();
    strcpy((char*)data_.mtSampleTimeUchar_, data_.mtSampleTime_.c_str());

    data_.mtWindSpeed_             = str.findNextInstanceOf("\"mtWindSpeed\">", "</meas>").toDouble();
    data_.mtAdjWindDir_            = str.findNextInstanceOf("\"mtAdjWindDir\">", "</meas>").toDouble();
    data_.mt3SecRollAvgWindSpeed_  = str.findNextInstanceOf("\"mt3SecRollAvgWindSpeed\">", "</meas>").toDouble();
    data_.mt3SecRollAvgWindDir_    = str.findNextInstanceOf("\"mt3SecRollAvgWindDir\">", "</meas>").toDouble();
    data_.mt2MinRollAvgWindSpeed_  = str.findNextInstanceOf("\"mt2MinRollAvgWindSpeed\">", "</meas>").toDouble();
    data_.mt2MinRollAvgWindDir_    = str.findNextInstanceOf("\"mt2MinRollAvgWindDir\">", "</meas>").toDouble();
    data_.mt10MinRollAvgWindSpeed_ = str.findNextInstanceOf("\"mt10MinRollAvgWindSpeed\">", "</meas>").toDouble();
    data_.mt10MinRollAvgWindDir_   = str.findNextInstanceOf("\"mt10MinRollAvgWindDir\">", "</meas>").toDouble();
    data_.mt10MinWindGustDir_      = str.findNextInstanceOf("\"mt10MinWindGustDir\">", "</meas>").toDouble();
    data_.mt10MinWindGustSpeed_    = str.findNextInstanceOf("\"mt10MinWindGustSpeed\">", "</meas>").toDouble();
    data_.mt10MinWindGustTime_     = str.findNextInstanceOf("\"mt10MinWindGustTime\">", "</meas>").str();
    data_.mt60MinWindGustDir_      = str.findNextInstanceOf("\"mt60MinWindGustDir\">", "</meas>").toDouble();
    data_.mt60MinWindGustSpeed_    = str.findNextInstanceOf("\"mt60MinWindGustSpeed\">", "</meas>").toDouble();
    data_.mt60MinWindGustTime_     = str.findNextInstanceOf("\"mt60MinWindGustTime\">", "</meas>").str();
    data_.mtTemp1_                 = str.findNextInstanceOf("\"mtTemp1\">", "</meas>").toDouble();
    data_.mtRelHumidity_           = str.findNextInstanceOf("\"mtRelHumidity\">", "</meas>").toDouble();
    data_.mtDewPoint_              = str.findNextInstanceOf("\"mtDewPoint\">", "</meas>").toDouble();
    data_.mtRawBaromPress_         = str.findNextInstanceOf("\"mtRawBaromPress\">", "</meas>").toDouble();
    data_.mtAdjBaromPress_         = str.findNextInstanceOf("\"mtAdjBaromPress\">", "</meas>").toDouble();
    data_.mtVaporPressure_         = str.findNextInstanceOf("\"mtVaporPressure\">", "</meas>").toDouble();
    data_.mtRainToday_             = str.findNextInstanceOf("\"mtRainToday\">", "</meas>").toDouble();
    data_.mtRainRate_              = str.findNextInstanceOf("\"mtRainRate\">", "</meas>").toDouble();
    data_.received_ = true;

  } catch(Exception& err) {

    COUT("Caught an exception: " << err.what());

    data_.received_ = false;
  }

}
