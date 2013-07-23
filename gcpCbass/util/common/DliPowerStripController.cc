#include "gcp/util/common/DliPowerStripController.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
DliPowerStripController::DliPowerStripController(std::string host, bool doSpawn) :
  SpawnableTask<DliPowerStripControllerMsg>(doSpawn), dli_(host)
{
}

/**.......................................................................
 * Destructor.
 */
DliPowerStripController::~DliPowerStripController() {}

/**.......................................................................
 * Main Task event loop: when this is called, the task blocks forever
 * in select(), or until a stop message is received.
 */
void DliPowerStripController::processMsg(DliPowerStripControllerMsg* msg)
{
  switch (msg->type_) {
  case DliPowerStripControllerMsg::ENABLE:
    if(msg->body.enable.enable_) {
      dli_.on(msg->body.enable.mask_);
    } else {
      dli_.off(msg->body.enable.mask_);
    }
    break;
  case DliPowerStripControllerMsg::CYCLE:
    dli_.cycle(msg->body.enable.mask_);
    break;
  case DliPowerStripControllerMsg::STATUS:
    {
      std::vector<DliPowerStrip::State> status = dli_.queryStatus();
      reportStatus(status);
    }
    break;
  default:
    ThrowError("Unrecognized message type: " << msg->type_);
    break;
  }
}

void DliPowerStripController::on(DliPowerStrip::Outlet mask)
{
  DliPowerStripControllerMsg msg;
  msg.genericMsgType_ = GenericTaskMsg::TASK_SPECIFIC;
  msg.type_ = DliPowerStripControllerMsg::ENABLE;
  
  msg.body.enable.enable_ = true;
  msg.body.enable.mask_ = mask;

  sendTaskMsg(&msg);
}

void DliPowerStripController::off(DliPowerStrip::Outlet mask)
{
  DliPowerStripControllerMsg msg;
  msg.genericMsgType_ = GenericTaskMsg::TASK_SPECIFIC;
  msg.type_ = DliPowerStripControllerMsg::ENABLE;
  
  msg.body.enable.enable_ = false;
  msg.body.enable.mask_ = mask;

  sendTaskMsg(&msg);
}

void DliPowerStripController::cycle(DliPowerStrip::Outlet mask)
{
  DliPowerStripControllerMsg msg;
  msg.genericMsgType_ = GenericTaskMsg::TASK_SPECIFIC;
  msg.type_ = DliPowerStripControllerMsg::CYCLE;
  
  msg.body.cycle.mask_ = mask;

  sendTaskMsg(&msg);
}

void DliPowerStripController::status()
{
  DliPowerStripControllerMsg msg;
  msg.genericMsgType_ = GenericTaskMsg::TASK_SPECIFIC;
  msg.type_ = DliPowerStripControllerMsg::STATUS;
  
  sendTaskMsg(&msg);
}
