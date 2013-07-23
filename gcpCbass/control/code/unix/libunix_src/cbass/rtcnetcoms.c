#define __FILEPATH__ "control/code/unix/libunix_src/common/rtcnetcoms.c"

#include <stddef.h>

#include "gcp/util/common/AntNum.h"

#include "genericregs.h"
#include "rtcnetcoms.h"

#define ARRAY_DIM(array) (sizeof(array)/sizeof(array[0]))

using namespace gcp::control;

/**-----------------------------------------------------------------------
 * Separately describe each of the RtcNetMsg objects in rtcnetcoms.h.
 */
static const NetObjMember greeting_msg_members[] = {
  {"revision", offsetof(NetGreetingMsg,revision), NET_INT, 1},
  {"nReg",     offsetof(NetGreetingMsg,nReg),     NET_INT, 1},
  {"nByte",    offsetof(NetGreetingMsg,nByte),    NET_INT, 1}
};

static const NetObjMember log_msg_members[] = {
  {"seq",    offsetof(NetLogMsg,seq),     NET_INT,   1},
  {"bad",    offsetof(NetLogMsg,bad),     NET_SHORT, 1},
  {"text",   offsetof(NetLogMsg,text),    NET_ASCII, NET_LOG_MAX+1},
  {"end",    offsetof(NetLogMsg,end),     NET_BOOL,  1},
};

static const NetObjMember caltert_done_msg_members[] = {
  {"seq",  offsetof(NetCalTertDoneMsg,seq), NET_INT, 1},
};

static const NetObjMember IFMod_done_msg_members[] = {
  {"seq",  offsetof(NetIFModDoneMsg,seq), NET_INT, 1},
};

static const NetObjMember can_done_msg_members[] = {
  {"seq",  offsetof(NetCanDoneMsg,seq), NET_INT, 1},
};

static const NetObjMember setreg_done_msg_members[] = {
  {"seq",  offsetof(NetSetregDoneMsg,seq), NET_INT, 1},
};

static const NetObjMember tv_offset_done_msg_members[] = {
  {"seq",  offsetof(NetTvOffsetDoneMsg,seq), NET_INT, 1},
};

static const NetObjMember drive_done_msg_members[] = {
  {"seq",      offsetof(NetDriveDoneMsg,seq),      NET_INT, 1},
};

static const NetObjMember bench_done_msg_members[] = {
  {"seq",      offsetof(NetBenchDoneMsg,seq),      NET_INT, 1},
};

static const NetObjMember scan_done_msg_members[] = {
  {"seq",      offsetof(NetScanDoneMsg,seq),      NET_INT, 1},
};

static const NetObjMember noise_done_msg_members[] = {
  {"seq",  offsetof(NetNoiseDoneMsg,seq), NET_INT, 1},
};

static const NetObjMember script_done_msg_members[] = {
  {"seq",  offsetof(NetScriptDoneMsg,seq), NET_INT, 1},
};

static const NetObjMember source_set_msg_members[] = {
  {"seq",  offsetof(NetSourceSetMsg,seq), NET_INT, 1},
};

/*
 * Collect all of the object definitions in an array.
 */
static const NetObjInfo msg_objects[] = {
  {"log",          log_msg_members,         ARRAY_DIM(log_msg_members),
   sizeof(NetLogMsg)},
  {"drive_done",    drive_done_msg_members,   ARRAY_DIM(drive_done_msg_members),
   sizeof(NetDriveDoneMsg)},
  {"bench_done",    bench_done_msg_members,   ARRAY_DIM(bench_done_msg_members),
   sizeof(NetBenchDoneMsg)},
  {"scan_done",    scan_done_msg_members,   ARRAY_DIM(scan_done_msg_members),
   sizeof(NetScanDoneMsg)},
  {"source_set",   source_set_msg_members,  ARRAY_DIM(source_set_msg_members),
   sizeof(NetSourceSetMsg)},
  {"setreg_done",  setreg_done_msg_members, ARRAY_DIM(setreg_done_msg_members),
   sizeof(NetSetregDoneMsg)},
  {"tv_offset_done",tv_offset_done_msg_members,
   ARRAY_DIM(tv_offset_done_msg_members), sizeof(NetTvOffsetDoneMsg)},
  {"id",           NULL,                    0,       0},
  {"frame_done",   NULL,                    0,       0},
  {"greeting",     greeting_msg_members,    ARRAY_DIM(greeting_msg_members),
   sizeof(NetGreetingMsg)},
  {"nav_update",   NULL,                    0,       0},
  {"caltert_done", caltert_done_msg_members,ARRAY_DIM(caltert_done_msg_members),
   sizeof(NetCalTertDoneMsg)},
  {"ifmod_done", IFMod_done_msg_members,ARRAY_DIM(IFMod_done_msg_members),
   sizeof(NetIFModDoneMsg)},
  {"can_done", can_done_msg_members,ARRAY_DIM(can_done_msg_members),
   sizeof(NetCanDoneMsg)},
  {"noise_done",    noise_done_msg_members,   ARRAY_DIM(noise_done_msg_members),
   sizeof(NetNoiseDoneMsg)},
  {"script_done",    script_done_msg_members,   ARRAY_DIM(script_done_msg_members),
   sizeof(NetScriptDoneMsg)},
};

/*
 * Form the global table of objects that is used by the functions
 * in netobj.h.
 */
const NetObjTable rtc_msg_table = {
  "msg", msg_objects, sizeof(msg_objects)/sizeof(NetObjInfo), sizeof(RtcNetMsg)
};

/*-----------------------------------------------------------------------
 * Separately describe each of the RtcNetCmd objects in rtcnetcoms.h.
 */
static const NetObjEnumTab shutdown_method[] = {
  {"hard_restart"},
  {"soft_restart"},
  {"hard_shutdown"},
  {"soft_shutdown"}
};
static const NetObjMember shutdown_cmd_members[] = {
  {"method",    offsetof(NetShutdownCmd,method),       NET_ENUM,     1,
   shutdown_method, ARRAY_DIM(shutdown_method)}
};

static const NetObjMember interval_cmd_members[] = {
  {"exponent",  offsetof(NetIntervalCmd, exponent),    NET_SHORT,    1},
};

static const NetObjMember inhibit_cmd_members[] = {
  {"flag",      offsetof(NetInhibitCmd, flag),         NET_BOOL,     1},
};

static const NetObjMember walshstate_cmd_members[] = {
  {"on",        offsetof(NetWalshStateCmd, on),        NET_BOOL,     1},
};

static const NetObjMember pager_cmd_members[] = {
  {"state",     offsetof(NetPagerCmd, state),          NET_ENUM,     1},
};

static const NetObjMember setreg_cmd_members[] = {
  {"value",     offsetof(NetSetregCmd, value),         NET_INT,     1},
  {"board",     offsetof(NetSetregCmd, board),         NET_SHORT,    1},
  {"block",     offsetof(NetSetregCmd, block),         NET_SHORT,    1},
  {"index",     offsetof(NetSetregCmd, index),         NET_SHORT,    1},
  {"nreg",      offsetof(NetSetregCmd, nreg),          NET_SHORT,    1},
  {"seq",       offsetof(NetSetregCmd, seq),           NET_INT,     1},
};

static const NetObjMember getreg_cmd_members[] = {
  {"board",     offsetof(NetGetregCmd, board),         NET_SHORT,    1},
  {"block",     offsetof(NetGetregCmd, block),         NET_SHORT,    1},
  {"index",     offsetof(NetGetregCmd, index),         NET_SHORT,    1},
};

static const NetObjMember setdio_cmd_members[] = {
  {"value",     offsetof(NetSetDioCmd, value),         NET_INT,     1},
  {"board",     offsetof(NetSetDioCmd, board),         NET_SHORT,    1},
  {"oper",      offsetof(NetSetDioCmd, oper),          NET_ENUM,     1},
};

static const NetObjMember unflag_cmd_members[] = {
  {"board",     offsetof(NetUnflagCmd, board),         NET_SHORT,    1},
};

static const NetObjMember phase_motor_cmd_members[] = {
  {"on",        offsetof(NetPhaseMotorCmd, on),        NET_BOOL,    1},
  {"receivers", offsetof(NetPhaseMotorCmd, receivers), NET_MASK,    1},
};

static const NetObjMember phase_shift_cmd_members[] = {
  {"seq",       offsetof(NetPhaseShiftCmd, seq),       NET_INT,    1},
  {"posn",      offsetof(NetPhaseShiftCmd, posn),      NET_INT,    1},
  {"half_step", offsetof(NetPhaseShiftCmd, half_step), NET_BOOL,    1},
  {"receivers", offsetof(NetPhaseShiftCmd, receivers), NET_MASK,    1},
};

static const NetObjMember rx_heater_cmd_members[] = {
  {"on",        offsetof(NetRxHeaterCmd, on),          NET_BOOL,    1},
  {"heaters",   offsetof(NetRxHeaterCmd, heaters),     NET_MASK,    1},
  {"receivers", offsetof(NetRxHeaterCmd, receivers),   NET_MASK,    1},
};

static const NetObjMember rx_polar_cmd_members[] = {
  {"receivers", offsetof(NetRxPolarCmd, receivers),    NET_MASK,    1},
  {"state",     offsetof(NetRxPolarCmd, state),        NET_ENUM,    1},
  {"posn",      offsetof(NetRxPolarCmd, posn),         NET_INT,    1},
  
};

static const NetObjMember rx_polwalsh_cmd_members[] = {
  {"seq",       offsetof(NetPolWalshCmd, seq),         NET_INT,    1},
  {"receivers", offsetof(NetPolWalshCmd, receivers),   NET_MASK,    1},
  {"half_step", offsetof(NetPolWalshCmd, half_step),   NET_INT,    1},
  {"walshstep", offsetof(NetPolWalshCmd, walshstep),   NET_INT,    1},
  
};

