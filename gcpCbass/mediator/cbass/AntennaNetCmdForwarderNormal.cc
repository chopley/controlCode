#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/ControlMsg.h"
#include "gcp/util/common/Debug.h"
#include "gcp/mediator/specific/AntennaNetCmdForwarderNormal.h"

using namespace std;

using namespace gcp::mediator;

/**.......................................................................
 * Constructor.
 */
AntennaNetCmdForwarderNormal::AntennaNetCmdForwarderNormal(Control* parent) :
  parent_(parent) {}

/**.......................................................................
 * Destructor.
 */
AntennaNetCmdForwarderNormal::~AntennaNetCmdForwarderNormal() {}

/**.......................................................................
 * Forward a network command intended for an antenna
 */
void AntennaNetCmdForwarderNormal::forwardNetCmd(gcp::util::NetCmd* netCmd)
{
  ControlMsg msg;
  AntennaControlMsg* antennaMsg = msg.getAntennaControlMsg();

  antennaMsg->packRtcNetCmdMsg(&netCmd->rtc_, netCmd->opcode_);
  antennaMsg->init_ = netCmd->init_;

  parent_->forwardAntennaControlMsg(&msg);
}

