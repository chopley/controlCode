#include "gcp/antenna/control/specific/CryoConResp.h"
#include "gcp/antenna/control/specific/AntennaException.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructors
 */
CryoConResp::CryoConResp() : Board() {
  share_ = 0;
}

CryoConResp::CryoConResp(SpecificShare* share, string name) : Board(share, name) {
  tempSensor_ = 0;
  heaterCurrent_ = 0;

  tempSensor_ = findReg("temperature_load");
  heaterCurrent_ = findReg("heater_current");
}

/**.......................................................................
 * Destructor.
 */
CryoConResp::~CryoConResp() {}

/**.......................................................................
 * Requests (and records) the load temperature
 */
void CryoConResp::requestLoadTemperature() {

  float tempVal;

  tempVal = queryChannelTemperature(0);
  if(share_){
    share_->writeReg(tempSensor_, tempVal);
  };
  return;
}



/**.......................................................................
 * Requests (and records) the heater current (in percentage)
 */
void CryoConResp::requestHeaterOutput() {

  float heatVal;

  heatVal = queryHeaterCurrent();
  if(share_){
    share_->writeReg(heaterCurrent_, heatVal);
  };
  return;
}