static const NetObjMember rx_coldhead_cmd_members[] = {
  {"on",        offsetof(NetRxColdheadCmd, on),        NET_BOOL,    1},
  {"receivers", offsetof(NetRxColdheadCmd, receivers), NET_MASK,    1},
};

static const NetObjMember rx_temp_cmd_members[] = {
  {"value",     offsetof(NetRxTempCmd, value),         NET_INT,    1},
  {"heaters",   offsetof(NetRxTempCmd, heaters),       NET_MASK,    1},
  {"receivers", offsetof(NetRxTempCmd, receivers),     NET_MASK,    1},
};

static const NetObjMember lo_cmd_members[] = {
  {"msgId",     offsetof(NetLoCmd, msgId),       NET_ENUM,    1},
  {"oscs",      offsetof(NetLoCmd, oscs),        NET_ENUM,    1},
  {"stages",    offsetof(NetLoCmd, stages),      NET_ENUM,    1},
  {"rxId",      offsetof(NetLoCmd, rxId),        NET_ENUM,    1},
  {"on",        offsetof(NetLoCmd, on),          NET_BOOL,    1},
  {"dampGain",  offsetof(NetLoCmd, dampGain),    NET_SHORT,   1},
  {"frequency", offsetof(NetLoCmd, frequency),   NET_SHORT,   1},
  {"loopGain",  offsetof(NetLoCmd, loopGain),    NET_SHORT,   1},
  {"voltage",   offsetof(NetLoCmd, voltage),     NET_SHORT,   1},
  {"id",        offsetof(NetLoCmd, id),          NET_BYTE,    1},
  {"day",       offsetof(NetLoCmd, day),         NET_BYTE,    1},
  {"month",     offsetof(NetLoCmd, month),       NET_BYTE,    1},
  {"year",      offsetof(NetLoCmd, year),        NET_BYTE,    1},
  {"coeff",     offsetof(NetLoCmd, coeff),       NET_FLOAT,   1},
};

static const NetObjMember fast_sampling_cmd_members[] = {
  {"channel", offsetof(NetFastSamplingCmd, channel),    NET_INT,  1},
  {"start",   offsetof(NetFastSamplingCmd, start),      NET_BOOL,  1},
};

static const NetObjMember set_antenna_coords_cmd_members[] = {
  {"x",         offsetof(NetSetAntennaCoordsCmd, x),         NET_DOUBLE,  1},
  {"y",         offsetof(NetSetAntennaCoordsCmd, y),         NET_DOUBLE,  1},
  {"z",         offsetof(NetSetAntennaCoordsCmd, z),         NET_DOUBLE,  1},
  {"longitude", offsetof(NetSetAntennaCoordsCmd, longitude), NET_DOUBLE,  1},
  {"latitude",  offsetof(NetSetAntennaCoordsCmd, latitude),  NET_DOUBLE,  1},
  {"axisMis",   offsetof(NetSetAntennaCoordsCmd, axisMis),   NET_DOUBLE,  1}
};

static const NetObjMember dds_cmd_members[] = {
  {"state",     offsetof(NetDDSCmd, state),        NET_ENUM,    1}
};

static const NetObjMember set_antenna_dds_cmd_members[] = {
  {"ddsId",     offsetof(NetSetAntennaDDSCmd, ddsId),        NET_INT,    1}
};

static const NetObjMember set_lr_phase_cmd_members[] = {
  {"input",     offsetof(NetSetLRPhaseCmd, input),        NET_INT,    1},
  {"phase",     offsetof(NetSetLRPhaseCmd, phase),        NET_DOUBLE,  1},
  {"type",      offsetof(NetSetLRPhaseCmd, type),         NET_ENUM,    1}
};

static const NetObjMember set_lr_freq_cmd_members[] = {
  {"input",     offsetof(NetSetLRFreqCmd, input),        NET_INT,    1},
  {"freq",      offsetof(NetSetLRFreqCmd, freq),         NET_DOUBLE,  1},
  {"type",      offsetof(NetSetLRFreqCmd, type),         NET_ENUM,    1}
};

static const NetObjMember enable_dds_walshing_cmd_members[] = {
  {"input",     offsetof(NetEnableDDSWalshingCmd, input),        NET_INT,    1},
  {"enable",    offsetof(NetEnableDDSWalshingCmd, enable),       NET_BOOL,    1},
};

static const NetObjMember set_dds_walsh_column_cmd_members[] = {
  {"input",    offsetof(NetSetDDSWalshColumnCmd, input),        NET_INT,    1},
  {"column",   offsetof(NetSetDDSWalshColumnCmd, column),       NET_INT,    1},
};

static const NetObjMember set_output_regs_cmd_members[] = {
  {"input",     offsetof(NetSetOutputRegsCmd, input),        NET_INT, 1},
  {"freg",      offsetof(NetSetOutputRegsCmd, freg),         NET_INT, 1},
  {"preg",      offsetof(NetSetOutputRegsCmd, preg),         NET_INT, 1}
};

static const NetObjMember set_lr_delay_cmd_members[] = {
  {"input",     offsetof(NetSetLRDelayCmd, input),      NET_INT,    1},
  {"delay",     offsetof(NetSetLRDelayCmd, delay),      NET_DOUBLE,  1},
  {"mjd",       offsetof(NetSetLRDelayCmd, mjd),        NET_DOUBLE,  1},
  {"disc",      offsetof(NetSetLRDelayCmd, disc),       NET_BOOL,    1},
};

static const NetObjMember set_antenna_phase_cmd_members[] = {
  {"phase",     offsetof(NetSetAntennaPhaseCmd, phase),  NET_DOUBLE,  1}
};

static const NetObjMember set_antenna_freq_cmd_members[] = {
  {"freq",     offsetof(NetSetAntennaFreqCmd, freq),   NET_DOUBLE,  1}
};

static const NetObjMember set_antenna_params_cmd_members[] = {
  {"pntRa",        offsetof(NetSetAntennaParamsCmd, pntRa),         NET_DOUBLE,  1},
  {"pntDec",       offsetof(NetSetAntennaParamsCmd, pntDec),        NET_DOUBLE,  1},
  {"phsRa",        offsetof(NetSetAntennaParamsCmd, phsRa),         NET_DOUBLE,  1},
  {"phsDec",       offsetof(NetSetAntennaParamsCmd, phsDec),        NET_DOUBLE,  1},
  {"freq",         offsetof(NetSetAntennaParamsCmd, freq),          NET_DOUBLE,  1},
  {"distance",     offsetof(NetSetAntennaParamsCmd, distance),      NET_DOUBLE,  1},
  {"mjd",          offsetof(NetSetAntennaParamsCmd, mjd),           NET_DOUBLE,  1},
  {"discontinuity",offsetof(NetSetAntennaParamsCmd, discontinuity), NET_BOOL,    1}
};

static const NetObjMember set_delay_cmd_members[] = {
  {"delay",     offsetof(NetSetDelayCmd, delay),     NET_DOUBLE,  1},
  {"delayType", offsetof(NetSetDelayCmd, delayType), NET_ENUM,    1},
};

static const NetObjMember set_weather_params_cmd_members[] = {
  {"airTemp",     offsetof(NetSetWeatherParamsCmd, airTemp),     NET_DOUBLE,  1},
  {"atmPressure", offsetof(NetSetWeatherParamsCmd, atmPressure), NET_DOUBLE,  1},
  {"dewPoint",    offsetof(NetSetWeatherParamsCmd, dewPoint),    NET_DOUBLE,  1},
  {"relHumid",    offsetof(NetSetWeatherParamsCmd, relHumid),    NET_DOUBLE,  1}
};

static const NetObjMember use_delay_cmd_members[] = {
  {"use",       offsetof(NetUseDelayCmd, use),       NET_BOOL,  1},
  {"delayType", offsetof(NetUseDelayCmd, delayType), NET_ENUM,  1},
};

static const NetObjMember fringe_tracking_cmd_members[] = {
  {"on",     offsetof(NetFringeTrackingCmd, on),          NET_BOOL,  1},
  {"target", offsetof(NetFringeTrackingCmd, target),      NET_ENUM,  1},
};

static const NetObjMember thermal_cmd_members[] = {
  {"msgId",  offsetof(NetThermalCmd, msgId),       NET_ENUM,  1},
  {"target", offsetof(NetThermalCmd, target),      NET_ENUM,  1},
  {"value",  offsetof(NetThermalCmd, value),       NET_FLOAT, 1},
  {"mode",   offsetof(NetThermalCmd, mode),        NET_ENUM,  1},
  {"state",  offsetof(NetThermalCmd, state),       NET_ENUM,  1},
};

static const NetObjMember setFilter_cmd_members[] = {
  {"mask",   offsetof(NetSetFilterCmd, mask),      NET_MASK,   1},
  {"freqHz", offsetof(NetSetFilterCmd, freqHz),    NET_DOUBLE, 1},
  {"ntaps",  offsetof(NetSetFilterCmd, ntaps),     NET_INT,    1},
};

static const NetObjMember runScript_cmd_members[] = {
  {"seq",    offsetof(NetRunScriptCmd, seq),     NET_INT,   1},
  {"script", offsetof(NetRunScriptCmd, script),  NET_ASCII, NET_LOG_MAX+1}
};

static const NetObjMember scriptDir_cmd_members[] = {
  {"dir",    offsetof(NetScriptDirCmd, dir),     NET_ASCII, NET_LOG_MAX+1}
};

