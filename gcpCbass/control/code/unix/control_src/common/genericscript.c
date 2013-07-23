#include <iostream>

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Axis.h"
#include "gcp/util/common/Collimation.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/OffsetMsg.h"
#include "gcp/util/common/PointingMode.h"
#include "gcp/util/common/PointingTelescopes.h"
#include "gcp/util/common/RegParser.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/Tracking.h"

#include "genericcontrol.h"
#include "genericscript.h"
#include "genericregs.h"
#include "genericscheduler.h"
#include "const.h"
#include "navigator.h"
#include "generictypes.h"
#include "arcfile.h"
#include "pathname.h"
#include "grabber.h"
#include "archiver.h"

using namespace gcp::control;
using namespace std;

/* Time and date inquiry functions */

static FUNC_FN(sc_date_fn);
static FUNC_FN(sc_time_fn);
static FUNC_FN(sc_today_fn);
static FUNC_FN(sc_tomorrow_fn);
static FUNC_FN(sc_after_fn);
static FUNC_FN(sc_between_fn);
static FUNC_FN(sc_elapsed_fn);
static LOOP_FN(sc_elapsed_loop_fn);
static CMD_FN(sc_update_year_cmd);
static double sc_time_of_day(Script *sc, char *caller, TimeScale type);

/* Real-time register manipulation and acquisition control commands */

static CMD_FN(sc_setreg_cmd);
static FUNC_FN(sc_regVal_fn);
static FUNC_FN(sc_intToString_fn);

/* Archive control commands */

static CMD_FN(sc_logdir_cmd);
static CMD_FN(sc_grabdir_cmd);
static CMD_FN(sc_open_cmd);
static CMD_FN(sc_flush_cmd);
static CMD_FN(sc_close_cmd);
static CMD_FN(sc_integrate_cmd);
static CMD_FN(sc_archive_cmd);
static FUNC_FN(sc_archiving_interval_fn);
static FUNC_FN(sc_archive_filtering_fn);
static CMD_FN(sc_inhibit_cmd);
static CMD_FN(sc_strobe_cmd);
static CMD_FN(sc_mark_cmd);
static CMD_FN(sc_newFrame_cmd);

/* Commands for shutting down or restarting control system components */

static CMD_FN(sc_shutdown_cmd);
static CMD_FN(sc_reboot_cmd);
static CMD_FN(sc_load_reboot_script_cmd);

/* Schedule control commands */

static CMD_FN(sc_schedule_cmd);
static CMD_FN(sc_abort_schedule_cmd);
static CMD_FN(sc_remove_schedule_cmd);
static CMD_FN(sc_advance_schedule_cmd);
static CMD_FN(sc_retard_schedule_cmd);
static CMD_FN(sc_suspend_schedule_cmd);
static CMD_FN(sc_resume_schedule_cmd);
static CMD_FN(sc_check_schedule_cmd);

/* Event management commands and functions */

static CMD_FN(sc_add_signals_cmd);
static CMD_FN(sc_signal_cmd);
static FUNC_FN(sc_iteration_fn);
static LOOP_FN(sc_iteration_loop_fn);
static FUNC_FN(sc_acquired_fn);

// Site specification commands 

static CMD_FN(sc_site_cmd);
static CMD_FN(sc_setAntennaLocation_cmd);

/* Source-catalog commands and functions */

static CMD_FN(sc_track_cmd);
static CMD_FN(sc_slew_cmd);
static CMD_FN(sc_stow_cmd);
static CMD_FN(sc_service_cmd);
static CMD_FN(sc_halt_cmd);
static CMD_FN(sc_stop_cmd);
static CMD_FN(sc_show_cmd);
static int output_almanac_time(OutputStream *output, double query_utc,
			       double event_utc);
static FUNC_FN(sc_elevation_fn);
static FUNC_FN(sc_azimuth_fn);
static CMD_FN(sc_catalog_cmd);
static CMD_FN(sc_ut1utc_cmd);
static CMD_FN(sc_horizon_cmd);

/* Scan catalog commands and functions */

static CMD_FN(sc_scan_cmd);
static CMD_FN(sc_scan_catalog_cmd);
static CMD_FN(sc_show_scan_cmd);
static FUNC_FN(sc_scan_len_fn);

// Transaction catalog 

static CMD_FN(sc_loadTransaction_cmd);
static CMD_FN(sc_logTransaction_cmd);

/* Pointing model commands */

static CMD_FN(sc_collimate_fixed_cmd);
static CMD_FN(sc_collimate_polar_cmd);
static CMD_FN(sc_encoder_cals_cmd);
static CMD_FN(sc_encoder_zeros_cmd);
static CMD_FN(sc_encoder_limits_cmd);
static CMD_FN(sc_tilts_cmd);
static CMD_FN(sc_flexure_cmd);
static CMD_FN(sc_model_cmd);
static CMD_FN(sc_offset_cmd);
static CMD_FN(sc_zeroScanOffsets_cmd);
static CMD_FN(sc_radec_offset_cmd);
static CMD_FN(sc_lst_offset_cmd);
static CMD_FN(sc_tv_offset_cmd);
static CMD_FN(sc_tv_angle_cmd);
static CMD_FN(sc_sky_offset_cmd);
static CMD_FN(sc_deck_mode_cmd);
static CMD_FN(sc_slew_rate_cmd);

/* GPIB access functions */

static CMD_FN(sc_gpib_send_cmd);
static CMD_FN(sc_gpib_read_cmd);

// Frame grabber and optical camera commands

static CMD_FN(sc_grabFrame_cmd);
static CMD_FN(sc_center_cmd);
static CMD_FN(sc_configureFrameGrabber_cmd);
static CMD_FN(sc_addSearchBox_cmd);
static CMD_FN(sc_remSearchBox_cmd);
static CMD_FN(sc_takeFlatfield_cmd);

static CMD_FN(sc_setOpticalCameraFov_cmd);
static CMD_FN(sc_setOpticalCameraAspect_cmd);
static CMD_FN(sc_setOpticalCameraCollimation_cmd);
static CMD_FN(sc_setOpticalCameraXImDir_cmd);
static CMD_FN(sc_setOpticalCameraYImDir_cmd);
static CMD_FN(sc_setDeckAngleRotationSense_cmd);
static CMD_FN(sc_setDefaultFrameGrabberChannel_cmd);
static CMD_FN(sc_setDefaultPtel_cmd);
static CMD_FN(sc_assignFrameGrabberChannel_cmd);

/*
 * Command to return the frame grabber peak offsets and staticstics about the
 * peak pixel.
 */
static FUNC_FN(sc_peak_fn);
static FUNC_FN(sc_imstat_fn);

/*
 * Command to control the auto queue mechanism
 */
static CMD_FN(sc_auto_queue_cmd);
/*
 * Commands to control the pager
 */
static CMD_FN(sc_pager_cmd);
static CMD_FN(sc_pagerEmailAddress_cmd);
static CMD_FN(sc_addPagerRegister_cmd);
static CMD_FN(sc_remPagerRegister_cmd);

//-----------------------------------------------------------------------
// General configuration commands
//-----------------------------------------------------------------------

static CMD_FN(sc_setDefaultAntennas_cmd);
static CMD_FN(sc_turnPower_cmd);
static CMD_FN(sc_configureCmdTimeout_cmd);
static CMD_FN(sc_configureDataTimeout_cmd);
static CMD_FN(sc_alias_cmd);

/* Host environment commands and functions */

static CMD_FN(sc_cd_cmd);
static CMD_FN(sc_pwd_cmd);

static Variable *add_hostname_variable(Script *sc, char *name);

static CMD_FN(sc_atmosphere_cmd);

static CMD_FN(sc_autoDoc_cmd);

#define GET_CHANNEL_MASK(channelMask) \
{\
  channelMask = gcp::grabber::Channel::NONE;\
  if(OPTION_HAS_VALUE(vchan)) {\
    channelMask |= SET_VARIABLE(vchan)->set;\
  }\
  if(OPTION_HAS_VALUE(vptel)) {\
    try {\
      gcp::util::PointingTelescopes::Ptel ptelMask;\
      ptelMask = (gcp::util::PointingTelescopes::Ptel)SET_VARIABLE(vptel)->set;\
      channelMask |= gcp::util::PointingTelescopes::getFgChannels(ptelMask);\
    } catch(gcp::util::Exception& err) {\
      lprintf(stderr, "%s\n", err.what());\
      return 1;\
    }\
  }\
  if(channelMask == gcp::grabber::Channel::NONE) {\
    try {\
      channelMask = (unsigned)gcp::util::PointingTelescopes::getDefaultFgChannels();\
    } catch(gcp::util::Exception& err) {\
      lprintf(stderr, "%s\n", err.what());\
      return 1;\
    }\
  }\
}

#define GET_SINGLE_CHANNEL(channelMask) \
{\
  channelMask = gcp::grabber::Channel::NONE;\
  if(OPTION_HAS_VALUE(vchan)) {\
    channelMask |= SET_VARIABLE(vchan)->set;\
  }\
  if(OPTION_HAS_VALUE(vptel)) {\
    try {\
      gcp::util::PointingTelescopes::Ptel ptelMask;\
      ptelMask = (gcp::util::PointingTelescopes::Ptel)SET_VARIABLE(vptel)->set;\
      channelMask |= gcp::util::PointingTelescopes::getFgChannels(ptelMask);\
    } catch(gcp::util::Exception& err) {\
      lprintf(stderr, "%s\n", err.what());\
      return 1;\
    }\
  }\
  if(channelMask != gcp::grabber::Channel::NONE) {\
    if(!gcp::grabber::Channel::isSingleChannel((gcp::grabber::Channel::FgChannel)channelMask)) {\
      lprintf(stderr, "You must specify a single channel");\
      return 1;\
    }\
  }\
}

/*.......................................................................
 * Create a new scripting environment.
 *
 * Input:
 *  cp    ControlProg *  The host control program.
 *  batch         int    True to create an environment for schedulable
 *                       scripts. False to create an environment for
 *                       single-line interactive commands.
 *  signals HashTable *  The symbol table of signals maintained by
 *                       the scheduler.
 * Output:
 *  return     Script *  The new object, or NULL on error.
 */
