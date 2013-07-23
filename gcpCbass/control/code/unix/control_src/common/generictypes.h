#ifndef generictypes_h
#define generictypes_h

#include "source.h"   /* SRC_NAME_MAX */
#include "scan.h"     /* SCAN_NAME_MAX */
#include "arraymap.h"   /* (RegMap *) */

/*
 * Provide a macro that returns the number of elements of an array.
 */
#define DIMENSION(array) (sizeof(array)/sizeof(array[0]))

/*-----------------------------------------------------------------------
 * The Date datatype uses astrom.h::input_utc() to read UTC date and
 * time values like:
 *
 *  dd-mmm-yyyy hh:mm:ss.s
 *
 * where mmm is a 3-letter month name. It stores the date as a Modified
 * Julian Date in DoubleVariable's. It provides + and - operators for
 * the addition and subtraction of Interval values, along with all of the
 * relational operators except the ~ operator.
 */
DataType *add_DateDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Register datatype uses regmap.h::input_RegMapReg() to read
 * standard SZA archive-register specifications (eg. board.name[3-4]).
 *
 * It is stored in a RegisterVariable and supports the != == !~ ~ operators.
 */
DataType *add_RegisterDataType(Script *sc, char *name, ArrayMap *arraymap);

typedef struct {
  Variable v;       /* The base-class members of the variable (see script.h) */
  short regmap;     /* Register-map number */
  short board;      /* Register-board number */
  short block;      /* Block number on specified board */
  short index;      /* The first element of the block to be addressed */
  short nreg;       /* The number of elements to be addressed */
} RegisterVariable;

#define REGISTER_VARIABLE(v)  ((RegisterVariable *)(v))


/*-----------------------------------------------------------------------
 * The Wdir datatype is used to record writable directory names.
 * The path name is stored in a StringVariable. It supports
 * tilde expansion of home directories.
 *
 * It is stored in a StringVariable and supports the != == operators.
 */
DataType *add_WdirDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Dir datatype is used to record the names of accessible directories.
 *
 * The path name is stored in a StringVariable. It supports
 * tilde expansion of home directories.
 *
 * It is stored in a StringVariable and supports the != == operators.
 */
DataType *add_DirDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Dev datatype is used to record the names of accessible devices
 *
 * The path name is stored in a StringVariable. It supports
 * tilde expansion of home directories.
 *
 * It is stored in a StringVariable and supports the != == operators.
 */
DataType *add_DevDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The IntTime datatype is used to specify the hardware integration time
 * of the SZA electronics, as a power-of-2 exponent to be used to scale
 * the basic sample interval of 4.096e-4 seconds.
 *
 * It is stored in a UintVariable and supports != == <= >= < >.
 */
DataType *add_IntTimeDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Board datatype is used to specify an archive register board by
 * name.
 *
 * It is stored in a UintVariable and supports the != == operator.
 */
DataType *add_BoardDataType(Script *sc, char *name, ArrayMap *arraymap);

/*-----------------------------------------------------------------------
 * The Time datatype is used to specify a time of day (UTC or LST).
 *
 * It is stored as decimal hours in a DoubleVariable. It supports the
 * standard arithmentic != == <= >= < > operators.
 */
DataType *add_TimeDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Interval datatype is used to specify a time interval.
 *
 * It is stored as decimal seconds in a DoubleVariable. It supports the
 * standard arithmentic != == <= >= < > operators.
 */
DataType *add_IntervalDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Antennas datatype is used to select a sub-set of the SZA
 * receivers.
 *
 * It is stored in a SetVariable and supports the standard
 * set + - !~ ~ != == operators.
 */ 
DataType *add_AntennasDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The SysType datatype is used to tell the shutdown and reboot commands
 * which sub-system to reset.
 *
 * It is stored in a ChoiceVariable and supports the != == operators.
 */
DataType *add_SysTypeDataType(Script *sc, char *name);
/*
 * Enumerate the subsystems addressed by the SysType datatype.
 */
