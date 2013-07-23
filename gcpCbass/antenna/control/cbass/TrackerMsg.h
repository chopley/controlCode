#ifndef TRACKERMSG_H
#define TRACKERMSG_H

/**
 * @file TrackerMsg.h
 * 
 * Tagged: Thu Nov 13 16:53:56 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/control/code/unix/libunix_src/common/genericregs.h" // SRC_LEN
#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h" // SCAN_NET_NPT

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/GenericTaskMsg.h"

#include "gcp/util/common/Axis.h"
#include "gcp/util/common/OffsetMsg.h"
#include "gcp/util/common/PointingMode.h"
#include "gcp/util/common/PointingTelescopes.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/Tracking.h"

#include "gcp/util/common/Collimation.h"
#include "gcp/antenna/control/specific/Refraction.h"

#include <string.h>

namespace gcp {
  namespace antenna {
    namespace control {
      
      class TrackerMsg :
      public gcp::util::GenericTaskMsg {
	
      public:
	
	/**
	 * Enumerate supported message types.
	 */
	enum MsgType {
	  COLLIMATION,
	  CONNECT_DRIVE,      // Attempt to connect to the drive.
	  DISCONNECT_DRIVE,   // Disconnect from the drive.
	  ENCODER_CALS,
	  ENCODER_LIMITS,
	  ENCODER_ZEROS,
	  EXTEND_EQNEQX,
	  EXTEND_UT1UTC,
	  FLAG_BOARD,
	  FLEXURE,
	  HALT,
	  LOCATION,
	  SITE,
	  OFFSET,
	  REBOOT_DRIVE,
	  SHUTDOWN,
	  REFRACTION,
	  RESTART,
	  SCAN,
	  SELECT_MODEL,
	  SLEW,
	  SLEWRATE,
	  START_TIMER,         // Start 1pps timer
	  STOP_TIMER,          // Stop 1pps timer
	  STROBE_SERVO,
	  TICK,                // 1-pps tick has arrived
	  TILTS,
	  TRACK,
	  TV_ANGLE,
	  YEAR,
	  WEATHER,
	  RX,
          BENCH_ZERO_POSITION,
          BENCH_OFFSET,
          BENCH_USE_BRAKES,
          BENCH_SET_ACQUIRED_THRESHOLD,
          BENCH_SET_FOCUS,
          TILT_METER_CORRECTION,
          TILT_METER_OFFSET,
          TILT_METER_ANGLE,
          TILT_METER_RANGE,
          TILT_METER_MOVING_AVERAGE_INTERVAL,
          LINEAR_SENSOR_CORRECTION,
          LINEAR_SENSOR_OFFSET,
          LINEAR_SENSOR_RANGE,
          LINEAR_SENSOR_MOVING_AVERAGE_INTERVAL,
	  STOP,
	  TEST_TRACK_TIME, 
	  SPECIFIC_SERVO_COMMAND,
	};

	/**
	 *  Tracker specific.
	 */
	enum SpecMsgType {
	  SERVO_INITIALIZE_ANTENNA,
	  SERVO_HARD_STOP,
	  SERVO_DISABLE_PADS,
	  SERVO_LOAD_PARAMETERS,
	};
	
	/**
	 * A type for this message
	 */
	MsgType type;
	
	/**
	 * A message body.
	 */
	union {
	  
	  /**------------------------------------------------------------
	   * Reboot the Drive
	   */
	  struct {
	    unsigned long seq;
	  } rebootDrive;
	  
	  /**------------------------------------------------------------
	   * Extend a scan ephemeris
	   */
	  struct {
	    char name[SCAN_LEN];
	    int seq;
	    unsigned nreps;
	    unsigned istart;
	    unsigned ibody;
	    unsigned iend;
	    unsigned npt;
	    unsigned msPerSample;
	    unsigned flag[SCAN_NET_NPT];
	    unsigned index[SCAN_NET_NPT];
	    int azoff[SCAN_NET_NPT];
	    int eloff[SCAN_NET_NPT];
	    int dkoff[SCAN_NET_NPT];
	    bool add;
	  } scan;

	  
	  /**------------------------------------------------------------
	   * Slew the telescope to a demanded position
	   */
	  struct {
	    unsigned long seq;
	    gcp::util::Axis::Type axes;
	    gcp::util::Tracking::SlewType slewType;
	    char source[SRC_LEN];
	    double az;
	    double el;
	    double pa;
	  } slew;
	  
	  /**------------------------------------------------------------
	   * Halt the telescope immediately
	   */
	  struct {
	    unsigned long seq;
	  } halt;
	  
	  /**------------------------------------------------------------
	   * A 1-pps tick
	   */
	  struct {
	    unsigned long mjdDays;
	    unsigned long mjdSeconds;
	  } tick;

	  /**------------------------------------------------------------
	   * A 1-pps strobe
	   */
	  struct {
	    unsigned long mjdDays;
	    unsigned long mjdSeconds;
	  } strobe;
	  
	  /**------------------------------------------------------------
	   * Extend the track of a source
	   */
	  struct {
	    unsigned long seq;
	    char source[SRC_LEN];
	    double mjd;  // The Terrestrial Time at which ra,dec are
			 // valid, expressed as a Modified Julian Day
			 // number
	    double ra;   // The desired apparent Right Ascension
			 // (0...360 degrees in radians)
	    double dec;  // The desired apparent Declination
			 // (-180..180 degrees in radians)
	    double dist; // The distance to the source if it is near
			 // enough for parallax to be
			 // significant. Specify the distance in
			 // AU. Send 0 for distant sources.
	  } track;
	  
	  /**------------------------------------------------------------
	   * Adjust the tracking offsets
	   */
	  struct {
	    unsigned long seq; // The sequence number of this request
	    gcp::util::OffsetMsg offset;  // A message containg the
					  // offset (see OffsetMsg.h)
	  } offset;
	  
	  /**------------------------------------------------------------
	   * Set the PA angle at which the vertical direction on the TV
	   * monitor of the optical telescope matches the direction of
	   * increasing topocentric elevation
	   */
	  struct {
	    double angle; // The PA angle at which the camera image is
			  // upright (radians)
	  } tvAngle;
	  
	  /**------------------------------------------------------------
	   * Update the refraction correction
	   *
	   */
	  struct {
	    gcp::util::PointingMode::Type mode; // Which refraction mode are we
	    // updating? (radio | optical)
	    double a; // The A term of the refraction correction
	    double b; // The B term of the refraction correction
	  } refraction;
	  
	  /**------------------------------------------------------------
	   * Commands used to send occasional updates of variable earth
	   * orientation parameters.
	   *
	   * For each command the control system retains a table of
	   * the 3 most recently received updates. These three values
	   * are quadratically interpolated to yield orientation
	   * parameters for the current time.  On connection to the
	   * control system, the control program is expected to send
	   * values for the start of the current day, the start of the
	   * following day and the start of the day after
	   * that. Thereafter, at the start of each new day, it should
	   * send parameters for a time two days in the future.
	   *
	   * On startup of the control system, requests for ut1utc and
	   * eqeqx will return zero. On receipt of the first
	   * earth-orientation command, requests for orientation
	   * parameters will return the received values.  On the
	   * receipt of the second, requesters will receive a linear
	   * interpolation of the parameters. On receipt of the third
	   * and subsequent commands, requesters will receive
	   * quadratically interpolated values using the parameters of
	   * the three most recently received commands.
	   */
	  /**------------------------------------------------------------
	   * The UT1-UTC correction
	   */
	  struct {
	    double mjd;     // The UTC to which this command refers as
			    // a Modified Julian Day number
	    double ut1utc;  // The value of ut1 - utc (seconds) 
	  } extendUt1Utc;
	  
	  /**------------------------------------------------------------
	   * The equation of the equinoxes.
	   */
	  struct {
	    double mjd;     // The Terrestrial Time to which this
			    // command refers, as a Modified Julian
			    // day
	    double eqneqx;  // The equation of the equinoxes (radians)
	  } extendEqnEqx;
	  
	  /**------------------------------------------------------------
	   * The slew_rate is used to set the slew speeds of each of
	   * the telescope axes. The speed is specified as a
	   * percentage of the maximum speed available.
	   */
	  struct {
	    unsigned long seq; // The tracker sequence number of this command 
	    gcp::util::Axis::Type axes;   // A bitwise union of
					  // Axis::Type enumerated
					  // bits, used to specify
					  // which of the following
					  // axis rates are to be
					  // applied.
	    long az;           // Azimuth slew rate (0-100) 
	    long el;           // Elevation slew rate (0-100)
	    long dk;           // Deck slew rate (0-100)
	  } slewRate;
	  
	  /**------------------------------------------------------------
	   * Calibrate the axis tilts of the telescope.
	   */
	  struct {
	    unsigned long seq; // The tracker sequence number of this command 
	    double ha;         // The hour-angle component of the
			       // azimuth-axis tilt (mas)
	    double lat;        // The latitude component of the
			       // azimuth-axis tilt (mas)
	    double el;         // The tilt of the elevation axis
			       // perpendicular to the azimuth ring,
			       // measured clockwise around the
			       // direction of the azimuth std::vector
			       // (mas)
	  } tilts;
	  
	  /**.......................................................................
	   * Set the gravitational flexure of the telescope.
	   */
	  struct {
	    unsigned long seq;  // The tracker sequence number of this
				// command
	    gcp::util::PointingMode::Type mode; // A PointingMode
						// enumeration
						// (optical|radio)

	    double sFlexure;    // Gravitational flexure (radians per
				// sine elevation)
	    double cFlexure;    // Gravitational flexure (radians per
				// cosine elevation)
	    gcp::util::PointingTelescopes::Ptel ptelMask;
	  } flexure;
	  
	  /**------------------------------------------------------------
	   * Calibrate the collimation of the optical or radio axes.
	   */
	  struct {
	    unsigned long seq; // The tracker sequence number of this
			       // command
	    gcp::util::PointingMode::Type mode; // Which collimation?
						// (optical|radio)
	    double x;        // The magnitude of the horizonatl
			     // collimation offset
	    double y;        // The magnitude of the vertical
			     // collimation offset
	    gcp::util::Collimation::Type type; // FIXED or POLAR?

	    double magnitude; // The magnitude of the polar
			      // collimation offset
	    double direction; // The direction of the polar
			      // collimation offset

	    gcp::util::OffsetMsg::Mode addMode; // ADD or SET?
	    gcp::util::PointingTelescopes::Ptel ptelMask;
	  } collimation;
	  
	  /**------------------------------------------------------------
	   * Set the calibation factors of the telescope encoders. 
	   */
	  struct {
	    unsigned long seq; // The sequence number of this command
	    long az;
	    long el;
	    long dk;
	  } encoderCountsPerTurn;

	  /**------------------------------------------------------------
	   * Tell the drive task what the limits on encoder values
	   * are.
	   */
	  struct {
	    unsigned long seq; // The tracker sequence number of this command 
	    long az_min;      // The lower azimuth limit (encoder counts) 
	    long az_max;      // The upper azimuth limit (encoder counts) 
	    long el_min;      // The lower elevation limit (encoder counts)
	    long el_max;      // The upper elevation limit (encoder counts)
	    long pa_min;      // The lower deck limit (encoder counts)
	    long pa_max;      // The upper deck limit (encoder counts)
	  } encoderLimits;
	  
	  /**------------------------------------------------------------
	   * Set the zero points of the telescope encoders. The angles
	   * are measured relative to the position at which the
	   * encoders show zero counts.
	   */
	  struct {
	    unsigned long seq; // The sequence number of this command
	    double az;  // Azimuth encoder angle at zero azimuth,
			// measured in the direction of increasing
			// azimuth (radians)
	    double el;  // Elevation encoder angle at zero elevation,
			// measured in the direction of increasing
			// elevation (radians)
	    double dk;  // Deck encoder angle at the deck reference
			// position, measured clockwise when looking
			// towards the sky (radians)
	  } encoderZeros;
	  
	  /**------------------------------------------------------------
	   * Select between the optical and radio pointing models.
	   */
	  struct {
	    unsigned long seq;  // The tracker sequence number of this
				// command
	    gcp::util::PointingMode::Type mode; // A PointingMode enumeration 
	    gcp::util::PointingTelescopes::Ptel ptelMask;
	  } selectModel;
	  
	  /**------------------------------------------------------------
	   * Tell the control system what the current year is. This is
	   * necessary because the gps time-code reader doesn't supply
	   * year information.
	   */
	  struct {
	    short year;   // The current Gregorian year 
	  } year;
	  
	  /**------------------------------------------------------------
	   * The following command is used to inform the control system of the
	   * site of this antenna
	   */
	  struct {
	    double lon;  // The SZA longitude (east +ve) [-pi..pi] (radians)
	    double lat;  // The SZA latitude [-pi/2..pi/2] (radians)
	    double alt;  // The SZA altitude (meters) 
	  } site;
	  
	  /**------------------------------------------------------------
	   * The following command is used to inform the control system of the
	   * site of this antenna
	   */
	  struct {
	    double north; // The antenna offset N (meters)
	    double east;  // The antenna offset E (meters)
	    double up;    // The antenna offset Up (meters)
	  } location;

	  /**------------------------------------------------------------
	   * Flag a board.
	   */
	  struct {
	    unsigned short board; // The register map index of the
	    // board to un/flag
	    bool flag;            // True to flag, false to unflag
	  } flagBoard;
	  
	  struct {
	    double airTemperatureInK;
	    double relativeHumidityInMax1;
	    double pressureInMbar;
	  } weather;
	  
          struct {
            unsigned int seq;
            double y1;
            double y2;
            double y3;
            double x4;
            double x5;
            double z6;
          } benchZeroPosition;

          struct {
            unsigned int seq;
            double y1;
            double y2;
            double y3;
            double x4;
            double x5;
            double z6;
          } benchOffset;

          struct {
            unsigned int seq;
            bool use_brakes;
          } benchUseBrakes;

          struct {
            unsigned int seq;
            double threshold;
          } benchSetAcquiredThreshold;

          struct {
            unsigned int seq;
            double focus;
          } benchSetFocus;

          struct {
            bool enable;
          } tilt_meter_correction;

          struct {
            double x; // x tilt meter offset (degrees)
            double y; // y tilt meter offset (degrees)
          } tilt_meter_offset;

          struct {
            double angle; // x tilt meter direction relative to 0 azimuth (degrees)
          } tilt_meter_angle;

          struct {
            double maxAngle;  // max allowed tilt meter reading (degrees)
          } tilt_meter_range;

          struct {
            double interval;  // tilt meter moving average interval (seconds)
          } tilt_meter_moving_average_interval;

          struct {
            bool enable;
          } linear_sensor_correction;

          struct {
            double L1;  // Left sensor 1 offset (millimeters)
            double L2;  // Left sensor 2 offset (millimeters)
            double R1;  // Right sensor 1 offset (millimeters)
            double R2;  // Right sensor 2 offset (millimeters)
          } linear_sensor_offset;

          struct {
            double maxDistance;  // max allowed linear sensor reading (millimeters)
          } linear_sensor_range;

          struct {
            double interval;  // linear sensor moving average interval (seconds)
          } linear_sensor_moving_average_interval;

	  struct {
	    unsigned sec;
	    unsigned msec;
	  } testTrackTime;

	  struct {
	    unsigned cmdId;
	    int intVal;
	    float fltVal;
	    float fltVals[10];
	  } servoCmd;


	} body;
	
	//------------------------------------------------------------
	// Methods for packing message to the tracker task.
	//
	// We explicitly initialize genericMsgType_ in each method,
	// since we cannot do this in a constructor, since objects
	// with explicit constructors apparently can't be union
	// members.
	//------------------------------------------------------------
	
        inline void packBenchZeroPositionMsg(unsigned int seq, double y1, double y2, double y3, 
					     double x4, double x5, double z6)
        {
          genericMsgType_ =
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = BENCH_ZERO_POSITION;

          body.benchZeroPosition.seq = seq;
          body.benchZeroPosition.y1 = y1;
          body.benchZeroPosition.y2 = y2;
          body.benchZeroPosition.y3 = y3;
          body.benchZeroPosition.x4 = x4;
          body.benchZeroPosition.x5 = x5;
          body.benchZeroPosition.z6 = z6;
        }
    
        inline void packBenchOffsetMsg(unsigned int seq, double y1, double y2, double y3, double x4, double x5, double z6)
        {
          genericMsgType_ =
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = BENCH_OFFSET;

          body.benchOffset.seq = seq;
          body.benchOffset.y1 = y1;
          body.benchOffset.y2 = y2;
          body.benchOffset.y3 = y3;
          body.benchOffset.x4 = x4;
          body.benchOffset.x5 = x5;
          body.benchOffset.z6 = z6;
        }
		
        inline void packBenchUseBrakesMsg(unsigned int seq, bool use_brakes)
        {
          genericMsgType_ =
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = BENCH_USE_BRAKES;

          body.benchUseBrakes.seq = seq;
          body.benchUseBrakes.use_brakes = use_brakes;
        }

        inline void packBenchSetAcquiredThresholdMsg(unsigned int seq, double threshold)
        {
          genericMsgType_ =
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = BENCH_SET_ACQUIRED_THRESHOLD;

          body.benchSetAcquiredThreshold.seq = seq;
          body.benchSetAcquiredThreshold.threshold = threshold;
        }

        inline void packBenchSetFocusMsg(unsigned int seq, double focus)
        {
          genericMsgType_ =
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = BENCH_SET_FOCUS;

          body.benchSetFocus.seq = seq;
          body.benchSetFocus.focus = focus;
        }

	/**
	 * Pack a message to connect to the pmac.
	 */
	inline void packConnectDriveMsg() {
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = CONNECT_DRIVE;
	}
	
	/**
	 * Pack a message to strobe the pmac.
	 */
	inline void packTickMsg(gcp::util::TimeVal& time)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = TICK;

	  body.tick.mjdDays    = time.getMjdDays();
	  body.tick.mjdSeconds = time.getMjdSeconds();
	}
	

	/**
	 * Pack a message to disconnect from the pmac.
	 */
	inline void packDisconnectDriveMsg() {
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  
	  type = DISCONNECT_DRIVE;
	}
	
	inline void packRebootDriveMsg(unsigned long seq)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = REBOOT_DRIVE;
	  body.rebootDrive.seq = seq;
	}

        inline void packTiltMeterCorrectionMsg(bool enable)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = TILT_METER_CORRECTION;
 
          body.tilt_meter_correction.enable = enable;
        }
	
        inline void packTiltMeterOffsetMsg(double x, double y)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = TILT_METER_OFFSET;
 
          body.tilt_meter_offset.x = x;
          body.tilt_meter_offset.y = y;
        }
	
        inline void packTiltMeterAngleMsg(double angle)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = TILT_METER_ANGLE;
 
          body.tilt_meter_angle.angle = angle;
        }
	
        inline void packTiltMeterRangeMsg(double maxAngle)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = TILT_METER_RANGE;
 
          body.tilt_meter_range.maxAngle = maxAngle;
        }
	
        inline void packTiltMeterMovingAverageIntervalMsg(double interval)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = TILT_METER_MOVING_AVERAGE_INTERVAL;
 
          body.tilt_meter_moving_average_interval.interval = interval;
        }
	
        inline void packLinearSensorCorrectionMsg(bool enable)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = LINEAR_SENSOR_CORRECTION;
 
          body.linear_sensor_correction.enable = enable;
        }
	
        inline void packLinearSensorOffsetMsg(double L1, double L2, double R1, double R2)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = LINEAR_SENSOR_OFFSET;
 
          body.linear_sensor_offset.L1 = L1;
          body.linear_sensor_offset.L2 = L2;
          body.linear_sensor_offset.R1 = R1;
          body.linear_sensor_offset.R2 = R2;
        }
	
        inline void packLinearSensorRangeMsg(double maxDistance)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = LINEAR_SENSOR_RANGE;
 
          body.linear_sensor_range.maxDistance = maxDistance;
        }
	
        inline void packLinearSensorMovingAverageIntervalMsg(double interval)
        {
          genericMsgType_ =
            gcp::util::GenericTaskMsg::TASK_SPECIFIC;

          type = LINEAR_SENSOR_MOVING_AVERAGE_INTERVAL;
 
          body.linear_sensor_moving_average_interval.interval = interval;
        }
	
	inline void packScanMsg(std::string name,
				int seq, 
				unsigned nreps, 
				unsigned istart, 
				unsigned ibody, 
				unsigned iend, 
				unsigned npt,
				unsigned msPerSample,
				unsigned* index,
				unsigned* flag,
				int* azoff,
				int* eloff,
				bool add)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = SCAN;

	  if(name.size() > SCAN_LEN)
	    ThrowError("scan name is too long");

	  if(npt > SCAN_NET_NPT)
	    ThrowError("scan npt > " << SCAN_NET_NPT);
	    
	  strncpy(body.scan.name, name.c_str(), SCAN_LEN);

	  body.scan.seq    = seq;
	  body.scan.nreps  = nreps;
	  body.scan.istart = istart;
	  body.scan.ibody  = ibody;
	  body.scan.iend   = iend;
	  body.scan.npt    = npt;
	  body.scan.msPerSample = msPerSample;

	  for(unsigned i=0; i < npt; i++) {
	    body.scan.index[i]  = index[i];
	    body.scan.flag[i]   = flag[i];
	    body.scan.azoff[i]  = azoff[i];
	    body.scan.eloff[i]  = eloff[i];
	  }

	  body.scan.add = add;
	}

	inline void packStopMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = STOP;
	}

	inline void packShutdownDriveMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = SHUTDOWN;
	}

	inline void packSlewMsg(unsigned long seq, 
				std::string source, 
				gcp::util::Axis::Type axes, 
				gcp::util::Tracking::SlewType slewType,
				double az, double el, double pa)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = SLEW;
	  body.slew.seq  = seq;
	  body.slew.axes = axes;
	  body.slew.slewType = slewType;
	    
	  if(source.size() > SRC_LEN)
	    ThrowError("source name is too long.\n");
	    
	  strncpy(body.slew.source, source.c_str(), SRC_LEN);
	    
	  body.slew.az = az;
	  body.slew.el = el;
	  body.slew.pa = pa;
	}
	inline void packHaltMsg(unsigned long seq)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = HALT;
	  body.halt.seq = seq;
	}
	
	inline void packTrackMsg(unsigned long seq, 
				 std::string source, 
				 double mjd, double ra,  
				 double dec, double dist)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = TRACK;
	  body.track.seq = seq;
	    
	  if(source.size() > SRC_LEN)
	    ThrowError("source name is too long.\n");
	    
	  strncpy(body.track.source, source.c_str(), SRC_LEN);
	    
	  body.track.mjd  = mjd;
	  body.track.ra   = ra;
	  body.track.dec  = dec;
	  body.track.dist = dist;
	}
	
	inline void packOffsetMsg(unsigned long seq, 
				  gcp::util::OffsetMsg offset)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = OFFSET;
	  body.offset.seq    = seq;
	  body.offset.offset = offset;
	}
	
	inline void packTvAngleMsg(double angle)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type               = TV_ANGLE;
	  body.tvAngle.angle = angle;
	}
	
	inline void packEncoderZerosMsg(unsigned long seq, 
					double az, double el, double dk)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = ENCODER_ZEROS;
	  body.encoderZeros.seq = seq;
	  body.encoderZeros.az  = az;
	  body.encoderZeros.el  = el;
	  body.encoderZeros.dk  = dk;
	}
	
	inline void packEncoderCountsPerTurnMsg(unsigned long seq, 
						long az, long el, long dk)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = ENCODER_CALS;
	  body.encoderCountsPerTurn.seq = seq;
	  body.encoderCountsPerTurn.az  = az;
	  body.encoderCountsPerTurn.el  = el;
	  body.encoderCountsPerTurn.dk  = dk;
	}
	
	inline void packRefractionMsg(gcp::util::PointingMode::Type mode, double a, 
				      double b)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = REFRACTION;
	  body.refraction.mode = mode;
	  body.refraction.a    = a;
	  body.refraction.b    = b;
	}
	
	inline void packRestartMsg()
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = RESTART;
	}
	
	inline void packExtendUt1UtcMsg(double mjd, double ut1utc)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = EXTEND_UT1UTC;
	  body.extendUt1Utc.mjd    = mjd;
	  body.extendUt1Utc.ut1utc = ut1utc;
	}
	
	inline void packExtendEqnEqxMsg(double mjd, double eqneqx)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = EXTEND_EQNEQX;
	  body.extendEqnEqx.mjd    = mjd;
	  body.extendEqnEqx.eqneqx = eqneqx;
	}
	
	inline void packSlewRateMsg(unsigned long seq, 
				    gcp::util::Axis::Type axes, 
				    long az, long el, long dk)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = SLEWRATE;
	  body.slewRate.seq  = seq;
	  body.slewRate.axes = axes;
	  body.slewRate.az   = az;
	  body.slewRate.el   = el;
	  body.slewRate.dk   = dk;
	}
	
	inline void packStrobeMsg(gcp::util::TimeVal& time)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = STROBE_SERVO;

	  body.strobe.mjdDays    = time.getMjdDays();
	  body.strobe.mjdSeconds = time.getMjdSeconds();
	}
	
	inline void packTiltsMsg(unsigned long seq, double ha, double lat, 
				 double el)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = TILTS;
	  body.tilts.seq = seq;
	  body.tilts.ha  = ha;
	  body.tilts.lat = lat;
	  body.tilts.el  = el;
	}
	
	inline void packFlexureMsg(unsigned long seq, 
				   gcp::util::PointingMode::Type mode, 
				   double sFlexure,
				   double cFlexure,
				   gcp::util::PointingTelescopes::Ptel ptelMask)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = FLEXURE;
	  body.flexure.seq      = seq;
	  body.flexure.mode     = mode;
	  body.flexure.sFlexure = sFlexure;
	  body.flexure.cFlexure = cFlexure;
	  body.flexure.ptelMask = ptelMask;
	}
	
	inline void packCollimationMsg(unsigned long seq, 
				       gcp::util::PointingMode::Type mode,
				       double x, double y,
				       gcp::util::Collimation::Type collType,
				       double mag, double dir,
				       gcp::util::OffsetMsg::Mode addMode,
				       gcp::util::PointingTelescopes::Ptel ptelMask)

	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = COLLIMATION;
	  body.collimation.seq       = seq;
	  body.collimation.mode      = mode;
	  body.collimation.x         = x;
	  body.collimation.y         = y;
	  body.collimation.type      = collType;
	  body.collimation.magnitude = mag;
	  body.collimation.direction = dir;
	  body.collimation.addMode   = addMode;
	  body.collimation.ptelMask  = ptelMask;
	}
	
	inline void packEncoderLimitsMsg(unsigned long seq, 
					 long az_min, long az_max, 
					 long el_min, long el_max, 
					 long pa_min, long pa_max)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = ENCODER_LIMITS;
	  body.encoderLimits.seq    = seq;
	  body.encoderLimits.az_min = az_min;
	  body.encoderLimits.az_max = az_max;
	  body.encoderLimits.el_min = el_min;
	  body.encoderLimits.el_max = el_max;
	  body.encoderLimits.pa_min = pa_min;
	  body.encoderLimits.pa_max = pa_max;
	}
	
	inline void packSelectModelMsg(unsigned long seq, 
				       gcp::util::PointingMode::Type mode,
				       gcp::util::PointingTelescopes::Ptel ptelMask)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = SELECT_MODEL;
	  body.selectModel.seq      = seq;
	  body.selectModel.mode     = mode;
	  body.selectModel.ptelMask = ptelMask;
	}
	
	inline void packYearMsg(short year)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = YEAR;
	  body.year.year = year;
	}
	
	inline void packSiteMsg(double lon, double lat, double alt)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = SITE;
	  body.site.lon = lon;
	  body.site.lat = lat;
	  body.site.alt = alt;
	}
	
	inline void packLocationMsg(double north, double east, double up)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = LOCATION;
	  body.location.north = north;
	  body.location.east = east;
	  body.location.up = up;
	}
	
	inline void packFlagBoardMsg(unsigned short board, bool flag)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = FLAG_BOARD;
	  body.flagBoard.board = board;
	  body.flagBoard.flag  = flag;
	}
	
	/**
	 * A method for packing a message to flag an antenna as
	 * un/reachable
	 */
	inline void packWeatherMsg(double airTempInK, double relHumidityInMax1, double pressureInMbar) 
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = WEATHER;
	    
	  body.weather.airTemperatureInK      = airTempInK;
	  body.weather.relativeHumidityInMax1 = relHumidityInMax1;
	  body.weather.pressureInMbar         = pressureInMbar;
	}
	
	inline void packTestTrackTimeMsg(unsigned sec, unsigned msec)
	{
	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	    
	  type = TEST_TRACK_TIME;
	  body.testTrackTime.sec  = sec;
	  body.testTrackTime.msec = msec;
	}

	inline void packServoCmdMsg(unsigned cmdId, float fltVal, int intVal, float* fltVals)
	{

	  genericMsgType_ = 
	    gcp::util::GenericTaskMsg::TASK_SPECIFIC;
	  type = SPECIFIC_SERVO_COMMAND;
	  body.servoCmd.cmdId = cmdId;
	  body.servoCmd.intVal = intVal;
	  body.servoCmd.fltVal = fltVal;
	  for (unsigned i=0; i<7; i++){
	    body.servoCmd.fltVals[i] = fltVals[i];
	  };
	}
	
	inline friend std::ostream& operator<<(std::ostream& os, const TrackerMsg& msg)
	{
	  os << "TrackerMsg ";
	  switch (msg.type) {	  
	  case COLLIMATION: os << "COLLIMATION"; break;
	  case CONNECT_DRIVE: os << "CONNECT_DRIVE"; break;
	  case DISCONNECT_DRIVE: os << "DISCONNECT_DRIVE"; break;
	  case ENCODER_CALS: os << "ENCODER_CALS"; break;
	  case ENCODER_LIMITS: os << "ENCODER_LIMITS"; break;
	  case ENCODER_ZEROS: os << "ENCODER_ZEROS"; break;
	  case EXTEND_EQNEQX: os << "EXTEND_EQNEQX"; break;
	  case EXTEND_UT1UTC: os << "EXTEND_UT1UTC"; break;
	  case FLAG_BOARD: os << "FLAG_BOARD"; break;
	  case FLEXURE: os << "FLEXURE"; break;
	  case HALT: os << "HALT"; break;
	  case LOCATION: os << "LOCATION"; break;
	  case SITE: os << "SITE"; break;
	  case OFFSET: os << "OFFSET"; break;
	  case REBOOT_DRIVE: os << "REBOOT_DRIVE"; break;
	  case REFRACTION: os << "REFRACTION"; break;
	  case RESTART: os << "RESTART"; break;
	  case SELECT_MODEL: os << "SELECT_MODEL"; break;
	  case SLEW: os << "SLEW"; break;
	  case SLEWRATE: os << "SLEWRATE"; break;
	  case START_TIMER: os << "START_TIMER"; break;
	  case STOP_TIMER: os << "STOP_TIMER"; break;
	  case STROBE_SERVO: os << "STROBE_SERVO"; break;
	  case TICK: os << "TICK"; break;
	  case TILTS: os << "TILTS"; break;
	  case TRACK: os << "TRACK"; break;
	  case TV_ANGLE: os << "TV_ANGLE"; break;
	  case YEAR: os << "YEAR"; break;
	  case WEATHER: os << "WEATHER"; break;
	  case RX: os << "RX"; break;
	  }
	  os << std::endl;
	}	  

      }; // End class TrackerMsg
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