Script* add_GenericScriptCommands(ControlProg *cp, Script* sc, int batch, 
				  HashTable *signals)
{
  if(!sc)
    return NULL;

  // Add time & date datatypes along with time & date inquiry
  // functions.

  if(!add_IntervalDataType(sc, "Interval") ||
     !add_TimeDataType(sc,     "Time") ||
     !add_DateDataType(sc,     "Date") ||
     !add_TimeScaleDataType(sc,"TimeScale"))
    return del_Script(sc);

  if(!add_BuiltinFunction(sc,  "Date date()", sc_date_fn, 
			  "Query the current date") ||
     !add_BuiltinFunction(sc,  "Time time(TimeScale scale)", sc_time_fn, 
			  "Query the current time") ||
     !add_BuiltinFunction(sc,  "Date today()", sc_today_fn,
			  "Return today's date") ||
     !add_BuiltinFunction(sc,  "Date tomorrow()", sc_tomorrow_fn,
			  "Return tomorrow's date") ||
     !add_BuiltinFunction(sc,  "Boolean after(Time time, TimeScale scale)",
			  sc_after_fn,
			  "Returns true when the current time is later the specified time") ||
     !add_BuiltinFunction(sc,
			  "Boolean between(Time start, Time end, TimeScale scale)",
			  sc_between_fn,
			  "Returns true while the current time is between the specified times") ||
     !add_LoopStateFunction(sc, "Interval elapsed()",
			    sc_elapsed_fn, sc_elapsed_loop_fn, "Date") ||
     !add_BuiltinCommand(sc, "update_year()", sc_update_year_cmd))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add real-time register manipulation and acquisition datatypes and
  // commands.
  //------------------------------------------------------------

  if(!add_RegisterDataType(sc,    "Register", cp_ArrayMap(cp)) ||
     !add_UintDataType(sc,        "RegValue", 0, sc_iterate_uint, "Integer") ||
     !add_BoardDataType(sc,       "Board", cp_ArrayMap(cp)) ||
     !add_BitMaskDataType(sc,     "BitMask") ||
     !add_BitMaskOperDataType(sc, "BitMaskOper"))
    return del_Script(sc);

  if(!add_BuiltinCommand(sc,  "setreg(Register reg, RegValue val)",
			 sc_setreg_cmd) ||
     !add_StringDataType(sc, "RegSpec", 0, 0) ||
     !add_BuiltinFunction(sc, "Double regVal(RegSpec reg)", sc_regVal_fn,
			  "Return an uncalibrated register value as a Double") ||
     !add_BuiltinFunction(sc, "String intToString(Integer int)", sc_intToString_fn, 
			  "Convert an Integer to a String"))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add archive control datatypes and commands.
  //------------------------------------------------------------

  if(!add_WdirDataType(sc,          "Wdir") ||
     !add_IntTimeDataType(sc,       "IntTime") ||
     !add_UintDataType(sc,          "Count", 0, sc_iterate_uint, "Integer") ||
     !add_FeaturesDataType(sc,      "Features") ||
     !add_ArcFileDataType(sc,       "ArcFile") ||
     !add_FeatureChangeDataType(sc, "FeatureChange"))
    return del_Script(sc);

  if(!add_BuiltinCommand(sc, "logdir(Wdir dir)", 
			 sc_logdir_cmd,
			 "Set the directory for logfiles") ||
     !add_BuiltinCommand(sc, "grabdir(Wdir dir)", 
			 sc_grabdir_cmd,
			 "Set the directory in which grabber files will be written") ||
     !add_BuiltinCommand(sc, "open(ArcFile file, [Wdir dir])", 
			 sc_open_cmd,
			 "Start archiving/logging/recording frame grabbber files") ||
     !add_BuiltinCommand(sc, "flush(ArcFile file)", 
			 sc_flush_cmd) ||
     !add_BuiltinCommand(sc, "close(ArcFile file)", 
			 sc_close_cmd) ||
     //     !add_BuiltinCommand(sc, "integrate(IntTime exponent)", 
     //			 sc_integrate_cmd) ||
     !add_BuiltinCommand(sc, "archive([Count combine, Wdir dir, Boolean filter, "
			 "Count file_size])",
			 sc_archive_cmd,
			 "Configure the archiver") ||
     !add_BuiltinFunction(sc, "Count archiving_interval()",
			  sc_archiving_interval_fn) ||
     !add_BuiltinFunction(sc, "Boolean archive_filtering()",
			  sc_archive_filtering_fn) ||
     //     !add_BuiltinCommand(sc, "inhibit(Boolean state)", 
     //			 sc_inhibit_cmd) ||
     //     !add_BuiltinCommand(sc, "strobe()", 
     //			 sc_strobe_cmd) ||
     !add_BuiltinCommand(sc, "mark(FeatureChange what, Features features)",
			 sc_mark_cmd,
			 "Configure the feature bits to be set high in the features register") ||
     !add_BuiltinCommand(sc, "newFrame()", 
			 sc_newFrame_cmd))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add Antenna datatype
  //------------------------------------------------------------
  
  if(!add_AntennasDataType(sc, "Antennas"))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Create commands involved with shutting down or restarting the
  // control system.
  //------------------------------------------------------------


  if(!add_ScriptDataType(sc,  "Script") ||
     !add_SysTypeDataType(sc, "SysType") ||
     !add_BuiltinCommand(sc,  "load_reboot_script(Script script)",
			 sc_load_reboot_script_cmd))

    return del_Script(sc);

#if 0  
  if(!add_BuiltinCommand(sc,  "shutdown(SysType type)", 
			 sc_shutdown_cmd) ||
     !add_BuiltinCommand(sc,  "reboot(SysType type, [Antennas ant])",   
			 sc_reboot_cmd))
    return del_Script(sc);
#endif
  
  //------------------------------------------------------------
  // Add schedule control commands.
  //------------------------------------------------------------

  if(!add_UintDataType(sc,   "QueueEntry", 0, sc_iterate_uint, "Integer") ||
     !add_BuiltinCommand(sc, "schedule(Script script, [QueueEntry n])", 
			 sc_schedule_cmd,
			 "Add a schedule file to the schedule queue") ||
     !add_BuiltinCommand(sc, "abort_schedule()", 
			 sc_abort_schedule_cmd,
			 "Abort the current schedule") ||
     !add_BuiltinCommand(sc, "remove_schedule(QueueEntry n)",
			 sc_remove_schedule_cmd,
			 "Remove the requested schedule") ||
     !add_BuiltinCommand(sc, "advance_schedule(QueueEntry n, Count dn)",
			 sc_advance_schedule_cmd,
			 "Move the requested schedule ahead by dn counts") ||
     !add_BuiltinCommand(sc, "retard_schedule(QueueEntry n, Count dn)",
			 sc_retard_schedule_cmd,
			 "Move the requested schedule back by dn counts") ||
     !add_BuiltinCommand(sc, "suspend_schedule()", 
			 sc_suspend_schedule_cmd,
			 "Suspend execution of the current schedule") ||
     !add_BuiltinCommand(sc, "resume_schedule()", 
			 sc_resume_schedule_cmd,
			 "Resume execution of the current schedule") ||
     !add_BuiltinCommand(sc, "check_schedule(Script script)",
			 sc_check_schedule_cmd,
			 "Check the syntax of a schedule"))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Event management functions.
  //------------------------------------------------------------
  
  if(!add_KeywordDataType(sc,        "Keyword", 1, 0) ||
     !add_BuiltinCommand(sc, "add_signals(listof Keyword keys)",
			 sc_add_signals_cmd,
			 "Add signals to the list of recognized signals") ||
     !add_BuiltinCommand(sc, "signal/op=send|clear|init(Signal signal)",
			 sc_signal_cmd,
			 "Send a signal to the currently running schedule") ||
     !add_LoopStateFunction(sc, "Count iteration()",
			    sc_iteration_fn, sc_iteration_loop_fn, "Count"))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add site-specification datatypes and commands.
  //------------------------------------------------------------
  
  if(!add_LatitudeDataType(sc,  "Latitude") ||
     !add_LongitudeDataType(sc, "Longitude") ||
     !add_AltitudeDataType(sc,  "Altitude"))
    del_Script(sc);

  if(!add_BuiltinCommand(sc, "site(Longitude longitude, Latitude latitude, Altitude altitude)",
			 sc_site_cmd,
			 "Set the location of the site") ||
     !add_BuiltinCommand(sc, "setAntennaLocation(Double up, Double east, "
			 "Double north, [Antennas ant])", 
			 sc_setAntennaLocation_cmd,
			 "Set the location of an antenna relative to the site location"))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add the commands, functions and datatypes used for interaction
  // with the navigator-thread.
  //------------------------------------------------------------
  
  if(!add_AzimuthDataType(sc,   "Azimuth") ||
     !add_DeckAngleDataType(sc, "DeckAngle") ||
     !add_ElevationDataType(sc, "Elevation") ||
     !add_SourceDataType(sc,    "Source") ||
     !add_ScanDataType(sc,      "Scan") ||
     !add_TrackingDataType(sc,  "Tracking"))
    return del_Script(sc);

  if(!add_BuiltinCommand(sc, "track(Source source, "
			 "[Tracking type, Antennas ant])", 
			 sc_track_cmd,
			 "Track a source") ||

     !add_BuiltinCommand(sc, "slew([Azimuth az, Elevation el, DeckAngle dk, "
			 "Antennas ant])", 
			 sc_slew_cmd,
			 "Slew the telescope to a given fixed position") ||

     !add_BuiltinCommand(sc, "stow([Antennas ant])", 
			 sc_stow_cmd,
			 "Send the telescope to the stow position") ||

     !add_BuiltinCommand(sc, "service([Antennas ant])", 
			 sc_service_cmd,
			 "Send the telescope to the service position") ||

     !add_BuiltinCommand(sc, "halt([Antennas ant])", 
			 sc_halt_cmd,
			 "Halt the telescope at the current position") ||

     !add_BuiltinCommand(sc, "stop([Antennas ant])", 
			 sc_stop_cmd,
			 "Enter STOP mode (ACU engineering command)") ||

     !add_BuiltinCommand(sc, "show(Source source, "
			 "[Tracking type, Antennas ant, Date utc, "
			 "Elevation horizon])", sc_show_cmd) ||

     !add_BuiltinFunction(sc, "Elevation elevation(Source source)",
			  sc_elevation_fn) ||

     !add_BuiltinFunction(sc, "Azimuth azimuth(Source source)",
			  sc_azimuth_fn) ||

     !add_BuiltinCommand(sc, "catalog(InputFile filename)", sc_catalog_cmd,
			 "Load a source catalog") ||

     !add_BuiltinCommand(sc, "ut1utc(InputFile filename)", sc_ut1utc_cmd,
			 "Load a UT1 - UTC ephemeris file") ||

     !add_BuiltinCommand(sc, "horizon(Elevation angle)", sc_horizon_cmd,
			 "Set the horizon to be used when displaying source "
			 "information with the 'show()' command") ||

     !add_BuiltinCommand(sc, "scan/add(Scan scan, [Integer nreps])", sc_scan_cmd,
			 "Start a scan (can only be used while tracking)") ||

     !add_BuiltinCommand(sc, "scan_catalog(InputFile filename)", 
			 sc_scan_catalog_cmd,
			 "Load a catalog of scans") ||

     !add_BuiltinCommand(sc, "show_scan(Scan scan)", sc_show_scan_cmd,
			 "Display information about a named scan") ||
    
     !add_BuiltinFunction(sc, "Time scan_len(Scan scan, [Integer nreps])", 
			  sc_scan_len_fn,
			  "Return information about a named scan"))
    
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Transaction catalog
  //------------------------------------------------------------
  
  if(!add_TransDevDataType(sc,      "TransDev") ||
     !add_TransLocationDataType(sc, "TransLocation") ||
     !add_TransSerialDataType(sc,   "TransSerial") ||
     !add_TransSerialDataType(sc,   "TransWho") ||
     !add_TransSerialDataType(sc,   "TransComment"))
    del_Script(sc);

  if(!add_BuiltinCommand(sc, "loadTransaction(InputFile filename, "
			 "[Boolean clear])", sc_loadTransaction_cmd) ||
     !add_BuiltinCommand(sc, "logTransaction(TransDev device, "
			 "TransSerial serial, TransLocation location, "
			 "Date date, TransWho who, [TransComment comment])",
			 sc_logTransaction_cmd))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add pointing-model datatypes and commands.
  //------------------------------------------------------------
  
  if(!add_PointingOffsetDataType(sc, "PointingOffset") ||
     !add_ModelDataType(sc,          "Model") ||
     !add_IntDataType(sc,            "EncoderCount", 0, sc_iterate_int, "Integer", 1) ||
     !add_FlexureDataType(sc,        "Flexure") ||
     !add_TiltDataType(sc,           "Tilt") ||
     !add_SlewRateDataType(sc,       "SlewRate") ||
     !add_PointingTelescopesDataType(sc, "PointingTelescopes") ||
     !add_DeckModeDataType(sc,       "DeckMode"))
    del_Script(sc);

  if(!add_BuiltinCommand(sc, "collimate_polar(Model model, Tilt size, "
			 "PointingOffset dir, [Antennas ant])", 
			 sc_collimate_polar_cmd,
			 "Set a collimation term in polar coordinates") ||

     !add_BuiltinCommand(sc, "collimate_fixed/add(Model model, "
			 "PointingOffset x, PointingOffset y, "
			 "[PointingTelescopes ptel, Antennas ant])", 
			 sc_collimate_fixed_cmd,
			 "Set a collimation term in great circle offsets") ||

     !add_BuiltinCommand(sc, "encoder_cals(EncoderCount az_turn, "
			 "EncoderCount el_turn, EncoderCount dk_turn,"
			 " [Antennas ant])", 
			 sc_encoder_cals_cmd,
			 "Set the calibration factors (counts per turn) for the"
			 "drive axis encoders") ||

     !add_BuiltinCommand(sc, "encoder_zeros(PointingOffset az, "
			 "PointingOffset el, PointingOffset dk, "
			 "[Antennas ant])", 
			 sc_encoder_zeros_cmd,
			 "Set the zero points of the drive axis encoders") ||

     !add_BuiltinCommand(sc, "encoder_limits(EncoderCount az_min,"
			 "EncoderCount az_max, EncoderCount el_min,"
			 "EncoderCount el_max, EncoderCount dk_min,"
			 "EncoderCount dk_max, [Antennas ant])", 
			 sc_encoder_limits_cmd,
			 "Set the positions beyond which the telescope will "
			 "not be commanded") ||

     !add_BuiltinCommand(sc, "tilts(Tilt ha, Tilt lat, Tilt el,"
			 "[Antennas ant])", 
			 sc_tilts_cmd,
			 "Set the pointing model tilt terms") ||

     !add_BuiltinCommand(sc, "flexure(Model model, Flexure sinEl,Flexure cosEl,"
			 "[PointingTelescopes ptel, Antennas ant])", 
			 sc_flexure_cmd,
			 "Set the telescope pointing model flexure terms") ||

     !add_BuiltinCommand(sc, "model(Model model, [PointingTelescopes ptel])", 
			 sc_model_cmd,
			 "Select which pointing model to use (radio or "
			 "optical). Note that there can now be multiple "
			 "optical models, corresponding to the different "
			 "pointing telescopes. When you select an optical "
			 "model, you must now also specify a ptel") ||

     !add_BuiltinCommand(sc, "offset/add([PointingOffset az, PointingOffset el,"
			 "DeckAngle dk, Antennas ant])",
			 sc_offset_cmd,
			 "Set or add a mount pointing offset.") ||

     !add_BuiltinCommand(sc, "zeroScanOffsets()",
			 sc_zeroScanOffsets_cmd, 
			 "Reset any legacy scan offsets to zero") ||

     !add_BuiltinCommand(sc, "radec_offset/add([PointingOffset ra, "
			 "PointingOffset dec, Time dt, Antennas ant])",
			 sc_radec_offset_cmd,
			 "Set or add a pointing offset in RA/DEC coordinates") ||

     !add_BuiltinCommand(sc, "tv_offset(PointingOffset right, PointingOffset up,"
			 "[Antennas ant])",
			 sc_tv_offset_cmd,
			 "Set an offset derived from the frame grabber") ||

     !add_BuiltinCommand(sc, "tv_angle(PointingOffset angle, [Antennas ant])",
			 sc_tv_angle_cmd) ||

     !add_BuiltinCommand(sc, "sky_offset/add([PointingOffset x, PointingOffset y,"
			 "Antennas ant])",
			 sc_sky_offset_cmd,
			 "Set or add a pointing offset along orthogonal great "
			 "circles passing through the current pointing "
			 "position") ||

     !add_BuiltinCommand(sc, "deck_mode(DeckMode mode, [Antennas ant])",
			 sc_deck_mode_cmd) ||

     !add_BuiltinCommand(sc, "slew_rate([SlewRate az, SlewRate el, "
			 "SlewRate dk, Antennas ant])", 
			 sc_slew_rate_cmd))

    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add frame grabber datatypes and commands
  //------------------------------------------------------------
  
  if(!add_PeakDataType(sc,          "Peak") ||
     !add_ImstatDataType(sc,        "Imstat") ||
     !add_FlatfieldModeDataType(sc, "FlatfieldMode") ||
     !add_SwitchStateDataType(sc,   "SwitchState") ||
     !add_FgChannelDataType(sc,     "FgChannel") ||

     !add_BuiltinCommand(sc, "grabFrame([FgChannel chan, "
			 "PointingTelescopes ptel])", 
			 sc_grabFrame_cmd,
			 "Acquire a frame from the frame grabber") ||

     !add_BuiltinCommand(sc, "center([FgChannel chan, "
			 "PointingTelescopes ptel])",
			 sc_center_cmd,
			 "Find the centroid of an image acquired from the "
			 "frame grabber") ||

     !add_BuiltinCommand(sc, "takeFlatfield([FgChannel chan, "
			 "PointingTelescopes ptel])",  
			 sc_takeFlatfield_cmd,
			 "Acquire a frame to be used as a flatfield image") ||

     !add_BuiltinCommand(sc, "configureFrameGrabber([Integer combine, "
			 "FlatfieldMode flatfield, Interval interval, "
			 "FgChannel chan, PointingTelescopes ptel])",
			 sc_configureFrameGrabber_cmd,
			 "Configure the frame grabber "
			 "(parameters can be configured separately for each "
			 "channel)") ||

     !add_BuiltinCommand(sc, "addSearchBox(Integer iXmin, Integer iYmin, "
			 "Integer iXmax, Integer iYmax, Boolean include, "
			 "[FgChannel chan, PointingTelescopes ptel])", 
			 sc_addSearchBox_cmd,
			 "Define a rectangular region (in pixel units) to be "
			 "included/excluded when searching a frame grabber "
			 "image for a peak. (parameters can be configured "
			 "separately for each channel)") ||

     !add_BuiltinCommand(sc, "remSearchBox([Integer x, Integer y, "
			 "FgChannel chan, PointingTelescopes ptel])", 
			 sc_remSearchBox_cmd,
			 "Delete the search box nearest the specified (pixel) "
			 "location. If no pixel is specified, all boxes will "
			 "be deleted.  Parameters can be configured separately "
			 "for each channel") ||

     !add_BuiltinCommand(sc, "setOpticalCameraFov([Double fov, FgChannel chan, "
			 "PointingTelescopes ptel])",  
			 sc_setOpticalCameraFov_cmd,
			 "Set the field of view of the optical camera") ||

     !add_BuiltinCommand(sc, "setOpticalCameraAspect([Double aspect, "
			 "FgChannel chan, PointingTelescopes ptel])",  
			 sc_setOpticalCameraAspect_cmd,
			 "Set the aspect ratio (y/x) of the optical camera") ||

     !add_BuiltinCommand(sc, "setOpticalCameraRotation([PointingOffset angle, "
			 "FgChannel chan, PointingTelescopes ptel])",  
			 sc_setOpticalCameraCollimation_cmd,
			 "Set the angle by which the optical camera is rotated "
			 "with respect to the horizon") ||

     !add_ImDirDataType(sc, "ImDir") ||

     !add_RotSenseDataType(sc, "RotSense") ||

     !add_BuiltinCommand(sc, "setOpticalCameraXImDir(ImDir dir, "
			 "[FgChannel chan, PointingTelescopes ptel])",  
			 sc_setOpticalCameraXImDir_cmd,
			 "Specify whether images from the frame grabber have "
			 "their x-axis inverted") ||

     !add_BuiltinCommand(sc, "setOpticalCameraYImDir(ImDir dir, "
			 "[FgChannel chan, PointingTelescopes ptel])",  
			 sc_setOpticalCameraYImDir_cmd,
			 "Specify whether images from the frame grabber have "
			 "their y-axis inverted") ||

     !add_BuiltinCommand(sc, "setDeckAngleRotationSense(RotSense sense, "
			 "[FgChannel chan, PointingTelescopes ptel])", 
			 sc_setDeckAngleRotationSense_cmd,
			 "For triaxial telescopes, specify whether a positive "
			 "deck angle corresponds to a clockwise, or "
			 "counter-clockwise roatation of the deck, looking out "
			 "along the rotation axis") ||

     !add_BuiltinFunction(sc, "PointingOffset peak(Peak offset, "
			  "[FgChannel chan, PointingTelescopes ptel])", 
			  sc_peak_fn,
			  "Return the pointing offset of the peak of the last "
			  "frame grabber image") ||

     !add_BuiltinFunction(sc, "Double imstat(Imstat stat, [FgChannel chan, "
			  "PointingTelescopes ptel])", 
			  sc_imstat_fn,
			  "Return statistics of the last grabber image") ||
     
     !add_BuiltinCommand(sc, "setDefaultFrameGrabberChannel(FgChannel chan)", 
			 sc_setDefaultFrameGrabberChannel_cmd,
			 "Set the default frame grabber channel mask to be used"
			 " with commands that take an FgChannel argument."
			 " Note that this overrides the "
			 "setDefaultPtel() command") ||

     !add_BuiltinCommand(sc, "setDefaultPtel(PointingTelescopes ptel)", 
			 sc_setDefaultPtel_cmd,
			 "Set the default set of pointing telescopes to be used"
			 " with commands that take a Ptel argument."
			 " Note that this overrides the "
			 "setDefaultFrameGrabberChannel() command") ||

     !add_BuiltinCommand(sc, "assignFrameGrabberChannel(FgChannel chan, "
			 "PointingTelescopes ptel)", 
			 sc_assignFrameGrabberChannel_cmd,
			 "Associates a frame grabber channel with a particular"
			 " pointing telescope.  This association will be used"
			 " when a pointing telescope mask is specified instead"
			 " of a set of channels in commands that "
			 "accept either."))
    
     return del_Script(sc);
  
  //------------------------------------------------------------
  // Add command for auto_queueing
  //------------------------------------------------------------
  
  if(!add_BuiltinCommand(sc, "auto_queue([Wdir dir, SwitchState state, "
			 "Interval dt])",
			 sc_auto_queue_cmd,
			 "Configure the auto queue mechanism."))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Host environment commands and functions.
  //------------------------------------------------------------
  
  if(!add_DirDataType(sc,       "Dir") ||
     !add_StringDataType(sc,    "Hostname", 0, 0) ||
     !add_BuiltinCommand(sc,    "cd(Dir path)", sc_cd_cmd) ||
     !add_BuiltinCommand(sc,    "pwd()", sc_pwd_cmd) ||
     !add_hostname_variable(sc, "hostname"))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add commands for paging
  //------------------------------------------------------------
  
  if(!add_PagerStateDataType(sc,  "PagerState")    ||
     !add_StringDataType(sc,      "Ip", 0, 0)      ||
     !add_StringDataType(sc,      "Email", 0, 0)   ||
     !add_EmailActionDataType(sc, "EmailAction")   ||
     !add_StringDataType(sc,      "RegName", 0, 0) ||
     !add_PagerDevDataType(sc,    "PagerDev")      ||

     !add_BuiltinCommand(sc, "pager(PagerState state, [Ip ip, "
			 "PagerDev dev, RegName register, Ip host])", 
			 sc_pager_cmd, 
			 "Control the pager") || 

     !add_BuiltinCommand(sc, "addPagerRegister(RegName register, Double min, "
			 "Double max, [Integer nFrame, Boolean delta, "
			 "Boolean outOfRange, String script])",
			 sc_addPagerRegister_cmd,
			 "Add a register to the set of registers that can"
			 "activate the pager") ||

     !add_BuiltinCommand(sc, "remPagerRegister(RegName register)", 
			 sc_remPagerRegister_cmd, 
			 "Remove a register condition on which to activate "
			 "the pager"),

     !add_BuiltinCommand(sc, "pagerEmailAddress(EmailAction action, "
			 "[Email email])",
			 sc_pagerEmailAddress_cmd,
			 "Configure the list of email addresses to be notified"
			 "when the pager is activated"))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add general configuration commands
  //------------------------------------------------------------
  
  if(!add_BuiltinCommand(sc, "setDefaultAntennas(Antennas ant)",
			 sc_setDefaultAntennas_cmd,
			 "For multi-antenna arrays, this specifies the "
			 "default set of antennas to which a command applies. "
			 "For single-antenna systems, this defaults to ant0 "
			 "and should not be modified."))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add general hardware commands
  //------------------------------------------------------------
  
  if(!add_AttenuationDataType(sc, "Attenuation") ||
     !add_BuiltinCommand(sc, "turnPower(SwitchState state, [Integer outlet])",
			 sc_turnPower_cmd))
    return del_Script(sc);
  
  //------------------------------------------------------------
  // Add atmosphere commands
  //------------------------------------------------------------

  if(!add_BuiltinCommand(sc, "atmosphere(Double temperature, "
			 "Double humidity, Double pressure, "
			 "[Antennas ant])", 
			 sc_atmosphere_cmd,
			 "A manual way of setting atmospheric parameters."
			 "During normal operation of the control system, "
			 "these are received from the weather station and "
			 "this command should not be used."))
    return del_Script(sc);     

  //------------------------------------------------------------
  // Add autodocumentation command
  //------------------------------------------------------------

  if(!add_BuiltinCommand(sc, "autoDoc([Wdir dir])", 
			 sc_autoDoc_cmd,
			 "Automatically generate documentation from the "
			 "control code.  This generates the command help"
			 " available from the viewer 'Help' menu."))
    return del_Script(sc);     

  // Add the command deadman configuration command

  if(!add_BuiltinCommand(sc, "configureCmdTimeout([SwitchState state, "
			 "Interval interval])", 
			 sc_configureCmdTimeout_cmd,
			 "Set a timeout after which the pager will be "
			 "activated if no commands are sent by the control "
			 "system.  This is to protect against pathological "
			 "scripts that hang during execution (for example, "
			 "if waiting to acquire a source that has "
			 "already set)"))
    return del_Script(sc);

  // Add the data deadman configuration command

  if(!add_BuiltinCommand(sc, "configureDataTimeout([SwitchState state, "
			 "Interval interval])", 
			 sc_configureDataTimeout_cmd,
			 "Set a timeout after which the pager will be "
			 "activated if no data are received by the control "
			 "system. This is to protect against pathological "
			 "hangups in the mediator"))
    return del_Script(sc);

  // Add the alias command

  if(!add_BuiltinCommand(sc, "alias(String command, String alias)",
			 sc_alias_cmd,
			 "Alias a command"))
    return del_Script(sc);

  return sc;
}

/*-----------------------------------------------------------------------*
 * Time and date inquiry functions                                       *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Implement a function that returns the current date and time (UTC).
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          (none)
 * Input/Output:
 *  result     Variable *  The return Date value.
 *  state      Variable *  Unused.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_date_fn)
{
  double utc;            /* The current UTC as a Modified Julian Date */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Get the current UTC as a modified Julian date.
   */
  utc = current_mjd_utc();
  if(utc < 0.0)
    return 1;
  /*
   * Return the date and time.
   */
  DOUBLE_VARIABLE(result)->d = utc;
  return 0;
}

/*.......................................................................
 * Implement a function that returns the current time of day for a given
 * timescale.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          scale - One of the following timescales:
 *                                   utc - Universal Coordinated time.
 *                                   lst - Local Sidereal Time.
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_time_fn)
{
  Variable *vscale;       /* The target timescale */
  double hours;           /* The current time of day (hours) */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vscale, NULL))
    return 1;
  /*
   * Get the current time of day in the specified time-scale.
   */
  hours = sc_time_of_day(sc, "$time()", 
			 (TimeScale)CHOICE_VARIABLE(vscale)->choice);
  if(hours < 0)
    return 1;
  /*
   * Get the time of day in hours and record it for return.
   */
  DOUBLE_VARIABLE(result)->d = hours;
  return 0;
}

