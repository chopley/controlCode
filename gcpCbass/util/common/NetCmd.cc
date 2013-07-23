#define __FILEPATH__ "util/common/NetCmd.cc"

#include "gcp/util/common/NetCmd.h"

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
NetCmd::NetCmd() {}

/**.......................................................................
 * Constructor.
 */
NetCmd::NetCmd(gcp::control::RtcNetCmd rtc, gcp::control::NetCmdId opcode) 
{
  rtc_    = rtc;
  opcode_ = opcode;
}

/**.......................................................................
 * Destructor.
 */
NetCmd::~NetCmd() {}

void NetCmd::packAtmosCmd(Temperature& airTemp, Percent& humidity,
			  Pressure& pressure, AntNum::Id antennas)
{
  opcode_ = gcp::control::NET_ATMOS_CMD;

  rtc_.cmd.atmos.temperatureInK = airTemp.K();
  rtc_.cmd.atmos.humidityInMax1 = humidity.percentMax1();
  rtc_.cmd.atmos.pressureInMbar = pressure.milliBar();

  // Send this to all antennas

  rtc_.antennas = (NetMask)antennas;
}
