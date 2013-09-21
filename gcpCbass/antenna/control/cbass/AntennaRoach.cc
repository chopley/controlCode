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
#include "gcp/antenna/control/specific/AntennaRoach.h"
#include "gcp/antenna/control/specific/RoachBackend.h"

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

#define HAVE_ROACH2 true

#define BACKEND_MISSED_COMM_LIMIT 10

using namespace std;

using namespace gcp::antenna::control;
using namespace gcp::util;

AntennaRoach* AntennaRoach::antennaRoach_ = 0;

/**.......................................................................
 * Create an AntennaRoach class
 */
AntennaRoach::AntennaRoach(AntennaMaster* parent, bool simRoach) :
  SpecificTask(), gcp::util::GenericTask<AntennaRoachMsg>::GenericTask()
{ 
  // Initialize internal pointers
  
  parent_    = 0;
  share_     = 0;
  antennaRoach_ = 0;
  roach1_    = 0;
  roach2_    = 0;
  sim_       = simRoach;

  // Keep a pointer to the parent task resources
  
  parent_ = parent;

  if(parent_){
    share_  = parent->getShare();
   // roach1_ = new RoachBackend(share_, "roach1", simRoach, "pumba", 1);
   // roach2_ = new RoachBackend(share_, "roach2", simRoach, "timon", 1);
//CJC- switch the ROACHES around to see if the dropouts correspond to hardware or GCP
    roach1_ = new RoachBackend(share_, "roach1", simRoach, "timon", 1);
    roach2_ = new RoachBackend(share_, "roach2", simRoach, "pumba", 1);
  } else {
   // roach1_ = new RoachBackend(simRoach, "pumba");
   // roach2_ = new RoachBackend(simRoach, "timon");
    roach1_ = new RoachBackend(simRoach, "timon");
    roach2_ = new RoachBackend(simRoach, "pumba");
  };

  antennaRoach_ = this;
};
/**.......................................................................
 * Create an AntennaRoach class
 */
AntennaRoach::AntennaRoach(AntennaMaster* parent) :
  SpecificTask(), gcp::util::GenericTask<AntennaRoachMsg>::GenericTask()
{ 
  // Initialize internal pointers
  
  parent_    = 0;
  share_     = 0;
  antennaRoach_ = 0;
  roach1_    = 0;
  roach2_    = 0;

  // Keep a pointer to the parent task resources
  
  parent_ = parent;

  if(parent_){
    share_  = parent->getShare();
    roach1_ = new RoachBackend(share_, "roach1", false, "pumba", 1);
    roach2_ = new RoachBackend(share_, "roach2", false, "timon", 1);
  } else {
    roach1_ = new RoachBackend(false, "pumba");
    roach2_ = new RoachBackend(false, "timon");
  };

  antennaRoach_ = this;
};

/**.......................................................................
 * Destructor function
 */
AntennaRoach::~AntennaRoach() {
  
  if(roach1_){
    delete roach1_;
    roach1_ = 0;
  };

  if(roach2_){
    delete roach2_;
    roach2_ = 0;
  };
};

/**.......................................................................
 * Process a message received on the AntennaRoach message queue
 */