/*.......................................................................
 * Implement a function that returns the current date (UTC).
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          (none)
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_today_fn)
{
  double utc;            /* The current UTC as a Modified Julian Date */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Get the current UTC as a modified Julian date.
   */
  utc = current_mjd_utc();
  if(utc < 0.0)
    return 1;
  /*
   * Round the returned date to the start of the current day.
   */
  DOUBLE_VARIABLE(result)->d = floor(utc);
  return 0;
}

/*.......................................................................
 * Implement a function that returns tomorrow's date (UTC).
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          (none)
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_tomorrow_fn)
{
  double utc;            /* The current UTC as a Modified Julian Date */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Get the current UTC as a modified Julian date.
   */
  utc = current_mjd_utc();
  if(utc < 0.0)
    return 1;
  /*
   * Round the returned date up to the start of the next day.
   */
  DOUBLE_VARIABLE(result)->d = ceil(utc);
  return 0;
}

/*.......................................................................
 * Implement a function that returns true if the current time of day, in
 * a given timescale, is later than a given value. Note that since time
 * wraps around on itself every 24 hours, we define later to mean within
 * the 12 hours following the specified time of day.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          time  -  The time of day to compare against.
 *                          scale -  The time-scale to use, from:
 *                                    utc  - Universal Coordinated Time.
 *                                    lst  - Local Sidereal Time.
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_after_fn)
{
  Variable *vtime;               /* The time-of-day argument */
  Variable *vscale;              /* The time-scale argument */
  double hours;                  /* The current time of day (hours) */
  double dt;                     /* The difference between the times */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vtime, &vscale, NULL))
    return 1;
  /*
   * Get the current time of day in the specified time-scale.
   */
  hours = sc_time_of_day(sc, "$after()", 
			 (TimeScale)CHOICE_VARIABLE(vscale)->choice);
  if(hours < 0)
    return 1;
  /*
   * Get the time difference modulo 24 hours.
   */
  dt = hours - DOUBLE_VARIABLE(vtime)->d;
  if(dt < 0)
    dt += 24.0;
  /*
   * See if the time is within the 12 hours following the specified time.
   */
  BOOL_VARIABLE(result)->boolvar = dt < 12.0;
  return 0;
}

/*.......................................................................
 * Implement a function that returns true if the current time of day,
 * on a given timescale, is between two given times of day. If the second
 * time is at a numerically earlier time of day than the first, it is
 * interpretted as belonging to the following day.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          start -  The start time of the window.
 *                          end   -  The end time of the window.
 *                          scale -  The timescale of the start and end
 *                                   times, chosen from:
 *                                     utc  - Universal Coordinated Time.
 *                                     lst  - Local Sidereal Time.
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_between_fn)
{
  Variable *va, *vb;   /* The arguments that hold the start and end times */
  Variable *vscale;    /* The argument that contains the timescale */
  double hours;        /* The current time of day (hours) */
  double ta, tb;       /* The two time-of-days to compare to hours */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &va, &vb, &vscale, NULL))
    return 1;
  /*
   * Get the current time of day in the specified time-scale.
   */
  hours = sc_time_of_day(sc, "$between()", 
			 (TimeScale)CHOICE_VARIABLE(vscale)->choice);
  if(hours < 0)
    return 1;
  /*
   * Get the limiting times of the window.
   */
  ta = DOUBLE_VARIABLE(va)->d;
  tb = DOUBLE_VARIABLE(vb)->d;
  /*
   * See if the current time is within the specified window.
   *
   * If the second time is numerically later in the day, interpret
   * the times as being in the same day. Otherwise interpret the second
   * as being from the following day.
   */
  if(tb > ta) {
    BOOL_VARIABLE(result)->boolvar = hours > ta && hours < tb;
  } else {
    BOOL_VARIABLE(result)->boolvar = hours > ta || hours < tb;
  };
  return 0;
}

/*.......................................................................
 * Implement a loop-aware function that returns the amount of time that
 * its containing loop has been executing.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          (none)
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  The loop-state object that contains the
 *                         start time of the loop as a MJD utc.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_elapsed_fn)
{
  double utc;                 /* The current utc expressed as a MJD */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Get the current UTC as a modified Julian date.
   */
  utc = current_mjd_utc();
  if(utc < 0.0)
    return 1;
  /*
   * Return the time that has elapsed so far (in seconds).
   */
  DOUBLE_VARIABLE(result)->d = (utc - DOUBLE_VARIABLE(state)->d) * 86400.0;
  return 0;
}

/*.......................................................................
 * This is the loop-state iterator function of the elapsed function.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  state      Variable *  The loop-state variable to be initialized.
 *                         No iteration of its value is needed.
 *  oper       LoopOper    The type of operation to perform on *state.
 */
static LOOP_FN(sc_elapsed_loop_fn)
{
  /*
   * On entry to the enclosing loop, initialize the state variable
   * with the current UTC, expressed as a MJD.
   */
  switch(oper) {
  case LOOP_ENTER:
    DOUBLE_VARIABLE(state)->d = current_mjd_utc();
    break;
  case LOOP_INCR:
  case LOOP_EXIT:
    break;
  };
}

/*.......................................................................
 * Implement a loop-aware function that returns the iteration count of
 * its enclosing loop or until statement.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          (none)
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  The loop-state object that contains the
 *                         start time of the loop as a MJD utc.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_iteration_fn)
{
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Return the time that has elapsed so far.
   */
  UINT_VARIABLE(result)->uint = UINT_VARIABLE(state)->uint;
  return 0;
}

/*.......................................................................
 * This is the loop-state iterator function of the iteration() function.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  state      Variable *  The loop-state variable to be initialized.
 *                         This countains the iteration count to be
 *                         incremented.
 *  oper       LoopOper    The type of operation to perform on *state.
 */
static LOOP_FN(sc_iteration_loop_fn)
{
  switch(oper) {
  case LOOP_ENTER:   /* Initialize the iteration count on loop entry */
    UINT_VARIABLE(state)->uint = 0;
    break;
  case LOOP_INCR:    /* Increment the iteration count */
    UINT_VARIABLE(state)->uint++;
  case LOOP_EXIT:
    break;
  };
}

/*.......................................................................
 * Implement the command that sends the year to the control system.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments: (none).
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_update_year_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  Date date;                     /* The current date (utc) */
  
  // Do we have a controller to send the command to?
  
  if(rtc_offline(sc, "update_year"))
    return 1;
  
  // Get the command-line arguments.
  
  if(get_Arguments(args, NULL))
    return 1;
  
  // Deduce the current utc date.
  
  if(current_date(&date))
    return 1;
  
  // Compose the real-time controller network command.
  
  rtc.cmd.year.year = date.year;
  
  // Send the command to the real-time controller.
  if(queue_rtc_command(cp, &rtc, NET_YEAR_CMD))
    return 1;
  
  return 0;
}

/*.......................................................................
 * This is a utility function to return the current time of day in a given
 * format.
 *
 * Input:
 *  sc       Script *   The host scripting environment.
 *  caller     char *   An identification prefix for error messages.
 *  type  TimeScale     The time-system to return.
 */
static double sc_time_of_day(Script *sc, char *caller, TimeScale type)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Date utc;                      /* The current date and time (utc) */
  /*
   * Get the current date and time (utc).
   */
  if(current_date(&utc))
    return -1.0;
  /*
   * Get the corresponding time of day on the specified time-scale.
   */
  switch(type) {
  case TIME_UTC:
    return date_to_time_of_day(&utc);
    break;
  case TIME_LST:
    {
      Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
      return date_to_lst(&utc, sch_Site(sch), 0.0, 0.0) * rtoh;
    };
    break;
  };
  lprintf(stderr, "%s: Unknown time system.\n", caller);
  return -1.0;
}

/*-----------------------------------------------------------------------*
 * Real-time register manipulation and acquisition control commands      *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Implement the command that sets a specified SZA register to a specified
 * value.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         reg  -  The register specification.
 *                         val  -  The register value.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_setreg_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The context of the
						   host control
						   program */
  Variable *vreg;   /* The register specification argument */
  Variable *vval;   /* The register value argument */
  RegisterVariable *regvar;  /* The derived type of vreg */
  RtcNetCmd rtc;    /* The network command object to be sent to the */
                    /*  real-time controller task */
  /*
   * Do we have a controller to send the command to?
   */
  if(rtc_offline(sc, "setreg"))
    return 1;
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vreg, &vval, NULL))
    return 1;
  regvar = REGISTER_VARIABLE(vreg);
  /*
   * Compose a network object to be sent to the real-time controller task.
   */
  rtc.cmd.setreg.value = UINT_VARIABLE(vval)->uint;
  rtc.cmd.setreg.board = regvar->board;
  rtc.cmd.setreg.block = regvar->block;
  rtc.cmd.setreg.index = regvar->index;
  rtc.cmd.setreg.nreg = regvar->nreg;
  rtc.cmd.setreg.seq = schNextSeq(cp_Scheduler(cp), 
				  TransactionStatus::TRANS_SETREG);
  /*
   * Queue the object to be sent to the controller.
   */
  return queue_rtc_command(cp, &rtc, NET_SETREG_CMD);
}

/*.......................................................................
 * Implement the command that returns the value of a specified SZA register
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         reg  -  The register specification.
 *                         val  -  The register value.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static FUNC_FN(sc_getreg_fn)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable *vreg;   /* The register specification argument */
  RegisterVariable *regvar;  /* The derived type of vreg */
  unsigned int val; /* The value of the register */
  /*
   * Do we have a controller to send the command to?
   */
  if(rtc_offline(sc, "getreg"))
    return 1;
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vreg, NULL))
    return 1;
  
  regvar = REGISTER_VARIABLE(vreg);
  /*
   * Get the value of the requested register from the last frame buffer.
   */
  if(get_reg_info(cp_Archiver(cp), regvar->regmap, regvar->board, 
		  regvar->block, regvar->index, &val))
    return 1;
  
  UINT_VARIABLE(result)->uint = val;
  return 0;
}

/*-----------------------------------------------------------------------*
 * Archive control commands                                              *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Tell the logger where to place subsequent log files.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         dir  -  The new log-file directory.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_logdir_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable *vdir;     /* The directory-name argument */
  LoggerMessage msg;  /* The message to be sent to the logger thread */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vdir, NULL))
    return 1;
  /*
   * Send the request to the logger thread.
   */
  if(pack_logger_chdir(&msg, STRING_VARIABLE(vdir)->string) ||
     send_LoggerMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
    return 1;
  return 0;
}

/*.......................................................................
 * Tell the grabber where to place subsequent fits files.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         dir  -  The new log-file directory.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_grabdir_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable *vdir;     /* The directory-name argument */
  GrabberMessage msg;  /* The message to be sent to the grabber thread */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vdir, NULL))
    return 1;
  /*
   * Send the request to the grabber thread.
   */
  if(pack_grabber_chdir(&msg, STRING_VARIABLE(vdir)->string) ||
     send_GrabberMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that tells the archiver or logger or grabber to 
 * open a new file.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         file -  The type of file to open.
 *                       Optional arguments.
 *                         dir  -  The name of the directory in which
 *                                 to open the file. If not specified, the
 *                                 directory from previous opens will be
 *                                 used, or if never specified the directory
 *                                 in which genericcontrol was started will be
 *                                 used.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_open_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable *vfile;                /* The type of file to open */
  Variable *vdir;                 /* The optional directory */
  char *dir;                      /* The archiving directory */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vfile, &vdir, NULL))
    return 1;
  /*
   * Get the target directory.
   */
  dir = OPTION_HAS_VALUE(vdir) ? STRING_VARIABLE(vdir)->string : (char* )"";
  /*
   * Send an open message to the specified thread.
   */
  switch(CHOICE_VARIABLE(vfile)->choice) {
  case ARC_LOG_FILE:
    {
      LoggerMessage msg;
      if(pack_logger_open(&msg, dir) ||
	 send_LoggerMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
	return 1;
    };
    break;
  case ARC_DAT_FILE:
    {
      ArchiverMessage msg;
      if(pack_archiver_open(&msg, dir) ||
	 send_ArchiverMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
	return 1;
    };
    break;
  case ARC_GRAB_FILE:
    {
      GrabberMessage msg;
      if(pack_grabber_open(&msg, dir) ||
	 send_GrabberMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
	return 1;
    };
    break;
  default:
    lprintf(stderr, "sc_open_cmd: Unknown type of archive file.\n");
    return 1;
  };
  return 0;
}

/*.......................................................................
 * Implement the command that tells the archiver or logger to flush the
 * stdio buffers of its file to the disk.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         file  -  The type of file.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_flush_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable *vfile;                /* The type of file */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vfile, NULL))
    return 1;
  /*
   * Send a close message to the specified thread.
   */
  switch(CHOICE_VARIABLE(vfile)->choice) {
  case ARC_LOG_FILE:
    {
      LoggerMessage msg;
      if(pack_logger_flush(&msg) ||
	 send_LoggerMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
	return 1;
    };
    break;
  case ARC_DAT_FILE:
    {
      ArchiverMessage msg;
      if(pack_archiver_flush(&msg) ||
	 send_ArchiverMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
	return 1;
    };
    break;
  case ARC_GRAB_FILE:
    {
      GrabberMessage msg;
      if(pack_grabber_flush(&msg) ||
	 send_GrabberMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
	return 1;
    };
    break;
  default:
    lprintf(stderr, "sc_flush_cmd: Unknown type of archive file.\n");
    return 1;
  };
  return 0;
}

/*.......................................................................
 * Implement the command that tells the archiver or logger to close its
 * current file.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         file  -  The type of file.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_close_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable *vfile;                /* The type of file */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vfile, NULL))
    return 1;
  /*
   * Send a close message to the specified thread.
   */
  switch(CHOICE_VARIABLE(vfile)->choice) {
  case ARC_LOG_FILE:
    {
      LoggerMessage msg;
      if(pack_logger_close(&msg) ||
	 send_LoggerMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
	return 1;
    };
    break;
  case ARC_DAT_FILE:
    {
      ArchiverMessage msg;
      if(pack_archiver_close(&msg) ||
	 send_ArchiverMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
	return 1;
    };
    break;
  case ARC_GRAB_FILE:
    {
      GrabberMessage msg;
      if(pack_grabber_close(&msg) ||
	 send_GrabberMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
	return 1;
    };
    break;
  default:
    lprintf(stderr, "sc_close_cmd: Unknown type of archive file.\n");
    return 1;
  };
  return 0;
}

/*.......................................................................
 * Implement the command that tells the scanner to change the hardware
 * integration interval.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         IntTime exponent - The desired integration
 *                             interval. The actual interval is
 *                             12.8us x 2^(5 + exponent).
 *                     
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_integrate_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vexp;                /* The sampling interval as an exponent */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  /*
   * Do we have a controller to send the command to?
   */
  if(rtc_offline(sc, "integrate"))
    return 1;
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vexp, NULL))
    return 1;
  /*
   * Compose a network object to be sent to the real-time controller task.
   */
  rtc.cmd.interval.exponent = UINT_VARIABLE(vexp)->uint;
  /*
   * Queue the object to be sent to the controller.
   */
  return queue_rtc_command(cp, &rtc, NET_INTERVAL_CMD);
}

/*.......................................................................
 * Implement the command that tells the archiver how many frames to
 * integrate per archived frame.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of optional command-line arguments:
 *                         combine   - The number of input frames per archived
 *                                     frame.
 *                         dir       - The default directory for archive files.
 *                         filter    - True to only include feature-marked
 *                                     frames in the archive.
 *                         file_size - The number of frames to archive before
 *                                     opening a new archive file.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_archive_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable *vcombine;  /* The number of frames to integrate */
  Variable *vdir;      /* The archiving directory */
  Variable *vfilter;   /* The boolean filtering flag */
  Variable *vfile_size;/* The frames_per_file argument */
  ArchiverMessage msg; /* The message to be sent to the archiver thread */
  /*
   * Get the cache which holds the last "archive combine" value commanded
   * by the user.
   */
  SchedCache *cache = sch_sched_cache(cp_Scheduler(cp));
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vcombine, &vdir, &vfilter, &vfile_size, NULL))
    return 1;
  /*
   * Send the requested configuration messages to the archiver.
   */
  if(OPTION_HAS_VALUE(vcombine)) {
    cache->archive.combine = UINT_VARIABLE(vcombine)->uint;
    if(pack_archiver_sampling(&msg, UINT_VARIABLE(vcombine)->uint) ||
       send_ArchiverMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
      return 1;
  };
  if(OPTION_HAS_VALUE(vdir) &&
     (pack_archiver_chdir(&msg, STRING_VARIABLE(vdir)->string) ||
      send_ArchiverMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR))
    return 1;
  if(OPTION_HAS_VALUE(vfilter)) {
    cache->archive.filter = BOOL_VARIABLE(vfilter)->boolvar;
    if(pack_archiver_filter(&msg, BOOL_VARIABLE(vfilter)->boolvar) ||
       send_ArchiverMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
      return 1;
  };
  if(OPTION_HAS_VALUE(vfile_size) &&
     (pack_archiver_file_size(&msg, UINT_VARIABLE(vfile_size)->uint) ||
      send_ArchiverMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement a function that returns the last commanded archiving interval.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        (none)
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static FUNC_FN(sc_archiving_interval_fn)
{
  /*
   * Get the cache which holds the last "archive combine" value commanded
   * by the user.
   */
  SchedCache *cache = 
    (SchedCache* )sch_sched_cache(cp_Scheduler((ControlProg* )sc->project));
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Record the result for return.
   */
  UINT_VARIABLE(result)->uint = cache->archive.combine;
  return 0;
}

/*.......................................................................
 * Implement a function that returns disposition of the last commanded
 * request for archiving filtering.
 * 
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        (none)
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static FUNC_FN(sc_archive_filtering_fn)
{
  /*
   * Get the cache which holds the last "archive filter" value commanded
   * by the user.
   */
  SchedCache *cache = 
    (SchedCache* )sch_sched_cache(cp_Scheduler((ControlProg* )sc->project));
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Record the result for return.
   */
  BOOL_VARIABLE(result)->boolvar = cache->archive.filter;
  return 0;
}

/*.......................................................................
 * Implement the command that tells the scanner to inhibit or enable the
 * system timing generator interrupter. This enables or disables
 * data acquisition.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         Boolean state
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_inhibit_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vstate;              /* Whether to inhibit or enable integration */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  /*
   * Do we have a controller to send the command to?
   */
  if(rtc_offline(sc, "inhibit"))
    return 1;
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vstate, NULL))
    return 1;
  /*
   * Compose a network object to be sent to the real-time controller task.
   */
  rtc.cmd.inhibit.flag = BOOL_VARIABLE(vstate)->boolvar;
  /*
   * Queue the object to be sent to the controller.
   */
  return queue_rtc_command(cp, &rtc, NET_INHIBIT_CMD);
}

/*.......................................................................
 * Implement the command that tells the scanner to snap a register
 * frame as though an interrupt had been recieved from the system
 * timing generator.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        (none).
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_strobe_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  /*
   * Do we have a controller to send the command to?
   */
  if(rtc_offline(sc, "strobe"))
    return 1;
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Queue the object to be sent to the controller.
   */
  return queue_rtc_command(cp, NULL, NET_STROBE_CMD);
}

