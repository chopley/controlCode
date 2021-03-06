#define __FILEPATH__ "util/specific/ArrayNetCmdForwarder.cc"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/specific/ArrayNetCmdForwarder.h"

#include <iostream>

using namespace gcp::util;
using namespace gcp::control;
using namespace std;

/**.......................................................................
 * Constructor.
 */
ArrayNetCmdForwarder::ArrayNetCmdForwarder() 
{
  antennaForwarder_ = 0;
  controlForwarder_ = 0;
  dcForwarder_      = 0;
  delayForwarder_   = 0;
  grabberForwarder_ = 0;
  scannerForwarder_ = 0;
  stripForwarder_   = 0;
  ptelForwarder_    = 0;
}

/**.......................................................................
 * Destructor.
 */
ArrayNetCmdForwarder::~ArrayNetCmdForwarder() 
{
  if(antennaForwarder_ != 0)
    delete antennaForwarder_;

  if(controlForwarder_ != 0)
    delete controlForwarder_;
  
  if(grabberForwarder_ != 0)
    delete grabberForwarder_;

  if(scannerForwarder_ != 0)
    delete scannerForwarder_;
}

/**.......................................................................
 * Forward a network command read from the network stream buffer.
 */
void ArrayNetCmdForwarder::forwardNetCmd(gcp::util::NetCmd* netCmd)
{
  COUT("Inside baseclass forwardNetCmd: init_= " << netCmd->init_);

  RtcNetCmd* rtc  = &netCmd->rtc_;
  NetCmdId opcode = netCmd->opcode_;

  //DBPRINT(true, Debug::DEBUGANY, "Got a command");

  // Interpret the command.
  
  switch(opcode) {

    //------------------------------------------------------------
    // Commands for the scanner task
    //------------------------------------------------------------

  case NET_FEATURE_CMD:
  case NET_SETFILTER_CMD:
    forwardScannerNetCmd(netCmd);
    break;

    //------------------------------------------------------------
    // Commands for the control task
    //------------------------------------------------------------

  case NET_INIT_CMD:
    COUT("Forwarding a NET_INIT_CMD");
    forwardControlNetCmd(netCmd);
    break;
  case NET_RUN_SCRIPT_CMD: // A script command for the receiver
			   // control task
    forwardControlNetCmd(netCmd);
    break;
  case NET_SCRIPT_DIR_CMD: // A script command for the receiver
			   // control task
    //    COUT("Got a script_dir command: dir = " << rtc->cmd.scriptDir.dir);

    forwardControlNetCmd(netCmd);
    break;

    //------------------------------------------------------------
    // Commands for the frame grabber task
    //------------------------------------------------------------

  case NET_CONFIGURE_FG_CMD: // Configure the frame grabber 
  case NET_GRABBER_CMD:      // Grab the next frame from the frame grabber 
    forwardGrabberNetCmd(netCmd);
    break;

    //------------------------------------------------------------
    // Commands for the pointing telescope task
    //------------------------------------------------------------

  case NET_PTEL_SHUTTER_CMD:  // Open and close pointing telescope shutter
    forwardPtelNetCmd(netCmd);
    break;
  case NET_PTEL_HEATER_CMD:   // Turn pointing telescope heater on and off
    forwardPtelNetCmd(netCmd);
    break;
  case NET_CABIN_SHUTTER_CMD: // Open and close cabin shutter
    forwardPtelNetCmd(netCmd);
    break;
  case NET_DEICING_HEATER_CMD: // Open and close cabin shutter
    forwardDeicingNetCmd(netCmd);
    break;

    //------------------------------------------------------------
    // Commands for the antennas
    //------------------------------------------------------------

  case NET_UNFLAG_CMD:
  case NET_SETREG_CMD:
  case NET_PAGER_CMD:
  case NET_REBOOT_DRIVE_CMD:

    // Tracking commands

  case NET_HALT_CMD: 
  case NET_STOP_CMD: 
  case NET_MOUNT_OFFSET_CMD:
  case NET_EQUAT_OFFSET_CMD:
    forwardAntennaNetCmd(netCmd);
    break;
  case NET_TV_OFFSET_CMD:
    DBPRINT(true, Debug::DEBUG3, "Got a tv offset cmd");
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
    forwardAntennaNetCmd(netCmd);
    break;
  case NET_ATMOS_CMD:
    forwardAntennaNetCmd(netCmd);

    // Forward optical camera and stepper motor commands to the
    // optical camera task.
    break;
  case NET_OPTCAM_CNTL_CMD:   // Control power to the camera/stepper 
  case NET_STEPPER_CMD:       // Step the stepper motor 
  case NET_CAMERA_CMD:        // Control the camera via the camera control box 
  case NET_FG_CMD:            // Write to a frame grabber register 
  case NET_FLATFIELD_CMD:     // Toggle flat fielding of frame grabber images 
  case NET_BENCH_ZERO_POSITION_CMD: // Set optical bench zero position
  case NET_BENCH_OFFSET_CMD:        // Set optical bench offset
  case NET_BENCH_USE_BRAKES_CMD:    // Set optical bench use brakes
  case NET_BENCH_SET_ACQUIRED_THRESHOLD_CMD: // Set optical bench
					     // acquired threshold
  case NET_BENCH_SET_FOCUS_CMD:     // Set optical bench focus

    //------------------------------------------------------------ 
    // The following commands are forwarded both to the antennas and
    // the delay engine
    //------------------------------------------------------------

  case NET_SLEW_CMD:              // Any of the following may contain a phase     
  case NET_TRACK_CMD: 		  // tracking command, so we			     
  case NET_SCAN_CMD:
  case NET_SITE_CMD:
  case NET_LOCATION_CMD:
  case NET_UT1UTC_CMD:            // UT1-UTC ephemeris
  case NET_EQNEQX_CMD:            // equation of the equinox ephemeris
    forwardAntennaNetCmd(netCmd); // Note deliberate fall-through
				  // here. We want the above commands
				  // to be forwarded to both tasks.
    break;
  
    //------------------------------------------------------------
    // Commands for the downconverter
    //------------------------------------------------------------

  case NET_RESET_CMD:
    forwardAntennaNetCmd(netCmd);
    break;
  case NET_GPIB_CMD:
    //    COUT("In arrayNetCmdForwarder");
    //    COUT("GPIB CMDID: " << netCmd->rtc_.cmd.gpib.cmdId);
    forwardAntennaNetCmd(netCmd);
    break;
  case NET_SERVO_CMD:
    //    COUT("IN arrayNetCmdForwarder");
    //    COUT("SERVO CMDID: " << netCmd->rtc_.cmd.servo.cmdId);
    forwardAntennaNetCmd(netCmd);
    break;

  case NET_RX_CMD:
  case NET_ROACH_CMD:
    //    COUT("IN arrayNetCmdForwarder");
    //    COUT("RX CMDID: " << netCmd->rtc_.cmd.rx.cmdId);
    forwardAntennaNetCmd(netCmd);
    break;
  case NET_LNA_CMD:
    COUT("IN arrayNetCmdForwarder");
    COUT("LNA CMDID: " << netCmd->rtc_.cmd.lna.cmdId);
    forwardAntennaNetCmd(netCmd);
    break;
    
  case NET_INHIBIT_CMD:
    break;

  case NET_POWER_CMD:
    COUT("IN util/specific/arrayNetCmdForwarder");
    COUT("DO SOMETHING");
    break;
    
  default:
    ReportError("Unknown opcode" << opcode);
    break;
  }
}

