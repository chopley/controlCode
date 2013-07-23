#include "gcp/antenna/control/specific/AdcMonitor.h"
#include "gcp/antenna/control/specific/AntennaException.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

#define TEMP_TOLERANCE 1.5
#define ADC_LABJACK_SERIAL_NUMBER 320050869

/**.......................................................................
 * Constructors
 */
AdcMonitor::AdcMonitor() : Board(), SpawnableTask<AdcMonitorMsg>(0) {
  share_ = 0;
  labjack_   = 0;
  connected_ = false;
  labjack_ = new gcp::util::Labjack();
}

AdcMonitor::AdcMonitor(SpecificShare* share, string name, bool spawn) : Board(share, name), SpawnableTask<AdcMonitorMsg>(spawn)   {

  adcVoltPtr_ = 0;
  adcVoltPtr_ = findReg("adc");
  connected_  = false;
  labjack_ = 0;
  labjack_ = new gcp::util::Labjack();

}

/**.......................................................................
 * Destructor.
 */
AdcMonitor::~AdcMonitor() {
  if(labjack_) {
    labjack_->disconnect();
  };
}

/**.......................................................................
 * Attempt to connect to the device
 */
void AdcMonitor::connect()
{
  if(labjack_->connected_){
    COUT("labjack already connected");
    connected_ = true;
  };
  bool isConn = false;
  try{
    // Connect and initialize
    isConn = labjack_->connect(ADC_LABJACK_SERIAL_NUMBER);
    connected_ = isConn;
    if(isConn){
      labjack_->getCalibrationInfo();
      labjack_->configAllIO();
      connected_ = true;
      //      COUT("connected to ADC labjack");
    }  else {
      COUT("did not find ADC labjack with proper serial number");
      connected_ = false;
      return;
    }
  
    return;
  } catch(Exception& err) {
    COUT("Labjack ADC monitoring:  Device not connected");
    connected_ = false;
  }
}



/**.......................................................................
 * Query all temperatures
 */
void AdcMonitor::requestAllVoltages()
{

  std::vector<float> voltages(NUM_LABJACK_VOLTS);
  int i;
  try {
    connect();

    if(connected_){
      voltages = labjack_->queryAllVoltages();
      
      /* record them to the data stream */
      if(share_) { 
	float voltVals[NUM_LABJACK_VOLTS];
	
	for (i=0;i<12;i++) { 
	  voltVals[i] = voltages[i];
	}
	
	// we write it
	share_->writeReg(adcVoltPtr_, voltVals);
      };
      /* that should do it */
      disconnect();
    } else {
      COUT("DID NOT CONNECT TO LABJACK");
    }
  } catch (...) { 
    COUT("ERROR READING FROM ADC LABJACK");
  };
  /* that should do it */
};


/**.......................................................................
 * disconnect
 */
bool AdcMonitor::disconnect()
{
  labjack_->disconnect();  
  return 0;
};



/**.......................................................................
 * definition of how to process the message received
 */
void AdcMonitor::processMsg(AdcMonitorMsg* msg) {
  
  // Check the type and do what needs to be done.

  switch(msg->type_) {
  case AdcMonitorMsg::CONNECT:
    connect();
    break;

  case AdcMonitorMsg::DISCONNECT:
    disconnect();
    break;

  case AdcMonitorMsg::GET_VOLTAGE:
    //    CTOUT("REQUESTING VOLTAGES FROM ADC");
    requestAllVoltages();
    //    CTOUT("REQUEST COMPLETE");
    break;

  default:
    ThrowError("AdcMonitor: REQUEST NOT RECOGNIZED");
    break;

  };

  //  std::cout<< "processMsg: exiting" << std::endl;
};
  

/**.......................................................................
 *
 */
void AdcMonitor::sendVoltRequest(){

  
  AdcMonitorMsg msg;
  msg.genericMsgType_ = gcp::util::GenericTaskMsg::TASK_SPECIFIC;
  msg.type_ = AdcMonitorMsg::GET_VOLTAGE;
  
  sendTaskMsg(&msg);
};