/*-----------------------------------------------------------------------*
 * Commands for shutting down or restarting control system components    *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Implement the command that tells the controller to shutdown.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         SysType type  -  The desired shutdown target.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_shutdown_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vtype;               /* The requested shutdown target */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vtype, NULL))
    return 1;
  /*
   * Compose a network object to be sent to the real-time controller task.
   */
  switch(CHOICE_VARIABLE(vtype)->choice) {
  case SYS_CPU:
    if(rtc_offline(sc, "shutdown"))
      return 1;
    rtc.cmd.shutdown.method = HARD_SHUTDOWN;
    return queue_rtc_command(cp, &rtc, NET_SHUTDOWN_CMD);
    break;
  case SYS_RTC:
    if(rtc_offline(sc, "shutdown"))
      return 1;
    rtc.cmd.shutdown.method = SOFT_SHUTDOWN;
    return queue_rtc_command(cp, &rtc, NET_SHUTDOWN_CMD);
    break;
  case SYS_CONTROL:
    return cp_request_shutdown(cp);
    break;
  };
  lprintf(stderr, "shutdown: Unknown target system.\n");
  return 1;
}

/*.......................................................................
 * Implement the command that tells the controller to reboot.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         SysType type  -  The desired reboot target.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_reboot_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vtype;               /* The requested reboot target */
  Variable *vant;                /* The requested reboot target */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vtype, &vant, NULL))
    return 1;

  // See if antennas were specified.  These will be ignored unless the
  // target is "pmac"

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  /*
   * Compose a network object to be sent to the real-time controller task.
   */
  switch(CHOICE_VARIABLE(vtype)->choice) {
  case SYS_CPU:
    if(rtc_offline(sc, "reboot"))
      return 1;
    rtc.cmd.shutdown.method = HARD_RESTART;
    return queue_rtc_command(cp, &rtc, NET_SHUTDOWN_CMD);
    break;
  case SYS_RTC:
    if(rtc_offline(sc, "reboot"))
      return 1;
    rtc.cmd.shutdown.method = SOFT_RESTART;
    return queue_rtc_command(cp, &rtc, NET_SHUTDOWN_CMD);
    break;
  case SYS_CONTROL:
    return cp_request_restart(cp);
    break;
  };
  lprintf(stderr, "reboot: Unknown target system.\n");
  return 1;
}

/*.......................................................................
 * Implement the command that loads a script to be executed whenever the
 * real-time controller is started.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         file  -  The name of the schedule file.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_load_reboot_script_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;   /* The
						     control-program
						     resource
						     container */
  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
  Variable *vsc;                   /* The script argument */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vsc, NULL))
    return 1;
  /*
   * Compile the contents of the schedule and queue it for execution.
   */
  if(sch_change_init_script(sch, SCRIPT_VARIABLE(vsc)->sc))
    return 1;
  return 0;
}

/*-----------------------------------------------------------------------*
 * Schedule control commands                                             *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Implement the command that queues a schedule for execution.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         sc  -  The scheduling script to queue.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_schedule_cmd)
{
  Variable* vsc=0;                   /* The schedule argument */
  Variable* vqueue=0;
  
  // Get the resource object of the parent thread.

  Scheduler *sch = 
    (Scheduler* )cp_ThreadData((ControlProg* )sc->project, CP_SCHEDULER);
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vsc, &vqueue, NULL))
    return 1;
  
  // Queue the schedule.

  int iQueue = OPTION_HAS_VALUE(vqueue) ? 
    (int)UINT_VARIABLE(vqueue)->uint : -1;

  if(sch_queue_schedule(sch, SCRIPT_VARIABLE(vsc)->sc, iQueue))
    return 1;

  return 0;
}

/*.......................................................................
 * Implement the command that kills the currently running schedule.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments (none).
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_abort_schedule_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;   /* The
						     control-program
						     resource
						     container */
  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Pass on the request to the scheduler.
   */
  if(sch_abort_schedule(sch))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that temporarily suspends execution the
 * currently running schedule.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments (none).
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_suspend_schedule_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;   /* The
						     control-program
						     resource
						     container */
  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Pass on the request to the scheduler.
   */
  if(sch_suspend_schedule(sch))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that temporarily resumes execution of the
 * currently running schedule, after it having been suspended.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments (none).
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_resume_schedule_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;   /* The
						     control-program
						     resource
						     container */
  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, NULL))
    return 1;
  /*
   * Pass on the request to the scheduler.
   */
  if(sch_resume_schedule(sch))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that remove a given schedule from the schedule
 * queue.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments.
 *                         number  -  The schedule queue index of the
 *                                    schedule to be removed.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_remove_schedule_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;   /* The
						     control-program
						     resource
						     container */
  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
  Variable *num;                   /* The queue index of the target schedule */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &num, NULL))
    return 1;
  /*
   * Pass on the request to the scheduler.
   */
  if(sch_remove_schedule(sch, UINT_VARIABLE(num)->uint))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that advances a given schedule within the schedule
 * queue.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments.
 *                         n  -  The schedule queue index of the
 *                               schedule to be moved.
 *                         dn -  The number of entries to advance the
 *                               schedule by.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_advance_schedule_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;   /* The
						     control-program
						     resource
						     container */
  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
  Variable *n;               /* The queue index of the target schedule */
  Variable *dn;              /* The number of entries to move the schedule */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &n, &dn, NULL))
    return 1;
  /*
   * Pass on the request to the scheduler.
   */
  if(sch_move_schedule(sch, UINT_VARIABLE(n)->uint, -UINT_VARIABLE(dn)->uint))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that retards a given schedule within the schedule
 * queue.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments.
 *                         n  -  The schedule queue index of the
 *                               schedule to be moved.
 *                         dn -  The number of entries to retard the
 *                               schedule by.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_retard_schedule_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;   /* The
						     control-program
						     resource
						     container */
  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
  Variable *n;               /* The queue index of the target schedule */
  Variable *dn;              /* The number of entries to move the schedule */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &n, &dn, NULL))
    return 1;
  /*
   * Pass on the request to the scheduler.
   */
  if(sch_move_schedule(sch, UINT_VARIABLE(n)->uint, UINT_VARIABLE(dn)->uint))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that checks wether a schedule is valid.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         sc  -  The scheduling script to check.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_check_schedule_cmd)
{
  Variable *vsc;                   /* The schedule argument */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vsc, NULL))
    return 1;
  /*
   * Since the schedule was checked when it was parsed as a Script variable,
   * we know that it is ok, if we got this far.
   */
  lprintf(stdout, "Schedule %s is ok.\n",
	  SCRIPT_VARIABLE(vsc)->sc->script.name);
  return 0;
}

/*.......................................................................
 * Implement the command that controls the scheduler autoqueue feature.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *
 *                       Optional arguments:
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_auto_queue_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vstate;              /* State of the queueing -- on or off */
  Variable *vdir;                /* Directory in which to look for files */
  Variable *vinterval;           /* Polling interval */
  SchedulerMessage msg;          /* The message to be sent to the scheduler
				    thread. */
                                 /*  real-time controller task */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vdir, &vstate, &vinterval, NULL))
    return 1;
  /*
   * Compose the scheduler message.
   */
  if(OPTION_HAS_VALUE(vdir)) {
    if(pack_scheduler_auto_dir(&msg, STRING_VARIABLE(vdir)->string) ||
       send_SchedulerMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
      return 1;
  }
  if(OPTION_HAS_VALUE(vinterval)) {
    /*
     * Convert to milliseconds
     */
    double ms = floor(DOUBLE_VARIABLE(vinterval)->d + 0.5)*1000; 
    if(pack_scheduler_auto_poll(&msg, (int)ms) ||
       send_SchedulerMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
      return 1;
  }
  if(OPTION_HAS_VALUE(vstate)) {
    if(pack_scheduler_auto_state(&msg, 
				 CHOICE_VARIABLE(vstate)->choice==SWITCH_ON) ||
       send_SchedulerMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
      return 1;
  }
  return 0;
}

/*.......................................................................
 * Implement the command that controls the pager
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *
 *                       Optional arguments:
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_pagerEmailAddress_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vaction;
  Variable *vemail;
  char* email=0;
  OutputStream *output = sc->output; /* The stream wrapper around stdout */

  /*  real-time controller task */

  TermMessage msg;
  
  // Get the command-line arguments.
  
  if(get_Arguments(args, &vaction, &vemail, NULL))
    return 1;
  
  email = OPTION_HAS_VALUE(vemail) ? STRING_VARIABLE(vemail)->string : NULL;
  
  // See what was requested
  
  try {
    switch((EmailAction)CHOICE_VARIABLE(vaction)->choice) {
    case EMAIL_ADD:
      if(pack_pager_email(&msg, true, email) ||
	 send_TermMessage(cp, &msg, PIPE_WAIT)==PIPE_ERROR)
	return 1;
      break;
    case EMAIL_CLEAR:
      if(pack_pager_email(&msg, false, email) ||
	 send_TermMessage(cp, &msg, PIPE_WAIT)==PIPE_ERROR)
	return 1;
      break;
    case EMAIL_LIST:
      std::vector<std::string>* emailList = getPagerEmailList(cp);
      
      for(unsigned i=0; i < emailList->size(); i++) {
	std::cout << emailList->at(i) << std::endl;
	if(output_printf(output, "%s\n", emailList->at(i).c_str())<0)
	  return 1;
      }
      std::cout << "Done" << std::endl;
      break;
    }
  } catch(...) {
    return 1;
  }
  
  return 0;
}

/*.......................................................................
 * Implement the command that controls the pager  
 */
static CMD_FN(sc_pager_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
                                                // resource object
  Variable *vstate=0; // State of the queueing -- on or off
  Variable *vip=0;    // The optional ip address
  Variable *vdev=0;   // The device we are addressing
  Variable *vreg=0;   // The register which activated the pager
  Variable *vhost=0;  // The optional host name/address

  PagerDev dev;
  char *ip=NULL;
  char *reg=NULL;
  char *host=NULL;

  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);

  TermMessage msg;
  OutputStream *output = sc->output; /* The stream wrapper around stdout */

  // Get the command-line arguments.

  if(get_Arguments(args, &vstate, &vip, &vdev, &vreg, &vhost, NULL))
    return 1;

  // Get the target IP address

  ip   = OPTION_HAS_VALUE(vip) ? STRING_VARIABLE(vip)->string : NULL;

  // Get the target device

  dev  = (PagerDev)(OPTION_HAS_VALUE(vdev) ? 
		    CHOICE_VARIABLE(vdev)->choice : PAGER_NONE);

  // Get the optional host name

  host = OPTION_HAS_VALUE(vhost) ? STRING_VARIABLE(vhost)->string : NULL;

  // Compose the scheduler message

  switch (CHOICE_VARIABLE(vstate)->choice) {

    // Tell the terminal thread to change the IP address of the pager

  case PAGER_IP:

    if(ip) {

      // Now the user must specify a device as well

      if(dev==PAGER_NONE) {
        lprintf(stderr, "pager_cmd: No device specified.\n");
        return 1;
      }

      // Pack the message for transmission to the terminal thread

      if(pack_pager_ip(&msg, dev, ip) ||
         send_TermMessage(cp, &msg, PIPE_WAIT)==PIPE_ERROR)
        return 1;
    } else {
      lprintf(stderr, "pager_cmd: No IP address specified.\n");
      return 1;
    }

    break;

  case PAGER_CLEAR:

    return sendClearPagerMsg(cp);

    break;

  case PAGER_STATUS:

    try {
      requestPagerStatus(cp);
    } catch(...) {
      return 1;
    }

    return 0;
    break;

  case PAGER_ENABLE:

    // And enable paging requests from clients

    if(sch_send_paging_state(sch, 1, NULL))
      return 1;

    // And send the messsage to disable the pager to the terminal
    // thread

    if(sendEnablePagerMsg(cp, true))
      return 1;

    break;

    // If we wish to activate the pager, send the disallow message to
    // the clients, and forward the appropriate command to the rtc

  case PAGER_ON:

    // We don't allow pager activation by anybody but szanet for now

    if(host != 0 && (strcmp(host, "sptcontrol")==0 ||
		     strcmp(host, "192.168.1.10")==0)) {

      // Disallow further paging commands

      if(sch_send_paging_state(sch, 0, NULL))
        return 1;

      // And activate the pager

      reg = OPTION_HAS_VALUE(vreg) ? STRING_VARIABLE(vreg)->string : NULL;

      // Use this wrapper to send pages

      if(send_reg_page_msg(cp, reg))
        return 1;

    } else {

      lprintf(stderr, "Request to activate pager by %s ignored.\n",
              (host ? host : "an unrecognized host"));

    }
    break;

    // De-activate the pager

  case PAGER_OFF:

    // Send the messsage to deactivate the pager

    if(pack_term_reg_page(&msg) || 
       send_TermMessage(cp, &msg, PIPE_WAIT)==PIPE_ERROR)
      return 1;

    break;

  case PAGER_DISABLE:

    // Disallow further paging commands from remote monitor clients

    if(sch_send_paging_state(sch, 0, NULL))
      return 1;

    // And send the messsage to disable the pager to the terminal
    // thread

    if(sendEnablePagerMsg(cp, false))
      return 1;

    break;
  }

  return 0;
}

//-----------------------------------------------------------------------
// Event management commands and functions                               
//-----------------------------------------------------------------------

/*.......................................................................
 * Add to the list of keywords that are accepted as signal names.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of optional command-line arguments:
 *                        keys  -  A list of keywords.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_add_signals_cmd)
{
  Variable *vkeys;          /* The list of keywords */
  ListNode *node;           /* A node in the list of keyword variables */
  /*
   * Get the resource object of the parent thread.
   */
  Scheduler *sch = (Scheduler* )cp_ThreadData((ControlProg *)sc->project, 
					      CP_SCHEDULER);
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vkeys, NULL))
    return 1;
  /*
   * Traverse the list of keywords to be added.
   */
  for(node=LIST_VARIABLE(vkeys)->list->head; node; node=node->next) {
    char *name = STRING_VARIABLE(node->data)->string;
    if(!sch_add_signal(sch, name))
      return 1;
  };
  return 0;
}

/*.......................................................................
 * Send/discard or initialize a given signal.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of optional command-line arguments:
 *                        op      -  One of the options send|clear|init.
 *                        signal  -  The name of the signal.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_signal_cmd)
{
  Variable *vop;        /* The operation to perform */
  Variable *vsig;       /* The signal name */
  char *op;             /* The value of *vop */
  Symbol *sig;          /* The value of *vsig */
  /*
   * Get the resource object of the parent thread.
   */
  Scheduler *sch = (Scheduler* )cp_ThreadData((ControlProg *)sc->project, 
					      CP_SCHEDULER);
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vop, &vsig, NULL))
    return 1;
  /*
   * Extract their values.
   */
  op = STRING_VARIABLE(vop)->string;
  sig = SIGNAL_VARIABLE(vsig)->sym;
  /*
   * Perform the specified operation.
   */
  if(strcmp(op, "send") == 0) {
    sch_signal_schedule(sch, sig);
  } else if(strcmp(op, "clear") == 0) {
    clear_script_signal(sig);
  } else if(strcmp(op, "init") == 0) {
    reset_script_signal(sig);
  } else {
    lprintf(stderr, "signal: Unhandled qualifier (%s).\n", op);
    return 1;
  };
  return 0;
}

/*-----------------------------------------------------------------------*
 * Site specification commands                                           *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Implement the command that records the location of the SZA.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        Longitude longitude -  The SZA longitude (-180..180).
 *                        Latitude latitude   -  The SZA latitude (-180..180).
 *                        Altitude altitude   -  The SZA altitude (meters).
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_site_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
  Variable *vlon;                /* The longitude of the SZA */
  Variable *vlat;                /* The latitude of the SZA */
  Variable *valt;                /* The altitude of the SZA */
  NavigatorMessage msg;          /* A message to send to the navigator thread */
  Site *site;                    /* The local site description object */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vlon, &vlat, &valt, NULL))
    return 1;
  /*
   * Get the scheduler's SZA site description object.
   */
  site = sch_Site(sch);
  /*
   * Set the new site parameters.
   */
  if(set_Site(site, DOUBLE_VARIABLE(vlon)->d * dtor,
	      DOUBLE_VARIABLE(vlat)->d * dtor, DOUBLE_VARIABLE(valt)->d))
    return 1;

  /*
   * Send the new site parameters to the navigator thread.
   */
  if(pack_navigator_site(&msg, site->longitude, site->latitude,
			 site->altitude) ||
     send_NavigatorMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
    return 1;
  return 0;
}

/*-----------------------------------------------------------------------*
 * Source-catalog commands and functions                                 *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Implement the command that starts the telescope tracking a new
 * source.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        source  -  The source to be tracked.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_track_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  Variable *vsrc;  // The source to be tracked 
  Variable *vtype; // The type of tracking
  Variable *vant;  // The set of antennas to command
  gcp::util::Tracking::Type type;

  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "track"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vsrc, &vtype, &vant, NULL))
    return 1;

  // If no type was specified, default to
  // pointing tracking (there is no phase pointing on SPT)

  type = OPTION_HAS_VALUE(vtype) ?
    (gcp::util::Tracking::Type)CHOICE_VARIABLE(vtype)->choice : 
    gcp::util::Tracking::TRACK_POINT;

  // Tell the navigator thread to send a track command to the control
  // system, update its source catalog to show this as the current
  // source, and arrange to send position updates for it when
  // necessary.

  return nav_track_source(cp_Navigator(cp),
			  SOURCE_VARIABLE(vsrc)->name, 
			  type,
			  // If no antennas were specified, default to
			  // all antennas
			  OPTION_HAS_VALUE(vant) ? 
			  SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId(),
			  schNextSeq(cp_Scheduler(cp), 
				     TransactionStatus::TRANS_PMAC));
}

/*.......................................................................
 * Implement the command that slews the telescope to a given location.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        Azimuth az     -  The target azimuth.
 *                        Elevation el   -  The target elevation.
 *                        DeckAngle dk   -  The target deck-rotator angle.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_slew_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vaz;  /* The optional target azimuth */
  Variable *vel;  /* The optional target elevation */
  Variable *vdk;  /* The optional target deck-rotator angle */
  Variable *vant; // The set of antennas to command

  // The set of axes to be slewed 

  unsigned mask = gcp::util::Axis::NONE;

  double az=0.0, el=0.0, dk=0.0; /* The target location of the slew (radians) */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "slew"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vaz, &vel, &vdk, &vant, NULL))
    return 1;
  
  // Determine which axes are to be slewed, and extract their target
  // positions.

  if(OPTION_HAS_VALUE(vaz)) {
    mask |= gcp::util::Axis::AZ;
    az = DOUBLE_VARIABLE(vaz)->d * dtor;
  };
  if(OPTION_HAS_VALUE(vel)) {
    mask |= gcp::util::Axis::EL;
    el = DOUBLE_VARIABLE(vel)->d * dtor;
  };
  if(OPTION_HAS_VALUE(vdk)) {
    mask |= gcp::util::Axis::PA;
    dk = DOUBLE_VARIABLE(vdk)->d * dtor;
  };

  /*
   * Tell the navigator thread to slew the telescope to the
   * specified coordinates, and update the catalog to make this
   * the current source.
   */
  return nav_slew_telescope(cp_Navigator(cp), mask, az, el, dk,
			    OPTION_HAS_VALUE(vant) ? 
			    SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId(),
			    schNextSeq(cp_Scheduler(cp), 
				       TransactionStatus::TRANS_PMAC));
}

