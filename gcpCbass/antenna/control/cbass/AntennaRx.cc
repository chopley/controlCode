#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include "gcp/util/common/Debug.h"
#include "gcp/util/specific/Directives.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include "gcp/antenna/control/specific/AntennaMaster.h"
#include "gcp/antenna/control/specific/AntennaControl.h"
#include "gcp/antenna/control/specific/AntennaRx.h"

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

#define BACKEND_MISSED_COMM_LIMIT 19

using namespace std;

using namespace gcp::antenna::control;
using namespace gcp::util;

AntennaRx* AntennaRx::antennaRx_ = 0;

/**.......................................................................
 * Create an AntennaRx class
 */
AntennaRx::AntennaRx(AntennaMaster* parent) :
  SpecificTask(), gcp::util::GenericTask<AntennaRxMsg>::GenericTask()
{ 
  // Initialize internal pointers
  
  parent_    = 0;
  share_     = 0;
  antennaRx_ = 0;
  backend_   = 0;

  // Keep a pointer to the parent task resources
  
  parent_ = parent;

  if(parent_){
    share_  = parent->getShare();
    backend_ = new Backend(share_, "receiver");
  } else {
    backend_ = new Backend();
  };

  antennaRx_ = this;
};

/**.......................................................................
 * Destructor function
 */
AntennaRx::~AntennaRx() {
  
  if(backend_){
    delete backend_;
    backend_ = 0;
  };
};

/**.......................................................................
 * Process a message received on the AntennaRx message queue
 */
void AntennaRx::processMsg(AntennaRxMsg* msg)
{
  gcp::util::TimeVal currTime;

  switch (msg->type) {
  case AntennaRxMsg::CONNECT:
    // if we're connected, don't do it.
    if(backend_->connected_)
      return;
    
    // else try and connect
    backend_->connect();
    if(backend_->connected_){
      COUT("just connected to the backend");
      sendRxConnectedMsg(true);
    }

    break;

  case AntennaRxMsg::DISCONNECT:
    backend_->disconnect();
    if(!backend_->connected_){
      COUT("just disconnected from backend");
      sendRxConnectedMsg(false);
    }
    break;
    
  case AntennaRxMsg::READ_DATA:
    //    CTOUT("in AntennaRx: got READ_DATA command");
    backend_->getData();

    // let's check if we're still connected
    if(backend_->missedCommCounter_ > BACKEND_MISSED_COMM_LIMIT){
      ReportSimpleError("Backend not communicating.  Killing connection");
      backend_->disconnect();
      sendRxConnectedMsg(false);
    };
    break;
    
  case AntennaRxMsg::WRITE_DATA:
    currTime.setTime(msg->currTime_);
    backend_->writeData2011(currTime);
    //backend_->writeData(currTime);
    break;
    
  case AntennaRxMsg::RX_CMD:
    //    CTOUT("in AntennaRx: About to execute rx command");
    executeRxCmd(msg);
    //    CTOUT("About to execute rx command: done");
    break;
    
  default:
    ReportError("AntennaRx::processMsg: Unrecognized message type: " << msg->type);
    break;
  };
};


/**.......................................................................
 * Send a message to the parent about the connection status of the
 * host
 */
void AntennaRx::sendRxConnectedMsg(bool connected)
{
  AntennaMasterMsg msg;

  msg.packRxConnectedMsg(connected);

  parent_->forwardMasterMsg(&msg);
}


/**.......................................................................
 * Executes a command to the backend.
 * 
 */
