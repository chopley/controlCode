#define __FILEPATH__ "mediator/specific/WxControl.cc"

#include <sstream>

#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/WxClientCbass.h"
#include "gcp/mediator/specific/WxControl.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/MonitorPoint.h"
#include "gcp/util/common/MonitorPointManager.h"

using namespace std;
using namespace gcp::mediator;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
WxControl::WxControl(Control* parent) :
  parent_(parent) 
{
  // Set up monitor points
  
  monitor_ = new MonitorPointManager(parent_->getArrayShare());
  
  monUtc_        = monitor_->addMonitorPoint("weather", "utc");
  monAirTemp_    = monitor_->addMonitorPoint("weather", "airTemperature");
  monRelHum_     = monitor_->addMonitorPoint("weather", "relativeHumidity");
  monWindSpeed_  = monitor_->addMonitorPoint("weather", "windSpeed");
  monWindDir_    = monitor_->addMonitorPoint("weather", "windDirection");
  monPressure_   = monitor_->addMonitorPoint("weather", "pressure");
  
  // And allocate the object that will collect data from the local
  // weather station

  wx_ = new WxClientCbass(this, parent_->wxHost());
} 

/**.......................................................................
 * Destructor.
 */
WxControl::~WxControl() 
{
  if(monitor_ != 0) {
    delete monitor_;
    monitor_ = 0;
  }

  if(wx_ != 0) {
    delete wx_;
    wx_ = 0;
  }

}

/**.......................................................................
 * Service our message queue.
 */
void WxControl::serviceMsgQ()
{
  bool stop   = false;
  int  nready = 0;
  
  // Our select loop will check the msgq file descriptor for
  // readability
  
  while(!stop) {
    
    nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
		  NULL, timeOut_);
    
    // A message on our message queue?
    
    if(fdSet_.isSetInRead(msgq_.fd())) 
      processTaskMsg(&stop);
    
  }
}

/**.......................................................................
 * Overwrite the base-class method.
 */
void WxControl::processMsg(WxControlMsg* msg)
{
  switch (msg->type) {
  default:
    ReportError("WxControl::processMsg: Unrecognized message type: "
		<< msg->type);
    break;
  }
}

/**.......................................................................
 * Write the new weather values to the frame
 */
void WxControl::writeVals(WxData& data)
{
  COUT("WE GETTIN THIS SHIT?");

  gcp::util::RegDate regDate;
  regDate.setMjd(data.sampleTime_.mjd());

  monUtc_->writeReg(false,       regDate.data());
  monAirTemp_->writeReg(false,   data.airTemperature_.C());
  monRelHum_->writeReg(false,    data.relativeHumidity_.percentMax1());   
  monWindSpeed_->writeReg(false, data.windSpeed_.metersPerSec());
  monWindDir_->writeReg(false,   data.windDirection_.degrees());
  monPressure_->writeReg(false,  data.pressure_.milliBar());
  
  // Distribute weather data to the antennas
  
  distributeWeatherData(data);
}

/**.......................................................................
 * Send pertinent data to the antennas for refraction calculations
 */
void WxControl::distributeWeatherData(WxData& data)
{
  gcp::util::NetCmd netCmd;
  netCmd.packAtmosCmd(data.airTemperature_, data.relativeHumidity_, data.pressure_);
  parent_->forwardNetCmd(&netCmd);
}