/*.......................................................................
 * Implement the command that stows the telescope.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        Azimuth az     -  The target azimuth.
 *                        Elevation el   -  The target elevation.
 *                        DeckAngle dk   -  The target deck-rotator angle.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_stow_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vant; // The set of antennas to command

  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "stow"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vant, NULL))
    return 1;
  
  // Tell the navigator thread to send a track command to the control
  // system, update its source catalog to show this as the current
  // source, and arrange to send position updates for it when
  // necessary.

  return nav_track_source(cp_Navigator(cp),
			  "stow",
			  gcp::util::Tracking::TRACK_BOTH,
			  // If no antennas were specified, default to
			  // all antennas
			  OPTION_HAS_VALUE(vant) ? 
			  SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId(),
			  schNextSeq(cp_Scheduler(cp), 
				     TransactionStatus::TRANS_PMAC));
}

/*.......................................................................
 * Implement the command that sends the telescope to service position
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        Azimuth az     -  The target azimuth.
 *                        Elevation el   -  The target elevation.
 *                        DeckAngle dk   -  The target deck-rotator angle.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_service_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vant; // The set of antennas to command

  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "service"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vant, NULL))
    return 1;
  
  // Tell the navigator thread to send a track command to the control
  // system, update its source catalog to show this as the current
  // source, and arrange to send position updates for it when
  // necessary.

  return nav_track_source(cp_Navigator(cp),
			  "service",
			  gcp::util::Tracking::TRACK_BOTH,
			  // If no antennas were specified, default to
			  // all antennas
			  OPTION_HAS_VALUE(vant) ? 
			  SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId(),
			  schNextSeq(cp_Scheduler(cp), 
				     TransactionStatus::TRANS_PMAC));
}

/*.......................................................................
 * Implement the command that halts the telescope.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments: (none).
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_halt_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vant; // The set of antennas to command


  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "halt"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vant, NULL))
    return 1;
  
  // Tell the navigator thread to halt the telescope and update its
  // catalog to make the current source a fixed source at the position
  // at which we stopped.

  return nav_halt_telescope(cp_Navigator(cp),
			    OPTION_HAS_VALUE(vant) ? 
			    SET_VARIABLE(vant)->set : 
			    cp_AntSet(cp)->getId(),
			    schNextSeq(cp_Scheduler(cp), 
				       TransactionStatus::TRANS_PMAC));
}

/*.......................................................................
 * Implement the command that puts the telescope in STOP mode
 */
static CMD_FN(sc_stop_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vant; // The set of antennas to command
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */

  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "stop"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vant, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  if(queue_rtc_command(cp, &rtc, NET_STOP_CMD))
    return 1;
}

/*.......................................................................
 * Display the contemporary statistics of a given source.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         source  -  The name of the source.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_show_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;     /* The
						       control-program
						       resource
						       object */
  OutputStream *output = sc->output; /* The stream wrapper around stdout */
  Variable *vsrc;                    /* The source-name argument */
  Variable *vtype;                   /* The optional antenna specifier */
  Variable *vant;                    /* The optional antenna specifier */
  Variable *vutc;                    /* The optional date specifier */
  Variable *vhorizon;                /* The optional horizon specifier */
  char *name;                        /* The value of vsrc */
  SourceInfo info;                   /* The requested information */
  SourceId id;                       /* The identification of the source */
  double utc;                        /* The time-stamp of the information */
  double horizon;                    /* The horizon to use when computing */
                                     /*  rise and set times. */
  unsigned antennas;
  gcp::util::Tracking::Type type;
  std::vector<std::pair<SourceId, gcp::util::AntNum::Id> > sourceList;

  // Get the opaque resource object of the navigator thread.

  Navigator *nav = cp_Navigator(cp);
  
  // Get the resource object of the scheduling task that is running
  // this script.

  Scheduler *sch = cp_Scheduler(cp);
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vsrc, &vtype, &vant, &vutc, &vhorizon, NULL))
    return 1;

  // See if a type was specified.  This will be ignored unless the
  // source is "current"

  type = OPTION_HAS_VALUE(vtype) ? 
    (gcp::util::Tracking::Type)CHOICE_VARIABLE(vtype)->choice : 
    gcp::util::Tracking::TRACK_POINT;

  // See if antennas were specified.  These will be ignored unless the
  // source is "current"

  antennas = OPTION_HAS_VALUE(vant) ? 
    SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  // Get the name of the source.  If the name is "current," this is an
  // alias for the current source of the specified antennas.

  name = SOURCE_VARIABLE(vsrc)->name;
  
  // Lookup the true name of the source.

  try {
    sourceList = navLookupSourceExtended(nav, name, type, antennas, 1);
  } catch(...) {
    lprintf(stderr, "show: Failed to lookup source: %s\n", name);
    return 1;
  };
  
  // Get the time for which the information should be computed.

  utc = OPTION_HAS_VALUE(vutc) ? DOUBLE_VARIABLE(vutc)->d : -1.0;
  
  // Get the horizon to use for computing rise and set times.

  horizon = OPTION_HAS_VALUE(vhorizon) ?
    DOUBLE_VARIABLE(vhorizon)->d * dtor : sch_get_horizon(sch);
  
  // Now loop over all returned sources, displaying the requested
  // information for each one.

  for(std::vector<std::pair<SourceId, gcp::util::AntNum::Id> >::iterator 
	isrc = sourceList.begin(); isrc != sourceList.end(); isrc++) 
  {
    gcp::util::AntNum antSet(isrc->second);

    id = isrc->first;

    // Request all information about the specified source for the
    // current time.
    
    if(nav_source_info(nav, id.number, utc, horizon, 
		       SIO_ALL, &info))
      return 1;
    
    // Display the requested information.
    
    // Only print the antennas if the current source was requested.

    if(navIsCurrent(name))
      if(output_printf(output, "\n%cAntennas: %s:\n", 
		       (isrc == sourceList.begin()) ? '\0' : '\n',
		       antSet.printAntennaSet().c_str())<0)
	return 1;
    
    if(output_printf(output, "Source: %s  (", id.name)<0 ||
       output_utc(output, "", 0, 0, info.utc) ||
       write_OutputStream(output, " UTC)\n") ||
       write_OutputStream(output, " AZ: ") ||
       (info.axis_mask & gcp::util::Axis::AZ ?
	output_sexagesimal(output, "", 13, 0, 3, info.coord.az * rtod) :
	output_string(output, 0, "", 13, -1, -1, "(unspecified)")) ||
       write_OutputStream(output, "  EL: ") ||
       (info.axis_mask & gcp::util::Axis::EL ?
	output_sexagesimal(output, "", 13, 0, 3, info.coord.el * rtod) :
	output_string(output, 0, "", 13, -1, -1, "(unspecified)")) ||
       write_OutputStream(output, "  PA: ") ||
       (info.axis_mask & gcp::util::Axis::PA ?
	output_sexagesimal(output, "", 13, 0, 3, info.coord.pa * rtod) :
	output_string(output, 0, "", 13, -1, -1, "(unspecified)")) ||
       write_OutputStream(output, "\n"))
      return 1;
    
    // Display its equatorial apparent geocentric coordinates.
    
    if(info.axis_mask & gcp::util::Axis::AZ && 
       info.axis_mask & gcp::util::Axis::EL) {
      if(write_OutputStream(output, " RA: ") ||
	 output_sexagesimal(output, "", 13, 0, 3, info.ra * rtoh) ||
	 write_OutputStream(output, " DEC: ") ||
	 output_sexagesimal(output, "", 13, 0, 3, info.dec * rtod) ||
	 write_OutputStream(output, "\n"))
	return 1;
      
      // Is the source above the local horizon?
      
      if(output_printf(output, " Currently %s the %.4g degree horizon (%s).\n",
		       info.coord.el > horizon ? "above":"below", 
		       horizon * rtod,
		       info.rates.el > 0.0 ? "rising":"setting") < 0)
	return 1;
      
      // Display the almanac of the source.
      
      switch(info.sched) {
      case SRC_NEVER_RISES:
	if(write_OutputStream(output, " Never rises.\n"))
	  return 1;
	break;
      case SRC_NEVER_SETS:
	if(write_OutputStream(output, " Never sets.\n"))
	  return 1;
	break;
      case SRC_ALTERNATES:
	if(info.rise < info.set) {
	  if(write_OutputStream(output, " Next rises in ") ||
	     output_almanac_time(output, info.utc, info.rise) ||
	     write_OutputStream(output, ", and sets in ") ||
	     output_almanac_time(output, info.utc, info.set) ||
	     write_OutputStream(output, ".\n"))
	    return 1;
	} else {
	  if(write_OutputStream(output, " Next sets in ") ||
	     output_almanac_time(output, info.utc, info.set) ||
	     write_OutputStream(output, ", and rises in ") ||
	     output_almanac_time(output, info.utc, info.rise) ||
	     write_OutputStream(output, ".\n"))
	    return 1;
	};
	break;
      case SRC_IRREGULAR:
      default:
	if(write_OutputStream(output, " Rise and set times indeterminate.\n"))
	  return 1;
	break;
      };
    };
  }

  return 0;
}

/*.......................................................................
 * A private function of sc_show_cmd() used to display the length of
 * time that remains before a given almanac event.
 *
 * Input:
 *  output   OutputStream *  The stream to write to.
 *  query_utc      double    The date and time of the query (UTC MJD).
 *  event_utc      double    The date and time of the event (UTC MJD).
 *  event            char *  The event description to place before the time of
 *                           the event.
 * Output:
 *  return            int    0 - OK.
 *                           1 - Error.
 */
static int output_almanac_time(OutputStream *output, double query_utc,
			       double event_utc)
{
  char *units;   /* The units of time to be used */
  /*
   * Work out the time remaining before the event.
   */
  double dt = event_utc - query_utc;
  /*
   * Convert it to fractional hours, minutes or seconds, where apropriate.
   */
  if(dt >= 1.0) {
    units = "days";
  } else if((dt *= 24.0) >= 1.0) {
    units = "hours";
  } else if((dt *= 60.0) >= 1.0) {
    units = "minutes";
  } else {
    dt *= 60.0;
    units = "seconds";
  };
  return output_printf(output, "%.1f %s", dt, units) < 0;
}

/*.......................................................................
 * Implement a function that returns the elevation of the current source.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          source -  The source to investigate.
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_elevation_fn)
{
  SourceInfo info;   /* The horizon coordinates of the source */
  Variable *vsrc;    /* The source variable */
  /*
   * Get the opaque resource object of the navigator thread.
   */
  Navigator *nav = (Navigator* )cp_Navigator((ControlProg* )sc->project);
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vsrc, NULL))
    return 1;
  /*
   * Ask the navigator thread for the current horizon coordinates of the source.
   */
  if(nav_source_info(nav, SOURCE_VARIABLE(vsrc)->name, -1.0, 0.0, SIO_HORIZ,
		     &info))
    return 1;
  /*
   * Return the elevation in degrees.
   */
  DOUBLE_VARIABLE(result)->d = info.coord.el * rtod;
  return 0;
}

/*.......................................................................
 * Implement a function that returns the azimuth of the current source.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          source -  The source to investigate.
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_azimuth_fn)
{
  SourceInfo info;   /* The horizon coordinates of the source */
  Variable *vsrc;    /* The source variable */
  /*
   * Get the opaque resource object of the navigator thread.
   */
  Navigator *nav = (Navigator* )cp_Navigator((ControlProg* )sc->project);
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vsrc, NULL))
    return 1;
  /*
   * Ask the navigator thread for the current horizon coordinates of the source.
   */
  if(nav_source_info(nav, SOURCE_VARIABLE(vsrc)->name, -1.0, 0.0, SIO_HORIZ,
		     &info))
    return 1;
  /*
   * Return the azimuth in degrees.
   */
  DOUBLE_VARIABLE(result)->d = info.coord.az * rtod;
  return 0;
}

/*.......................................................................
 * Tell the navigator thread to read a given source catalog.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         file  -  The name of the source-catalog file.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_catalog_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;   /* The
						     control-program
						     resource
						     container */
  NavigatorMessage msg;          /* A message to send to the navigator thread */
  Variable *vfile;     /* The file-name argument */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vfile, NULL))
    return 1;
  /*
   * Send the source-catalog file-name to the navigator thread.
   */
  if(pack_navigator_catalog(&msg, STRING_VARIABLE(vfile)->string) ||
     send_NavigatorMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
    return 1;
  return 0;
}

/*-----------------------------------------------------------------------*
 * Transactions
 *-----------------------------------------------------------------------*/
/*.......................................................................
 * Tell the logger thread to read a transaction catalog
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         file  -  The name of the source-catalog file.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_loadTransaction_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource container
  LoggerMessage msg;          /* A message to send to the navigator thread */
  Variable* vfile;            /* The file-name argument */
  Variable* vclear;
  bool clear=true;

  // Get the command-line arguments.

  if(get_Arguments(args, &vfile, &vclear, NULL))
    return 1;

  // If the clear argument was specified, set it now

  if(OPTION_HAS_VALUE(vclear))
    clear = BOOL_VARIABLE(vclear)->boolvar;

  // Send the transaction catalog file-name to the logger thread

  if(pack_logger_transaction_catalog(&msg, STRING_VARIABLE(vfile)->string,
				     clear) ||
     send_LoggerMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
    return 1;

  return 0;
}

/*.......................................................................
 * Tell the logger thread to read a transaction catalog
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         file  -  The name of the source-catalog file.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_logTransaction_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource container
  LoggerMessage msg;          // A message to send to the navigator thread 
  Variable* vdevice;          // The device argument 
  Variable* vlocation;        // The location argument 
  Variable* vserial;          // The serial argument 
  Variable* vdate;            // The date
  Variable* vwho;             // A string identifying the culprit
  Variable* vcomment;         // A comment

  // Get the command-line arguments.

  if(get_Arguments(args, &vdevice, &vserial, &vlocation, &vdate, &vwho,
		   &vcomment, NULL))
    return 1;

  // Send the transaction to the logger thread
  
  if(pack_logger_log_transaction(&msg, 
				 TRANSDEV_VARIABLE(vdevice)->name,
				 STRING_VARIABLE(vserial)->string,
				 STRING_VARIABLE(vlocation)->string,
				 DOUBLE_VARIABLE(vdate)->d,
				 STRING_VARIABLE(vwho)->string,
				 OPTION_HAS_VALUE(vcomment) ?
				 STRING_VARIABLE(vcomment)->string : (char*)"") 
     || send_LoggerMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
    return 1;

  return 0;
}

/*-----------------------------------------------------------------------*
 * Scan-catalog commands and functions                                 *
 *-----------------------------------------------------------------------*/
/*.......................................................................
 * Tell the navigator thread to read a given scan catalog.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         file  -  The name of the source-catalog file.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_scan_catalog_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource container
  NavigatorMessage msg;          /* A message to send to the navigator thread */
  Variable *vfile;               /* The file-name argument */

  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vfile, NULL))
    return 1;

  /*
   * Send the source-catalog file-name to the navigator thread.
   */
  if(pack_navigator_scan_catalog(&msg, STRING_VARIABLE(vfile)->string) ||
     send_NavigatorMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
    return 1;

  return 0;
}

/*.......................................................................
 * Tell the navigator thread to read a given ephemeris of UT1-UTC.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         file  -  The name of the ephemeris file.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_ut1utc_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource
						   container */
  NavigatorMessage msg;          /* A message to send to the navigator thread */
  Variable *vfile;               /* The file-name argument */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vfile, NULL))
    return 1;
  /*
   * Send the ephemeris file-name to the navigator thread.
   */
  if(pack_navigator_ut1utc(&msg, STRING_VARIABLE(vfile)->string) ||
     send_NavigatorMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
    return 1;
  return 0;
}

/*.......................................................................
 * Tell the scheduler the default horizon to use when getting source
 * information from the navigator.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         angle  -  The elevation of the horizon.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_horizon_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource
						   container */
  Variable *vangle;              /* The horizon angle argument */
  /*
   * Get the parent thread of this script.
   */
  Scheduler *sch = cp_Scheduler(cp);
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vangle, NULL))
    return 1;
  /*
   * Record the new horizon.
   */
  return sch_set_horizon(sch, DOUBLE_VARIABLE(vangle)->d * dtor);
}

//-----------------------------------------------------------------------
// Scan commands
//-----------------------------------------------------------------------

/*.......................................................................
 * Implement the command that starts the telescope tracking a new
 * source.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        source  -  The source to be tracked.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_scan_cmd)
{
  // The control-program resource object 

  ControlProg *cp = (ControlProg*)sc->project; 
  Variable *vadd;                /* The add-offset modifier argument */
  Variable *vscan;                /* The scan to start */
  Variable *vreps;                /* The number of times to run the scan */

  /*
   * Do we have a controller to send the command to?
   */
  if(rtc_offline(sc, "scan"))
    return 1;

  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vadd, &vscan, &vreps, NULL))
    return 1;

  /*
   * Tell the navigator thread to send a track command to the control
   * system, update its source catalog to show this as the current
   * source, and arrange to send position updates for it when
   * necessary.
   */
  return nav_start_scan(cp_Navigator(cp),
                        SCAN_VARIABLE(vscan)->name,
                        OPTION_HAS_VALUE(vreps) ? 
			UINT_VARIABLE(vreps)->uint : 1,
			schNextSeq(cp_Scheduler(cp), 
				   TransactionStatus::TRANS_SCAN),
			BOOL_VARIABLE(vadd)->boolvar);
}

/*.......................................................................
 * Display the statistics of a given scan.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         source  -  The name of the source.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_show_scan_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  Variable *vscan;                   /* The source-name argument */
  char *name;                        /* The value of vsrc */
  ScanId id;                         /* The identification of the source */

  // Get the opaque resource object of the navigator thread.

  Navigator *nav = cp_Navigator(cp);
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vscan, NULL))
    return 1;

  name = SCAN_VARIABLE(vscan)->name;

  // Lookup the true name of the scan

  if(nav_print_scan_info(nav, name, 1, &id)) {
    lprintf(stderr, "show: Failed to lookup scan: %s\n", name);
    return 1;
  };
  return 0;
}

/*.......................................................................
 * Display the statistics of a given scan.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         source  -  The name of the source.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static FUNC_FN(sc_scan_len_fn)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  Variable* vscan=0;                 /* The source-name argument */
  Variable* vint=0;                 /* The source-name argument */
  char* name=0;                      /* The value of vsrc */
  ScanId id;                         /* The identification of the source */

  // Get the opaque resource object of the navigator thread.

  Navigator *nav = cp_Navigator(cp);
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vscan, &vint, NULL))
    return 1;

  name = SCAN_VARIABLE(vscan)->name;
  
  unsigned nreps = OPTION_HAS_VALUE(vint) ? UINT_VARIABLE(vint)->uint : 
    1;

  unsigned ms=0;
  if(nav_get_scan_del(nav, name, 1, &id, nreps, ms)) {
    lprintf(stderr, "scan_len: Failed to lookup scan: %s\n", name);
    return 1;
  };

  // Get the interval, in seconds and return it

  DOUBLE_VARIABLE(result)->d =(double)(ms)/(1000 * 3600);

  return 0;
}

