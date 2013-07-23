#include "gcp/mediator/specific/WxClientCbass.h"
#include "gcp/mediator/specific/WxControl.h"
#include "gcp/util/common/String.h"

using namespace std;
using namespace gcp::mediator;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
WxClientCbass::WxClientCbass(WxControl* parent, std::string host, unsigned port) :
  WxClientSA(true, host, port), parent_(parent) {}

/**.......................................................................
 * Destructor.
 */
WxClientCbass::~WxClientCbass() {}

/**.......................................................................
 * Called when data have been read from the server
 */
void WxClientCbass::processServerData() 
{
  if(parent_ != 0) {

    WxControl::WxData data;

    //    COUT(wxData_);

    // there is a problem where sometimes the weather station will
    // report back all zeros.  When this is the case, we should just
    // not record the data from the weather station -- and also not
    // have the server crash -- so we check that the pressure being returned is non-zero
    float pressVal = wxData_.mtAdjBaromPress_;
    bool isValid = pressVal != 0 ;
    COUT("isValid: " << isValid);

    // one thing I have to ask Erik if it just repeats the previous
    // value or whether it will report back all zeros.
    if(isValid){
      data.sampleTime_.setToCbassWxString((const char*)wxData_.mtSampleTimeUchar_);
      data.airTemperature_.setF(wxData_.mtTemp1_);
      data.windSpeed_.setMilesPerHour(wxData_.mtWindSpeed_);
      data.windDirection_.setDegrees(wxData_.mtAdjWindDir_);
      data.pressure_.setInHg(wxData_.mtAdjBaromPress_);
      data.relativeHumidity_.setPercentMax100(wxData_.mtRelHumidity_);
    
      parent_->writeVals(data);
    }
    // otherwise we do nothing.
  }
}