static const NetObjMember scan_cmd_members[] = {
  {"name",      offsetof(NetScanCmd, name),           NET_ASCII,   SCAN_LEN},
  {"seq",       offsetof(NetScanCmd, seq),            NET_INT,    1},
  {"nreps",     offsetof(NetScanCmd, nreps),          NET_INT,    1},
  {"istart",    offsetof(NetScanCmd, istart),         NET_INT,    1},
  {"ibody",     offsetof(NetScanCmd, ibody),          NET_INT,    1},
  {"iend",      offsetof(NetScanCmd, iend),           NET_INT,    1},
  {"npt",       offsetof(NetScanCmd, npt),            NET_INT,    1},
  {"msPerSample",offsetof(NetScanCmd, msPerSample),   NET_INT,    1},
  {"index",     offsetof(NetScanCmd, index),          NET_INT,    SCAN_NET_NPT},
  {"flag",      offsetof(NetScanCmd, flag),           NET_INT,    SCAN_NET_NPT},
  {"azoff",     offsetof(NetScanCmd, azoff),          NET_INT,    SCAN_NET_NPT},
  {"eloff",     offsetof(NetScanCmd, eloff),          NET_INT,    SCAN_NET_NPT},  {"add",       offsetof(NetScanCmd, add),            NET_BOOL,    1},
};

static const NetObjMember rx_quad_cmd_members[] = {
  {"state",     offsetof(NetRxQuadCmd, state),         NET_ENUM,    1},
  {"receivers", offsetof(NetRxQuadCmd, receivers),     NET_MASK,    1},
};

static const NetObjMember site_cmd_members[] = {
  {"lon",       offsetof(NetSiteCmd, lon),             NET_DOUBLE,    1},
  {"lat",       offsetof(NetSiteCmd, lat),             NET_DOUBLE,    1},
  {"alt",       offsetof(NetSiteCmd, alt),             NET_DOUBLE,    1},
};

static const NetObjMember location_cmd_members[] = {
  {"north",     offsetof(NetLocationCmd, north),       NET_DOUBLE,    1},
  {"east",      offsetof(NetLocationCmd, east),        NET_DOUBLE,    1},
  {"up",        offsetof(NetLocationCmd, up),          NET_DOUBLE,    1},
};

static const NetObjMember delayref_cmd_members[] = {
  {"east",      offsetof(NetDelayRefCmd, east),        NET_DOUBLE,    1},
  {"north",     offsetof(NetDelayRefCmd, north),       NET_DOUBLE,    1},
  {"up",        offsetof(NetDelayRefCmd, up),          NET_DOUBLE,    1},
};

static const NetObjMember init_cmd_members[] = {
  {"start",     offsetof(NetInitCmd, start),           NET_BOOL,    1},
};

static const NetObjMember halt_cmd_members[] = {
  {"seq",       offsetof(NetHaltCmd, seq),             NET_INT,    1},
  {"antennas",  offsetof(NetHaltCmd, antennas),        NET_MASK,    1},
};

static const NetObjMember reboot_drive_cmd_members[] = {
  {"seq",       offsetof(NetRebootDriveCmd, seq),       NET_INT,    1},
};

static const NetObjMember slew_cmd_members[] = {
  {"source",    offsetof(NetSlewCmd, source),          NET_ASCII,   SRC_LEN},
  {"number",    offsetof(NetSlewCmd, number),          NET_INT,    1},
  {"seq",       offsetof(NetSlewCmd, seq),             NET_INT,    1},
  {"mask",      offsetof(NetSlewCmd, mask),            NET_MASK,    1},
  {"az",        offsetof(NetSlewCmd, az),              NET_DOUBLE,  1},
  {"el",        offsetof(NetSlewCmd, el),              NET_DOUBLE,  1},
  {"dk",        offsetof(NetSlewCmd, dk),              NET_DOUBLE,  1},
  {"type",      offsetof(NetSlewCmd, type),            NET_ENUM,    1},
  {"slewType",  offsetof(NetSlewCmd, slewType),        NET_ENUM,    1}
};

static const NetObjMember track_cmd_members[] = {
  {"source",    offsetof(NetTrackCmd, source),         NET_ASCII,   SRC_LEN},
  {"number",    offsetof(NetTrackCmd, number),         NET_INT,    1},
  {"srcType",   offsetof(NetTrackCmd, srcType),        NET_ENUM,    1},
  {"seq",       offsetof(NetTrackCmd, seq),            NET_INT,    1},
  {"mjd",       offsetof(NetTrackCmd, mjd),            NET_INT,    1},
  {"tt",        offsetof(NetTrackCmd, tt),             NET_INT,    1},
  {"ra",        offsetof(NetTrackCmd, ra),             NET_INT,    1},
  {"dec",       offsetof(NetTrackCmd, dec),            NET_INT,    1},
  {"dist",      offsetof(NetTrackCmd, dist),           NET_INT,    1},
  {"type",      offsetof(NetTrackCmd, type),           NET_ENUM,    1}
};

static const NetObjMember mount_offset_cmd_members[] = {
  {"seq",       offsetof(NetMountOffsetCmd, seq),      NET_INT,    1},
  {"axes",      offsetof(NetMountOffsetCmd, axes),     NET_MASK,    1},
  {"mode",      offsetof(NetMountOffsetCmd, mode),     NET_ENUM,    1},
  {"az",        offsetof(NetMountOffsetCmd, az),       NET_DOUBLE,  1},
  {"el",        offsetof(NetMountOffsetCmd, el),       NET_DOUBLE,  1},
  {"dk",        offsetof(NetMountOffsetCmd, dk),       NET_DOUBLE,  1},
};

static const NetObjMember tilt_meter_correction_cmd_members[] = {
  {"enable",    offsetof(NetTiltMeterCorrectionCmd, enable),  NET_BOOL,  1},
};

static const NetObjMember tilt_meter_offset_cmd_members[] = {
  {"x"     ,    offsetof(NetTiltMeterOffsetCmd, x),  NET_DOUBLE,  1},
  {"y"     ,    offsetof(NetTiltMeterOffsetCmd, y),  NET_DOUBLE,  1},
};

static const NetObjMember tilt_meter_angle_cmd_members[] = {
  {"angle",    offsetof(NetTiltMeterAngleCmd, angle),  NET_DOUBLE,  1},
};

static const NetObjMember tilt_meter_range_cmd_members[] = {
  {"maxAngle",    offsetof(NetTiltMeterRangeCmd, maxAngle),  NET_DOUBLE,  1},
};

static const NetObjMember tilt_meter_moving_average_interval_cmd_members[] = {
  {"interval",    offsetof(NetTiltMeterMovingAverageIntervalCmd, interval),  NET_DOUBLE,  1},
};

static const NetObjMember linear_sensor_correction_cmd_members[] = {
  {"enable",    offsetof(NetLinearSensorCorrectionCmd, enable),  NET_BOOL,  1},
};

static const NetObjMember linear_sensor_offset_cmd_members[] = {
  {"L1"     ,    offsetof(NetLinearSensorOffsetCmd, L1),  NET_DOUBLE,  1},
  {"L2"     ,    offsetof(NetLinearSensorOffsetCmd, L2),  NET_DOUBLE,  1},
  {"R1"     ,    offsetof(NetLinearSensorOffsetCmd, R1),  NET_DOUBLE,  1},
  {"R2"     ,    offsetof(NetLinearSensorOffsetCmd, R2),  NET_DOUBLE,  1},
};

static const NetObjMember linear_sensor_range_cmd_members[] = {
  {"maxDistance",    offsetof(NetLinearSensorRangeCmd, maxDistance),  NET_DOUBLE,  1},
};

static const NetObjMember linear_sensor_moving_average_interval_cmd_members[] = {
  {"interval",    offsetof(NetLinearSensorMovingAverageIntervalCmd, interval),  NET_DOUBLE,  1},
};

static const NetObjMember equat_offset_cmd_members[] = {
  {"seq",       offsetof(NetEquatOffsetCmd, seq),      NET_INT,    1},
  {"axes",      offsetof(NetEquatOffsetCmd, axes),     NET_MASK,    1},
  {"mode",      offsetof(NetEquatOffsetCmd, mode),     NET_ENUM,    1},
  {"ra",        offsetof(NetEquatOffsetCmd, ra),       NET_INT,    1},
  {"dec",       offsetof(NetEquatOffsetCmd, dec),      NET_INT,    1},
};

static const NetObjMember tv_offset_cmd_members[] = {
  {"seq",       offsetof(NetTvOffsetCmd, seq),         NET_INT,    1},
  {"up",        offsetof(NetTvOffsetCmd, up),          NET_INT,    1},
  {"right",     offsetof(NetTvOffsetCmd, right),       NET_INT,    1},
};

static const NetObjMember tv_angle_cmd_members[] = {
  {"angle",     offsetof(NetTvAngleCmd, angle),        NET_INT,    1},
};

static const NetObjMember sky_offset_cmd_members[] = {
  {"seq",       offsetof(NetSkyOffsetCmd, seq),        NET_INT,    1},
  {"axes",      offsetof(NetSkyOffsetCmd, axes),       NET_MASK,    1},
  {"mode",      offsetof(NetSkyOffsetCmd, mode),       NET_ENUM,    1},
  {"x",         offsetof(NetSkyOffsetCmd, x),          NET_INT,    1},
  {"y",         offsetof(NetSkyOffsetCmd, y),          NET_INT,    1},
};

static const NetObjMember ut1utc_cmd_members[] = {
  {"mjd",       offsetof(NetUt1UtcCmd, mjd),           NET_INT,    1},
  {"utc",       offsetof(NetUt1UtcCmd, utc),           NET_INT,    1},
  {"ut1utc",    offsetof(NetUt1UtcCmd, ut1utc),        NET_INT,    1},
};

static const NetObjMember eqneqx_cmd_members[] = {
  {"mjd",       offsetof(NetEqnEqxCmd, mjd),           NET_INT,    1},
  {"tt",        offsetof(NetEqnEqxCmd, tt),            NET_INT,    1},
  {"eqneqx",    offsetof(NetEqnEqxCmd, eqneqx),        NET_INT,    1},
};

