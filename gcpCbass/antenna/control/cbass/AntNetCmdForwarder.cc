#define __FILEPATH__ "antenna/control/specific/AntNetCmdForwarder.cc"

#include "gcp/util/specific/Directives.h"
#include "gcp/util/common/Debug.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include "gcp/antenna/control/specific/AntNetCmdForwarder.h"
#include "gcp/antenna/control/specific/AntennaMaster.h"

using namespace gcp::control;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor.
 */
AntNetCmdForwarder::AntNetCmdForwarder(AntennaMaster* parent) 
{
  parent_ = parent;
}

/**.......................................................................
 * Destructor.
 */
AntNetCmdForwarder::~AntNetCmdForwarder() {}

/**.......................................................................
 * Forward a tracker net command
 */
void AntNetCmdForwarder::forwardTrackerNetCmd(gcp::util::NetCmd* netCmd)
{
  AntennaMasterMsg msg;
  TrackerMsg* trackerMsg = msg.getDriveMsg()->getTrackerMsg();
  RtcNetCmd* rtc = &netCmd->rtc_;
  NetCmdId opcode = netCmd->opcode_;

  switch(opcode) {
  case NET_COLLIMATE_CMD:
    
    // Convert from mas to radians
    
    trackerMsg->
      packCollimationMsg(rtc->cmd.collimate.seq, 
			 (gcp::util::PointingMode::Type)rtc->cmd.collimate.mode, 
			 (double)rtc->cmd.collimate.x * mastor, 
			 (double)rtc->cmd.collimate.y * mastor,
			 (gcp::util::Collimation::Type)rtc->cmd.collimate.type, 
			 (double)rtc->cmd.collimate.magnitude * mastor,
			 (double)rtc->cmd.collimate.direction * mastor,
			 (OffsetMsg::Mode)rtc->cmd.collimate.addMode,
			 (gcp::util::PointingTelescopes::Ptel)rtc->cmd.collimate.ptelMask);
    break;
  case NET_ENCODER_CALS_CMD:
    trackerMsg->packEncoderCountsPerTurnMsg(rtc->cmd.encoder_cals.seq, 
					    rtc->cmd.encoder_cals.az, 
					    rtc->cmd.encoder_cals.el,
					    rtc->cmd.encoder_cals.dk);
    break;
  case NET_ENCODER_LIMITS_CMD:
    trackerMsg->packEncoderLimitsMsg(rtc->cmd.encoder_limits.seq, 
				     rtc->cmd.encoder_limits.az_min, 
				     rtc->cmd.encoder_limits.az_max, 
				     rtc->cmd.encoder_limits.el_min, 
				     rtc->cmd.encoder_limits.el_max, 
				     rtc->cmd.encoder_limits.pa_min, 
				     rtc->cmd.encoder_limits.pa_max);
    break;
  case NET_ENCODER_ZEROS_CMD:
    
    // The control program will send us zero points in radians.
    
    trackerMsg->packEncoderZerosMsg(rtc->cmd.encoder_zeros.seq, 
				    rtc->cmd.encoder_zeros.az, 
				    rtc->cmd.encoder_zeros.el,
				    rtc->cmd.encoder_zeros.dk);
    break;
  case NET_FLEXURE_CMD:
    
    // Convert from mas to radians
    
    trackerMsg->packFlexureMsg(rtc->cmd.flexure.seq, 
			       (PointingMode::Type)rtc->cmd.flexure.mode, 
			       (double)rtc->cmd.flexure.sFlexure * mastor,
			       (double)rtc->cmd.flexure.cFlexure * mastor,
			       (gcp::util::PointingTelescopes::Ptel)rtc->cmd.flexure.ptelMask);
    break;
  case NET_EQNEQX_CMD:
    {
      double mjd = rtc->cmd.eqneqx.mjd + 
	(double)rtc->cmd.eqneqx.tt/daysec/1000.0;
      
      // Convert from mas to radians
      
      trackerMsg->packExtendEqnEqxMsg(mjd, rtc->cmd.eqneqx.eqneqx * mastor);
    }
    break;
  case NET_EQUAT_OFFSET_CMD:
    {
      OffsetMsg offset;
      
      // Convert from mas to radians
      
      offset.packEquatOffsets((OffsetMsg::Mode)rtc->cmd.equat_offset.mode,
			      (OffsetMsg::Axis)rtc->cmd.equat_offset.axes,
			      (double)rtc->cmd.equat_offset.ra * mastor,
			      (double)rtc->cmd.equat_offset.dec * mastor);
      
      trackerMsg->packOffsetMsg(rtc->cmd.equat_offset.seq, offset);
    }
    break;
  case NET_HALT_CMD:
    trackerMsg->packHaltMsg(rtc->cmd.halt.seq);
    break;
  case NET_STOP_CMD:
    trackerMsg->packStopMsg();
    break;
  case NET_MODEL_CMD:
    trackerMsg->
      packSelectModelMsg(rtc->cmd.model.seq, 
			 (gcp::util::PointingMode::Type)rtc->cmd.model.mode,
			 (gcp::util::PointingTelescopes::Ptel)rtc->cmd.model.ptelMask);
    break;
  case NET_MOUNT_OFFSET_CMD:
    {
      OffsetMsg offset;
      
      // Convert from mas to radians
      
      offset.packMountOffsets((OffsetMsg::Mode)rtc->cmd.mount_offset.mode,
			      (OffsetMsg::Axis)rtc->cmd.mount_offset.axes,
			      (double)rtc->cmd.mount_offset.az,
			      (double)rtc->cmd.mount_offset.el,
			      (double)rtc->cmd.mount_offset.dk);
      
      trackerMsg->packOffsetMsg(rtc->cmd.mount_offset.seq, offset);
    }
    break;

  case NET_SCAN_CMD:

    // Pack a scan command, angles in mas

    trackerMsg->packScanMsg(rtc->cmd.scan.name,
			    rtc->cmd.scan.seq,
			    rtc->cmd.scan.nreps,
			    rtc->cmd.scan.istart,
			    rtc->cmd.scan.ibody,
			    rtc->cmd.scan.iend,
			    rtc->cmd.scan.npt,
			    rtc->cmd.scan.msPerSample,
			    rtc->cmd.scan.index,
			    rtc->cmd.scan.flag,
			    rtc->cmd.scan.azoff,
			    rtc->cmd.scan.eloff,
			    rtc->cmd.scan.add);
    break;
  case NET_SITE_CMD:
    
    // Convert angles to radians, and altitude to meters
    
    trackerMsg->packSiteMsg((double)rtc->cmd.site.lon * mastor, 
			    (double)rtc->cmd.site.lat * mastor, 
			    (double)rtc->cmd.site.alt / 1000.0);
    break;
  case NET_SLEW_CMD:
    
    // Convert angles to radians
    
    trackerMsg->packSlewMsg(rtc->cmd.slew.seq,
			    rtc->cmd.slew.source,
			    (Axis::Type)rtc->cmd.slew.mask,
			    (Tracking::SlewType)rtc->cmd.slew.slewType,
			    (double)rtc->cmd.slew.az * mastor,
			    (double)rtc->cmd.slew.el * mastor, 
			    (double)rtc->cmd.slew.dk * mastor);
    break;
  case NET_SLEW_RATE_CMD:
    
    // Convert angles to radians

    trackerMsg->packSlewRateMsg(rtc->cmd.slew_rate.seq,
				(Axis::Type)rtc->cmd.slew_rate.mask,
				rtc->cmd.slew_rate.az,
				rtc->cmd.slew_rate.el, 
				rtc->cmd.slew_rate.dk);
    break;
  case NET_SKY_OFFSET_CMD:
    {
      OffsetMsg offset;
      
      // Convert from mas to radians
      
      offset.packSkyOffsets((OffsetMsg::Mode)rtc->cmd.sky_offset.mode,
			    (OffsetMsg::Axis)rtc->cmd.sky_offset.axes,
			    (double)rtc->cmd.sky_offset.x * mastor,
			    (double)rtc->cmd.sky_offset.y * mastor);
      
      trackerMsg->packOffsetMsg(rtc->cmd.sky_offset.seq, offset);
    }
    break;
  case NET_TV_OFFSET_CMD:
    {
      OffsetMsg offset;

      // Convert from mas to radians
      
      offset.packTvOffsets((double)rtc->cmd.tv_offset.up    * mastor,
			   (double)rtc->cmd.tv_offset.right * mastor);
      
      trackerMsg->packOffsetMsg(rtc->cmd.tv_offset.seq, offset);
    }
    break;
  case NET_TILTS_CMD:
    
    // Convert from mas to radians
    
    trackerMsg->
      packTiltsMsg(rtc->cmd.tilts.seq, 
		   (double)rtc->cmd.tilts.ha  * mastor, 
		   (double)rtc->cmd.tilts.lat * mastor, 
		   (double)rtc->cmd.tilts.el  * mastor);
    break;
  case NET_TRACK_CMD:
    {
      double mjd = rtc->cmd.track.mjd + 
	(double)rtc->cmd.track.tt/daysec/1000.0;
      
      // Convert to internal units
      
      trackerMsg->packTrackMsg(rtc->cmd.track.seq, rtc->cmd.track.source, 
			       mjd,
			       rtc->cmd.track.ra  * mastor,  
			       rtc->cmd.track.dec * mastor,
			       rtc->cmd.track.dist / 1.0e6); // micro AU -> AU
    }
    break;
  case NET_TV_ANGLE_CMD:
    
    // Convert from mas to radians
    
    trackerMsg->packTvAngleMsg((double)rtc->cmd.tv_angle.angle * mastor);
    break;
  case NET_UT1UTC_CMD:
    {
      double mjd = rtc->cmd.ut1utc.mjd + 
	(double)rtc->cmd.ut1utc.utc/daysec/1000.0;
      
      // Convert from microseconds to seconds.
      
      trackerMsg->packExtendUt1UtcMsg(mjd, 
				      (double)rtc->cmd.ut1utc.ut1utc / 1.0e6);
    }
    break;
  case NET_LOCATION_CMD:
    
    trackerMsg->packLocationMsg(rtc->cmd.location.north,
				rtc->cmd.location.east,
				rtc->cmd.location.up);
    break;
  case NET_ATMOS_CMD:
    trackerMsg->packWeatherMsg(rtc->cmd.atmos.temperatureInK, 
			       rtc->cmd.atmos.humidityInMax1, 
			       rtc->cmd.atmos.pressureInMbar);

    break;
  case NET_BENCH_ZERO_POSITION_CMD: // Set optical bench zero position

    trackerMsg->packBenchZeroPositionMsg(
      rtc->cmd.bench_zero_position.seq,
      rtc->cmd.bench_zero_position.y1,
      rtc->cmd.bench_zero_position.y2,
      rtc->cmd.bench_zero_position.y3,
      rtc->cmd.bench_zero_position.x4,
      rtc->cmd.bench_zero_position.x5,
      rtc->cmd.bench_zero_position.z6);
    break;
  case NET_BENCH_OFFSET_CMD: // Set optical bench offset
    trackerMsg->packBenchOffsetMsg(
      rtc->cmd.bench_offset.seq,
      rtc->cmd.bench_offset.y1,
      rtc->cmd.bench_offset.y2,
      rtc->cmd.bench_offset.y3,
      rtc->cmd.bench_offset.x4,
      rtc->cmd.bench_offset.x5,
      rtc->cmd.bench_offset.z6);
    break;
  case NET_BENCH_USE_BRAKES_CMD: // Set optical bench use brakes
    trackerMsg->packBenchUseBrakesMsg(
      rtc->cmd.bench_use_brakes.seq,
      rtc->cmd.bench_use_brakes.use_brakes);
    break;
  case NET_BENCH_SET_ACQUIRED_THRESHOLD_CMD: // Set optical bench acquired threshold
    trackerMsg->packBenchSetAcquiredThresholdMsg(
      rtc->cmd.bench_set_acquired_threshold.seq,
      rtc->cmd.bench_set_acquired_threshold.acquired_threshold);
    break;
  case NET_BENCH_SET_FOCUS_CMD: // Set optical bench focus
    trackerMsg->packBenchSetFocusMsg(
      rtc->cmd.bench_set_focus.seq,
      rtc->cmd.bench_set_focus.focus);
    break;

  case NET_SERVO_CMD:
    trackerMsg->packServoCmdMsg(rtc->cmd.servo.cmdId, rtc->cmd.servo.fltVal, rtc->cmd.servo.intVal, rtc->cmd.servo.fltVals);
    //    trackerMsg->packServoCmdMsg(rtc->cmd.servo.cmdId, rtc->cmd.servo.fltVal, rtc->cmd.servo.intVal);
    break;

  default:
    ReportError("here");
    ReportError("Unknown opcode " << opcode);
    break;
  }
  
  // And forward the message to the AntennaControl task.
  
  parent_->forwardMasterMsg(&msg);

  if(Debug::debugging(Debug::DEBUG7))
    Debug::remLevel(Debug::DEBUG2);
}

