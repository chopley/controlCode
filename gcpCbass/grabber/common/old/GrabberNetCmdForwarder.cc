#define __FILEPATH__ "grabber/GrabberNetCmdForwarder.cc"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/Debug.h"

#include "gcp/grabber/common/Master.h"
#include "gcp/grabber/common/GrabberNetCmdForwarder.h"

using namespace std;
using namespace gcp::control;
using namespace gcp::grabber;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
GrabberNetCmdForwarder::GrabberNetCmdForwarder(Master* parent) :
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
  MasterMsg masterMsg;
  ScannerMsg* scannerMsg = masterMsg.getScannerMsg();
  RtcNetCmd* rtc = &netCmd->rtc_;
  NetCmdId opcode = netCmd->opcode_;
  gcp::util::LogStream errStr;

  switch(opcode) {
  case NET_GRABBER_CMD:
    scannerMsg->packGrabFrameMsg();
    break;
  case NET_CONFIGURE_FG_CMD:
    DBPRINT(true, Debug::DEBUG9, "Got a configure command");
    scannerMsg->packConfigureMsg((gcp::control::FgOpt)rtc->cmd.configureFrameGrabber.mask,
				 rtc->cmd.configureFrameGrabber.channelMask,
				 rtc->cmd.configureFrameGrabber.nCombine,
				 rtc->cmd.configureFrameGrabber.flatfield,
				 rtc->cmd.configureFrameGrabber.seconds);
    break;
  default:
    cout << "GrabberNetCmdForwarder Unrecognized message - opcode = " << opcode << endl;
    //ThrowError("Unrecognized message");
    break;
  }

  // And forward the message to the Control task.
  
  forwardScannerMsg(&masterMsg);
}

/**.......................................................................
 * Forward a message to the scanner task.
 */
void GrabberNetCmdForwarder::forwardScannerMsg(MasterMsg* msg)
{
  parent_->forwardScannerMsg(msg);
}