typedef enum {
  SYS_CPU,        /* Reboot or shutdown the CPU */
  SYS_RTC,        /* Reboot or shutdown the real-time-control system */
  SYS_CONTROL, /* Restart or shutdown the control program */
  SYS_PMAC        /* Restart the pmac */
} SysType;

/*-----------------------------------------------------------------------
 * The TimeScale datatype is used with Time variables to specify which
 * time system they refer to.
 *
 * It is stored in a ChoiceVariable and supports the != == operators.
 */
DataType *add_TimeScaleDataType(Script *sc, char *name);
/*
 * Enumerate the supported time systems.
 */
typedef enum {
  TIME_UTC,       /* Universal Coordinated Time */
  TIME_LST        /* Local Sidereal Time */
} TimeScale;

/*-----------------------------------------------------------------------
 * The SwitchState datatype is used to specify whether to turn a switch
 * on or off.
 *
 * It is stored in a ChoiceVariable and supports the != == operators.
 */
DataType *add_SwitchStateDataType(Script *sc, char *name);
/*
 * Enumerate the supported switch states.
 */
typedef enum {SWITCH_ON, SWITCH_OFF} SwitchState;

/*-----------------------------------------------------------------------
 * The AcquireTargets datatype is used to tell the until() command which
 * operations to wait for.
 *
 * It is stored in a SetVariable and supports the standard
 * set + - !~ ~ != == operators.
 */
DataType *add_DelayTargetDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Scan datatype is used for specification of a scan by its name
 * in the scan catalog.
 *
 * It is stored in a ScanVariable and supports the != == operators.
 */
DataType *add_ScanDataType(Script *sc, char *name);

typedef struct {
  Variable v;              /* Generic members of the variable (see script.h) */
  char name[gcp::control::SCAN_NAME_MAX]; /* The name of the scan */
} ScanVariable;

#define SCAN_VARIABLE(v)  ((ScanVariable *)(v))

/*-----------------------------------------------------------------------
 * The Source datatype is used for specification of a source by its name
 * in the source catalog.
 *
 * It is stored in a SourceVariable and supports the != == operators.
 */
DataType *add_SourceDataType(Script *sc, char *name);

typedef struct {
  Variable v;              /* Generic members of the variable (see script.h) */
  char name[gcp::control::SRC_NAME_MAX]; /* The name of the source */
} SourceVariable;

#define SOURCE_VARIABLE(v)  ((SourceVariable *)(v))

/*-----------------------------------------------------------------------
 * The TransDev datatype is used for specification of a device by its name
 * in the transaction catalog.
 *
 * It is stored in a TransDevVariable and supports the != == operators.
 */
DataType *add_TransDevDataType(Script *sc, char *name);

typedef struct {
  Variable v;              // Generic members of the variable (see script.h) 
  // The name of the device
  char name[gcp::control::TransactionManager::DEV_NAME_MAX+1]; 
} TransDevVariable;

#define TRANSDEV_VARIABLE(v)  ((TransDevVariable *)(v))

/*-----------------------------------------------------------------------
 * The TransLocation datatype is used for specification of a location
 * by its name in the transaction catalog.
 *
 * It is stored in a TransLocationVariable
 */
DataType *add_TransLocationDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The TransSerial datatype is used for specification of a serial
 * by its name in the transaction catalog.
 *
 * It is stored in a TransSerialVariable
 */
DataType *add_TransSerialDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The TransWho datatype is used for specification of the person
 * logging the transaction
 *
 * It is stored in a TransWhoVariable
 */
DataType *add_TransWhoDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The TransComment datatype is used for specification of the person
 * logging the transaction
 *
 * It is stored in a TransCommentVariable
 */
DataType *add_TransCommentDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Model datatype is used to select between the optical and radio
 * pointing models.
 *
 * It is stored in a ChoiceVariable and supports the != == operators.
 */
DataType *add_ModelDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Latitude datatype is used to specify the latitude of a location
 * on the surface of the Earth (-90..90 degrees).
 *
 * It is stored in degrees as a DoubleVariable and supports the
 * standard arithmetic != == <= >= < > operators.
 */