/*-----------------------------------------------------------------------*
 * Pointing model commands                                               *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Implement the command that sets the collimation terms of the pointing
 * model.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        Model model    -  The type of model (optical|radio).
 *                        Tilt size      -  The size of the collimation tilt.
 *                        DeckAngle dir  -  The deck angle at which the
 *                                          tilt is directed radially
 *                                          outwards.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_collimate_fixed_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable* vadd;
  Variable* vmodel;              /* The collimation axis */
  Variable* vaz;                /* The size of the azimuth offset */
  Variable* vel;                /* The size of the elevation offset */ 
  Variable* vptel;
  Variable* vant;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "collimate_fixed"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vadd, &vmodel, &vaz, &vel, &vptel, &vant, NULL))
    return 1;

  // Compose the real-time controller network command.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.collimate.seq     
    = schNextSeq(cp_Scheduler(cp), TransactionStatus::TRANS_PMAC);

  rtc.cmd.collimate.mode    = CHOICE_VARIABLE(vmodel)->choice;

  rtc.cmd.collimate.addMode = BOOL_VARIABLE(vadd)->boolvar ? 
    gcp::util::OffsetMsg::ADD : gcp::util::OffsetMsg::SET;

  rtc.cmd.collimate.type    = gcp::util::Collimation::FIXED;

  // mill-arcsec 

  rtc.cmd.collimate.x = (int)(DOUBLE_VARIABLE(vaz)->d * dtomas);
  rtc.cmd.collimate.y = (int)(DOUBLE_VARIABLE(vel)->d * dtomas);
 
  // Get the ptels to apply this command to

  if(rtc.cmd.collimate.mode == gcp::util::PointingMode::OPTICAL) {

    if(!OPTION_HAS_VALUE(vptel)) {
      lprintf(stderr, "You must specify a ptel\n");
      return 1;
    }

    try {
      
      rtc.cmd.collimate.ptelMask = SET_VARIABLE(vptel)->set;
      
    } catch(gcp::util::Exception& err) {
      lprintf(stderr, "%s", err.what());
      return 1;
    }
  }

  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_COLLIMATE_CMD))
    return 1;

  return 0;
}

/*.......................................................................
 * Implement the command that sets the collimation terms of the pointing
 * model.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        Model model    -  The type of model (optical|radio).
 *                        Tilt size      -  The size of the collimation tilt.
 *                        DeckAngle dir  -  The deck angle at which the
 *                                          tilt is directed radially
 *                                          outwards.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_collimate_polar_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vmodel;              /* The collimation axis */
  Variable *vmag;                /* The size of the azimuth offset */
  Variable *vdir;                /* The size of the elevation offset */ 
  Variable* vant;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "collimate_deck"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vmodel, &vmag, &vdir, &vant, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.collimate.seq = 
    schNextSeq(cp_Scheduler(cp), TransactionStatus::TRANS_PMAC);

  rtc.cmd.collimate.mode = CHOICE_VARIABLE(vmodel)->choice;
  rtc.cmd.collimate.type = gcp::util::Collimation::POLAR;

  // mill-arcsec 

  rtc.cmd.collimate.magnitude = (int)(DOUBLE_VARIABLE(vmag)->d * dtomas);
  rtc.cmd.collimate.direction = (int)(DOUBLE_VARIABLE(vdir)->d * dtomas);
 
  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_COLLIMATE_CMD))
    return 1;

  return 0;
}

/*.......................................................................
 * Implement the command that sets the encoder scales and directions.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        az_turn - The # of azimuth encoder counts per turn.
 *                        el_turn - The # of elevation encoder counts per turn.
 *                        dk_turn - The # of deck encoder counts per turn.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_encoder_cals_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vaz_t;               /* Azimuth encoder counts per turn */
  Variable *vel_t;               /* Elevation encoder counts per turn */
  Variable *vdk_t;               /* Deck encoder counts per turn */
  Variable *vant;                /* Deck encoder counts per turn */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "encoder_cals"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vaz_t, &vel_t, &vdk_t, &vant, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.antennas  = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.encoder_cals.seq  = schNextSeq(cp_Scheduler(cp), 
					 TransactionStatus::TRANS_PMAC);

  rtc.cmd.encoder_cals.az   = INT_VARIABLE(vaz_t)->i;
  rtc.cmd.encoder_cals.el   = INT_VARIABLE(vel_t)->i;
  rtc.cmd.encoder_cals.dk   = INT_VARIABLE(vdk_t)->i;
  
  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_ENCODER_CALS_CMD))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that sets the encoder zero points.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        az - The encoder angle at zero azimuth.
 *                        el - The encoder angle at zero elevation.
 *                        dk - The encoder angle at the deck reference
 *                             position.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_encoder_zeros_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vaz,*vel, *vdk;   /* Azimuth,elevation and deck zero points */
  Variable *vant;
  RtcNetCmd rtc;        /* The network object to be sent to the
			   real-time controller task */
  /*
   * Do we have a controller to send the command to?
   */
  if(rtc_offline(sc, "encoder_zeros"))
    return 1;
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vaz, &vel, &vdk, &vant, NULL))
    return 1;

  // Compose the real-time controller network command.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  
  // Compose the real-time controller network command.

  rtc.cmd.encoder_zeros.seq = schNextSeq(cp_Scheduler(cp), 
					 TransactionStatus::TRANS_PMAC);

  // Convert from degrees to radians

  rtc.cmd.encoder_zeros.az = (DOUBLE_VARIABLE(vaz)->d * dtor); 
  rtc.cmd.encoder_zeros.el = (DOUBLE_VARIABLE(vel)->d * dtor);
  rtc.cmd.encoder_zeros.dk = (DOUBLE_VARIABLE(vdk)->d * dtor);
  
  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_ENCODER_ZEROS_CMD))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that tells the control system the encoder
 * limit terms of the pointing model.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        az_min - The minimum azimuth encoder count.
 *                        az_max - The maximum azimuth encoder count.
 *                        el_min - The minimum elevation encoder count.
 *                        el_max - The maximum elevation encoder count.
 *                        dk_min - The minimum deck encoder count.
 *                        dk_max - The maximum deck encoder count.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_encoder_limits_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vaz_min;             /* The upper azimuth encoder limit */
  Variable *vaz_max;             /* The lower azimuth encoder limit */
  Variable *vel_min;             /* The upper elevation encoder limit */
  Variable *vel_max;             /* The lower elevation encoder limit */
  Variable *vdk_min;             /* The upper dk encoder limit */
  Variable *vdk_max;             /* The lower dk encoder limit */
  Variable *vant;                /* The set of antennas */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "encoder_limits"))
    return 1;

  // Get the command-line arguments.

  if(get_Arguments(args, &vaz_min, &vaz_max, &vel_min, &vel_max, &vdk_min, 
		   &vdk_max, &vant, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.antennas  = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.encoder_limits.seq    = schNextSeq(cp_Scheduler(cp), 
					     TransactionStatus::TRANS_PMAC);

  rtc.cmd.encoder_limits.az_min = INT_VARIABLE(vaz_min)->i;
  rtc.cmd.encoder_limits.az_max = INT_VARIABLE(vaz_max)->i;
  rtc.cmd.encoder_limits.el_min = INT_VARIABLE(vel_min)->i;
  rtc.cmd.encoder_limits.el_max = INT_VARIABLE(vel_max)->i;
  rtc.cmd.encoder_limits.pa_min = INT_VARIABLE(vdk_min)->i;
  rtc.cmd.encoder_limits.pa_max = INT_VARIABLE(vdk_max)->i;
  
  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_ENCODER_LIMITS_CMD))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that tells the control system the axis-tilt
 * terms of the pointing model.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        ha      - The azimuth tilt in the direction of
 *                                  increasing hour angle.
 *                        lat     - The azimuth tilt in the direction of
 *                                  increasing latitude.
 *                        el      - The tilt of the elevation axis clockwise
 *                                  around the direction of the azimuth vector.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_tilts_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vha;                 /* The azimuth tilt parallel to hour angle */
  Variable *vlat;                /* The azimuth tilt parallel to latitude */
  Variable *vel;                 /* The elevation tilt */
  Variable* vant;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "tilts"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vha, &vlat, &vel, &vant, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.tilts.seq = schNextSeq(cp_Scheduler(cp), 
				 TransactionStatus::TRANS_PMAC);

  // milli-arcsec 

  rtc.cmd.tilts.ha = (int)(DOUBLE_VARIABLE(vha)->d * dtomas);        
  rtc.cmd.tilts.lat = (int)(DOUBLE_VARIABLE(vlat)->d * dtomas);      
  rtc.cmd.tilts.el = (int)(DOUBLE_VARIABLE(vel)->d * dtomas);        
  
  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_TILTS_CMD))
    return 1;

  return 0;
}

/*.......................................................................
 * Implement the command that tells the control system the gravitational
 * flexure term of the pointing model.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        flexure - The gravitational flexure of the mount
 *                                  per cosine of elevation.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_flexure_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vsflex;               /* The gravitational flexure of the mount */
  Variable *vcflex;               /* The gravitational flexure of the mount */
  Variable *vmodel;               /* The gravitational flexure of the mount */
  Variable* vptel;
  Variable* vant;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "flexure"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vmodel, &vsflex, &vcflex, &vptel, &vant, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.flexure.seq  = schNextSeq(cp_Scheduler(cp), 
				    TransactionStatus::TRANS_PMAC);

  rtc.cmd.flexure.mode = CHOICE_VARIABLE(vmodel)->choice;

  // Radians per sine and cosine elevation

  rtc.cmd.flexure.sFlexure = (int)(DOUBLE_VARIABLE(vsflex)->d * dtomas);
  rtc.cmd.flexure.cFlexure = (int)(DOUBLE_VARIABLE(vcflex)->d * dtomas);

  // Get the ptels to apply this command to

  // Get the ptels to apply this command to

  if(rtc.cmd.flexure.mode == gcp::util::PointingMode::OPTICAL) {

    if(!OPTION_HAS_VALUE(vptel)) {
      lprintf(stderr, "You must specify a ptel\n");
      return 1;
    }

    try {
      
      rtc.cmd.flexure.ptelMask = SET_VARIABLE(vptel)->set;
      
    } catch(gcp::util::Exception& err) {
      lprintf(stderr, "%s", err.what());
      return 1;
    }
  }
  
  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_FLEXURE_CMD))
    return 1;

  return 0;
}

/*.......................................................................
 * Implement the command that selects between the optical and radio
 * pointing models.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        Model model  -  The model type (optical or radio).
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_model_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable* vmodel;              /* The model type */
  Variable* vptel;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "model"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vmodel, &vptel, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.model.seq = schNextSeq(cp_Scheduler(cp), 
				 TransactionStatus::TRANS_PMAC);

  rtc.cmd.model.mode = CHOICE_VARIABLE(vmodel)->choice;

  // Get the ptels to apply this command to

  if(rtc.cmd.model.mode == gcp::util::PointingMode::OPTICAL) {

    if(!OPTION_HAS_VALUE(vptel)) {
      lprintf(stderr, "You must specify a ptel\n");
      return 1;
    }

    try {
      
      rtc.cmd.model.ptelMask = SET_VARIABLE(vptel)->set;
      
    } catch(gcp::util::Exception& err) {
      lprintf(stderr, "%s", err.what());
      return 1;
    }

  } else {
    rtc.cmd.model.ptelMask = gcp::util::PointingTelescopes::PTEL_NONE;
  }
  
  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_MODEL_CMD))
    return 1;

  return 0;
}

/*.......................................................................
 * Implement the command that adds to pointing offsets on one or more
 * pointing axes.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        mode     -  Whether to add or replace any existing
 *                                    offsets.
 *                       Optional arguments:
 *                        az,el,dk -  Horizon pointing offsets.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_zeroScanOffsets_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "zeroScanOffset"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, NULL))
    return 1;
  
  // Compose a network command for sending horizon offsets.

  rtc.antennas = cp_AntSet(cp)->getId();

  rtc.cmd.scan.npt =  0;
  rtc.cmd.scan.seq = -1;

  if(queue_rtc_command(cp, &rtc, NET_SCAN_CMD))
    return 1;

  return 0;
}

/*.......................................................................
 * Implement the command that adds to pointing offsets on one or more
 * pointing axes.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        mode     -  Whether to add or replace any existing
 *                                    offsets.
 *                       Optional arguments:
 *                        az,el,dk -  Horizon pointing offsets.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_offset_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vadd;                /* The add-offset modifier argument */
  Variable *vaz,*vel,*vdk;       /* The optional horizon offsets */
  Variable* vant;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "offset"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vadd, &vaz, &vel, &vdk, &vant, NULL))
    return 1;
  
  // Compose a network command for sending horizon offsets.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.mount_offset.seq = schNextSeq(cp_Scheduler(cp), 
					TransactionStatus::TRANS_PMAC);

  rtc.cmd.mount_offset.axes = gcp::util::OffsetMsg::NONE;

  rtc.cmd.mount_offset.mode = BOOL_VARIABLE(vadd)->boolvar ? 
    gcp::util::OffsetMsg::ADD : gcp::util::OffsetMsg::SET;

  if(OPTION_HAS_VALUE(vaz)) {
    rtc.cmd.mount_offset.axes |= gcp::util::OffsetMsg::AZ;
    rtc.cmd.mount_offset.az = DOUBLE_VARIABLE(vaz)->d * dtor;
  } else {
    rtc.cmd.mount_offset.az = 0;
  };
  if(OPTION_HAS_VALUE(vel)) {
    rtc.cmd.mount_offset.axes |= gcp::util::OffsetMsg::EL;
    rtc.cmd.mount_offset.el = DOUBLE_VARIABLE(vel)->d * dtor;
  } else {
    rtc.cmd.mount_offset.el = 0;
  };
  if(OPTION_HAS_VALUE(vdk)) {
    rtc.cmd.mount_offset.axes |= gcp::util::OffsetMsg::PA;;
    rtc.cmd.mount_offset.dk = DOUBLE_VARIABLE(vdk)->d * dtor;
  } else {
    rtc.cmd.mount_offset.dk = 0;
  };
  
  // Send the message to the tracker.

  return queue_rtc_command(cp, &rtc, NET_MOUNT_OFFSET_CMD);
}

/*.......................................................................
 * Implement the command that adds to the equatorial pointing offsets.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        mode     -  Whether to add or replace any existing
 *                                    offsets.
 *                       Optional arguments:
 *                        ra,dec   -  Right Ascension and Declination offsets.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_radec_offset_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable* vadd;           /* The add-offset modifier argument */
  Variable* vra;            /* The optional equatorial offsets */
  Variable* vdec;           /* The optional equatorial offsets */
  Variable* vdt;
  Variable* vant;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "radec_offset"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vadd, &vra, &vdec, &vdt, &vant, NULL))
    return 1;
  
  // Compose a network command for sending equatorial offsets.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.equat_offset.seq = schNextSeq(cp_Scheduler(cp), 
					TransactionStatus::TRANS_PMAC);

  rtc.cmd.equat_offset.axes = gcp::util::OffsetMsg::NONE;
  rtc.cmd.equat_offset.mode = BOOL_VARIABLE(vadd)->boolvar ? 
    gcp::util::OffsetMsg::ADD : gcp::util::OffsetMsg::SET;

  // Don't let the user specify both ra and dt

  if(OPTION_HAS_VALUE(vra) && OPTION_HAS_VALUE(vdt)) {
    lprintf(stderr, "You can't specify both ra and dt\n");
    return 1;
  }

  if(!(OPTION_HAS_VALUE(vra) || OPTION_HAS_VALUE(vdt))) {
    rtc.cmd.equat_offset.ra = 0;
  } else if(OPTION_HAS_VALUE(vra)) {
    rtc.cmd.equat_offset.axes |= gcp::util::OffsetMsg::RA;
    rtc.cmd.equat_offset.ra = (int)(DOUBLE_VARIABLE(vra)->d * dtomas);
  } else if(OPTION_HAS_VALUE(vdt)) {
    rtc.cmd.equat_offset.axes |= gcp::util::OffsetMsg::RA;
    rtc.cmd.equat_offset.ra = (int)(DOUBLE_VARIABLE(vdt)->d * 180.0/12 * dtomas);
  };

  if(OPTION_HAS_VALUE(vdec)) {
    rtc.cmd.equat_offset.axes |= gcp::util::OffsetMsg::DEC;
    rtc.cmd.equat_offset.dec = (int)(DOUBLE_VARIABLE(vdec)->d * dtomas);
  } else {
    rtc.cmd.equat_offset.dec = 0;
  };
  
  // Send the message to the tracker.

  return queue_rtc_command(cp, &rtc, NET_EQUAT_OFFSET_CMD);
}

/*.......................................................................
 * Implement the command that arranges for the az and el tracking offsets
 * to be adjusted such that the image on the optical-pointing TV display
 * moves a given amount horizontally and vertically.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        right - The amount to move the image rightwards.
 *                        up    - The amount to move the image upwards.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_tv_offset_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vright;              /* The amount to move the image rightwards */
  Variable *vup;                 /* The amount to move the image upwards */
  Variable* vant;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "tv_offset"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vright, &vup, &vant, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.tv_offset.seq   = schNextSeq(cp_Scheduler(cp), 
				       TransactionStatus::TRANS_PMAC);

  rtc.cmd.tv_offset.right = (int)(DOUBLE_VARIABLE(vright)->d * dtomas);
  rtc.cmd.tv_offset.up    = (int)(DOUBLE_VARIABLE(vup)->d * dtomas);
  

  lprintf(stdout, "(VIEWER) Sending tv offset command with: right = %+d (mas)\n", rtc.cmd.tv_offset.right);
  lprintf(stdout, "(VIEWER) Sending tv offset command with: up    = %+d (mas)\n", rtc.cmd.tv_offset.up);

  // Send the message to the tracker.

  return queue_rtc_command(cp, &rtc, NET_TV_OFFSET_CMD);
}

/*.......................................................................
 * Implement the command that configures the orientation angle of the
 * tv display.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        angle - The deck angle at which the TV display
 *                                shows the sky in its normal orientation.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_tv_angle_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vangle;              /* The orientation angle of the camera */
  Variable* vant;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "tv_angle"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vangle, &vant, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.tv_angle.angle = (int)(DOUBLE_VARIABLE(vangle)->d * dtomas);
  
  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_TV_ANGLE_CMD))
    return 1;
  return 0;
}

