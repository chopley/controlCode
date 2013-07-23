#define __FILEPATH__ "util/specific/AntennaNetCmdForwarder.cc"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/specific/AntennaNetCmdForwarder.h"

using namespace std;

using namespace gcp::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
AntennaNetCmdForwarder::AntennaNetCmdForwarder() 
{
}

/**.......................................................................
 * Destructor.
 */
AntennaNetCmdForwarder::~AntennaNetCmdForwarder() {}

/**.......................................................................
 * Forward a network command intended for an antenna
 */
void AntennaNetCmdForwarder::forwardNetCmd(gcp::util::NetCmd* netCmd)
{
  RtcNetCmd* rtc  = &netCmd->rtc_;
  NetCmdId opcode = netCmd->opcode_;

  //  COUT("made it to antenna net cmd forwarder: opcode = " << opcode << " GPIB is: " << NET_GPIB_CMD);


  // Interpret the command.

  switch(opcode) {
  case NET_FEATURE_CMD: 
    forwardScannerNetCmd(netCmd);
    break;
  case NET_UNFLAG_CMD:
    forwardBoardNetCmd(netCmd);
    break;
    
    // A special case -- this is a message for both the scanner and
    // rx tasks.  The rx task needs to know how to associate
    // polarization states with encoder positions, and the scanner
    // task needs to recognize them.  I handle this by forwarding
    // the message to the scanner task, which in turn forwards it to
    // the rx task.  That way the valid states are guaranteed to be
    // defined before they can be used
    

  case NET_INIT_CMD:
  case NET_GPIB_CMD:
    forwardControlNetCmd(netCmd);
    break;
  case NET_SLEW_CMD:              // Any of the following may contain a phase     
  case NET_TRACK_CMD: 		  // tracking command, so we forward to both task methods.			     
  case NET_SCAN_CMD:
  case NET_SERVO_CMD:
    forwardTrackerNetCmd(netCmd);
    break;
  case NET_SITE_CMD:
  case NET_LOCATION_CMD:
  case NET_REBOOT_DRIVE_CMD:
  case NET_UT1UTC_CMD:
  case NET_EQNEQX_CMD:
  case NET_HALT_CMD: 
  case NET_STOP_CMD: 
  case NET_MOUNT_OFFSET_CMD:
  case NET_EQUAT_OFFSET_CMD:
  case NET_TV_OFFSET_CMD:
  case NET_TV_ANGLE_CMD:
  case NET_SKY_OFFSET_CMD:
  case NET_SLEW_RATE_CMD:
  case NET_TILTS_CMD:
  case NET_FLEXURE_CMD:
  case NET_COLLIMATE_CMD:
  case NET_ENCODER_LIMITS_CMD:
  case NET_ENCODER_CALS_CMD:
  case NET_ENCODER_ZEROS_CMD:
  case NET_MODEL_CMD:
  case NET_YEAR_CMD:
  case NET_DECK_MODE_CMD:
  case NET_ATMOS_CMD:
    forwardTrackerNetCmd(netCmd);
    break;
  case NET_BENCH_ZERO_POSITION_CMD: // Set optical bench zero position
  case NET_BENCH_OFFSET_CMD:        // Set optical bench offset
  case NET_BENCH_USE_BRAKES_CMD:    // Set optical bench use brakes
  case NET_BENCH_SET_ACQUIRED_THRESHOLD_CMD: // Set optical bench acquired threshold
  case NET_BENCH_SET_FOCUS_CMD: // Set optical bench focus
  case NET_RXSIM_CMD:
    forwardRxSimulatorNetCmd(netCmd);
    break;
    
    // Forward optical camera and stepper motor commands to the
    // optical camera task.
    
  case NET_OPTCAM_CNTL_CMD: // Control power to the camera/stepper 
  case NET_STEPPER_CMD:    // Step the stepper motor 
  case NET_CAMERA_CMD:     // Control the camera via the camera control box 
  case NET_FG_CMD:         // Write to a frame grabber register 
  case NET_FLATFIELD_CMD:  // Toggle flat fielding of frame grabber images 
    forwardOpticalCameraNetCmd(netCmd);
    break;
  case NET_RX_CMD:
    forwardRxNetCmd(netCmd);
    break;
  case NET_ROACH_CMD:
    forwardRoachNetCmd(netCmd);
    break;
  case NET_LNA_CMD:
    COUT("util/specific/AntennaNetCmdFowarder");
    COUT("must define forwardLnaNetCmd");
    COUT("CMDID: " << rtc->cmd.lna.cmdId);
    forwardLnaNetCmd(netCmd);
    break;
  default:
    ReportError("unknown opcode " << opcode);
    break;
  }
}

/**.......................................................................,
 * Forward a control command 
 */
void AntennaNetCmdForwarder::forwardControlNetCmd(gcp::util::NetCmd* netCmd) {}

/**.......................................................................
 * Optical Camera commands
 */
void AntennaNetCmdForwarder::forwardOpticalCameraNetCmd(gcp::util::NetCmd* netCmd) {}

/**.......................................................................
 * RxSim commands
 */
void AntennaNetCmdForwarder::forwardRxSimulatorNetCmd(gcp::util::NetCmd* netCmd) {}

/**.......................................................................
 * Forward a command for the weather station
 */
void AntennaNetCmdForwarder::forwardWeatherNetCmd(gcp::util::NetCmd* netCmd) {};

/**.......................................................................
 * Forward a tracker net command
 */
void AntennaNetCmdForwarder::forwardTrackerNetCmd(gcp::util::NetCmd* netCmd) {}

/**.......................................................................
 * Scanner task commands
 */
void AntennaNetCmdForwarder::forwardScannerNetCmd(gcp::util::NetCmd* netCmd) {}

/**.......................................................................
 * AntennaRx task commands
 */
void AntennaNetCmdForwarder::forwardRxNetCmd(gcp::util::NetCmd* netCmd) {}

/**.......................................................................
 * AntennaRoach task commands
 */
void AntennaNetCmdForwarder::forwardRoachNetCmd(gcp::util::NetCmd* netCmd) {}

/**.......................................................................
 * AntennaLna task commands
 */
void AntennaNetCmdForwarder::forwardLnaNetCmd(gcp::util::NetCmd* netCmd) {}

/**.......................................................................
 * Board flagging commands
 */
void AntennaNetCmdForwarder::forwardBoardNetCmd(gcp::util::NetCmd* netCmd){}

