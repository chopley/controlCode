#include "gcp/antenna/control/specific/LnaBiasMonitor.h"
#include "gcp/antenna/control/specific/AntennaException.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

#define TEMP_TOLERANCE 1.5
#define LNA_LABJACK_SERIAL_NUMBER 320050844

/**.......................................................................
 * Constructors
 */
LnaBiasMonitor::LnaBiasMonitor() : Board(), SpawnableTask<LnaBiasMonitorMsg>(0) {
  share_ = 0;
  labjack_   = 0;
  connected_ = false;
  reConnectCounter_ = 0;

  labjack_ = new gcp::util::Labjack();

  // set up previous temperatures
  int i;
  for (i=0;i<NUM_RECEIVER_AMPLIFIERS;i++){
    prevDrainCurrentVals_[i] = 0;
    prevDrainVoltageVals_[i] = 0;
    prevGateVoltageVals_[i] = 0;
  };
}

LnaBiasMonitor::LnaBiasMonitor(SpecificShare* share, string name, bool spawn) : Board(share, name), SpawnableTask<LnaBiasMonitorMsg>(spawn)   {
  connected_    = false;
  drainCurrent_ = 0;
  drainVoltage_ = 0;
  gateVoltage_  = 0;

  drainCurrent_ = findReg("drainCurrent");
  drainVoltage_ = findReg("drainVoltage");
  gateVoltage_  = findReg("gateVoltage");

  labjack_ = 0;
  reConnectCounter_ = 0;
  labjack_ = new gcp::util::Labjack();
  // set up previous temperatures
  int i;
  for (i=0;i<NUM_RECEIVER_AMPLIFIERS;i++){
    prevDrainCurrentVals_[i] = 0;
    prevDrainVoltageVals_[i] = 0;
    prevGateVoltageVals_[i] = 0;
  };
}

/**.......................................................................
 * Destructor.
 */
LnaBiasMonitor::~LnaBiasMonitor() {
  if(labjack_) {
    labjack_->disconnect();
  };
}

/**.......................................................................
 * Attempt to connect to the device
 */
void LnaBiasMonitor::connect()
{
  if(labjack_->connected_){
    COUT("labjack already connected");
    connected_ = true;
  };
  bool isConn = false;
  try{
    // Connect and initialize
    isConn = labjack_->connect(LNA_LABJACK_SERIAL_NUMBER);
    connected_ = isConn;
    if(isConn){
      labjack_->getCalibrationInfo();
      labjack_->configAllIO();
      connected_ = true;
      //      COUT("connected to labjack");
    }  else {
      COUT("did not find LNA labjack with proper serial number BIAS");
      connected_ = false;
      return;
    }
  
    return;
  } catch(Exception& err) {
    COUT("Labjack LNA monitoring:  Device not connected");
    connected_ = false;
  }
}



/**.......................................................................
 * Query all temperatures
 */
void LnaBiasMonitor::requestAllVoltages()
{

    int i;
    std::vector<float> voltages(NUM_TOTAL_VOLTAGES);

  try{
    connect();
    voltages = labjack_->queryAllVoltages();
    
    /* record them to the data stream */
    if(share_) { 
      float IdVals[NUM_RECEIVER_AMPLIFIERS];
      float VdVals[NUM_RECEIVER_AMPLIFIERS];
      float VgVals[NUM_RECEIVER_AMPLIFIERS];
      
      for (i=0;i<4;i++) { 
	VgVals[i] = voltages[i];
      }
      for (i=4;i<8;i++) {
	VdVals[i-4] = voltages[i];
      }
      for (i=8;i<12;i++) {
	IdVals[i-8] = voltages[i];
      }
      
      // we write it
      share_->writeReg(drainCurrent_, IdVals);
      share_->writeReg(drainVoltage_, VdVals);
      share_->writeReg(gateVoltage_, VgVals);
    };

    disconnect();
  } catch (...) { 
    COUT("ERROR READING FROM LNA LABJACK");
  }
  /* that should do it */
};


/**.......................................................................
 * disconnect
 */
bool LnaBiasMonitor::disconnect()
{
  labjack_->disconnect();  
  connected_ = false;
  return 0;
};



/**.......................................................................
 * definition of how to process the message received
 */
void LnaBiasMonitor::processMsg(LnaBiasMonitorMsg* msg) {
  
  // Check the type and do what needs to be done.
  //  COUT("GOT A MESSAGE");

  switch(msg->type_) {
  case LnaBiasMonitorMsg::CONNECT:
    COUT("got connect msg");
    connect();
    break;

  case LnaBiasMonitorMsg::DISCONNECT:
    COUT("DISCONNECT");
    disconnect();
    break;

  case LnaBiasMonitorMsg::GET_VOLTAGE:
    //    CTOUT("LNA BIAS REQUEST");
    requestAllVoltages();
    //    CTOUT("LNA BIAS REQUEST COMPLETE");
    break;

  default:
    ThrowError("LnaBiasMonitor: REQUEST NOT RECOGNIZED");
    break;

  };

  //  std::cout<< "processMsg: exiting" << std::endl;
};
  

/**.......................................................................
 *
 */
void LnaBiasMonitor::sendVoltRequest(){

  
  LnaBiasMonitorMsg msg;
  msg.genericMsgType_ = gcp::util::GenericTaskMsg::TASK_SPECIFIC;
  msg.type_ = LnaBiasMonitorMsg::GET_VOLTAGE;
  
  sendTaskMsg(&msg);
};