/*.......................................................................
 * The sky_offset command asks the telescope to track a point displaced
 * from the normal pointing center by a given fixed angle on the sky,
 * irrespective of the elevation or declination (ie. no cos(el) dependence).
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        mode     -  Whether to add or replace any existing
 *                                    offsets.
 *                       Optional arguments:
 *                        x,y      -  The offsets on the sky, with x directed
 *                                    towards the zenith along the great circle
 *                                    that connects the zenith to the pole, and
 *                                    y along the perpendicular great circle
 *                                    that goes through the zenith.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_sky_offset_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vadd;                /* The add-offset modifier argument */
  Variable *vx,*vy;              /* The orthoganol offsets to request */
  Variable* vant;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "sky_offset"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vadd, &vx, &vy, &vant, NULL))
    return 1;
  
  // Compose a network command for sending horizon offsets.

  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.sky_offset.seq = schNextSeq(cp_Scheduler(cp), 
				      TransactionStatus::TRANS_PMAC);

  rtc.cmd.sky_offset.axes = gcp::util::OffsetMsg::NONE;
  rtc.cmd.sky_offset.mode = BOOL_VARIABLE(vadd)->boolvar ? 
    gcp::util::OffsetMsg::ADD : gcp::util::OffsetMsg::SET;

  if(OPTION_HAS_VALUE(vx)) {
    rtc.cmd.sky_offset.axes |= gcp::util::OffsetMsg::X;
    rtc.cmd.sky_offset.x = (int)(DOUBLE_VARIABLE(vx)->d * dtomas);
  } else {
    rtc.cmd.sky_offset.x = 0;
  };
  if(OPTION_HAS_VALUE(vy)) {
    rtc.cmd.sky_offset.axes |= gcp::util::OffsetMsg::Y;
    rtc.cmd.sky_offset.y = (int)(DOUBLE_VARIABLE(vy)->d * dtomas);
  } else {
    rtc.cmd.sky_offset.y = 0;
  };
  
  // Send the message to the tracker.

  return queue_rtc_command(cp, &rtc, NET_SKY_OFFSET_CMD);
}

/*.......................................................................
 * Implement the command that tells the tracker how to position the
 * deck axis while tracking a source.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        DeckMode mode  -  The deck-axis tracking mode.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_deck_mode_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vmode;               /* The tracking mode */
  Variable *vant;  // The set of antennas to command
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "deck_mode"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vmode, &vant, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.cmd.deckMode.seq = schNextSeq(cp_Scheduler(cp), 
				    TransactionStatus::TRANS_PMAC);

  rtc.cmd.deckMode.mode = CHOICE_VARIABLE(vmode)->choice;
  
  rtc.antennas = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_DECK_MODE_CMD))
    return 1;

  return 0;
}

/*.......................................................................
 * Implement the command that reduces the slew rate of one or more
 * telescope axes.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        SlewRate az   -  The azimuth slew rate (%).
 *                        SlewRate el   -  The elevation slew rate (%).
 *                        SlewRate dk   -  The deck slew rate (%).
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_slew_rate_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable* vant;
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  Variable *vaz;                 /* The optional azimuth slew rate */
  Variable *vel;                 /* The optional elevation slew rate */
  Variable *vdk;                 /* The optional deck slew rate */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "slew_rate"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vaz, &vel, &vdk, &vant, NULL))
    return 1;
  
  // Compose the network command to be sent to the control system.

  rtc.antennas = OPTION_HAS_VALUE(vant) ? 
    SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  rtc.cmd.slew_rate.seq = schNextSeq(cp_Scheduler(cp), 
				     TransactionStatus::TRANS_PMAC);

  rtc.cmd.slew_rate.mask = gcp::util::Axis::NONE;
  
  // Determine which axes are to be slewed, and extract their target
  // positions.

  if(OPTION_HAS_VALUE(vaz)) {
    rtc.cmd.slew_rate.mask |= gcp::util::Axis::AZ;
    rtc.cmd.slew_rate.az = UINT_VARIABLE(vaz)->uint;
  } else {
    rtc.cmd.slew_rate.az = 0;
  };

  if(OPTION_HAS_VALUE(vel)) {
    rtc.cmd.slew_rate.mask |= gcp::util::Axis::EL;
    rtc.cmd.slew_rate.el = UINT_VARIABLE(vel)->uint;
  } else {
    rtc.cmd.slew_rate.el = 0;
  };

  if(OPTION_HAS_VALUE(vdk)) {
    rtc.cmd.slew_rate.mask |= gcp::util::Axis::PA;
    rtc.cmd.slew_rate.dk = UINT_VARIABLE(vdk)->uint;
  } else {
    rtc.cmd.slew_rate.dk = 0;
  };
  
  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_SLEW_RATE_CMD))
    return 1;

  return 0;
}

/*-----------------------------------------------------------------------*
 * GPIB access functions
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Implement the command that sends a specified command string to a
 * GPIB device.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        dev    -  The GPIB address to target.
 *                        cmd    -  The command string to send to the
 *                                  GPIB device.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_gpib_send_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vdev;                /* The target GPIB device */
  Variable *vcmd;                /* The command string to be sent */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  /*
   * Do we have a controller to send the command to?
   */
  if(rtc_offline(sc, "gpib_send"))
    return 1;
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vdev, &vcmd, NULL))
    return 1;
  /*
   * Compose the real-time controller network command.
   */
  rtc.cmd.gpib_send.device = UINT_VARIABLE(vdev)->uint;
  strcpy(rtc.cmd.gpib_send.message, STRING_VARIABLE(vcmd)->string);
  /*
   * Send the command to the real-time controller.
   */
  if(queue_rtc_command(cp, &rtc, NET_GPIB_SEND_CMD))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that requests that a message be read from a
 * given GPIB device.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        dev    -  The GPIB address to listen to.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_gpib_read_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vdev;                /* The target GPIB device */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task */
  /*
   * Do we have a controller to send the command to?
   */
  if(rtc_offline(sc, "gpib_read"))
    return 1;
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vdev, NULL))
    return 1;
  /*
   * Compose the real-time controller network command.
   */
  rtc.cmd.gpib_read.device = UINT_VARIABLE(vdev)->uint;
  /*
   * Send the command to the real-time controller.
   */
  if(queue_rtc_command(cp, &rtc, NET_GPIB_READ_CMD))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that controls what feature markers are to be
 * recorded with subsequent archive frames.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        FeatureChange what  - What to do with the
 *                                              specified set of features.
 *                        Features features   - The chosen set of features.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_mark_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vmode;               /* What to do with the feature set */
  Variable *vfeatures;           /* The set of features */
  RtcNetCmd rtc;                 /* The network object to be sent to the */
                                 /*  real-time controller task. */
  /*
   * Do we have a controller to send the command to?
   */
  if(rtc_offline(sc, "mark"))
    return 1;
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vmode, &vfeatures, NULL))
    return 1;
  /*
   * Compose the network command that is to be sent to the real-time
   * controller.
   */
  rtc.cmd.feature.seq  = schNextSeq(cp_Scheduler(cp), 
				    TransactionStatus::TRANS_MARK);
  rtc.cmd.feature.mode = CHOICE_VARIABLE(vmode)->choice;
  rtc.cmd.feature.mask = SET_VARIABLE(vfeatures)->set;
  /*
   * Send the command to the real-time controller.
   */
  if(queue_rtc_command(cp, &rtc, NET_FEATURE_CMD))
    return 1;
  return 0;
}

/*.......................................................................
 * Implement the command that forces the current frame to be written,
 * and new frame to be started
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        FeatureChange what  - What to do with the
 *                                              specified set of features.
 *                        Features features   - The chosen set of features.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_newFrame_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */

  ArchiverMessage msg;

  if(pack_archiver_newFrame(&msg, schNextSeq(cp_Scheduler(cp), 
					     TransactionStatus::TRANS_FRAME)) ||
     send_ArchiverMessage(cp, &msg, PIPE_WAIT) == PIPE_ERROR)
    return 1;

  return 0;
}

/*-----------------------------------------------------------------------*
 * Host environment commands and functions                               *
 *-----------------------------------------------------------------------*/

/*.......................................................................
 * Change the current working directory of the process.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of optional command-line arguments:
 *                        Dir path  -  The pathname of the new working
 *                                     directory.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_cd_cmd)
{
  Variable *vpath;               /* The path of the new directory */
  /*
   * Get the command-line arguments.
   */
  if(get_Arguments(args, &vpath, NULL))
    return 1;
  /*
   * Attempt to change the directory.
   */
  if(set_working_directory(STRING_VARIABLE(vpath)->string))
    return 1;
  return 0;
}

/*.......................................................................
 * Display the current working directory.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of optional command-line arguments:
 *                        (none)
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_pwd_cmd)
{
  /*
   * Get the path name.
   */
  char *path = get_working_directory();
  if(!path)
    return 1;
  /*
   * Display the pathname to the user.
   */
  lprintf(stdout, "%s\n", path);
  /*
   * Discard the redundant copy of the path name.
   */
  free(path);
  return 0;
}

/*.......................................................................
 * Add a variable containing the name of the current host to the current
 * scripting environment.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  name         char *  The name to give the variable.
 * Output:
 *  return   Variable *  The new variable, or NULL on error.
 */
static Variable *add_hostname_variable(Script *sc, char *name)
{
  Variable *var;   /* The new variable */
  char *host;      /* The name of the host computer */
  /*
   * Lookup the hostname datatype.
   */
  DataType *dt = find_DataType(sc, NULL, "Hostname");
  if(!dt)
    return NULL;
  /*
   * Get the name of the current host.
   */
  host = get_name_of_host();
  if(!host)
    return NULL;
  /*
   * Create the variable.
   */
  var = add_BuiltinVariable(sc, "Hostname hostname");
  if(!var)
    return NULL;
  /*
   * Allocate a copy of this string from the string segment of the program.
   */
  STRING_VARIABLE(var)->string = new_ScriptString(sc, host);
  free(host);
  if(!STRING_VARIABLE(var)->string)
    return NULL;
  /*
   * Mark the variable as having a constant value.
   */
  var->flags = VAR_IS_CONST;
  return var;
}

/**.......................................................................
 * Implement the command that controls the optical camera stepper motor,
 * camera and camera controls.
 */
static CMD_FN(sc_grabFrame_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vchan;               // The desired channel
  Variable *vptel;               // The desired telescope
  RtcNetCmd rtc;                 /* The network object to be sent to the 
				    real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "grabFrame"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vchan, &vptel, NULL))
    return 1;

  // Get the channel, if any, that was specified
  
  unsigned channelMask;
  GET_CHANNEL_MASK(channelMask);

  Scheduler *sch = (Scheduler* )cp_ThreadData((ControlProg *)sc->project, 
						    CP_SCHEDULER);

  return sch_grab_send(sch, channelMask);
}

/**.......................................................................
 * Implement the command that finds the centroid of an image and
 * offsets the telescope to that position
 */
static CMD_FN(sc_center_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vchan;               // The desired channel
  Variable *vptel;               // The desired telescope
  RtcNetCmd rtc;                 /* The network object to be sent to the 
				    real-time controller task */
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "tv_offset"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vchan, &vptel, NULL))
    return 1;

  unsigned channelMask;
  GET_SINGLE_CHANNEL(channelMask);

  gcp::util::Angle xoff, yoff;
  unsigned ipeak, jpeak;

  // Get the offsets from the grabber thread. (in degrees)
  
  try {
    grabber_offset_info(cp, xoff, yoff, ipeak, jpeak, channelMask);
  } catch(gcp::util::Exception& err) {
    lprintf(stderr, "%s\n", err.what());
    return 1;
  }

  sendPeakOffsets(cp, ipeak, jpeak, channelMask);
  
  // Compose the real-time controller network command.
  
  rtc.cmd.tv_offset.seq   = schNextSeq(cp_Scheduler(cp), 
				       TransactionStatus::TRANS_PMAC);

  rtc.cmd.tv_offset.right = (int)(xoff.mas());
  rtc.cmd.tv_offset.up    = (int)(yoff.mas());
  
  rtc.antennas = cp_AntSet(cp)->getId();
  
  lprintf(stdout, "(CENTER) Sending tv offset command with: "
	  "right = %+d (mas)\n", rtc.cmd.tv_offset.right);

  lprintf(stdout, "(CENTER) Sending tv offset command with: "
	  "up    = %+d (mas)\n", rtc.cmd.tv_offset.up);
  
  // Send the command to the real-time controller.
  
  if(queue_rtc_command(cp, &rtc, NET_TV_OFFSET_CMD))
    return 1;

  return 0;
}

/**.......................................................................
 * Implement the command that configures the frame grabber
 */
static CMD_FN(sc_configureFrameGrabber_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  

  Variable *vchan;      // The mask of channels to which this command
			// should be applied
  Variable *vptel;      // Alternately, the mask of telescopes to
			// which this command should be applied
  Variable *vcombine;   // The number of images to combine 
  Variable *vflatfield; // Flatfield the image? 
  Variable *vinterval;  // Interval for auto-grab 
  RtcNetCmd rtc;        // The network command object to be sent to
			// the real-time controller task
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "configureFrameGrabber"))
    return 1;

  // Get the command-line arguments.
  
  if(get_Arguments(args, &vcombine, &vflatfield, &vinterval, &vchan, &vptel, 
		   NULL))
    return 1;
  
  // Compose a network object to be sent to the real-time controller
  // task.
  
  rtc.cmd.configureFrameGrabber.mask    = FG_NONE;
  
  // Get the channels, if any, that were specified

  unsigned channelMask;
  GET_CHANNEL_MASK(channelMask);

  rtc.cmd.configureFrameGrabber.channelMask = channelMask;

  //------------------------------------------------------------
  // See if a combine rate was specified
  //------------------------------------------------------------

  if(OPTION_HAS_VALUE(vcombine)) {
    rtc.cmd.configureFrameGrabber.nCombine = UINT_VARIABLE(vcombine)->uint;
    rtc.cmd.configureFrameGrabber.mask    |= FG_COMBINE;
    
    // Forward to the grabber thread

    if(setGrabberCombine(cp, rtc.cmd.configureFrameGrabber.nCombine,
			 rtc.cmd.configureFrameGrabber.channelMask))
      return 1;
  }

  //------------------------------------------------------------
  // See if a flatfield type was specified
  //------------------------------------------------------------

  if(OPTION_HAS_VALUE(vflatfield)) {

    rtc.cmd.configureFrameGrabber.flatfield = 
      CHOICE_VARIABLE(vflatfield)->choice;

    rtc.cmd.configureFrameGrabber.mask     |= FG_FLATFIELD;

    // Forward to the grabber thread

    if(setGrabberFlatfieldType(cp, rtc.cmd.configureFrameGrabber.flatfield,
			       rtc.cmd.configureFrameGrabber.channelMask))
      return 1;
  }

  //------------------------------------------------------------
  // See if an interval was specified
  //------------------------------------------------------------

  if(OPTION_HAS_VALUE(vinterval)) {
    rtc.cmd.configureFrameGrabber.seconds =
      (unsigned)floor(DOUBLE_VARIABLE(vinterval)->d);

    rtc.cmd.configureFrameGrabber.mask     |= FG_INTERVAL;
  }

  // Queue the object to be sent to the controller.

  return queue_rtc_command(cp, &rtc, NET_CONFIGURE_FG_CMD);
}

/**.......................................................................
 * Implement the command that configures a search box
 */
static CMD_FN(sc_addSearchBox_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  
  Variable* vxmin;
  Variable* vymin;
  Variable* vxmax;
  Variable* vymax;
  Variable* vinclude;
  Variable* vchan;      // The mask of channels to which this command
			// should be applied
  Variable* vptel;      // Alternately, the mask of telescopes to
			// which this command should be applied

  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "addSearchBox"))
    return 1;

  // Get the command-line arguments.
  
  if(get_Arguments(args, &vxmin, &vymin, &vxmax, &vymax, &vinclude, 
		   &vchan, &vptel, NULL))
    return 1;
  
  // Get the channels, if any, that were specified

  unsigned channelMask;
  GET_CHANNEL_MASK(channelMask);

  // Forward to the grabber thread
    
  if(setFrameGrabberSearchBox(cp, 
			      UINT_VARIABLE(vxmin)->uint, 
			      UINT_VARIABLE(vymin)->uint, 
			      UINT_VARIABLE(vxmax)->uint,
			      UINT_VARIABLE(vymax)->uint,
			      BOOL_VARIABLE(vinclude)->boolvar,
			      channelMask))
    return 1;

  return 0;
}

/**.......................................................................
 * Implement the command that deletes a search box
 */
static CMD_FN(sc_remSearchBox_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  
  Variable* vx;
  Variable* vy;
  Variable* vchan;      // The mask of channels to which this command
			// should be applied
  Variable* vptel;      // Alternately, the mask of telescopes to
			// which this command should be applied

  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "remSearchBox"))
    return 1;

  // Get the command-line arguments.
  
  if(get_Arguments(args, &vx, &vy, &vchan, &vptel, NULL))
    return 1;
  
  // Get the channels, if any, that were specified

  unsigned channelMask;
  GET_CHANNEL_MASK(channelMask);

  // Forward to the grabber thread
    
  if(OPTION_HAS_VALUE(vx) && OPTION_HAS_VALUE(vy)) {

    if(remFrameGrabberSearchBox(cp, UINT_VARIABLE(vx)->uint, 
				UINT_VARIABLE(vy)->uint,
				channelMask))
      return 1;

  } else  if(!OPTION_HAS_VALUE(vx) && !OPTION_HAS_VALUE(vy)) {

    if(remAllFrameGrabberSearchBoxes(cp, channelMask))
      return 1;

  } else {

    lprintf(stderr, "You must either specify a complete location (ix, iy), "
	    "to delete a specific box, or no location, to delete all boxes\n");
    return 1;

  }

  return 0;
}

/*.......................................................................
 * Implement the command that takes and stores a flatfield frame
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         register -  The name of the register to write to.
 *                         val      -  The value to write.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_takeFlatfield_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable *vchan;  // The desired channel
  Variable *vptel;  // The desired telescope
  RtcNetCmd rtc;    // The network command object to be sent to the 
                    //  real-time controller task 
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "takeFlatfield"))
    return 1;

  // Get the command-line arguments.

  if(get_Arguments(args, &vchan, &vptel, NULL))
    return 1;

  unsigned channelMask;
  GET_CHANNEL_MASK(channelMask);

  // Compose a network object to be sent to the real-time controller
  // task.
  
  rtc.cmd.configureFrameGrabber.mask = FG_TAKE_FLATFIELD;
  rtc.cmd.configureFrameGrabber.channelMask = channelMask;

  // Queue the object to be sent to the controller.

  return queue_rtc_command(cp, &rtc, NET_CONFIGURE_FG_CMD);
}

/*.......................................................................
 * Implement the command that sets the optical camera fov
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         register -  The name of the register to write to.
 *                         val      -  The value to write.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_setOpticalCameraFov_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  
  Variable* vfov;  // The FOV, in arcminutes
  Variable* vchan;
  Variable* vptel;

  RtcNetCmd rtc;   // The network object to be sent to the real-time
		   // controller task


  // Get the command-line arguments.
  
  if(get_Arguments(args, &vfov, &vchan, &vptel, NULL))
    return 1;
  
  // Get the channel, if any, that was specified

  unsigned chanMask;
  GET_CHANNEL_MASK(chanMask);

  rtc.cmd.configureFrameGrabber.channelMask = chanMask;
    
  gcp::util::Angle angle(gcp::util::Angle::ArcMinutes(), 
			 DOUBLE_VARIABLE(vfov)->d);

  if(OPTION_HAS_VALUE(vfov)) {
    return setOpticalCameraFov(cp, angle, chanMask);
  } else {
    return setOpticalCameraFov(cp, chanMask);
  }

  return 0;
}

/*.......................................................................
 * Implement the command that sets the optical camera aspect
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         register -  The name of the register to write to.
 *                         val      -  The value to write.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_setOpticalCameraAspect_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable* vaspect;       /* The ASPECT */
  Variable* vchan;
  Variable* vptel;

  // Get the command-line arguments.
  
  if(get_Arguments(args, &vaspect, &vchan, &vptel, NULL))
    return 1;
 
  unsigned chanMask;
  GET_CHANNEL_MASK(chanMask);

  if(OPTION_HAS_VALUE(vaspect)) {
    return setOpticalCameraAspectRatio(cp, 
				       DOUBLE_VARIABLE(vaspect)->d, chanMask);
  } else {
    return setOpticalCameraAspectRatio(cp, chanMask);
  }

  return 0;
}