static const NetObjMember encoder_cals_cmd_members[] = {
  {"seq",       offsetof(NetEncoderCalsCmd, seq),      NET_INT,    1},
  {"az",        offsetof(NetEncoderCalsCmd, az),       NET_INT,    1},
  {"el",        offsetof(NetEncoderCalsCmd, el),       NET_INT,    1},
  {"dk",        offsetof(NetEncoderCalsCmd, dk),       NET_INT,    1},
};

static const NetObjMember encoder_zeros_cmd_members[] = {
  {"seq",       offsetof(NetEncoderZerosCmd, seq),     NET_INT,    1},
  {"az",        offsetof(NetEncoderZerosCmd, az),      NET_DOUBLE,  1},
  {"el",        offsetof(NetEncoderZerosCmd, el),      NET_DOUBLE,  1},
  {"dk",        offsetof(NetEncoderZerosCmd, dk),      NET_DOUBLE,  1},
};

static const NetObjMember slew_rate_cmd_members[] = {
  {"seq",       offsetof(NetSlewRateCmd, seq),         NET_INT,    1},
  {"mask",      offsetof(NetSlewRateCmd, mask),        NET_MASK,    1},
  {"az",        offsetof(NetSlewRateCmd, az),          NET_INT,    1},
  {"el",        offsetof(NetSlewRateCmd, el),          NET_INT,    1},
  {"dk",        offsetof(NetSlewRateCmd, dk),          NET_INT,    1},
};

static const NetObjMember tilts_cmd_members[] = {
  {"seq",        offsetof(NetTiltsCmd, seq),           NET_INT,    1},
  {"ha",         offsetof(NetTiltsCmd, ha),            NET_INT,    1},
  {"lat",        offsetof(NetTiltsCmd, lat),           NET_INT,    1},
  {"el",         offsetof(NetTiltsCmd, el),            NET_INT,    1},
};

static const NetObjMember flexure_cmd_members[] = {
  {"seq",        offsetof(NetFlexureCmd, seq),         NET_INT,    1},
  {"mode",       offsetof(NetFlexureCmd, mode),        NET_ENUM,   1},
  {"sFlexure",   offsetof(NetFlexureCmd, sFlexure),    NET_INT,    1},
  {"cFlexure",   offsetof(NetFlexureCmd, cFlexure),    NET_INT,    1},
  {"ptelMask",   offsetof(NetFlexureCmd, ptelMask),    NET_MASK,   1},
};

static const NetObjMember collimate_cmd_members[] = {
  {"seq",        offsetof(NetCollimateCmd, seq),       NET_INT,    1},
  {"mode",       offsetof(NetCollimateCmd, mode),      NET_ENUM,   1},
  {"x",          offsetof(NetCollimateCmd, x),         NET_INT,    1},
  {"y",          offsetof(NetCollimateCmd, y),         NET_INT,    1},
  {"type",       offsetof(NetCollimateCmd, type),      NET_ENUM,   1},
  {"magnitude",  offsetof(NetCollimateCmd, magnitude), NET_INT,    1},
  {"direction",  offsetof(NetCollimateCmd, direction), NET_INT,    1},
  {"addMode",    offsetof(NetCollimateCmd, addMode),   NET_ENUM,   1},
  {"ptelMask",   offsetof(NetCollimateCmd, ptelMask),  NET_MASK,   1},
};

static const NetObjMember encoder_limits_cmd_members[] = {
  {"seq",        offsetof(NetEncoderLimitsCmd, seq),   NET_INT,    1},
  {"az_min",     offsetof(NetEncoderLimitsCmd, az_min),NET_INT,    1},
  {"az_max",     offsetof(NetEncoderLimitsCmd, az_max),NET_INT,    1},
  {"el_min",     offsetof(NetEncoderLimitsCmd, el_min),NET_INT,    1},
  {"el_max",     offsetof(NetEncoderLimitsCmd, el_max),NET_INT,    1},
  {"pa_min",     offsetof(NetEncoderLimitsCmd, pa_min),NET_INT,    1},
  {"pa_max",     offsetof(NetEncoderLimitsCmd, pa_max),NET_INT,    1},
};

static const NetObjMember model_cmd_members[] = {
  {"seq",        offsetof(NetModelCmd, seq),           NET_INT,    1},
  {"mode",       offsetof(NetModelCmd, mode),          NET_ENUM,   1},
  {"ptelMask",   offsetof(NetModelCmd, ptelMask),      NET_MASK,   1},
};

static const NetObjMember year_cmd_members[] = {
  {"year",       offsetof(NetYearCmd, year),           NET_SHORT,   1},
};

static const NetObjMember deck_mode_cmd_members[] = {
  {"seq",        offsetof(NetDeckModeCmd, seq),        NET_INT,    1},
  {"mode",       offsetof(NetDeckModeCmd, mode),       NET_ENUM,    1},
};

static const NetObjMember atmos_cmd_members[] = {
  {"temperatureInK", offsetof(NetAtmosCmd, temperatureInK), NET_DOUBLE,   1},
  {"humidityInMax1", offsetof(NetAtmosCmd, humidityInMax1), NET_DOUBLE,   1},
  {"pressureInMbar", offsetof(NetAtmosCmd, pressureInMbar), NET_DOUBLE,   1},
};

static const NetObjMember feature_cmd_members[] = {
  {"seq",        offsetof(NetFeatureCmd, seq),         NET_INT,   1},
  {"mode",       offsetof(NetFeatureCmd, mode),        NET_ENUM,   1},
  {"mask",       offsetof(NetFeatureCmd, mask),        NET_INT,   1},
};

static const NetObjMember gpib_send_cmd_members[] = {
  {"device",     offsetof(NetGpibSendCmd, device),     NET_SHORT,  1},
  {"message",    offsetof(NetGpibSendCmd, message),    NET_ASCII,
   GPIB_MAX_DATA+1},
};

static const NetObjMember gpib_read_cmd_members[] = {
  {"device",     offsetof(NetGpibReadCmd, device),     NET_SHORT,  1},
};

static const NetObjMember power_cmd_members[] = {
  {"breaker",    offsetof(NetPowerCmd, breaker),     NET_INT,   1},
  {"power",    offsetof(NetPowerCmd, power),     NET_BOOL,   1},
};

static const NetObjMember power_dt_cmd_members[] = {
  {"seconds",    offsetof(NetPowerDtCmd, seconds),     NET_INT,   1},
};

static const NetObjMember power_meter_cmd_members[] = {
  {"cmd",        offsetof(NetPowerMeterCmd, cmd),      NET_ENUM,   1},
};

static const NetObjMember noise_cal_cmd_members[] = {
  {"on",         offsetof(NetNoiseCalCmd, on),         NET_BOOL,    1},
};

static const NetObjMember chzr_power_cmd_members[] = {
  {"seq",        offsetof(NetChzrPowerCmd, seq),       NET_INT,    1},
  {"bands",      offsetof(NetChzrPowerCmd, bands),     NET_MASK,    1},
  {"receivers",  offsetof(NetChzrPowerCmd, receivers), NET_MASK,    1},
  {"power",      offsetof(NetChzrPowerCmd, power),     NET_FLOAT,   1},
};

static const NetObjMember chzr_zero_cmd_members[] = {
  {"seq",        offsetof(NetChzrZeroCmd, seq),        NET_INT,    1},
  {"bands",      offsetof(NetChzrZeroCmd, bands),      NET_MASK,    1},
  {"receivers",  offsetof(NetChzrZeroCmd, receivers),  NET_MASK,    1},
};

static const NetObjMember chzr_attn_cmd_members[] = {
  {"bands",      offsetof(NetChzrAttnCmd, bands),      NET_MASK,    1},
  {"receivers",  offsetof(NetChzrAttnCmd, receivers),  NET_MASK,    1},
  {"attn",       offsetof(NetChzrAttnCmd, attn),       NET_INT,    1},
  {"mode",       offsetof(NetChzrAttnCmd, mode),       NET_ENUM,    1},
};

static const NetObjMember chzr_switch_cmd_members[] = {
  {"bands",      offsetof(NetChzrSwitchCmd, bands),    NET_MASK,    1},
  {"receivers",  offsetof(NetChzrSwitchCmd, receivers),NET_MASK,    1},
  {"on",         offsetof(NetChzrSwitchCmd, on),       NET_BOOL,    1},
};

static const NetObjMember chzr_enable_cmd_members[] = {
  {"on",         offsetof(NetChzrEnableCmd, on),       NET_BOOL,    1},
};

static const NetObjMember rxsim_cmd_members[] = {
  {"what",       offsetof(NetRxSimCmd, what),          NET_MASK,    1},
  {"noise",      offsetof(NetRxSimCmd, noise),         NET_MASK,    1},
  {"quad",       offsetof(NetRxSimCmd, quad),          NET_MASK,    1},
  {"sky",        offsetof(NetRxSimCmd, sky),           NET_BOOL,    1},
};

static const NetObjMember selectRx_cmd_members[] = {
  {"band",      offsetof(NetSelectRxCmd, band),       NET_ENUM,    1},
  {"seq",       offsetof(NetSelectRxCmd, seq),        NET_INT,    1},
};

static const NetObjMember setBias_cmd_members[] = {
  {"amp",      offsetof(NetSetBiasCmd, amp),        NET_ENUM,     1},
  {"bias",     offsetof(NetSetBiasCmd, bias),       NET_SHORT,    1},
  {"biasType", offsetof(NetSetBiasCmd, biasType),   NET_ENUM,     1},
  {"rxId",     offsetof(NetSetBiasCmd, rxId),       NET_ENUM,     1},
  {"seq",      offsetof(NetSetBiasCmd, seq),        NET_INT,     1},
};

static const NetObjMember thermo_cmd_members[] = {
  {"command",   offsetof(NetThermoCmd, command),       NET_ENUM,    1},
  {"address",   offsetof(NetThermoCmd, address),       NET_BYTE,    16},
  {"index",     offsetof(NetThermoCmd, index),         NET_INT,    1},
};