DataType *add_LatitudeDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Longitude datatype is used to specify the longitude of a location
 * on the surface of the Earth (-180..180 degrees).
 *
 * It is stored in degrees as a DoubleVariable and supports the
 * standard arithmetic != == <= >= < > operators.
 */
DataType *add_LongitudeDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Azimuth datatype is used to specify a target azimuth for the
 * telescope. The azimuths of the compass points are N=0, E=90, S=180
 * and W=270 degrees (-360..360 degrees).
 *
 * It is stored in degrees as a DoubleVariable and supports the
 * standard arithmetic != == <= >= < > operators.
 */
DataType *add_AzimuthDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The DeckAngle datatype is used to specify the position of the rotating
 * platform on which the dishes are mounted (-360..360 degrees).
 *
 * It is stored in degrees as a DoubleVariable and supports the
 * standard arithmetic != == <= >= < > operators.
 */
DataType *add_DeckAngleDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Elevation datatype is used to specify the target elevation angle
 * of the telescope (-90..90 degrees).
 *
 * It is stored in degrees as a DoubleVariable and supports the
 * standard arithmetic != == <= >= < > operators.
 */
DataType *add_ElevationDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The PointingOffset datatype is used to specify a temporary offset of
 * one of the telescope axes from the current pointing (-180..180).
 *
 * It is stored in degrees as a DoubleVariable and supports the
 * standard arithmetic != == <= >= < > operators.
 */
DataType *add_PointingOffsetDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Flexure datatype is used to specify the degree of gravitational
 * drooping of the telescope as a function of elevation (-90..90 degrees).
 *
 * It is stored in degrees per cosine of elevation as a DoubleVariable
 * and supports the standard arithmetic != == <= >= < > operators.
 */
DataType *add_FlexureDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Tilt datatype is used to specify the misalignment tilt of a
 * telescope axis (-90..90 degrees).
 *
 * It is stored in degrees as a DoubleVariable and supports the
 * standard arithmetic != == <= >= < > operators.
 */
DataType *add_TiltDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Tracking data type is used for specifying the type of tracking,
 * phase tracking or pointing tracking
 */
DataType *add_TrackingDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Altitude datatype is used to specify the height of the telescope
 * above the standard geodetic spheroid (-10000..10000 meters).
 *
 * It is stored in meters as a DoubleVariable and supports the
 * standard arithmetic != == <= >= < > operators.
 */
DataType *add_AltitudeDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The GpibDev datatype is used to select a device on the GPIB bus (0..30).
 *
 * It is stored in a UintVariable and supports the standard arithmetic
 * != == <= >= < > operators.
 */
DataType *add_GpibDevDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The GpibCmd datatype is used to specify a command string to be sent
 * to a device on the GPIB bus.
 *
 * It is stored as a string of at most rtcnetcoms.h::GPIB_MAX_DATA
 * characters in a StringVariable and supports the != == !~ ~ operators.
 */
DataType *add_GpibCmdDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Features datatype is used to specify one or more features to
 * be highlighted in a subsequent archive frame.
 */
DataType *add_FeaturesDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The DeckMode datatype is used to tell the tracker task how to
 * position the deck axis while tracking sources.
 *
 * It is stored in a ChoiceVariable and supports the != == operators.
 */
DataType *add_DeckModeDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Attenuation datatype is used for configuring channelizer
 * attenuators to insert attenuations in steps of 1db over the range
 * 0..31db.
 *
 * It is stored in a UintVariable and supports != == <= >= < >.
 */
DataType *add_AttenuationDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The ArcType datatype is used to tell the close, flush and open
 * commands which archiving file to operate on.
 *
 * It is stored in a ChoiceVariable and supports the != == operators.
 * The enumerators that become stored in ArcType values are taken from
 * arcfile.h::ArcFileType.
 */
DataType *add_ArcFileDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The FeatureChange datatype is used to tell the mark command what to
 * do with the set of features that it is given.
 *
 * It is stored in a ChoiceVariable and supports the != == operators.
 * The enumerators that become stored in FeatureChange values are taken from
 * rtcnetcoms.h::FeatureMode.
 */