/*.......................................................................
 * Implement the command that sets the optical camera collimation
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         register -  The name of the register to write to.
 *                         val      -  The value to write.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_setOpticalCameraCollimation_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable* vcollimation;       // The COLLIMATION, in degrees
  Variable* vchan;
  Variable* vptel;

  // Get the command-line arguments.
  
  if(get_Arguments(args, &vcollimation, &vchan, &vptel, NULL))
    return 1;
  
  unsigned chanMask;
  GET_CHANNEL_MASK(chanMask);

  gcp::util::Angle angle(gcp::util::Angle::Degrees(), 
			 DOUBLE_VARIABLE(vcollimation)->d);

  if(OPTION_HAS_VALUE(vcollimation)) {
    return setOpticalCameraRotationAngle(cp, angle, chanMask);
  } else {
    return setOpticalCameraRotationAngle(cp, chanMask);
  }

  return 0;
}

/*.......................................................................
 * Implement the command that sets the X image direction
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         register -  The name of the register to write to.
 *                         val      -  The value to write.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_setOpticalCameraXImDir_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;
  Variable* vimdir;
  Variable* vchan;
  Variable* vptel;

  // Get the command-line arguments.
  
  if(get_Arguments(args, &vimdir, &vchan, &vptel, NULL))
    return 1;
  
  unsigned chanMask;
  GET_CHANNEL_MASK(chanMask);

  return setOpticalCameraXImDir(cp, (ImDir)CHOICE_VARIABLE(vimdir)->choice, 
				chanMask);
}

/*.......................................................................
 * Implement the command that sets the Y image direction
 */
static CMD_FN(sc_setOpticalCameraYImDir_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; 
  Variable* vimdir;
  Variable* vchan;
  Variable* vptel;

  // Get the command-line arguments.
  
  if(get_Arguments(args, &vimdir, &vchan, &vptel, NULL))
    return 1;
  
  unsigned chanMask;
  GET_CHANNEL_MASK(chanMask);

  return setOpticalCameraYImDir(cp, (ImDir)CHOICE_VARIABLE(vimdir)->choice, 
				chanMask);
}

/*.......................................................................
 * Implement the command that sets the X image direction
 */
static CMD_FN(sc_setDeckAngleRotationSense_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable* vrot;
  Variable* vchan;
  Variable* vptel;

  // Get the command-line arguments.
  
  if(get_Arguments(args, &vrot, &vchan, &vptel, NULL))
    return 1;
  
  unsigned chanMask;
  GET_CHANNEL_MASK(chanMask);

  return setDeckAngleRotationSense(cp, 
				   (RotationSense)CHOICE_VARIABLE(vrot)->choice,
				   chanMask);
}

/*.......................................................................
 * Implement a function that returns the peak offsets of the current frame
 * grabber image.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          source -  The source to investigate.
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_peak_fn)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable *vchan;  // The desired channel
  Variable *vptel;  // The desired telescope
  Variable *vpeak;    /* The source variable */
  gcp::util::Angle xoff, yoff;  /* The offset of the peak of the fg image */
  unsigned ipeak, jpeak;

  // Get the command-line arguments.

  if(get_Arguments(args, &vpeak, &vchan, &vptel, NULL))
    return 1;
  
  // Get the requested channel

  unsigned channelMask;
  GET_SINGLE_CHANNEL(channelMask);

  // Get the offsets from the grabber thread. (in mas)

  try {
    grabber_offset_info(cp, xoff, yoff, ipeak, jpeak, channelMask);
  } catch(gcp::util::Exception& err) {
    lprintf(stderr, "%s\n", err.what());
    return 1;
  }

  // Return the requested offset in degrees (to be consistent with the
  // PointingOffset data type.

  switch (CHOICE_VARIABLE(vpeak)->choice) {
  case PEAK_X:
    DOUBLE_VARIABLE(result)->d = xoff.degrees();
    break;
  case PEAK_Y:
    DOUBLE_VARIABLE(result)->d = yoff.degrees();
    break;
  case PEAK_XABS:
    DOUBLE_VARIABLE(result)->d = fabs(xoff.degrees());
    break;
  case PEAK_YABS:
    DOUBLE_VARIABLE(result)->d = fabs(yoff.degrees());
    break;
  default:
    break;
  }

  return 0;
}

/*.......................................................................
 * Implement a function that returns the snr of the peak of the current frame
 * grabber image.
 *
 * Input:
 *  sc           Script *  The host scripting environment.
 *  args   VariableList *  The list of command-line arguments:
 *                          source -  The source to investigate.
 * Input/Output:
 *  result     Variable *  The return value.
 *  state      Variable *  Unused.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static FUNC_FN(sc_imstat_fn)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable* vpeak;    /* The source variable */
  Variable* vchan;
  Variable* vptel;
  double snr;         /* The SNR of the peak of the fg image */
  double peak;         /* The peak pixel value */
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vpeak, &vchan, &vptel, NULL))
    return 1;
  
  // Get the requested channel

  unsigned channelMask;
  GET_SINGLE_CHANNEL(channelMask);

  try {
    grabber_peak_info(cp, peak, snr, channelMask);
  } catch(gcp::util::Exception& err) {
    lprintf(stderr, "%s\n", err.what());
    return 1;
  }
  
  // Return the requested statistic as a double.

  switch (CHOICE_VARIABLE(vpeak)->choice) {
  case IMSTAT_SNR:
    DOUBLE_VARIABLE(result)->d = snr;
    break;
  case IMSTAT_PEAK:
    DOUBLE_VARIABLE(result)->d = peak;
    break;
  default:
    break;
  }

  return 0;
}

/**.......................................................................
 * Implement the command to set the default antenna set.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *
 *                        state - on|off
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_turnPower_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  Variable *vbreaker; // Which breaker
  Variable *vstate;   // on/off?
  
  RtcNetCmd rtc;    // The network object to be sent to the real-time
		    // controller task
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "turnPower"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vstate, &vbreaker, NULL))
    return 1;
  
  // Turn the power on/off?

  rtc.cmd.power.power = CHOICE_VARIABLE(vstate)->choice == SWITCH_ON;

  // Record the breaker to cycle.  Default to outlet 0 (all outlets)
  // if none was specified

  if(OPTION_HAS_VALUE(vbreaker))
    rtc.cmd.power.breaker = INT_VARIABLE(vbreaker)->i;
  else {
    lprintf(stderr, "You must specify an outlet.\n");
    return 1;
  }

  // Default to all antennas (even if you only have one)
  rtc.antennas = cp_AntSet(cp)->getId();

  return queue_rtc_command(cp, &rtc, NET_POWER_CMD);
}

/**.......................................................................
 * Implement the command to set the default antenna set.
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *
 *                        state - on|off
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_setDefaultAntennas_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  // Get the resource object of the parent thread.

  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
  Variable *vant;   // Antennas
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vant, NULL))
    return 1;
  
  // Install these antennas as the default

  cp_AntSet(cp)->set((gcp::util::AntNum::Id)SET_VARIABLE(vant)->set);

  // And queue a message to all connected clients that the selection
  // has changed

  sch_send_antenna_selection(sch, NULL);

  return 0;
}

/*.......................................................................
 * Implement the command that sets the Ebox equilibrium mode
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                        Receivers rx      -  The set of target receivers.
 *                        Stages stages     -  The set of target stages.
 *                        SwitchState state -  The desired switch state.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_atmosphere_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
						   resource object */
  Variable* vtemp;     // The air temperature
  Variable* vhumidity; // The humidity
  Variable* vpressure; // The pressure
  Variable* vant;
  RtcNetCmd rtc;   // The network object to be sent to
		   // the real-time controller task
                    
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "atmosphere"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vtemp, &vhumidity, &vpressure, &vant, NULL))
    return 1;
  
  // Compose the real-time controller network command.

  rtc.cmd.atmos.temperatureInK = DOUBLE_VARIABLE(vtemp)->d;
  rtc.cmd.atmos.humidityInMax1 = DOUBLE_VARIABLE(vhumidity)->d;
  rtc.cmd.atmos.pressureInMbar = DOUBLE_VARIABLE(vpressure)->d;

  // Parse optional antenna argument here.  If no argument was given,
  // use the default antenna set

  rtc.antennas  = 
    OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set : cp_AntSet(cp)->getId();

  // Send the command to the real-time controller.

  if(queue_rtc_command(cp, &rtc, NET_ATMOS_CMD))
    return 1;

  return 0;
}

/**.......................................................................
 * Implement the command to set an antenna location
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *
 *                        state - on|off
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_setAntennaLocation_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object
  Variable *veast;  // East
  Variable *vnorth; // North
  Variable *vup;    // Up
  Variable *vant;   // Antennas
  RtcNetCmd rtc;    // The network object to be sent to the real-time
		    // controller task
  
  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "setAntennaLocation"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vup, &veast, &vnorth, &vant, NULL))
    return 1;
  
  // Set the location

  rtc.cmd.location.up    = DOUBLE_VARIABLE(vup)->d;
  rtc.cmd.location.east  = DOUBLE_VARIABLE(veast)->d;
  rtc.cmd.location.north = DOUBLE_VARIABLE(vnorth)->d;

  // Default to all antennas if none was given.

  rtc.antennas = OPTION_HAS_VALUE(vant) ? SET_VARIABLE(vant)->set :
    cp_AntSet(cp)->getId();

  return queue_rtc_command(cp, &rtc, NET_LOCATION_CMD);
}

/**.......................................................................
 * Generate auto documentation
 */
static CMD_FN(sc_autoDoc_cmd)
{
  Variable *vdir;      /* The archiving directory */

  // Get the command-line arguments.

  if(get_Arguments(args, &vdir, NULL))
    return 1;
  
  // Send the requested configuration messages to the archiver.

  generateAutoDocumentation(sc, OPTION_HAS_VALUE(vdir) ? 
			    STRING_VARIABLE(vdir)->string : ".");

  return 0;
}

/**.......................................................................
 * Implement the command that configures the dead-man timeout
 */
static CMD_FN(sc_configureCmdTimeout_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
                                                   resource object */
  Variable* vstate;    // Turn the timeout on/off
  Variable* vint; // The timeout interval

  // Get the command-line arguments.

  if(get_Arguments(args, &vstate, &vint, NULL))
    return 1;

  // If an interval was specified, set the timeout, in seconds

  if(OPTION_HAS_VALUE(vint))
    configureCmdTimeout(cp,(unsigned int)floor(DOUBLE_VARIABLE(vint)->d + 0.5));

  // If a request to de/activate the timeout was received, do it now

  if(OPTION_HAS_VALUE(vstate)) {
    if(CHOICE_VARIABLE(vstate)->choice == SWITCH_ON) {
      configureCmdTimeout(cp, true);
    } else {
      configureCmdTimeout(cp, false);
    }
  }

  return 0;
}

/**.......................................................................
 * Implement the command that configures the data dead-man timeout
 */
static CMD_FN(sc_configureDataTimeout_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; /* The control-program
                                                   resource object */
  Variable* vstate;    // Turn the timeout on/off
  Variable* vint; // The timeout interval

  // Get the command-line arguments.

  if(get_Arguments(args, &vstate, &vint, NULL))
    return 1;

  // If an interval was specified, set the timeout, in seconds

  if(OPTION_HAS_VALUE(vint))
    configureDataTimeout(cp,(unsigned int)floor(DOUBLE_VARIABLE(vint)->d + 0.5));

  // If a request to de/activate the timeout was received, do it now

  if(OPTION_HAS_VALUE(vstate)) {
    if(CHOICE_VARIABLE(vstate)->choice == SWITCH_ON) {
      configureDataTimeout(cp, true);
    } else {
      configureDataTimeout(cp, false);
    }
  }

  return 0;
}

/*.......................................................................
 * Implement the command that sets a register on which to page
 */
static CMD_FN(sc_addPagerRegister_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// // resource object

  Variable* vReg=0;        // The register which should activate the
                           // pager

  Variable* vMin=0;        // The min

  Variable* vMax=0;        // The max

  Variable* vNframe=0;     // The number of frames before this
                           // condition should activate the pager

  Variable* vDelta=0;      // True if this condition should be applied
                           // to the delta, instead of the value.

  Variable* vOutOfRange=0; // True if the pager should be activated
                           // when the value falls out of the
                           // specified range.  False if
  Variable* vScript=0;     // A script to execute if this register
			   // goes out of range

  // Get the command-line arguments.      

  if(get_Arguments(args, &vReg, &vMin, &vMax, &vNframe, &vDelta, 
		   &vOutOfRange, &vScript, NULL))
    return 1;

  // Get the register specification                                                                                 

  char*  reg      = STRING_VARIABLE(vReg)->string;
  double min      = DOUBLE_VARIABLE(vMin)->d;
  double max      = DOUBLE_VARIABLE(vMax)->d;

  bool delta =
    OPTION_HAS_VALUE(vDelta) ? BOOL_VARIABLE(vDelta)->boolvar : false;

  unsigned nFrame = 
    OPTION_HAS_VALUE(vNframe) ? UINT_VARIABLE(vNframe)->uint : 2;

  bool outOfRange = OPTION_HAS_VALUE(vOutOfRange) ? 
    BOOL_VARIABLE(vOutOfRange)->boolvar : true;

  return sendAddPagerRegisterMsg(cp, reg, min, max, delta, nFrame, outOfRange,
				 OPTION_HAS_VALUE(vScript) ? STRING_VARIABLE(vScript)->string : "");
}

/*.......................................................................
 * Implement the command that sets a register on which to page
 *
 * Input:
 *
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *
 *                       Optional arguments:
 * Output:
 *
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static CMD_FN(sc_remPagerRegister_cmd)
{
  ControlProg *cp = (ControlProg* )sc->project; // The control-program
						// resource object

  Variable *vReg=0;        // The register which should activate the
			   // pager

  // Get the command-line arguments.

  if(get_Arguments(args, &vReg, NULL))
    return 1;
  
  // Get the register specification

  char*  reg      = STRING_VARIABLE(vReg)->string;

  return sendRemPagerRegisterMsg(cp, reg);
}

/*.......................................................................
 * Implement the command that returns the value of a specified SZA register
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         reg  -  The register specification.
 *                         val  -  The register value.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static FUNC_FN(sc_regVal_fn)
{
  ControlProg *cp = (ControlProg* )sc->project;  /* The context of the
						    host control
						    program */
  Variable *vreg;      /* The register specification argument */
  double val;          /* The value of the register */

  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "regVal"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vreg, NULL))
    return 1;

  try {

    static gcp::util::RegParser parser(false);

    std::string regSpec(STRING_VARIABLE(vreg)->string);
    gcp::util::RegDescription regDesc = parser.inputReg(regSpec);    

    // Get the value of the requested register from the last frame
    // buffer.
    
    DOUBLE_VARIABLE(result)->d = getRegVal(cp_Archiver(cp), regDesc);

  } catch(gcp::util::Exception& err) {
    lprintf(stderr, "%s\n", err.what());
    return 1;
  } catch(...) {
    lprintf(stderr, "Caught an unknown exception\n");
    return 1;
  }

  return 0;
}

/*.......................................................................
 * Implement the command that converts from an integer to a string
 *
 * Input:
 *  sc         Script *  The host scripting environment.
 *  args VariableList *  The list of command-line arguments:
 *                         reg  -  The register specification.
 *                         val  -  The register value.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
static FUNC_FN(sc_intToString_fn)
{
  Variable *vint;      /* The register specification argument */

  // Do we have a controller to send the command to?

  if(rtc_offline(sc, "intToString"))
    return 1;
  
  // Get the command-line arguments.

  if(get_Arguments(args, &vint, NULL))
    return 1;

  try {

    std::ostringstream os;
    os << UINT_VARIABLE(vint)->uint;

    // Allocate a copy of the string from the string-segment of the
    // program.

    char* s = new_ScriptString(sc, (char*)os.str().c_str());

    if(!s)
      return 1;

    STRING_VARIABLE(result)->string = s;

  } catch(gcp::util::Exception& err) {
    lprintf(stderr, "%s\n", err.what());
    return 1;
  } catch(...) {
    lprintf(stderr, "Caught an unknown exception\n");
    return 1;
  }

  return 0;
}

static CMD_FN(sc_assignFrameGrabberChannel_cmd)
{
  Variable* vchan;
  Variable* vptel;

   // Do we have a controller to send the command to?

  if(rtc_offline(sc, "assignFrameGrabberChannel"))
    return 1;
  
  // Get the command-line arguments.
  
  if(get_Arguments(args, &vchan, &vptel, NULL))
    return 1;
  
  gcp::util::PointingTelescopes::Ptel ptel = 
    (gcp::util::PointingTelescopes::Ptel) SET_VARIABLE(vptel)->set;

  gcp::grabber::Channel::FgChannel        chan = 
    (gcp::grabber::Channel::FgChannel) SET_VARIABLE(vchan)->set;

  try {
    gcp::util::PointingTelescopes::assignFgChannel(ptel, chan);
  } catch (gcp::util::Exception& err) {
    lprintf(stderr, "%s\n", err.what());
    return 1;
  }

  // And forward the association to connected clients

  if(sendFgChannelAssignment((ControlProg* )sc->project, chan, ptel))
    return 1;

  return 0;
}

static CMD_FN(sc_setDefaultFrameGrabberChannel_cmd)
{
  Variable* vchan;

   // Do we have a controller to send the command to?

  if(rtc_offline(sc, "setDefaultFrameGrabberChannel"))
    return 1;
  
  // Get the command-line arguments.
  
  if(get_Arguments(args, &vchan, NULL))
    return 1;
  
  gcp::util::PointingTelescopes::
    setDefaultFgChannels((gcp::grabber::Channel::FgChannel)
			 SET_VARIABLE(vchan)->set);

  return 0;
}

static CMD_FN(sc_setDefaultPtel_cmd)
{
  Variable* vptel;

   // Do we have a controller to send the command to?

  if(rtc_offline(sc, "setDefaultPtel"))
    return 1;
  
  // Get the command-line arguments.
  
  if(get_Arguments(args, &vptel, NULL))
    return 1;
  
  try {

    gcp::util::PointingTelescopes::
      setDefaultPtels((gcp::util::PointingTelescopes::Ptel)
		      SET_VARIABLE(vptel)->set);
    
  } catch(gcp::util::Exception& err) {
    lprintf(stderr, "%s\n", err.what());
    return 1;
  }

  return 0;
}

static CMD_FN(sc_alias_cmd)
{
  Variable* vCmdName;
  Variable* vAlias;

  ControlProg *cp = (ControlProg* )sc->project;
  Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);

  // Get the command-line arguments.
  
  if(get_Arguments(args, &vCmdName, &vAlias, NULL))
    return 1;
  
  return add_UserAlias(sch, STRING_VARIABLE(vCmdName)->string, STRING_VARIABLE(vAlias)->string);
}