/**.......................................................................
 * Forward a control net command
 */
void AntNetCmdForwarder::forwardControlNetCmd(gcp::util::NetCmd* netCmd)

{  
  AntennaMasterMsg msg;
  AntennaControlMsg* controlMsg = msg.getControlMsg();
  RtcNetCmd* rtc = &netCmd->rtc_;
  NetCmdId opcode = netCmd->opcode_;

  switch(opcode) {
  case NET_GPIB_CMD:
    controlMsg->getGpibMsg()->packGpibCmdMsg(rtc->cmd.gpib.device, 
					     rtc->cmd.gpib.cmdId, 
					     rtc->cmd.gpib.intVals, 
					     rtc->cmd.gpib.fltVal);

    break;
  default:
    break;
  }

  // And forward the message to the AntennaControl task.
  
  parent_->forwardMasterMsg(&msg);

  if(Debug::debugging(Debug::DEBUG7))
    Debug::remLevel(Debug::DEBUG2);
}

/**.......................................................................
 * Forward an antenna rx net command
 */
void AntNetCmdForwarder::forwardRxNetCmd(gcp::util::NetCmd* netCmd)
{
  AntennaMasterMsg msg;
  AntennaRxMsg* rxMsg = msg.getRxMsg();

  RtcNetCmd* rtc = &netCmd->rtc_;
  NetCmdId opcode = netCmd->opcode_;

  switch(opcode) {
  case NET_RX_CMD:

    rxMsg->packRxCmdMsg(rtc->cmd.rx.cmdId, rtc->cmd.rx.fltVal, rtc->cmd.rx.stageNumber, rtc->cmd.rx.channelNumber);

    // And forward the message to the AntennaRx task.
  
    parent_->forwardMasterMsg(&msg);
    
    break;
  default:
    break;
  }

}