DataType *add_FeatureChangeDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The BitMask datatype is used to set bit-mask as a set of 32 bits.
 *
 * It is stored in a SetVariable and supports the standard
 * set + - !~ ~ != == operators.
 */ 
DataType *add_BitMaskDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The BitMaskOper datatype is used to specify whether a given bit-mask
 * should be used to set bits, clear bits or to be used verbatim.
 *
 * It is stored in a ChoiceVariable using the enumerators defined in
 * rtcnetcoms.h, and supports the standard set != and == operators.
 */
DataType *add_BitMaskOperDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The Script datatype is used for entering the name and arguments
 * of a script.
 */
DataType *add_ScriptDataType(Script *sc, char *name);

typedef struct {
  Variable v;       /* The base-class members of the variable (see script.h) */
  Script *sc;       /* The compiled script */
} ScriptVariable;

#define SCRIPT_VARIABLE(v)  ((ScriptVariable *)(v))

/*-----------------------------------------------------------------------
 * The OptCamTarget datatype is used to specify the target of an optical 
 * camera command.
 */
DataType *add_OptCamTargetDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The OptCamAction datatype is used to specify the action of an optical 
 * camera command.
 */
DataType *add_OptCamActionDataType(Script *sc, char *name);

/*.......................................................................
 * The OptCamCount datatype is used for specifying optical camera integer 
 * stepper motor steps.
 *
 * It is stored in an IntVariable with +10000 used to specify
 * a device to be switched on, and -10000 used to specify a device to be 
 * switched off. It supports the standard arithmentic
 * != == <= >= < > operators.
 */
DataType *add_OptCamCountDataType(Script *sc, char *name);

/*.......................................................................
 * The Peak datatype is used for specifying which peak offset from the
 * frame grabber to return.
 */
DataType *add_PeakDataType(Script *sc, char *name);

/*.......................................................................
 * The Imstat datatype is used for specifying which frame grabber image 
 * statistic to return
 */
DataType *add_ImstatDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The following datatype is used to set slew rates, in terms of a
 * percentage of the maximum speed available.
 *
 * It is stored in a UintVariable and supports != == <= >= < >.
 */
DataType *add_SlewRateDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The EmailAction datatype is used to specify whether to enable,
 * disable, or turn the pager on
 *
 * It is stored in a ChoiceVariable using enumerators defined in
 * rtcnetcoms.h, and supports the != == operators.  
*/
DataType *add_EmailActionDataType(Script *sc, char *name);

typedef enum {EMAIL_ADD, EMAIL_CLEAR, EMAIL_LIST} EmailAction;

/*-----------------------------------------------------------------------
 * The PagerState datatype is used to specify whether to enable,
 * disable, or turn the pager on
 *
 * It is stored in a ChoiceVariable using enumerators defined in
 * rtcnetcoms.h, and supports the != == operators.  
*/
DataType *add_PagerStateDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The PagerDev datatype is used to specify a pager device
 *
 * It is stored in a ChoiceVariable using enumerators defined in
 * rtcnetcoms.h, and supports the != == operators.  
*/
DataType *add_PagerDevDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The DcPower datatype is used to specify the target power level for
 * the downconverter.  A value < 0 is used to denote a preset level.
 *
 * It is stored in a Double variabel and supports the standard
 * arithmetic != == <= >= < > operators.
 */
DataType *add_DcPowerDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The DelayType datatype is used to specify the type of delay to
 * configure
 */
DataType *add_DelayTypeDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The RotSense datatype is used to select the sens of the deck
 * rotation.
 */ 
DataType *add_RotSenseDataType(Script *sc, char *name);

/*-----------------------------------------------------------------------
 * The ImDir datatype is used to select the orientation of the frame
 * grabber image axes
 */ 
DataType *add_ImDirDataType(Script *sc, char *name);


DataType *add_FlatfieldModeDataType(Script *sc, char *name);

DataType *add_FgChannelDataType(Script *sc, char *name);

DataType *add_PointingTelescopesDataType(Script *sc, char *name);

#endif


