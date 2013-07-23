#define __FILEPATH__ "mediator/specific/ControlNetCmdForwarder.cc"

#include "gcp/util/common/LogStream.h"
#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/ControlMsg.h"
#include "gcp/mediator/specific/ControlNetCmdForwarder.h"

using namespace std;

using namespace gcp::control;
using namespace gcp::mediator;

/**.......................................................................
 * Constructor.
 */
ControlNetCmdForwarder::ControlNetCmdForwarder(Control* parent) :
  parent_(parent) {}

/**.......................................................................
 * Destructor.
 */
ControlNetCmdForwarder::~ControlNetCmdForwarder() {}

/**.......................................................................,
 * Forward a control command 
 */
void ControlNetCmdForwarder::forwardNetCmd(gcp::util::NetCmd* netCmd)
{
  gcp::util::LogStream errStr;
  ControlMsg controlMsg;
  RtcNetCmd* rtc = &netCmd->rtc_;
  NetCmdId opcode = netCmd->opcode_;
  
  switch(opcode) {
  case NET_INIT_CMD:
    controlMsg.packInitMsg(rtc->cmd.init.start);
    break;
  case NET_RUN_SCRIPT_CMD:
    controlMsg.getReceiverControlMsg()->
      packCommandMsg(rtc->cmd.runScript.script, rtc->cmd.runScript.seq);
    break;
  case NET_SCRIPT_DIR_CMD:
    controlMsg.getReceiverControlMsg()->
      packDirectoryMsg(rtc->cmd.scriptDir.dir);
    break;
  default:
    ReportError("Unrecognized message: id = " << opcode);
    break;
  }
  
  // And forward the message to the Control task.
  
  forwardControlMsg(&controlMsg);
}

/**.......................................................................
 * Forward a message to the control task.
 */
void ControlNetCmdForwarder::forwardControlMsg(ControlMsg* msg)
{
  parent_->forwardControlMsg(msg);
}