/**.......................................................................
 * Forward an antenna rx net command
 */
void AntNetCmdForwarder::forwardRoachNetCmd(gcp::util::NetCmd* netCmd)
{
  AntennaMasterMsg msg;
  AntennaRoachMsg* roachMsg = msg.getRoachMsg();

  RtcNetCmd* rtc = &netCmd->rtc_;
  NetCmdId opcode = netCmd->opcode_;

  switch(opcode) {
  case NET_ROACH_CMD:

    roachMsg->packRoachCmdMsg(rtc->cmd.roach.fltVal, rtc->cmd.roach.roachNum, rtc->cmd.roach.stringCommand);

    // And forward the message to the AntennaRoach task.
  
    parent_->forwardMasterMsg(&msg);
    
    break;
  default:
    break;
  }

}


/**.......................................................................
 * Forward an antenna lna net command
 */
void AntNetCmdForwarder::forwardLnaNetCmd(gcp::util::NetCmd* netCmd)
{
  AntennaMasterMsg msg;
  AntennaControlMsg* controlMsg = msg.getControlMsg(); 
  AntennaLnaMsg* lnaMsg = controlMsg->getLnaMsg();

  RtcNetCmd* rtc = &netCmd->rtc_;
  NetCmdId opcode = netCmd->opcode_;

  switch(opcode) {
  case NET_LNA_CMD:

    lnaMsg->packLnaCmdMsg(rtc->cmd.lna.cmdId, rtc->cmd.lna.drainVoltage, rtc->cmd.lna.drainCurrent, rtc->cmd.lna.lnaNumber, rtc->cmd.lna.stageNumber);

    // And forward the message to the AntennaRx task.
    COUT("antenna/control/specific/AntNetCmdForwarder.c");
    COUT("lna msg cmdId: " << rtc->cmd.lna.cmdId);
    COUT("lna msg cmdId: " << lnaMsg->body.lna.lnaCmdId);

    parent_->forwardMasterMsg(&msg);
    
    break;
  default:
    break;
  }

}
