#define __FILEPATH__ "mediator/specific/TransNetCmdForwarder.cc"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/Tracking.h"

#include "gcp/mediator/specific/AntennaNetCmdForwarderNormal.h"
#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/ControlMsg.h"
#include "gcp/mediator/specific/ControlNetCmdForwarder.h"
#include "gcp/mediator/specific/GrabberNetCmdForwarder.h"
#include "gcp/mediator/specific/ScannerNetCmdForwarder.h"
#include "gcp/mediator/specific/TransNetCmdForwarder.h"

#include "gcp/antenna/control/specific/TrackerMsg.h"

using namespace gcp::control;
using namespace gcp::util;
using namespace gcp::mediator;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor.
 */
TransNetCmdForwarder::TransNetCmdForwarder(Control* parent) :
  parent_(parent)
{
  receivingInitScript_ = false;

  
  // Instantiate approriate subsystem control objects

  
  antennaForwarder_ = new AntennaNetCmdForwarderNormal(parent);
  controlForwarder_ = new gcp::mediator::ControlNetCmdForwarder(parent);
  grabberForwarder_ = new gcp::mediator::GrabberNetCmdForwarder(parent);
  scannerForwarder_ = new gcp::mediator::ScannerNetCmdForwarder(parent);
}

/**.......................................................................
 * Destructor.
 */
TransNetCmdForwarder::~TransNetCmdForwarder() {}

//-----------------------------------------------------------------------
// Command forwarding methods
//-----------------------------------------------------------------------

/**.......................................................................
 * Overwrite the base-class method to intercept a net command.
 */
void TransNetCmdForwarder::forwardNetCmd(gcp::util::NetCmd* netCmd)
{
  // If we are receiving an intialization script, mark the command as
  // an intialization command
  
  netCmd->init_ = false;
  
  if(receivingInitScript_) {
    
    // Don't record ephemeris commands -- we'll need a fresh update of
    // these whenever an antenna connects
    
    if(!(netCmd->opcode_==gcp::control::NET_UT1UTC_CMD ||
	 netCmd->opcode_==gcp::control::NET_EQNEQX_CMD))
      netCmd->init_ = true;
  }
  
  // Call the base-class method to forward this command in the
  // appropriate way
  
  gcp::util::ArrayNetCmdForwarder::forwardNetCmd(netCmd);
  
  // If this command was an initialization command, start recording an
  // init script.
  
  if(netCmd->opcode_ == gcp::control::NET_INIT_CMD) {
    if(netCmd->rtc_.cmd.init.start)
      receivingInitScript_= true;
    else
      receivingInitScript_= false;
  }
}

/**.......................................................................
 * Forward a message to the AntennaControl task.
 */
void TransNetCmdForwarder::forwardAntennaControlMsg(ControlMsg* msg)
{
  parent_->forwardAntennaControlMsg(msg);
}

/**.......................................................................
 * Forward a message to the GrabberControl task.
 */
void TransNetCmdForwarder::forwardGrabberControlMsg(ControlMsg* msg)
{
  parent_->forwardGrabberControlMsg(msg);
}