static const NetObjMember ds_dt_cmd_members[] = {
  {"seconds",   offsetof(NetDsDtCmd, seconds),         NET_INT,   1},
};
/*
 * Commands forwarded to the camera controller.
 */
static const NetObjMember camera_cmd_members[] = {
  {"target",    offsetof(NetOptCamCmd, target),        NET_ENUM,    1},
  {"action",    offsetof(NetOptCamCmd, action),        NET_ENUM,    1},
};
/*
 * Commands to turn the camer/stepper motor on/off
 */
static const NetObjMember optcam_cntl_cmd_members[] = {
  {"target",    offsetof(NetOptCamCntlCmd, target),    NET_ENUM,    1},
  {"on",        offsetof(NetOptCamCntlCmd, on),        NET_BOOL,    1},
};
/*
 * Command to step the stepper motor.
 */
static const NetObjMember stepper_cmd_members[] = {
  {"count",    offsetof(NetStepperCmd, count),         NET_INT,    1},
};
/*
 * Command to write to the frame grabber registers.
 */
static const NetObjMember fg_cmd_members[] = {
  {"target",   offsetof(NetFgCmd, target),             NET_ENUM,    1},
  {"value",    offsetof(NetFgCmd, value),              NET_INT,    1}
};
/*
 * Command to flat field frame grabber images
 */
static const NetObjMember flatfield_cmd_members[] = {
  {"on",   offsetof(NetFlatFieldCmd, on),              NET_BOOL,    1},
};

/*
 * Command to control pointing telescope shutter
 */
 
static const NetObjMember ptel_shutter_cmd_members[] = {
  {"target",    offsetof(NetPTelShutterCmd, target), NET_ENUM, 1},
  {"open",      offsetof(NetPTelShutterCmd, open),   NET_BOOL, 1}
};

/*
 * Command to control pointing telescope heater
 */
 
static const NetObjMember ptel_heater_cmd_members[] = {
  {"target",    offsetof(NetPTelHeaterCmd,     target),       NET_ENUM, 1},
  {"on",        offsetof(NetPTelHeaterCmd,     on),           NET_BOOL, 1},
  {"pid",       offsetof(NetPTelHeaterCmd,     pid),          NET_BOOL, 1},
  {"tune",      offsetof(NetPTelHeaterCmd,     tune),         NET_BOOL, 1},
  {"setpoint",  offsetof(NetPTelHeaterCmd,     setpoint),     NET_DOUBLE, 1},
  {"proportional", offsetof(NetPTelHeaterCmd,  proportional), NET_DOUBLE, 1},
  {"integral", offsetof(NetPTelHeaterCmd,      integral),     NET_DOUBLE, 1},
  {"derivative", offsetof(NetPTelHeaterCmd,    derivative),   NET_DOUBLE, 1}

};

/*
 * Command to control deicing heater
 */
 
static const NetObjMember deicing_heater_cmd_members[] = {
  {"target",    offsetof(NetDeicingHeaterCmd,  target), NET_ENUM, 1},
  {"on",        offsetof(NetDeicingHeaterCmd,  on),     NET_BOOL, 1}
};

/*
 * Command to control cabin shutter
 */
 
static const NetObjMember cabin_shutter_cmd_members[] = {
  {"open",      offsetof(NetCabinShutterCmd, open),    NET_BOOL, 1}
};
 
/*
 * Command to control optical bench zero position
 */
 
static const NetObjMember bench_zero_position_cmd_members[] = {
  {"seq",       offsetof(NetBenchZeroPositionCmd, seq), NET_INT, 1},
  {"y1",        offsetof(NetBenchZeroPositionCmd, y1), NET_DOUBLE, 1},
  {"y2",        offsetof(NetBenchZeroPositionCmd, y2), NET_DOUBLE, 1},
  {"y3",        offsetof(NetBenchZeroPositionCmd, y3), NET_DOUBLE, 1},
  {"x4",        offsetof(NetBenchZeroPositionCmd, x4), NET_DOUBLE, 1},
  {"x5",        offsetof(NetBenchZeroPositionCmd, x5), NET_DOUBLE, 1},
  {"z6",        offsetof(NetBenchZeroPositionCmd, z6), NET_DOUBLE, 1}
};

/*
 * Command to control optical bench offset
 */
 
static const NetObjMember bench_offset_cmd_members[] = {
  {"seq",       offsetof(NetBenchOffsetCmd, seq), NET_INT, 1},
  {"y1",        offsetof(NetBenchOffsetCmd, y1), NET_DOUBLE, 1},
  {"y2",        offsetof(NetBenchOffsetCmd, y2), NET_DOUBLE, 1},
  {"y3",        offsetof(NetBenchOffsetCmd, y3), NET_DOUBLE, 1},
  {"x4",        offsetof(NetBenchOffsetCmd, x4), NET_DOUBLE, 1},
  {"x5",        offsetof(NetBenchOffsetCmd, x5), NET_DOUBLE, 1},
  {"z6",        offsetof(NetBenchOffsetCmd, z6), NET_DOUBLE, 1}
};

/*
 * Command to set optical bench use brakes flag
 */

static const NetObjMember bench_use_brakes_cmd_members[] = {
  {"seq",        offsetof(NetBenchUseBrakesCmd, seq), NET_INT, 1},
  {"use_brakes", offsetof(NetBenchUseBrakesCmd, use_brakes), NET_BOOL, 1}
};

/* 
 * Command to set optical bench dead band
 */

static const NetObjMember bench_set_acquired_threshold_cmd_members[] = {
  {"seq",                offsetof(NetBenchSetAcquiredThresholdCmd, seq), NET_INT, 1},
  {"acquired_threshold", offsetof(NetBenchSetAcquiredThresholdCmd, acquired_threshold), NET_DOUBLE, 1}
};

/*
 * Command to set optical bench focus offset
 */

static const NetObjMember bench_set_focus_cmd_members[] = {
  {"seq",   offsetof(NetBenchSetFocusCmd, seq), NET_INT, 1},
  {"focus", offsetof(NetBenchSetFocusCmd, focus), NET_DOUBLE, 1}
};

/*
 * Command to configure the frame grabber.
 */
static const NetObjMember configureFrameGrabber_cmd_members[] = {
  {"mask",       offsetof(NetConfigureFrameGrabberCmd, mask),       NET_MASK,1},
  {"channelMask",offsetof(NetConfigureFrameGrabberCmd, channelMask),NET_MASK,1},
  {"ncombine",   offsetof(NetConfigureFrameGrabberCmd, nCombine),   NET_INT, 1},
  {"flatfield",  offsetof(NetConfigureFrameGrabberCmd, flatfield),  NET_INT, 1},
  {"seconds",    offsetof(NetConfigureFrameGrabberCmd, seconds),    NET_INT, 1}
};

/*
 * Command to install total power calibrations
 */
static const NetObjMember tpcal_cmd_members[] = {
  {"bands",         offsetof(NetTpcalCmd, bands),          NET_MASK, 1},
  {"receivers",     offsetof(NetTpcalCmd, receivers),      NET_MASK, 1},
  {"factor",        offsetof(NetTpcalCmd, factor),         NET_FLOAT,1},
};

//------------------------------------------------------------
// Downconverter commands
//------------------------------------------------------------

// Command to set the Psys power

static const NetObjMember psys_cmd_members[] = {
  {"power",     offsetof(NetPsysCmd, power),        NET_DOUBLE, 1},
  {"preset",    offsetof(NetPsysCmd, preset),       NET_BOOL,   1},
  {"bands",     offsetof(NetPsysCmd, bands),        NET_MASK,   1},
};

// Command to set the Psys attenuation

static const NetObjMember psys_atten_cmd_members[] = {
  {"atten",     offsetof(NetPsysAttenCmd, atten),   NET_DOUBLE, 1},
  {"bands",     offsetof(NetPsysAttenCmd, bands),   NET_MASK,   1},
};

// Command to set the Ifout power

static const NetObjMember ifout_cmd_members[] = {
  {"power",     offsetof(NetIfoutCmd, power),       NET_DOUBLE, 1},
  {"preset",    offsetof(NetIfoutCmd, preset),      NET_BOOL,   1},
  {"bands",     offsetof(NetIfoutCmd, bands),       NET_MASK,   1},
};

// Command to set the Ifout attenuation

static const NetObjMember ifout_atten_cmd_members[] = {
  {"atten",     offsetof(NetIfoutAttenCmd, atten),   NET_DOUBLE, 1},
  {"bands",     offsetof(NetIfoutAttenCmd, bands),   NET_MASK,   1},
};

// Command to enable the RF amplifier

static const NetObjMember rf_amp_cmd_members[] = {
  {"enable",    offsetof(NetRfAmpCmd, enable),      NET_BOOL,   1},
  {"bands",     offsetof(NetRfAmpCmd, bands),       NET_MASK,   1},
};

// Command to enable the IF automatic level control

static const NetObjMember if_alc_cmd_members[] = {
  {"enable",    offsetof(NetIfAlcCmd, enable),      NET_BOOL,   1},
  {"bands",     offsetof(NetIfAlcCmd, bands),       NET_MASK,   1},
};

//-----------------------------------------------------------------------
// Noise source commands
//-----------------------------------------------------------------------

// Command to set the output noise power.

static const NetObjMember noise_power_cmd_members[] = {
  {"power",     offsetof(NetNoisePowerCmd, power),  NET_DOUBLE, 1},
  {"preset",    offsetof(NetNoisePowerCmd, preset), NET_BOOL,   1},
};

// Command to set the noise attenuation

static const NetObjMember noise_atten_cmd_members[] = {
  {"atten",     offsetof(NetNoiseAttenCmd, atten),  NET_SHORT, 1},
};

// Command to set the tone attenuation

static const NetObjMember tone_atten_cmd_members[] = {
  {"atten",     offsetof(NetToneAttenCmd, atten),   NET_SHORT, 1},
};

