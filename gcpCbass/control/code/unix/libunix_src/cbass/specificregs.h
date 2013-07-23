#ifndef sptregs_h
#define sptregs_h

#include "arraymap.h"
#include "genericregs.h"

// Number of antenna pointing positions reported per archive frame

static const unsigned int DATA_SAMPLES_PER_FRAME     = 100; // Must be an integer multiple of POSITIONS_PER_FRAME
static const unsigned int POSITION_SAMPLES_PER_FRAME = 1;
static const unsigned int SERVO_POSITION_SAMPLES_PER_FRAME = 5;
static const unsigned int SERVO_NUMBER_MOTORS = 4;
static const unsigned int THERMAL_SAMPLES_PER_FRAME  = 5;
static const unsigned int RECEIVER_SAMPLES_PER_FRAME = 100;
static const unsigned int RECEIVER_SAMPLES_PER_TRANSFER = 5;
static const unsigned int NUM_LABJACK_VOLTS             = 12;

static const unsigned int TOTAL_NUMBER_OF_CHANNELS       = 128;
static const unsigned int CHANNELS_PER_ROACH             = 64;
static const unsigned int NUM_RECEIVER_CHANNELS          = 6;
static const unsigned int NUM_RECEIVER_AMPLIFIERS        = 4;
static const unsigned int NUM_AMPLIFIER_STAGES           = 1;
static const unsigned int NUM_RECEIVER_SWITCH_CHANNELS   = 24;
static const unsigned int NUM_RECEIVER_DIAGNOSTICS       = 4;
static const unsigned int NUM_TEMP_SENSORS               = 8;
static const unsigned int NUM_DLP_TEMP_SENSORS           = 8;
static const unsigned int NUM_POWER_OUTLETS              = 8;
static const unsigned int NUM_ALPHA_CORRECTIONS          = 4;


static const unsigned int MS_PER_DATA_SAMPLE           = 1000 / DATA_SAMPLES_PER_FRAME;
static const unsigned int MS_PER_POSITION_SAMPLE       = 1000 / POSITION_SAMPLES_PER_FRAME;
static const unsigned int MS_PER_SERVO_POSITION_SAMPLE = 1000 / SERVO_POSITION_SAMPLES_PER_FRAME;
static const unsigned int MS_PER_THERMAL_SAMPLE        = 1000 / THERMAL_SAMPLES_PER_FRAME;
static const unsigned int MS_PER_RECEIVER_SAMPLE       = 1000 / RECEIVER_SAMPLES_PER_FRAME;

static const unsigned int NUM_BOLOMETERS = 961; // 960 bolometers plus calibrator phase
static const unsigned int NUM_SQUIDS = NUM_BOLOMETERS / 8;
static const unsigned int NUM_SQUID_CONTROLLERS = 18;
static const unsigned int NUM_BOLOMETERS_PER_BOARD = 16;
static const unsigned int NUM_BOARDS = 60;

static const unsigned int NUM_POINTING_TELESCOPES = 3;
static const unsigned int NUM_POINTING_TELESCOPE_AD_VALUES = 8;
static const unsigned int NUM_POINTING_TELESCOPE_DIO_BITS = 24;

static const unsigned int NUM_DEICING_AD_VALUES = 256;
static const unsigned int NUM_DEICING_DIO_BITS  = 24;

static const unsigned int NUM_OPTICAL_BENCH_ACTUATORS = 6;

static const unsigned int NUM_STRUCTURE_TEMP_SENSORS = 60;

static const unsigned int NUM_RAW_ENCODERS = 2;
static const unsigned int NUM_MOTOR_CURRENTS = 4;
static const unsigned int NUM_SUM_TACHS = 2;
static const unsigned int NUM_LINEAR_SENSORS = 4;

// The length of the NULL-terminated string needed to store a
// bolo/squid string id

static const unsigned int DIO_ID_LEN = 7;

typedef RegMap SzaRegMap;
typedef ArrayMap SzaArrayMap;

RegMap *new_ReceiverRegMap(void);
RegMap *new_SquidRegMap(void);
RegMap *new_ReceiverConfigRegMap(void);
RegMap *new_PointingTelRegMap(void);
RegMap *new_DeicingRegMap(void);
RegMap *new_HoloRegMap(void);
RegMap *new_SecondaryRegMap(void);
RegMap *new_FridgeRegMap(void);
RegMap *del_ReceiverRegMap(void);

RegMap *new_SptArrRegMap(void);
RegMap *del_SptArrRegMap(SzaRegMap *regs);
long net_SptArrRegMap_size(void);
int net_put_SptArrRegMap(gcp::control::NetBuf *net);

#endif
