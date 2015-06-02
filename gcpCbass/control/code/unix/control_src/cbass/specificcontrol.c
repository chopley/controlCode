#define __FILEPATH__ "control/code/unix/control_src/spt/sptcontrol.c"

#include "genericcontrol.h"

#include "gcp/util/common/NewNetMsg.h"


using namespace gcp::control;
using namespace gcp::util;

int processSpecificMsg(ControlProg* cp, SockChan* sock, NewRtcNetMsg& netmsg)
{
  switch(netmsg.msg->getType()) {

    // Send phase-shifter and channelizer transaction completion
    // messages to the scheduler thread.
  
  case NewNetMsg::NET_DRIVE_DONE_MSG:
    //    CTOUT("Got a DRIVE DONE message");
    if(send_scheduler_rtcnetmsg(cp, netmsg))
      return 1;
    break;
  case NewNetMsg::NET_BENCH_DONE_MSG:
    if(send_scheduler_rtcnetmsg(cp, netmsg))
      return 1;
    break;
  case NewNetMsg::NET_SOURCE_SET_MSG:
    //    CTOUT("Got a SOURCE SET message");
    if(send_scheduler_rtcnetmsg(cp, netmsg))
      return 1;
    break;
  case NewNetMsg::NET_SETREG_DONE_MSG:
    //    CTOUT("Got a SETREG DONE message");
    if(send_scheduler_rtcnetmsg(cp, netmsg))
      return 1;
    break;
  case NewNetMsg::NET_SCAN_DONE_MSG:
    //    CTOUT("Got a SCAN DONE message");
    if(send_scheduler_rtcnetmsg(cp, netmsg))
      return 1;
    break;
  case NewNetMsg::NET_SCRIPT_DONE_MSG:
    //   COUT("Got a net_script_done+msg");
    if(send_scheduler_rtcnetmsg(cp, netmsg))
      return 1;
    break;
  default:
    if(add_readable_channel(cp, &sock->head))
      return 1;
    break;
  };

  return 0;
}
 