// Command to turn the noise source on

static const NetObjMember noise_cmd_members[] = {
  {"enable",    offsetof(NetNoiseCmd, enable),      NET_BOOL,   1},
  {"mask",      offsetof(NetNoiseCmd, mask),        NET_MASK,   1},
  {"seq",       offsetof(NetNoiseCmd, seq),         NET_INT,   1},
};

// Command to turn the tone source on

static const NetObjMember tone_cmd_members[] = {
  {"enable",    offsetof(NetToneCmd, enable),       NET_BOOL,   1},
};

//-----------------------------------------------------------------------
// Quad Mod commands
//-----------------------------------------------------------------------

// Command to set the output noise power.

static const NetObjMember quad_power_cmd_members[] = {
  {"power",    offsetof(NetQuadPowerCmd, power),    NET_DOUBLE, 1},
  {"preset",   offsetof(NetQuadPowerCmd, preset),   NET_BOOL,   1},
};

// Command to set the quad attenuation.

static const NetObjMember quad_atten_cmd_members[] = {
  {"atten",    offsetof(NetQuadAttenCmd, atten),    NET_SHORT,  1},
};

// Command to set the walshstate column

static const NetObjMember quad_walshcol_cmd_members[] = {
  {"column",   offsetof(NetQuadWalshColCmd, column),   NET_SHORT, 1},
};

// Command to turn the quad on

static const NetObjMember quad_cmd_members[] = {
  {"enable",   offsetof(NetQuadCmd, enable),        NET_BOOL, 1},
};

// Command to set the Quad phase

static const NetObjMember quad_phase_cmd_members[] = {
  {"phase",    offsetof(NetQuadPhaseCmd, phase),    NET_SHORT, 1},
};

//-----------------------------------------------------------------------
// Generic CAN module commands
//-----------------------------------------------------------------------

// Reset a module

static const NetObjMember reset_cmd_members[] = {
  {"modules",   offsetof(NetResetCmd, modules),     NET_ENUM,   1},
  {"bands",     offsetof(NetResetCmd, bands),       NET_MASK,   1},
  {"hard",      offsetof(NetResetCmd, hard),        NET_BOOL,   1},
};

//-----------------------------------------------------------------------
// IntMod command
//-----------------------------------------------------------------------

static const  NetObjMember intmod_cmd_members[] = {
  {"msgId",        offsetof(NetIntModCmd, msgId),        NET_ENUM,  1},
  {"atten",        offsetof(NetIntModCmd, atten),        NET_BYTE,  1},
};

static const NetObjMember flipDelay_cmd_members[] = {
  {"target",      offsetof(NetFlipDelayCmd, target),      NET_ENUM, 1},
  {"delayTarget", offsetof(NetFlipDelayCmd, delayTarget), NET_ENUM, 1},
  {"delay",       offsetof(NetFlipDelayCmd, delay),       NET_BOOL, 1},
  {"rate",        offsetof(NetFlipDelayCmd, rate),        NET_BOOL, 1}
};

//-----------------------------------------------------------------------
// CalTert command
//-----------------------------------------------------------------------

static const  NetObjMember caltert_cmd_members[] = {
  {"msgId",        offsetof(NetCalTertCmd, msgId),        NET_ENUM,  1},
  {"rxId",         offsetof(NetCalTertCmd, rxId),         NET_ENUM,  1},
  {"tertPosition", offsetof(NetCalTertCmd, tertPosition), NET_SHORT, 1},
  {"calPosition",  offsetof(NetCalTertCmd, calPosition),  NET_ENUM,  1},
  {"enable",       offsetof(NetCalTertCmd, enable),       NET_BOOL,  1},
  {"owDevice",     offsetof(NetCalTertCmd, owDevice),     NET_ENUM,  1},
  {"owCommand",    offsetof(NetCalTertCmd, owCommand),    NET_ENUM,  1},
  {"seq",          offsetof(NetCalTertCmd, seq),          NET_INT,  1},
};

//-----------------------------------------------------------------------
// IFMod command
//-----------------------------------------------------------------------

static const  NetObjMember IFMod_cmd_members[] = {
  {"msgId",    offsetof(NetIFModCmd, msgId),    NET_ENUM,   1},
  {"band",     offsetof(NetIFModCmd, band),     NET_ENUM,   1},
  {"level",    offsetof(NetIFModCmd, level),    NET_DOUBLE, 1},
  {"attenSet", offsetof(NetIFModCmd, attenSet), NET_ENUM,   1},
  {"total",    offsetof(NetIFModCmd, total),    NET_DOUBLE, 1},
  {"input",    offsetof(NetIFModCmd, input),    NET_DOUBLE, 1},
  {"output",   offsetof(NetIFModCmd, output),   NET_DOUBLE, 1},
  {"seq",      offsetof(NetIFModCmd, seq),      NET_INT,    1},
};

//-----------------------------------------------------------------------
// TestTrackTime command
//-----------------------------------------------------------------------

static const  NetObjMember testTrackTime_cmd_members[] = {
  {"sec",    offsetof(NetTestTrackTimeCmd, sec),  NET_INT,  1},
  {"msec",   offsetof(NetTestTrackTimeCmd, msec), NET_INT,  1},
};

//-----------------------------------------------------------------------
// NetGpibCmd command
//-----------------------------------------------------------------------

static const  NetObjMember gpib_cmd_members[] = {
  {"gpibDevice", offsetof(NetGpibCmd, device),  NET_INT,   1},
  {"cmdId",      offsetof(NetGpibCmd, cmdId),   NET_INT,   1},
  {"intVals",    offsetof(NetGpibCmd, intVals), NET_INT,   2},
  {"fltVal",     offsetof(NetGpibCmd, fltVal),  NET_FLOAT, 1}
};

//-----------------------------------------------------------------------
// NetServoCmd command
//-----------------------------------------------------------------------

static const  NetObjMember servo_cmd_members[] = {
  {"cmdId",      offsetof(NetServoCmd, cmdId),   NET_INT,   1},
  {"intVal",     offsetof(NetServoCmd, intVal),  NET_INT,   1},
  {"fltVal",     offsetof(NetServoCmd, fltVal),  NET_FLOAT, 1},
  {"fltVals",    offsetof(NetServoCmd, fltVals), NET_FLOAT, 10}
};

//-----------------------------------------------------------------------
// NetRxCmd command
//-----------------------------------------------------------------------

static const  NetObjMember rx_cmd_members[] = {
  {"cmdId",         offsetof(NetRxCmd, cmdId),   NET_INT,   1},
  {"fltVal",        offsetof(NetRxCmd, fltVal),  NET_FLOAT, 1},
  {"stageNumber",   offsetof(NetRxCmd, stageNumber),  NET_INT, 1},
  {"channelNumber", offsetof(NetRxCmd, channelNumber),  NET_INT, 1}
};

//-----------------------------------------------------------------------
// NetRoachCmd command
//-----------------------------------------------------------------------

static const  NetObjMember roach_cmd_members[] = {
  {"cmdId",         offsetof(NetRoachCmd, cmdId),   NET_INT,   1},
  {"stringCommand", offsetof(NetRoachCmd, stringCommand),  NET_ASCII, 11},
  {"fltVal",        offsetof(NetRoachCmd, fltVal),  NET_FLOAT, 1},
  {"roachNum",      offsetof(NetRoachCmd, roachNum),  NET_INT, 1},
};


//-----------------------------------------------------------------------
// NetLnaCmd command
//-----------------------------------------------------------------------

static const  NetObjMember lna_cmd_members[] = {
  {"cmdId",       offsetof(NetLnaCmd, cmdId),   NET_INT,   1},
  {"drainVoltage",      offsetof(NetLnaCmd, drainVoltage),  NET_FLOAT, 1},
  {"drainCurrent",      offsetof(NetLnaCmd, drainCurrent),  NET_FLOAT, 1},
  {"lnaNumber",   offsetof(NetLnaCmd, lnaNumber),  NET_INT, 1},
  {"stageNumber", offsetof(NetLnaCmd, stageNumber),  NET_INT, 1}
};

/**
 * Collect all of the object definitions in an array. 
 *
 * N.B.: Every command defined in rtcnetcoms.h::RtcNetCmd must have a
 * corresponding entry in the following array, in the same order, or
 * net_obj_info() will return the wrong object id when (un)packing the
 * object (from)to the network buffer.
 */
