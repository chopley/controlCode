//-----------------------------------------------------------------------
// CBASS register map is defined here
//-----------------------------------------------------------------------

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"
#include "arraymap.h"
#include "arraytemplate.h"
#include "miscregs.h"

#include <map>

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/TimeVal.h"

using namespace gcp::util;
using namespace gcp::control;

/**.......................................................................
 * Create a virtual board of Tracker software registers.
 */
static RegBlockTemp cbassTracker[] = {
  
  //------------------------------------------------------------
  // Slow registers
  //------------------------------------------------------------

  RegBlockTemp("The '\\0' terminated source name ",
	       "source",                 "", REG_UCHAR,  0,  SRC_LEN),
	       
  RegBlockTemp("The '\\0' terminated scan name ",
	       "scan_name",              "", REG_UCHAR,  0,  SCAN_LEN),
	       
  RegBlockTemp("The encoder zero points {azimuth,elevation} "
	       "(milli-arcseconds) ",
	       "encoder_off",            "", REG_INT,     0, 2),
   
  RegBlockTemp("The encoder multiplier",
	       "encoder_mul",            "", REG_INT,     0, 2),
   
  RegBlockTemp("The azimuth software limits {min,max} in topocentric mount "
	       "angles (milli-arcseconds) ",
	       "az_limits",              "", REG_INT,     0, 2),
  
  RegBlockTemp("The elevation software limits {min,max} in topocentric mount "
	       "angles (milli-arcseconds) ",
	       "el_limits",              "", REG_INT,     0, 2),
   
  RegBlockTemp("Axis tilts {hour-angle, latitude, elevation} "
	       "(milli-arcseconds) ",
	       "tilts",                  "", REG_INT,     0, 3),
  
  RegBlockTemp("The maximum allowed absolute value of telescope tilt meter readings (milli-arcseconds)",
	       "tilt_max",               "", REG_INT,     0, 1),
  
  RegBlockTemp("The zeroing offset added to telescope tilt meter readings when calculating tilts (milli-arcseconds)",
	       "tilt_xy_offset",         "[0 - x, 1 - y]", REG_INT,     0, 2),
  
  RegBlockTemp("The angle between Grid North and telescope tilt meter x axis (milli-arcseconds)",
	       "tilt_theta",             "", REG_INT,     0, 1),
  
  RegBlockTemp("The interval over which telescope tilt meter readings are averaged (seconds)",
	       "tilt_average_interval",   "", REG_DOUBLE,     0, 1),
  
  RegBlockTemp("The gravitational flexure of the telescope. "
	       "First index is the coefficient of the sin(el) term "
	       "(milli-arcseconds per sin(el), "
	       "second index is the coefficient of the cos(el) term "
	       "(milli-arcseconds per cos(el))",
	       "flexure",                "", REG_INT,     0, 2),
  
  RegBlockTemp("The pointing model being used (true if radio, false if "
	       "optical) ",
	       "axis",                   "", REG_DEFAULT, 0, 1),

  RegBlockTemp("If model is optical, this register encodes the corresponding "
	       "pointing telescope",
	       "ptel",                   "", REG_UCHAR,   0, 1),

  RegBlockTemp("The fixed collimation value {x, y} (milli-arcseconds)",
	       "fixedCollimation",       "", REG_INT,     0, 2),

  RegBlockTemp("The actual location of the site {longitude (mas), "
	       "latitude (mas), altitude (mm)}",
	       "siteActual",             "", REG_INT,     0, 3),
  
  RegBlockTemp("The fiducial location of the site {longitude (mas), latitude "
	       "(mas), altitude (mm)}",
	       "siteFiducial",           "", REG_INT,     0, 3),
  
  RegBlockTemp("Offset from the nominal site {Up(mm), East(mm), North(mm)}",
	       "location",               "", REG_INT,     0, 3),

  RegBlockTemp("Servo is busy", 
	       "servoBusy",               "",REG_BOOL,     0, 1),
  
  //------------------------------------------------------------
  // Fast registers
  //------------------------------------------------------------

  RegBlockTemp("A bitwise union of gcp::util::Pointing::Parameter "
	       "enumerators. Each represents an unreceived pointing model "
	       "parameter (see gcp::util::Pointing for definitions). ",
	       "lacking",                "", REG_DEFAULT, 0, 1, POSITION_SAMPLES_PER_FRAME),
         
  RegBlockTemp("The local apparent sidereal time (milliseconds)",
	       "lst",                    "", REG_DEFAULT, 0, 1, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The value of UT1-UTC (milliseconds) ",
	       "ut1utc",                 "", REG_INT,     0, 1, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The value of the equation of the equinoxes (milliseconds) ",
	       "eqneqx",                 "", REG_INT,     0, 1, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The GPS time minus the system time (provided to other systems via Network Time Protocol) "
               "at the end of the sample period (nanoseconds)",
               "time_diff",              "", REG_INT,     0, 1, POSITION_SAMPLES_PER_FRAME),
	       
  RegBlockTemp("The MJD date and time of each position",
               "utc",              "[time]", REG_UTC,     0, 1, POSITION_SAMPLES_PER_FRAME),
	       
  RegBlockTemp("The tracking mode (A DriveMode enumerator from rtcnetcoms.h) ",
	       "mode",             "[time]", REG_DEFAULT, 0, 1, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The A and B refraction terms (micro-arcseconds) and the "
	       "resulting offset in elevation (milli-arcseconds)",
	       "refraction",       "[time]", REG_INT,     0, 3, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("User-supplied offsets in {Azimuth,Elevation} "
	       "(milli-arcsec)",
	       "scan_off",               "", REG_INT,    0,  2, POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("A register that records the scan flag",
	       "scan_flag",              "", REG_UINT,   0,  1, POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("A register that records the index of the current scan offsets",
	       "scan_ind",               "", REG_UINT,   0,  1, POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("A register that records the current scan mode.",
	       "scan_mode",      "", REG_UINT|REG_UNION, 0,  1, POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("Which repetition of a multi-rep scan are we on?",
	       "scan_rep",               "", REG_UINT,   0,  1, POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("The geocentric apparent {RA (mas), Dec (mas), "
	       "Distance (micro-AU)} ",
	       "equat_geoc",       "[time]", REG_INT,    0,  3, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("User-supplied equatorial offsets {RA,Dec} (milli-arcsec) ",
	       "equat_off",        "[time]", REG_INT,    0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The geocentric apparent {Az,El} (milli-arcseconds) ",
	       "horiz_geoc",       "[time]", REG_INT,    0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The topocentric apparent {Az,El} (milli-arcseconds) ",
	       "horiz_topo",       "[time]", REG_INT,    0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The mount {Az,El} (milli-arcseconds) ",
	       "horiz_mount",      "[time]", REG_DOUBLE, 0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("User-supplied offsets in {Azimuth, Elevation} "
	       "(milli-arcsec) ",
	       "horiz_off",        "[time]", REG_DOUBLE, 0,  2, POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("Sky-based offsets {x,y} (milli-arcsec), with y directed towards "
	       "the zenith along the great circle that joins the zenith and the "
	       "(un-offset) pointing center, and x directed along the "
	       "perpendicular great circle that goes through the (un-offset) "
	       "pointing center and is perpendicular to y",
	       "sky_xy_off",  "[time]",      REG_INT,    0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The demanded move rates {Azimuth, Elevation} "
	       "(milli-counts/second) ",
	       "expectedRates",     "[time]", REG_INT,  0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The actual move rates {Azimuth, Elevation} "
	       "(milli-counts/second) ",
	       "actualRates",       "[time]", REG_INT,  0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The demanded move rates {Azimuth, Elevation} "
	       "(milli-counts/second) ",
	       "expectedCounts",     "[time]", REG_INT,  0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The actual move counts {Azimuth, Elevation} "
	       "(milli-counts/second) ",
	       "actualCounts",       "[time]", REG_INT,  0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The actual position of the telescope at the end of the sample period {Azimuth, Elevation} in milli-arcseconds",
	       "actual",      "[time]", REG_DOUBLE,      0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The actual raw elevation encoder positions of the telescope at the end of the sample period in milli-arcseconds",
	       "raw_encoder",      "[encoder][time]", REG_DOUBLE,      0,  NUM_RAW_ENCODERS, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The telescope drive motor currents",
	       "motor_current",      "[motor][time]", REG_SHORT,      0,  NUM_MOTOR_CURRENTS, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The telescope elevation drive tachometer readings",
	       "sum_tach",      "[side][time]", REG_SHORT,      0,  NUM_SUM_TACHS, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The raw telescope tilt meter readings (milliradians * 32768 / 8)",
	       "tilt_xy",      "[0 - x, 1 - y][time]", REG_SHORT,      0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The moving average of telescope tilt meter readings (milli-arcseconds)",
	       "tilt_xy_avg",      "[0 - x, 1 - y][time]", REG_INT,      0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The telescope tilt meter corrections are enabled by command",
	       "tilt_enabled",      "[time]", REG_BOOL,      0,  1, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The telescope tilt meter temperature",
	       "tilt_temp",      "[time]", REG_SHORT,      0,  1, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The expected position of the telescope at the end of the sample period "
	       "{Azimuth, Elevation} in milli-arcseconds",
	       "expected",    "[time]", REG_DOUBLE,      0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The difference between the actual and expected positions "
	       "of the telescope ",
	       "errors",      "[time]", REG_DOUBLE,      0,  2, POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("The tracking status, represented by one of the TrackingStatus "
	       "enumerators defined in gcp::util::TrackingStatus.h:<br><ul>"
	       "<li>0 -- Lacking</li>"
	       "<li>1 -- Time error</li>"
	       "<li>2 -- Updating</li>"
	       "<li>3 -- Halted</li>"
	       "<li>4 -- Slewing</li>"
	       "<li>5 -- Tracking</li>"
	       "<li>6 -- Too low</li>"
	       "<li>7 -- Too high</li></ul>",
	       "state",       "[time]", REG_UCHAR|REG_UNION,  0, 1, POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("A bit mask of the tracking status:<br><ul>"
	       "<li>Bit 0 high -- Lacking</li>"
	       "<li>Bit 1 high -- Time error</li>"
	       "<li>Bit 2 high -- Updating</li>"
	       "<li>Bit 3 high -- Halted</li>"
	       "<li>Bit 4 high -- Slewing</li>"
	       "<li>Bit 5 high -- Tracking</li>"
	       "<li>Bit 6 high -- Too low</li>"
	       "<li>Bit 7 high -- Too high</li></ul>",
	       "stateMask",   "[time]", REG_UCHAR|REG_UNION,  0, 1, POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("0 if on source. 1 if off source",
	       "offSource",  "[time]", REG_UCHAR|REG_UNION,  0, 1, POSITION_SAMPLES_PER_FRAME),
	              
};

/**
 * Create a virtual board of weather-data software registers.
 */
static RegBlockTemp cbassWeather[] = {       

  RegBlockTemp("Current weather station sample time ",
	       "utc",                 "", REG_UTC),
  
  RegBlockTemp("The air temperature around the weather station (C) ",
	       "airTemperature",      "", REG_DOUBLE|REG_PREAVG),
  
  RegBlockTemp("The relative humidity (0-1)",
	       "relativeHumidity",    "", REG_DOUBLE|REG_PREAVG),
  
  RegBlockTemp("The wind speed measured by the weather station (m/s) ",
	       "windSpeed",           "", REG_DOUBLE|REG_PREAVG),
  
  RegBlockTemp("The azimuth from which the wind is blowing (degrees)",
	       "windDirection",       "", REG_DOUBLE|REG_PREAVG),
  
  RegBlockTemp("The atmospheric pressure (millibars) ",
	       "pressure",            "", REG_DOUBLE|REG_PREAVG),
};

/**
 * Create a virtual board of power supply status registers
 */
static RegBlockTemp cbassPower[] = {       

  RegBlockTemp("Status", 
	       "status",                 "", REG_DOUBLE, 0, NUM_POWER_OUTLETS),
};


/**.......................................................................
 * Define the register map of the Servo
 */
static RegBlockTemp cbassServoOvro[] = {

  RegBlockTemp("The MJD date and time of each position",
               "utc",                     "", REG_UTC,            0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),
	       
  RegBlockTemp("",
	       "fast_az_pos",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_el_pos",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_az_err",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_el_err",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),


  RegBlockTemp("",
	       "servo_error_count",       "", REG_INT, 0, 1), 

  RegBlockTemp("",
	       "slow_az_pos",             "", REG_FLOAT, 0, 1), 

  RegBlockTemp("",
	       "slow_el_pos",             "", REG_FLOAT, 0, 1), 

  RegBlockTemp("",
	       "command_current_az1",     "", REG_FLOAT, 0, 1), 

  RegBlockTemp("",
	       "command_current_az2",     "", REG_FLOAT, 0, 1), 

  RegBlockTemp("",
	       "command_current_el1",     "", REG_FLOAT, 0, 1), 

  RegBlockTemp("",
	       "actual_current_az1",      "", REG_FLOAT, 0, 1), 

  RegBlockTemp("",
	       "actual_current_az2",      "", REG_FLOAT, 0, 1), 

  RegBlockTemp("",
	       "actual_current_el1",      "", REG_FLOAT, 0, 1), 

  RegBlockTemp("",
	       "enable_status_az1",       "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "enable_status_az2",       "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "enable_status_el1",       "", REG_BOOL, 0, 1), 

  // All Entries from the Status Register

  RegBlockTemp("",
	       "task_loop",               "", REG_INT, 0, 1), 

  RegBlockTemp("",
	       "encoder_status",          "", REG_INT, 0, 1), 

  RegBlockTemp("",
	       "el_up_soft_limit",        "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "el_down_soft_limit",      "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "el_up_hard_limit",        "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "el_down_hard_limit",      "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "az_cw_soft_limit",        "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "az_ccw_soft_limit",       "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "az_cw_hard_limit",        "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "az_ccw_hard_limit",       "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "az_no_wrap",              "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "az_brake_on",             "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "el_brake_on",             "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "emer_stop_on",            "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "simulator_running",       "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "pps_present",             "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "fast_mtr_pos",            "", REG_FAST|REG_INT, 0, 5, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_mtr_com_pos",        "", REG_FAST|REG_INT, 0, 5, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_mtr_mean_err",       "", REG_FAST|REG_INT, 0, 5, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_mtr_vel",            "", REG_FAST|REG_INT, 0, 5, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_mtr_com_vel",        "", REG_FAST|REG_INT, 0, 5, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_mtr_com_i",          "", REG_FAST|REG_INT, 0, 5, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_aux_input",          "", REG_FAST|REG_INT, 0, 6, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_new_position",       "", REG_FAST|REG_INT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),
};

/**.......................................................................
 * Define the register map of the Servo
 */
static RegBlockTemp cbassServo[] = {

  RegBlockTemp("The MJD date and time of each position",
               "utc",                     "", REG_UTC,            0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),
	       
  RegBlockTemp("",
	       "fast_az_pos",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_el_pos",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_az_err",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "fast_el_err",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("",
	       "slow_az_pos",             "", REG_FLOAT, 0, 1), 

  RegBlockTemp("",
	       "slow_el_pos",             "", REG_FLOAT, 0, 1), 

  RegBlockTemp("NTP second on the servo",
	       "ntpSecond",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("NTP usecond on the servo",
	       "ntpUSecond",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("Fast Az Tacho1",
	       "az_tacho1",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("Fast Az Tacho2",
	       "az_tacho2",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("Fast El Tacho1",
	       "el_tacho1",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),
  
  RegBlockTemp("Fast El Tacho2",
	       "el_tacho2",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("AZ Pid1",
	       "az_pid1",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("AZ Pid2",
	       "az_pid2",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("EL Pid1",
	       "el_pid1",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),

  RegBlockTemp("EL Pid2",
	       "el_pid2",             "", REG_FAST|REG_FLOAT, 0, 1, SERVO_POSITION_SAMPLES_PER_FRAME),
  // All Entries from the Status Register
  // These should be booleans, but i can't have an array of booleans apparently.

  RegBlockTemp("",
	       "servo_status",            "", REG_FLOAT, 0, 1, SERVO_NUMBER_MOTORS), 

  RegBlockTemp("",
	       "thermal_cutout",          "", REG_FLOAT, 0, 1, SERVO_NUMBER_MOTORS), 

  RegBlockTemp("",
	       "contactor",               "", REG_FLOAT, 0, 1, SERVO_NUMBER_MOTORS), 

  RegBlockTemp("",
	       "circuit_breaker",         "", REG_FLOAT, 0, 1, SERVO_NUMBER_MOTORS), 

  RegBlockTemp("",
	       "mechanical_brakes",       "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "drive_lids",              "", REG_BOOL, 0, 1), 

  RegBlockTemp("",
	       "az_no_wrap",              "", REG_FLOAT, 0, 1), 

};

/**.......................................................................
 * Define the register map of the Thermal Module and Controller
 */
static RegBlockTemp cbassThermal[] = {

  RegBlockTemp("The MJD date and time of each measurement",
               "utc",                  "", REG_FAST|REG_UTC,   0, 1, THERMAL_SAMPLES_PER_FRAME),
	       
  // The 8 temperature sensors should be probed every 5-10 seconds.

  RegBlockTemp("",
	       "lsTemperatureSensors", "", REG_FLOAT, 0, NUM_TEMP_SENSORS),

  // In the cryocon we want to read the load temperature and the current on the heater.

  RegBlockTemp("",
	       "ccTemperatureLoad",    "", REG_FAST|REG_FLOAT, 0, 1, THERMAL_SAMPLES_PER_FRAME), 

  RegBlockTemp("",
	       "ccHeaterCurrent",      "", REG_FLOAT, 0, 1),

  // Now for the Dlp usb device
  RegBlockTemp("",
	       "dlpTemperatureSensors", "", REG_FLOAT, 0, NUM_DLP_TEMP_SENSORS),

};


/**.......................................................................
 * Define the register map of the Thermal Module and Controller
 */
static RegBlockTemp cbassKoekblik[] = {

  // Now for the Dlp usb device
  RegBlockTemp("ADC values from labjack",
	       "adc", "", REG_FLOAT, 0, NUM_LABJACK_VOLTS),

};

/**.......................................................................
 * Define the register map of the Receiver/Backend/Power supply 
 */
static RegBlockTemp cbassReceiver[] = {

  // LNA biases

  RegBlockTemp("LNA Drain currents",
	       "drainCurrent", "", REG_FLOAT, 0, NUM_RECEIVER_AMPLIFIERS),
  RegBlockTemp("LNA Drain voltages",
	       "drainVoltage", "", REG_FLOAT, 0, NUM_RECEIVER_AMPLIFIERS),
  RegBlockTemp("LNA Gate voltages",
               "gateVoltage", "", REG_FLOAT, 0, NUM_RECEIVER_AMPLIFIERS),

};


/**.......................................................................
 * Define the register map of the individual roaches
 */
static RegBlockTemp cbassRoach1[] = {

  RegBlockTemp("The MJD date and time of each measurement",
               "utc",                  "", REG_FAST|REG_UTC,   0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("version number",
	       "version", "", REG_FLOAT, 0, 1, 1),

  RegBlockTemp("int count", 
	       "intCount", "", REG_FLOAT, 0, 1, 1),

  RegBlockTemp("int length", 
	       "intLength", "", REG_FLOAT, 0, 1, 1), 
  
  RegBlockTemp("Buffer Backlog", 
	       "buffBacklog", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),  

  RegBlockTemp("mode", 
	       "mode", "", REG_FLOAT, 0, 1, 1), 

  RegBlockTemp("channel number", 
	       "channel", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1), 

  RegBlockTemp("Left-Left data",
	       "LL", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Right-Right data",
	       "RR", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Q data",
	       "Q", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("U data",
	       "U", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Load 1 data",
	       "load1", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Load 2 data",
	       "load2", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Switch Status Vector",
	       "switchstatus", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Time Average Left-Left data",
	       "LLtime", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Time Average Right-Right data",
	       "RRtime", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Time Average Q data",
	       "Qtime", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Time Average U data",
	       "Utime", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Time Average Load 1 data",
	       "load1time", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Time Average Load 2 data",
	       "load2time", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Freq Average Left-Left data",
	       "LLfreq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Freq Average Right-Right data",
	       "RRfreq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Freq Average Q data",
	       "Qfreq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Freq Average U data",
	       "Ufreq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Freq Average Load 1 data",
	       "load1freq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Freq Average Load 2 data",
	       "load2freq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),
  
  RegBlockTemp("Roach NTP Seconds",
	       "ntpSeconds", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),
  
  RegBlockTemp("Roach NTP uSeconds",
	       "ntpUSeconds", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Roach FPGA Clock",
	       "fpgaClockStamp", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),
  
  
  RegBlockTemp("Roach Coefficients",
	      "roachCof", "", REG_FLOAT, 0, 32*8*2, 1), //32 channels (channels are split into odd and even) * 8 (4 total signal chains and odd/even of both) * 2 (for real/imaginary coefficients
  
};

/**.......................................................................
 * Define the register map of the individual roaches
 */
static RegBlockTemp cbassRoach2[] = {

  RegBlockTemp("The MJD date and time of each measurement",
               "utc",                  "", REG_FAST|REG_UTC,   0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("version number",
	       "version", "", REG_FLOAT, 0, 1, 1),

  RegBlockTemp("int count", 
	       "intCount", "", REG_FLOAT, 0, 1, 1),

  RegBlockTemp("int length", 
	       "intLength", "", REG_FLOAT, 0, 1, 1), 

  RegBlockTemp("Buffer Backlog", 
	       "buffBacklog", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),  
 
  RegBlockTemp("mode", 
	       "mode", "", REG_FLOAT, 0, 1, 1), 

  RegBlockTemp("channel number", 
	       "channel", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1), 

  RegBlockTemp("Left-Left data",
	       "LL", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Right-Right data",
	       "RR", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Q data",
	       "Q", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("U data",
	       "U", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Load 1 data",
	       "load1", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Load 2 data",
	       "load2", "", REG_FAST|REG_FLOAT, 0, CHANNELS_PER_ROACH, RECEIVER_SAMPLES_PER_FRAME),
  
  RegBlockTemp("Switch Status Vector",
	       "switchstatus", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),


  RegBlockTemp("Time Average Left-Left data",
	       "LLtime", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Time Average Right-Right data",
	       "RRtime", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Time Average Q data",
	       "Qtime", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Time Average U data",
	       "Utime", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Time Average Load 1 data",
	       "load1time", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Time Average Load 2 data",
	       "load2time", "", REG_FLOAT, 0, CHANNELS_PER_ROACH, 1),

  RegBlockTemp("Freq Average Left-Left data",
	       "LLfreq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Freq Average Right-Right data",
	       "RRfreq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Freq Average Q data",
	       "Qfreq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Freq Average U data",
	       "Ufreq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Freq Average Load 1 data",
	       "load1freq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Freq Average Load 2 data",
	       "load2freq", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),
  
  RegBlockTemp("Roach NTP Seconds",
	       "ntpSeconds", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),
  
  RegBlockTemp("Roach NTP uSeconds",
	       "ntpUSeconds", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),

  RegBlockTemp("Roach FPGA Clock",
	       "fpgaClockStamp", "", REG_FAST|REG_FLOAT, 0, 1, RECEIVER_SAMPLES_PER_FRAME),
  
  RegBlockTemp("Roach Coefficients",
	       "roachCof", "", REG_FLOAT, 0, 32*8*2, 1), //32 channels (channels are split into odd and even) * 8 (4 total signal chains and odd/even of both) * 2 (for real/imaginary coefficients
};

//-----------------------------------------------------------------------
// Collect the cbass per-antenna boards into an array and give them names.
//-----------------------------------------------------------------------

static RegBoardTemp cbass_antenna_boards[] = {

  {"servo",     cbassServo,     ARRAY_DIM(cbassServo),    {0x0},
   "Servo registers"},      

  {"tracker",   cbassTracker,   ARRAY_DIM(cbassTracker),  {0x0},     
   "Tracking registers"},  

  {"thermal",   cbassThermal,   ARRAY_DIM(cbassThermal),  {0x0},     
   "Thermal Module registers"},  

  {"receiver",   cbassReceiver,   ARRAY_DIM(cbassReceiver),  {0x0},     
   "Receiver registers"},

  {"koekblik", cbassKoekblik, ARRAY_DIM(cbassKoekblik), {0x0}, 
   "ADC in the Koekblik"}, 

  {"roach1",   cbassRoach1,   ARRAY_DIM(cbassRoach1),  {0x0},     
   "Data from roach 1"},

  {"roach2",   cbassRoach2,   ARRAY_DIM(cbassRoach2),  {0x0},     
   "Data from roach 2"},

  {"power",   cbassPower,   ARRAY_DIM(cbassPower),  {0x0},     
   "Ethernet Power Strip registers"},

};

/**.......................................................................
 * Create a template for a single antenna
 */

static RegTemplate cbass_antenna_template = {
  cbass_antenna_boards,   ARRAY_DIM(cbass_antenna_boards)
};

/**.......................................................................
 * Declare methods for return the templates
 */

RegTemplate* specificAntennaTemplate()
{
  return &cbass_antenna_template;
}

//-----------------------------------------------------------------------
// Template for array-specific regs
//-----------------------------------------------------------------------

/**.......................................................................
 * Collect the array-specific boards into an array and give them names.
 */

static RegBoardTemp cbass_array_boards[] = {
  {"weather",         cbassWeather,       ARRAY_DIM(cbassWeather),      {0x0},
   "Registers of the weather station"},  
};

/**.......................................................................
 * Create a template for the array.
 */
static RegTemplate cbass_array_template = {
  cbass_array_boards,   ARRAY_DIM(cbass_array_boards)
};

/**.......................................................................
 * Declare a method for return the antenna template
 */
RegTemplate* specificArrayRegMapTemplate()
{
  return &cbass_array_template;
}


//-----------------------------------------------------------------------
// Template for the entire array.

/**.......................................................................
 * Collect all cbass templates into an array and give them names.
 */
static RegTemp cbass_regtemplates[] = {
  {"array",         &cbass_array_template,          "General housekeeping"},
  {"antenna0",      &cbass_antenna_template,        "Antenna"},
};	       

/**.......................................................................
 * Create a template for the whole array
 */

static ArrayTemplate cbass_template = {
  cbass_regtemplates,   ARRAY_DIM(cbass_regtemplates)
};


/**.......................................................................
 * Declare a method for returning the array template
 */
ArrayTemplate* specificArrayTemplate()
{
  return &cbass_template;
}


