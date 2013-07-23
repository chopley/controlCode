#include "gcp/antenna/control/specific/LakeShoreTemps.h"
#include "gcp/antenna/control/specific/AntennaException.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructors
 */
LakeShoreTemps::LakeShoreTemps() : Board() {
  share_ = 0;
}

LakeShoreTemps::LakeShoreTemps(SpecificShare* share, string name) : Board(share, name) {
  tempSensors_ = 0;
  tempSensors_ = findReg("temperature_sensors");
}

/**.......................................................................
 * Destructor.
 */
LakeShoreTemps::~LakeShoreTemps() {}

/**.......................................................................
 * Query all temperatures
 */
void LakeShoreTemps::requestAllTemperatures()
{
  int i;
  std::string tempVals;
  String tempString(tempVals);
  float temps[8];

   COUT("Lakeshore Requestin Temps");
  /* There are 8 probes, we ask for each individually */
  for(i=1;i<9;i++){
    //    tempVals = requestMonitor(i);
    String tempString(tempVals);
    temps[i] = tempString.toFloat();
    COUT("Writing the following "<<temps[i]<< "to shared memory");
  };
    COUT("Writing the following "<<temps[0]<< "to shared memory");

  /* record them to the data stream */
  if(share_) { 
    share_->writeReg(tempSensors_, temps);
  };
  /* that should do it */
};

