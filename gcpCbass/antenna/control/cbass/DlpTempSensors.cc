#include "gcp/antenna/control/specific/DlpTempSensors.h"
#include "gcp/antenna/control/specific/AntennaException.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

#define TEMP_TOLERANCE 1.5

/**.......................................................................
 * Constructors
 */
DlpTempSensors::DlpTempSensors() : Board(), SpawnableTask<DlpTempSensorsMsg>(0) {
  share_ = 0;
  dlp_   = 0;
  // set up previous temperatures
  int i;
  for (i=0;i<NUM_DLP_TEMP_SENSORS;i++){
    prevTempVals_[i] = 0;
  };
  connect();

}

DlpTempSensors::DlpTempSensors(SpecificShare* share, string name, bool spawn) : Board(share, name), SpawnableTask<DlpTempSensorsMsg>(spawn)   {
  dlpTempSensors_ = 0;
  dlpTempSensors_ = findReg("dlpTemperatureSensors");
  dlp_ = 0;
  // set up previous temperatures
  int i;
  for (i=0;i<NUM_DLP_TEMP_SENSORS;i++){
    prevTempVals_[i] = 0;
  };
  connect();
}

/**.......................................................................
 * Destructor.
 */
DlpTempSensors::~DlpTempSensors() {
  if(dlp_) {
    dlp_->disconnect();
  };
}

/**.......................................................................
 * Attempt to connect to the device
 */
bool DlpTempSensors::connect()
{

  try{
    dlp_ = new gcp::util::DlpUsbThermal();

    // Connect and initialize
    dlp_->connect();
    dlp_->setupDefault();

  } catch(Exception& err) {

    ThrowError("DlpTempSensors: Device not connected");
    return false;
  }

  return true;
}



/**.......................................................................
 * Query all temperatures
 */
void DlpTempSensors::requestAllTemperatures()
{
  int i;
  std::vector<float> temps(NUM_DLP_TEMP_SENSORS);
  temps = dlp_->queryAllTemps();
  float deltaTemp;
  
  // Check to see if we have a bogus response.  if we do, don't write
  // out the frame..
  isValid_ = true;
  int numZeros = 0;

  // check that sensors reported back decent values
  for (i=0;i<NUM_DLP_TEMP_SENSORS-1;i++) { 
    // we only have seven sensors.
    if(temps[i]>100 | temps[i] < -30 | temps[i]==0) {
      temps[i] = prevTempVals_[i];
    }
  }  

  // if the value in temps is 0, it means we still haven't gotten a
  // good reading for that sensor
  // likewise, if the value in prevTempVals is 0, it's the first try
  //  otherwise, we check that the delta temperature is sensible.
  for (i=0;i<NUM_DLP_TEMP_SENSORS-1; i++){
    if(temps[i] != 0 && prevTempVals_[i] != 0 ) {
      deltaTemp = temps[i] - prevTempVals_[i];
      // check the deltaTemp
      if(deltaTemp > TEMP_TOLERANCE || deltaTemp < -TEMP_TOLERANCE){
	temps[i] = prevTempVals_[i];
      };
    };
  };


  /* record them to the data stream */
  if(share_) { 
    float tempVals[NUM_DLP_TEMP_SENSORS];
    for (i=0;i<NUM_DLP_TEMP_SENSORS;i++) { 
      tempVals[i] = temps[i];
      prevTempVals_[i] = temps[i];
    }
    // we write it
    share_->writeReg(dlpTempSensors_, tempVals);
  };
  /* that should do it */
};


/**.......................................................................
 * disconnect
 */
bool DlpTempSensors::disconnect()
{
  dlp_->disconnect();  
  return 0;
};



/**.......................................................................
 * definition of how to process the message received
 */
void DlpTempSensors::processMsg(DlpTempSensorsMsg* msg) {
  
  // Check the type and do what needs to be done.

  switch(msg->type_) {
  case DlpTempSensorsMsg::CONNECT:
    connect();
    break;

  case DlpTempSensorsMsg::DISCONNECT:
    disconnect();
    break;

  case DlpTempSensorsMsg::GET_TEMPS:
    requestAllTemperatures();
    break;

  default:
    ThrowError("DlpTempSensors: REQUEST NOT RECOGNIZED");
    break;

  };

  //  std::cout<< "processMsg: exiting" << std::endl;
};
  

/**.......................................................................
 *
 */
void DlpTempSensors::sendTempRequest(){

  
  DlpTempSensorsMsg msg;
  msg.genericMsgType_ = gcp::util::GenericTaskMsg::TASK_SPECIFIC;
  msg.type_ = DlpTempSensorsMsg::GET_TEMPS;
  
  sendTaskMsg(&msg);
};


