#define __FILEPATH__ "mediator/specific/GrabberNetCmdForwarder.cc"

#include "gcp/util/common/Debug.h"

#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/ControlMsg.h"
#include "gcp/mediator/specific/GrabberControlMsg.h"
#include "gcp/mediator/specific/GrabberNetCmdForwarder.h"

using namespace std;
using namespace gcp::control;
using namespace gcp::mediator;

/**.......................................................................
 * Constructor.
 */
GrabberNetCmdForwarder::GrabberNetCmdForwarder(Control* parent) :
  parent_(parent) {}

/**.......................................................................
 * Destructor.
 */
GrabberNetCmdForwarder::~GrabberNetCmdForwarder() {}

/**.......................................................................
 * Forward a network command intended for the frame grabber
 */
void GrabberNetCmdForwarder::forwardNetCmd(gcp::util::NetCmd* netCmd)
{
  ControlMsg msg;
  
  GrabberControlMsg* grabberMsg = msg.getGrabberControlMsg();
  
  grabberMsg->packRtcNetCmdMsg(&netCmd->rtc_, netCmd->opcode_);
  
  parent_->forwardGrabberControlMsg(&msg);
}