void AntennaRx::executeRxCmd(AntennaRxMsg* msg)
{
  if(!backend_->connected_){
    COUT("Backend not connected, will not execute");
    return;
  }
  
  unsigned char length[3];
  unsigned char value;
  int val;
  int channel, stage;
  bool goodVal;

  // else we are connected
  switch(msg->body.rx.cmdId){
  case gcp::control::RX_SETUP_ADC:
    backend_->issueCommand(gcp::util::CbassBackend::SETUP_ADC);
    break;

  case gcp::control::RX_RESET_FPGA:
    backend_->issueCommand(gcp::util::CbassBackend::FPGA_RESET);
    break;

  case gcp::control::RX_RESET_FIFO:
    backend_->issueCommand(gcp::util::CbassBackend::FIFO_RESET);
    break;

  case gcp::control::RX_SET_BURST_LENGTH:
    // first we parse the data.
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > pow(2.0, 26)) {
      ReportSimpleError("setBurstLength argument not within allowable range");
    } else {
      val = (int) msg->body.rx.fltVal;
      length[2] = val / 65536;
      length[1] = val % 65536 / 256;
      length[0] = val % 65536 % 256;

      backend_->issueCommand(gcp::util::CbassBackend::SET_BURST_LENGTH, length);
    };
    break; 

  case gcp::control::RX_SET_SWITCH_PERIOD:
    //    ReportSimpleError("setSwitchPeriod no longer supported");
    goodVal = msg->body.rx.fltVal==1 || msg->body.rx.fltVal==2 || msg->body.rx.fltVal==4 || msg->body.rx.fltVal==8 || msg->body.rx.fltVal==16 || msg->body.rx.fltVal==32 || msg->body.rx.fltVal==64 || msg->body.rx.fltVal==128; 
    if(!goodVal){
      ReportSimpleError("setSwitchPeriod argument not within allowable range");      
    } else {
      value = (int) msg->body.rx.fltVal;
      backend_->issueCommand(gcp::util::CbassBackend::SET_SWITCH_PERIOD, value);
    };
    break;     
    
  case gcp::control::RX_SET_INTEGRATION_PERIOD:
    ReportSimpleError("Set Integration period no longer supported");
    break; 

  case gcp::control::RX_SET_TRIM_LENGTH:
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > 256) {
      ReportSimpleError("setTrimLength argument not within allowable range");      
    } else {
      val = (int) msg->body.rx.fltVal;
      value = val;
      
      backend_->issueCommand(gcp::util::CbassBackend::WALSH_TRIM_LENGTH, value);
    };
    break;

  case gcp::control::RX_ENABLE_SIMULATOR:
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > 1) {
      ReportSimpleError("enableRxSimulator argument not within allowable range");      
    } else {
      val = (int) msg->body.rx.fltVal;
      value = val;
      backend_->issueCommand(gcp::util::CbassBackend::ENABLE_SIMULATOR, value);
    }
    break;

  case gcp::control::RX_ENABLE_NOISE:
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > 1) {
      ReportSimpleError("enableRxNoise argument not within allowable range");      
    } else {
      val = (int) msg->body.rx.fltVal;
      value = val;
      backend_->issueCommand(gcp::util::CbassBackend::ENABLE_NOISE, value);
    };
    break;
    
  case gcp::control::RX_ENABLE_WALSHING:
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > 1) {
      ReportSimpleError("enableWalshing argument not within allowable range");      
    } else {
      val = (int) msg->body.rx.fltVal;
      value = val;
      backend_->issueCommand(gcp::util::CbassBackend::ENABLE_WALSH, value);
    };
    break;
    
  case gcp::control::RX_ENABLE_ALT_WALSHING:
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > 1) {
      ReportSimpleError("enableAltWalshing argument not within allowable range");      
    } else {
      val = (int) msg->body.rx.fltVal;
      value = val;
      backend_->issueCommand(gcp::util::CbassBackend::ENABLE_WALSH_ALT, value);
    };
    break;


  case gcp::control::RX_ENABLE_FULL_WALSHING:
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > 1) {
      ReportSimpleError("enableFullWalshing argument not within allowable range");      
    } else {
      val = (int) msg->body.rx.fltVal;
      value = val;
      backend_->issueCommand(gcp::util::CbassBackend::ENABLE_WALSH_FULL, value);
    };
    break;
    
  case gcp::control::RX_ENABLE_NONLINEARITY:
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > 1) {
      ReportSimpleError("enableAltWalshing argument not within allowable range");      
    } else {
      val = (int) msg->body.rx.fltVal;
      value = val;
      backend_->issueCommand(gcp::util::CbassBackend::NON_LINEARITY, value);
    };
    break;
    
  case gcp::control::RX_GET_BURST_DATA:
    getBurstData();
    break;

  case gcp::control::RX_ENABLE_ALPHA:
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > 1) {
      ReportSimpleError("enableAlphaCorrection argument not within allowable range");      
    } else {
      val = (int) msg->body.rx.fltVal;
      value = val;
      backend_->issueCommand(gcp::util::CbassBackend::ENABLE_ALPHA, value);
    };
    break;

  case gcp::control::RX_SET_ALPHA:
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > pow(2.0, 26)) {
      ReportSimpleError("setAlphaValue argument not within allowable range");
    } else {
      val = (int) msg->body.rx.fltVal;
      length[2] = val / 65536;
      length[1] = val % 65536 / 256;
      length[0] = val % 65536 % 256;
      stage = (int) msg->body.rx.stageVal;
      channel = (int) msg->body.rx.chanVal;
      val = 0;
      backend_->issueCommand(gcp::util::CbassBackend::SET_ALPHA, val, length, channel, stage);
    }
    break;

  case gcp::control::RX_SET_NONLIN:
    if( msg->body.rx.fltVal <0 || msg->body.rx.fltVal > pow(2.0, 26)) {
      ReportSimpleError("setNonlin argument not within allowable range");
    } else {
      val = (int) msg->body.rx.fltVal;
      length[2] = val / 65536;
      length[1] = val % 65536 / 256;
      length[0] = val % 65536 % 256;
      stage = (int) msg->body.rx.stageVal;
      channel = (int) msg->body.rx.chanVal;
      val = 0;
      backend_->issueCommand(gcp::util::CbassBackend::SET_NONLIN, val, length, channel, stage);
    }
    break;

  };

  return;
}