static const NetObjInfo cmd_objects[] = {
  {"shutdown",	   shutdown_cmd_members,    ARRAY_DIM(shutdown_cmd_members),
   sizeof(NetShutdownCmd)},
  {"interval",	   interval_cmd_members,    ARRAY_DIM(interval_cmd_members),
   sizeof(NetIntervalCmd)},
  {"inhibit",	   inhibit_cmd_members,	    ARRAY_DIM(inhibit_cmd_members),
   sizeof(NetInhibitCmd)},
  {"strobe",	   NULL,		    0,	    0},
  {"setreg",	   setreg_cmd_members,	    ARRAY_DIM(setreg_cmd_members),
   sizeof(NetSetregCmd)},
  {"getreg",	   getreg_cmd_members,	    ARRAY_DIM(getreg_cmd_members),
   sizeof(NetGetregCmd)},
  {"setdio",	   setdio_cmd_members,	    ARRAY_DIM(setdio_cmd_members),
   sizeof(NetSetDioCmd)},
  {"unflag",	   unflag_cmd_members,	    ARRAY_DIM(unflag_cmd_members),
   sizeof(NetUnflagCmd)},
  {"phase_motor",  phase_motor_cmd_members, ARRAY_DIM(phase_motor_cmd_members),
   sizeof(NetPhaseMotorCmd)},
  {"phase_shift",  phase_shift_cmd_members, ARRAY_DIM(phase_shift_cmd_members),
   sizeof(NetPhaseShiftCmd)},
  {"selectRx",	   selectRx_cmd_members,   ARRAY_DIM(selectRx_cmd_members),
   sizeof(NetSelectRxCmd)},
  {"setBias",	   setBias_cmd_members,    ARRAY_DIM(setBias_cmd_members),
   sizeof(NetSetBiasCmd)},
  {"rx_heater",	   rx_heater_cmd_members,   ARRAY_DIM(rx_heater_cmd_members),
   sizeof(NetRxHeaterCmd)},
  {"rx_coldhead",  rx_coldhead_cmd_members, ARRAY_DIM(rx_coldhead_cmd_members),
   sizeof(NetRxColdheadCmd)},
  {"rx_temp",	   rx_temp_cmd_members,	    ARRAY_DIM(rx_temp_cmd_members),
   sizeof(NetRxTempCmd)},
  {"lo",	   lo_cmd_members,          ARRAY_DIM(lo_cmd_members),
   sizeof(NetLoCmd)},
  {"rx_quad",	   rx_quad_cmd_members,	    ARRAY_DIM(rx_quad_cmd_members),
   sizeof(NetRxQuadCmd)},
  {"rx_polar",	   rx_polar_cmd_members,    ARRAY_DIM(rx_polar_cmd_members),
   sizeof(NetRxPolarCmd)},
  {"rx_polwalsh",  rx_polwalsh_cmd_members, ARRAY_DIM(rx_polwalsh_cmd_members),
   sizeof(NetPolWalshCmd)},
  {"site",	   site_cmd_members,	    ARRAY_DIM(site_cmd_members),
   sizeof(NetSiteCmd)},
  {"location",	   location_cmd_members,    ARRAY_DIM(location_cmd_members),
   sizeof(NetLocationCmd)},
  {"delayref",	   delayref_cmd_members,    ARRAY_DIM(delayref_cmd_members),
   sizeof(NetDelayRefCmd)},
  {"init",	   init_cmd_members,	    ARRAY_DIM(init_cmd_members),
   sizeof(NetInitCmd)},
  {"halt",	   halt_cmd_members,	    ARRAY_DIM(halt_cmd_members),
   sizeof(NetHaltCmd)},
  {"stop",	   NULL,		    0,	    0},
  {"slew",	   slew_cmd_members,	    ARRAY_DIM(slew_cmd_members),
   sizeof(NetSlewCmd)},
  {"track",	   track_cmd_members,	    ARRAY_DIM(track_cmd_members),
   sizeof(NetTrackCmd)},
  {"mount_offset", mount_offset_cmd_members,ARRAY_DIM(mount_offset_cmd_members),
   sizeof(NetMountOffsetCmd)},
  {"equat_offset", equat_offset_cmd_members,ARRAY_DIM(equat_offset_cmd_members),
   sizeof(NetEquatOffsetCmd)},
  {"tv_offset",	   tv_offset_cmd_members,   ARRAY_DIM(tv_offset_cmd_members),
   sizeof(NetTvOffsetCmd)},
  {"tv_angle",	   tv_angle_cmd_members,    ARRAY_DIM(tv_angle_cmd_members),
   sizeof(NetTvAngleCmd)},
  {"sky_offset",   sky_offset_cmd_members,  ARRAY_DIM(sky_offset_cmd_members),
   sizeof(NetSkyOffsetCmd)},
  {"ut1utc",	   ut1utc_cmd_members,	    ARRAY_DIM(ut1utc_cmd_members),
   sizeof(NetUt1UtcCmd)},
  {"eqneqx",	   eqneqx_cmd_members,	    ARRAY_DIM(eqneqx_cmd_members),
   sizeof(NetEqnEqxCmd)},
  {"encoder_cals", encoder_cals_cmd_members,ARRAY_DIM(encoder_cals_cmd_members),
   sizeof(NetEncoderCalsCmd)},
  {"encoder_limits", encoder_limits_cmd_members, 
   ARRAY_DIM(encoder_limits_cmd_members),
   sizeof(NetEncoderLimitsCmd)},
  {"encoder_zeros", encoder_zeros_cmd_members,
   ARRAY_DIM(encoder_zeros_cmd_members),
   sizeof(NetEncoderZerosCmd)},
  {"slew_rate",	   slew_rate_cmd_members,   ARRAY_DIM(slew_rate_cmd_members),
   sizeof(NetSlewRateCmd)},
  {"tilts",	   tilts_cmd_members,	    ARRAY_DIM(tilts_cmd_members),
   sizeof(NetTiltsCmd)},
  {"flexure",	   flexure_cmd_members,	    ARRAY_DIM(flexure_cmd_members),
   sizeof(NetFlexureCmd)},
  {"collimate",	   collimate_cmd_members,   ARRAY_DIM(collimate_cmd_members),
   sizeof(NetCollimateCmd)},
  {"model",	   model_cmd_members,	    ARRAY_DIM(model_cmd_members),
   sizeof(NetModelCmd)},
  {"year",	   year_cmd_members,	    ARRAY_DIM(year_cmd_members),
   sizeof(NetYearCmd)},
  {"deckMode",	   deck_mode_cmd_members,   ARRAY_DIM(deck_mode_cmd_members),
   sizeof(NetDeckModeCmd)},
  {"atmos",	   atmos_cmd_members,	    ARRAY_DIM(atmos_cmd_members),
   sizeof(NetAtmosCmd)},
  {"feature",	   feature_cmd_members,	    ARRAY_DIM(feature_cmd_members),
   sizeof(NetFeatureCmd)},
  {"gpib_send",	   gpib_send_cmd_members,   ARRAY_DIM(gpib_send_cmd_members),
   sizeof(NetGpibSendCmd)},
  {"gpib_read",	   gpib_read_cmd_members,   ARRAY_DIM(gpib_read_cmd_members),
   sizeof(NetGpibReadCmd)},
  {"power",	   power_cmd_members,       ARRAY_DIM(power_cmd_members),
   sizeof(NetPowerCmd)},
  {"power_dt",	   power_dt_cmd_members,    ARRAY_DIM(power_dt_cmd_members),
   sizeof(NetPowerDtCmd)},
  {"power_meter",  power_meter_cmd_members, ARRAY_DIM(power_meter_cmd_members),
   sizeof(NetPowerMeterCmd)},
  {"noise_cal",	   noise_cal_cmd_members,   ARRAY_DIM(noise_cal_cmd_members),
   sizeof(NetNoiseCalCmd)},
  {"chzr_power",   chzr_power_cmd_members,  ARRAY_DIM(chzr_power_cmd_members),
   sizeof(NetChzrPowerCmd)},
  {"chzr_zero",	   chzr_zero_cmd_members,   ARRAY_DIM(chzr_zero_cmd_members),
   sizeof(NetChzrZeroCmd)},
  {"chzr_attn",	   chzr_attn_cmd_members,   ARRAY_DIM(chzr_attn_cmd_members),
   sizeof(NetChzrAttnCmd)},
  {"chzr_switch",  chzr_switch_cmd_members, ARRAY_DIM(chzr_switch_cmd_members),
   sizeof(NetChzrSwitchCmd)},
  {"chzr_enable",  chzr_enable_cmd_members, ARRAY_DIM(chzr_enable_cmd_members),
   sizeof(NetChzrEnableCmd)},
  {"rxsim",	   rxsim_cmd_members,	    ARRAY_DIM(rxsim_cmd_members),
   sizeof(NetRxSimCmd)},
  {"thermo",	   thermo_cmd_members,	    ARRAY_DIM(thermo_cmd_members),
   sizeof(NetThermoCmd)},
  {"ds_dt",	   ds_dt_cmd_members,	    ARRAY_DIM(ds_dt_cmd_members),
   sizeof(NetDsDtCmd)},
  
  // Optical camera-related commands.

  {"camera",	   camera_cmd_members,	    ARRAY_DIM(camera_cmd_members),
   sizeof(NetOptCamCmd)},
  {"optcam_cntl",  optcam_cntl_cmd_members, ARRAY_DIM(optcam_cntl_cmd_members), 
   sizeof(NetOptCamCntlCmd)},
  {"pTelShutter",  ptel_shutter_cmd_members, ARRAY_DIM(ptel_shutter_cmd_members),
   sizeof(NetPTelShutterCmd)},
  {"pTelHeater",   ptel_heater_cmd_members,  ARRAY_DIM(ptel_heater_cmd_members),
   sizeof(NetPTelHeaterCmd)},
  {"deicingHeater",   deicing_heater_cmd_members,  ARRAY_DIM(deicing_heater_cmd_members),
   sizeof(NetDeicingHeaterCmd)},
  {"cabinShutter", cabin_shutter_cmd_members, ARRAY_DIM(cabin_shutter_cmd_members),
   sizeof(NetCabinShutterCmd)},

  // Optical bench related commands

  {"benchZeros",   bench_zero_position_cmd_members, ARRAY_DIM(bench_zero_position_cmd_members),
   sizeof(NetBenchZeroPositionCmd)},
  {"benchOffset",   bench_offset_cmd_members, ARRAY_DIM(bench_offset_cmd_members),
   sizeof(NetBenchOffsetCmd)},
  {"benchUseBrakes",   bench_use_brakes_cmd_members, ARRAY_DIM(bench_use_brakes_cmd_members),
   sizeof(NetBenchUseBrakesCmd)},
  {"benchAcquiredThreshold",   bench_set_acquired_threshold_cmd_members, ARRAY_DIM(bench_set_acquired_threshold_cmd_members),
   sizeof(NetBenchSetAcquiredThresholdCmd)},
  {"benchFocus",   bench_set_focus_cmd_members, ARRAY_DIM(bench_set_focus_cmd_members),
   sizeof(NetBenchSetFocusCmd)},

  {"stepper",	   stepper_cmd_members,	    ARRAY_DIM(stepper_cmd_members),
   sizeof(NetStepperCmd)},
  {"grabber",	   NULL,		    0,	0},
  {"flatfield",	   flatfield_cmd_members,   ARRAY_DIM(flatfield_cmd_members),
   sizeof(NetFlatFieldCmd)},
  {"fg",	   fg_cmd_members,	    ARRAY_DIM(fg_cmd_members),
   sizeof(NetFgCmd)},
  {"configureFrameGrabber", configureFrameGrabber_cmd_members,	    
   ARRAY_DIM(configureFrameGrabber_cmd_members),
   sizeof(NetConfigureFrameGrabberCmd)},
  {"tpcal",	   tpcal_cmd_members,	    ARRAY_DIM(tpcal_cmd_members),
   sizeof(NetTpcalCmd)},
  {"walshstate",   walshstate_cmd_members,  ARRAY_DIM(walshstate_cmd_members),
   sizeof(NetWalshStateCmd)},
  {"pager",	   pager_cmd_members,	    ARRAY_DIM(pager_cmd_members),
   sizeof(NetPagerCmd)},
  {"reboot_acu",  reboot_drive_cmd_members, ARRAY_DIM(reboot_drive_cmd_members),
   sizeof(NetRebootDriveCmd)},

  // Downconverter commands

  {"psys",       psys_cmd_members,        ARRAY_DIM(psys_cmd_members),
   sizeof(NetPsysCmd)},
  {"psys_atten", psys_atten_cmd_members,  ARRAY_DIM(psys_atten_cmd_members),
   sizeof(NetPsysAttenCmd)},
  {"ifout",      ifout_cmd_members,       ARRAY_DIM(ifout_cmd_members),
   sizeof(NetIfoutCmd)},
  {"ifout_atten",ifout_atten_cmd_members, ARRAY_DIM(ifout_atten_cmd_members),
   sizeof(NetIfoutAttenCmd)},
  {"rf_amp",     rf_amp_cmd_members,      ARRAY_DIM(rf_amp_cmd_members),
   sizeof(NetRfAmpCmd)},
  {"if_alc",     if_alc_cmd_members,      ARRAY_DIM(if_alc_cmd_members),
   sizeof(NetIfAlcCmd)},

  // Noise source commands

  {"noise_power",noise_power_cmd_members, ARRAY_DIM(noise_power_cmd_members),
   sizeof(NetNoisePowerCmd)},
  {"noise_atten",noise_atten_cmd_members, ARRAY_DIM(noise_atten_cmd_members),
   sizeof(NetNoiseAttenCmd)},
  {"tone_atten", tone_atten_cmd_members,  ARRAY_DIM(tone_atten_cmd_members),
   sizeof(NetToneAttenCmd)},
  {"noise",      noise_cmd_members,       ARRAY_DIM(noise_cmd_members),
   sizeof(NetNoiseCmd)},
  {"tone",       tone_cmd_members,        ARRAY_DIM(tone_cmd_members),
   sizeof(NetToneCmd)},

  // Quad Mod commands

  {"quad_power",     quad_power_cmd_members, ARRAY_DIM(quad_power_cmd_members),
   sizeof(NetQuadPowerCmd)},
  {"quad_atten",     quad_atten_cmd_members,  ARRAY_DIM(quad_atten_cmd_members),
   sizeof(NetQuadAttenCmd)},
  {"quad_walshcol", quad_walshcol_cmd_members, 
   ARRAY_DIM(quad_walshcol_cmd_members), sizeof(NetQuadWalshColCmd)},
  {"quad",      quad_cmd_members,        ARRAY_DIM(quad_cmd_members),
   sizeof(NetQuadCmd)},
  {"quad_walshtable", NULL, 0, 0},
  {"quad_phase", quad_phase_cmd_members, ARRAY_DIM(quad_phase_cmd_members), 
   sizeof(NetQuadPhaseCmd)},

  // CAN module reset

  {"reset",      reset_cmd_members,       ARRAY_DIM(reset_cmd_members),
   sizeof(NetResetCmd)},

  // Receiver card sampling rate

  {"fast_sampling", fast_sampling_cmd_members, ARRAY_DIM(fast_sampling_cmd_members),
   sizeof(NetFastSamplingCmd)},

  // Delay commands

  {"set_antenna_coords", set_antenna_coords_cmd_members, ARRAY_DIM(set_antenna_coords_cmd_members),
   sizeof(NetSetAntennaCoordsCmd)},

  {"dds", dds_cmd_members, ARRAY_DIM(dds_cmd_members),
   sizeof(NetDDSCmd)},
  {"set_antenna_dds", set_antenna_dds_cmd_members, ARRAY_DIM(set_antenna_dds_cmd_members),
   sizeof(NetSetAntennaDDSCmd)},
  {"set_lr_phase", set_lr_phase_cmd_members, ARRAY_DIM(set_lr_phase_cmd_members),
   sizeof(NetSetLRPhaseCmd)},
  {"set_lr_freq", set_lr_freq_cmd_members, ARRAY_DIM(set_lr_freq_cmd_members),
   sizeof(NetSetLRFreqCmd)},
  {"enable_dds_walshing", enable_dds_walshing_cmd_members, ARRAY_DIM(enable_dds_walshing_cmd_members),
   sizeof(NetEnableDDSWalshingCmd)},

  {"set_dds_walsh_column", set_dds_walsh_column_cmd_members, ARRAY_DIM(set_dds_walsh_column_cmd_members),
   sizeof(NetSetDDSWalshColumnCmd)},

  {"set_output_regs", set_output_regs_cmd_members, ARRAY_DIM(set_output_regs_cmd_members),
   sizeof(NetSetOutputRegsCmd)},
  {"set_lr_delay", set_lr_delay_cmd_members, ARRAY_DIM(set_lr_delay_cmd_members),
   sizeof(NetSetLRDelayCmd)},

  {"set_antenna_phase", set_antenna_phase_cmd_members, ARRAY_DIM(set_antenna_phase_cmd_members),
   sizeof(NetSetAntennaPhaseCmd)},
  {"set_antenna_freq", set_antenna_freq_cmd_members, ARRAY_DIM(set_antenna_freq_cmd_members),
   sizeof(NetSetAntennaFreqCmd)},

  {"set_antenna_params", set_antenna_params_cmd_members, ARRAY_DIM(set_antenna_params_cmd_members),
   sizeof(NetSetAntennaParamsCmd)},
  {"set_delay", set_delay_cmd_members, ARRAY_DIM(set_delay_cmd_members),
   sizeof(NetSetDelayCmd)},
  {"setRefAnt", NULL, 0, 0},
  {"set_weather_params", set_weather_params_cmd_members, ARRAY_DIM(set_weather_params_cmd_members),
   sizeof(NetSetWeatherParamsCmd)},
  {"use_delay", use_delay_cmd_members, ARRAY_DIM(use_delay_cmd_members),
   sizeof(NetUseDelayCmd)},
  {"scan", scan_cmd_members, ARRAY_DIM(scan_cmd_members),
   sizeof(NetScanCmd)},
  {"caltert", caltert_cmd_members, ARRAY_DIM(caltert_cmd_members),
   sizeof(NetCalTertCmd)},
  {"IFMod", IFMod_cmd_members, ARRAY_DIM(IFMod_cmd_members),
   sizeof(NetIFModCmd)},
  {"intmod", intmod_cmd_members, ARRAY_DIM(intmod_cmd_members),
   sizeof(NetIntModCmd)},
  {"flipDelay", flipDelay_cmd_members, ARRAY_DIM(flipDelay_cmd_members),
   sizeof(NetFlipDelayCmd)},
  {"fringe_tracking", fringe_tracking_cmd_members, ARRAY_DIM(fringe_tracking_cmd_members),
   sizeof(NetFringeTrackingCmd)},
  {"thermal",   thermal_cmd_members,   ARRAY_DIM(thermal_cmd_members),
   sizeof(NetThermalCmd)},
  {"setFilter", setFilter_cmd_members, ARRAY_DIM(setFilter_cmd_members),
   sizeof(NetSetFilterCmd)},
  {"runScript", runScript_cmd_members, ARRAY_DIM(runScript_cmd_members),
   sizeof(NetRunScriptCmd)},
  {"scriptDir", scriptDir_cmd_members, ARRAY_DIM(scriptDir_cmd_members), 
   sizeof(NetRunScriptCmd)},
  {"gpib",      gpib_cmd_members,      ARRAY_DIM(gpib_cmd_members),  
   sizeof(NetGpibCmd)},
  {"servo",     servo_cmd_members,     ARRAY_DIM(servo_cmd_members), 
   sizeof(NetServoCmd)},
  {"rx",        rx_cmd_members,        ARRAY_DIM(rx_cmd_members), 
   sizeof(NetRxCmd)},
  {"roach",        roach_cmd_members,  ARRAY_DIM(roach_cmd_members), 
   sizeof(NetRoachCmd)},
  {"lna",        lna_cmd_members,      ARRAY_DIM(lna_cmd_members), 
   sizeof(NetLnaCmd)},
};

/*
 * Form the global table of objects that is used by the functions
 * in netobj.h.
 */
const NetObjTable rtc_cmd_table = {
  "cmd", cmd_objects, ARRAY_DIM(cmd_objects), sizeof(RtcNetCmd)
};

/**.......................................................................
 * Get the size of an RtcNetMsg object
 */
unsigned RtcNetMsg::size()
{
  // We want the maximum size of the data members of RtcNetMsg here.
  // Note that sizeof(NetMsg) and net_max_obj_size() can differ
  // because ASCII string members are sent with an additional int
  // specifying the length of the string, so to account for this, I
  // subtract off the size of the NetMsg container, then add back in
  // the net_max_obj_size().  Note that if RtcNetMsg contains methods
  // too, the size will be larger than necesary to accomodate just the
  // data members, but that's ok - we just don't want it to be
  // smaller!

  return sizeof(RtcNetMsg) - sizeof(NetMsg) + net_max_obj_size(&rtc_msg_table);
}