void AntennaRoach::processMsg(AntennaRoachMsg* msg)
{
  gcp::util::TimeVal currTime;
  struct timespec delay;
  delay.tv_sec  =       0;
  delay.tv_nsec = 2000000;
	
  
  if(sim_){
    return;
  }

  switch (msg->type) {
  case AntennaRoachMsg::CONNECT:
    CTOUT("in AntennaRoach: got CONNECT command");
    // if we're connected, don't do it.
    if(!roach1_->connected_){
      // try to connect
      roach1_->connect();
      if(roach1_->connected_){
	COUT("just connected to the first roach");
	//	sendRoachConnectedMsg(true);
      }
    }
#if(HAVE_ROACH2)
    if(!roach2_->connected_){
      // try to connect
      roach2_->connect();
      if(roach2_->connected_){
	COUT("just connected to the second roach");
      }
    }
#endif
    if(roach1_->connected_ & roach2_->connected_){
      sendRoachConnectedMsg(true);
    } else {
      sendRoachConnectedMsg(false);      
    }
    break;

  case AntennaRoachMsg::DISCONNECT:
    CTOUT("in AntennaRoach: got DISCONNECT command");
    roach1_->disconnect();
    if(!roach1_->connected_){
      COUT("just disconnected from roach1");
    }
#if(HAVE_ROACH2)
    roach2_->disconnect();
    if(!roach2_->connected_){
      COUT("just disconnected from roach2");
    }
#endif
    if( (!roach1_->connected_) & (!roach2_->connected_)){
      sendRoachConnectedMsg(false);
    } else {
      sendRoachConnectedMsg(true);      
    }


    break;
    
  case AntennaRoachMsg::READ_DATA:
    //    CTOUT("in AntennaRoach: got READ_DATA command");
    if(roach1_->roachIsConnected()){
      try{
	nanosleep(&delay, 0);
	roach1_->getData();
      } catch(...) {
	// we're not connected
	roach1_->disconnect();
	sendRoachConnectedMsg(false);
      }  
      //check to see if still connected -- shoudlbe obsolte right now.
      if(roach1_->missedCommCounter_ > BACKEND_MISSED_COMM_LIMIT){
	ReportSimpleError("Roach1 not communicating.  Killing connection");
	CTOUT("Roach1 not communicating.  Killing connection");
	roach1_->disconnect();
	sendRoachConnectedMsg(false);
      };
    } else { 
      COUT("AntennaRoach: roach1 not connected, not reading data");
    };
#if(HAVE_ROACH2)
    if(roach2_->roachIsConnected()){
      try {
	nanosleep(&delay, 0);
	roach2_->getData();
      } catch(...) {
	roach2_->disconnect();
	sendRoachConnectedMsg(false);
      }
      //check to see if still connected
      if(roach2_->missedCommCounter_ > BACKEND_MISSED_COMM_LIMIT){
	ReportSimpleError("Roach2 not communicating.  Killing connection");
	roach2_->disconnect();
	sendRoachConnectedMsg(false);
      };
    } else {
      COUT("AntennaRoach::roach2 not connected, not reading data");
    }
#endif
    break;
    
  case AntennaRoachMsg::WRITE_DATA:
    currTime.setTime(msg->currTime_);
    if(roach1_->roachIsConnected()) {
      roach1_->writeData(currTime);
    } else {
      CTOUT("AntennaRoach:: roach1 not connected, not writing data");
    }
#if(HAVE_ROACH2)
    if(roach2_->roachIsConnected()){
      roach2_->writeData(currTime);
    } else {
      CTOUT("AntennaRoach:: roach2 not connected, not writing data");
    };
#endif
    break;
    
  case AntennaRoachMsg::ROACH_CMD:
    CTOUT("MESSAGE: " << msg->body.roach.stringCommand);
    nanosleep(&delay,0);
    executeRoachCmd(msg);
    break;
    
  default:
    ReportError("AntennaRoach::processMsg: Unrecognized message type: " << msg->type);
    break;
  };
};


/**.......................................................................
 * Send a message to the parent about the connection status of the
 * host
 */
void AntennaRoach::sendRoachConnectedMsg(bool connected)
{
  AntennaMasterMsg msg;

  msg.packRoachConnectedMsg(connected);

  parent_->forwardMasterMsg(&msg);
}


/**.......................................................................
 * Executes a command to the backend.
 * 
 */
void AntennaRoach::executeRoachCmd(AntennaRoachMsg* msg)
{

  std::ostringstream os;
  os << msg->body.roach.stringCommand << "," << msg->body.roach.fltVal;


  switch(msg->body.roach.roachNum){
  case 0:
    // it's a message for the first roach
    if(!roach1_->connected_){
      COUT("First roach not connected.   Will not execute");
    } else {
      // pack the command
      COUT("stringsize: " << os.str().size());
      roach1_->command_.packRoachCmdMsg(os.str());
      COUT("COMMAND TO SEND to roach 1: " << roach1_->command_.messageToSend_);
      try{
	roach1_->issueCommand(roach1_->command_);
      } catch (...) {
	COUT("ROACH1 NOT CONNECTED");
	roach1_->disconnect();
	sendRoachConnectedMsg(false);
      }
    }
    break;

  case 1:
    // it's a message for the second roach
    if(!roach2_->connected_){
      COUT("Second roach not connected.   Will not execute");
    } else {
      // pack the command
      roach2_->command_.packRoachCmdMsg(os.str());
      COUT("roach2 COMMAND TO SEND: " << roach2_->command_.messageToSend_);
      try{
	roach2_->issueCommand(roach2_->command_);
      } catch (...) {
	COUT("ROACH2 NOT CONNECTED");
	roach2_->disconnect();
	sendRoachConnectedMsg(false);
      }
    };
    break;

  case 2:
    if(!roach1_->connected_){
      COUT("First roach not connected.   Will not execute");
    } else {
      // pack the command
      roach1_->command_.packRoachCmdMsg(os.str());
      try{
	roach1_->issueCommand(roach1_->command_);
      } catch (...) {
	COUT("ROACH1 NOT CONNECTED");
	roach1_->disconnect();
	sendRoachConnectedMsg(false);
      }
    }

    // it's a message for the second roach
    if(!roach2_->connected_){
      COUT("Second roach not connected.   Will not execute");
    } else {
      // pack the command
      roach2_->command_.packRoachCmdMsg(os.str());
      try{
	roach2_->issueCommand(roach2_->command_);
      } catch (...) {
	COUT("ROACH2 NOT CONNECTED");
	roach2_->disconnect();
	sendRoachConnectedMsg(false);
      }
    };


  }  
  return;
}

/**.......................................................................
 * Send a message to the parent about the connection status of the
 * host
 */
void AntennaRoach::sendRoachTimerMsg(bool timeOn)
{
  AntennaMasterMsg msg;

  msg.packRoachTimerMsg(timeOn);

  parent_->forwardMasterMsg(&msg);
}