/**.......................................................................
 * This function does whatever is needed to get the burst mode data.
 * 
 */
void AntennaRx::getBurstData() {


  struct timespec delay;
  delay.tv_sec  =         3;
  delay.tv_nsec = 000000000;  // 2 seconds

  // this function has many steps to it, which must be followed for it
  // to work.
  char val;
  
  // First, we need to tell the timers to shut off in the main thread.
  sendRxTimerMsg(false);

  // next we disable continous
  val = 0;
  backend_->issueCommand(gcp::util::CbassBackend::ENABLE_CONTINUOUS, val);
  backend_->cbassBackend_.burst_ = true;

  // clear any old data
  backend_->issueCommand(gcp::util::CbassBackend::FPGA_RESET);  
  backend_->issueCommand(gcp::util::CbassBackend::FIFO_RESET);

  // acquire data and trigger
  val = 1;
  backend_->issueCommand(gcp::util::CbassBackend::ACQUIRE_DATA, val);  

  backend_->issueCommand(gcp::util::CbassBackend::TRIGGER);  
  
  // next we read the data until the buffer is empty, writing it to a
  // file
  // wait the 5s to fill up data.
  CTOUT("WAITING");
  nanosleep(&delay, 0);
  CTOUT("DONE WAITING");

  backend_->getBurstData();


  // once we're done, we reset the fpga, fifo, re-enable continous
  // mode, resume acquiring data
  backend_->issueCommand(gcp::util::CbassBackend::FPGA_RESET);  
  backend_->issueCommand(gcp::util::CbassBackend::FIFO_RESET);

  val = 1;

  backend_->issueCommand(gcp::util::CbassBackend::ENABLE_CONTINUOUS, val);
  backend_->cbassBackend_.burst_ = false;
  backend_->issueCommand(gcp::util::CbassBackend::ACQUIRE_DATA, val);  

  // lastly we reset the timers on in the Master thread.
  sendRxTimerMsg(true);

  return;
};


/**.......................................................................
 * Send a message to the parent about the connection status of the
 * host
 */
void AntennaRx::sendRxTimerMsg(bool timeOn)
{
  AntennaMasterMsg msg;

  msg.packRxTimerMsg(timeOn);

  parent_->forwardMasterMsg(&msg);
}