/**.......................................................................
 * Forward a command to the antenna subsystem
 */
void ArrayNetCmdForwarder::forwardAntennaNetCmd(gcp::util::NetCmd* netCmd)
{
  if(antennaForwarder_ != 0) {
    antennaForwarder_->forwardNetCmd(netCmd);
  }
}

/**.......................................................................
 * Forward a control command
 */
void ArrayNetCmdForwarder::forwardControlNetCmd(gcp::util::NetCmd* netCmd)
{
  if(controlForwarder_ != 0)
    controlForwarder_->forwardNetCmd(netCmd);
}

/**.......................................................................
 * Forward a command to the delay subsystem
 */
void ArrayNetCmdForwarder::forwardDelayNetCmd(gcp::util::NetCmd* netCmd)
{
  if(delayForwarder_ != 0)
    delayForwarder_->forwardNetCmd(netCmd);
}

/**.......................................................................
 * Forward a command to the dc subsystem
 */
void ArrayNetCmdForwarder::forwardDcNetCmd(gcp::util::NetCmd* netCmd)
{
  if(dcForwarder_ != 0)
    dcForwarder_->forwardNetCmd(netCmd);
}

/**.......................................................................
 * Forward a command to the grabber subsystem
 */
void ArrayNetCmdForwarder::forwardGrabberNetCmd(gcp::util::NetCmd* netCmd)
{
  if(grabberForwarder_ != 0)
    grabberForwarder_->forwardNetCmd(netCmd);
}

/**.......................................................................
 * Forward a command to the ptel subsystem
 */
void ArrayNetCmdForwarder::forwardPtelNetCmd(gcp::util::NetCmd* netCmd)
{
  if(ptelForwarder_ != 0)
    ptelForwarder_->forwardNetCmd(netCmd);
}

/**.......................................................................
 * Forward a command to the deicing subsystem
 */
void ArrayNetCmdForwarder::forwardDeicingNetCmd(gcp::util::NetCmd* netCmd)
{
  if(deicingForwarder_ != 0)
    deicingForwarder_->forwardNetCmd(netCmd);
}

/**.......................................................................
 * Forward a scanner command
 */
void ArrayNetCmdForwarder::forwardScannerNetCmd(gcp::util::NetCmd* netCmd)
{
  if(scannerForwarder_ != 0)
    scannerForwarder_->forwardNetCmd(netCmd);
}

/**.......................................................................
 * Forward a strip command
 */
void ArrayNetCmdForwarder::forwardStripNetCmd(gcp::util::NetCmd* netCmd)
{
  if(stripForwarder_ != 0)
    stripForwarder_->forwardNetCmd(netCmd);
}

