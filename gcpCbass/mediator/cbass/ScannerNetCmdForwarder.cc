#define __FILEPATH__ "mediator/specific/ScannerNetCmdForwarder.cc"

#include "gcp/util/common/Debug.h"

#include "gcp/mediator/specific/Master.h"
#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/ScannerNetCmdForwarder.h"

using namespace std;
using namespace gcp::control;
using namespace gcp::mediator;

/**.......................................................................
 * Constructor.
 */
ScannerNetCmdForwarder::ScannerNetCmdForwarder(Control* parent) :
  parent_(parent) {}

/**.......................................................................
 * Destructor.
 */
ScannerNetCmdForwarder::~ScannerNetCmdForwarder() {}

/**.......................................................................,
 * Forward a scanner command 
 */
void ScannerNetCmdForwarder::forwardNetCmd(gcp::util::NetCmd* netCmd)
{
  gcp::util::LogStream errStr;
  MasterMsg masterMsg;
  ScannerMsg* scannerMsg = masterMsg.getScannerMsg();
  RtcNetCmd* rtc = &netCmd->rtc_;
  NetCmdId opcode = netCmd->opcode_;
  
  switch(opcode) {
  case NET_FEATURE_CMD:
    DBPRINT(true, gcp::util::Debug::DEBUG3, "Got a feature message");
    scannerMsg->packFeatureMsg((unsigned)rtc->cmd.feature.seq, 
			       (unsigned)rtc->cmd.feature.mode,
			       (unsigned)rtc->cmd.feature.mask);
    break;
  case NET_SETFILTER_CMD:
    scannerMsg->getDioMsg()->
      packSetFilterMsg(rtc->cmd.setFilter.mask,
		       rtc->cmd.setFilter.freqHz,
		       rtc->cmd.setFilter.ntaps);
    break;
  default:
    errStr.appendMessage(true, "Unrecognized message");
    errStr.report();
    break;
  }
  
  // And forward the message to the Scanner task.
  
  forwardScannerMsg(&masterMsg);
}

/**.......................................................................
 * Forward a message to the scanner task.
 */
void ScannerNetCmdForwarder::forwardScannerMsg(MasterMsg* msg)
{
  parent_->forwardMasterMsg(msg);
}
