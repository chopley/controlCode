#ifndef GCP_ANTENNA_CONTROL_TFP_H
#define GCP_ANTENNA_CONTROL_TFP_H

/**
 * @file Tfp.h
 * 
 * Tagged: Fri 13-Jan-06 22:11:35
 * 
 * @author username: Command not found.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <vector>

#include "gcp/util/common/RegDate.h"
#include "gcp/util/common/TimeVal.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      class Tfp {
      public:
	
	//------------------------------------------------------------
	// Device register offsets from BAR0 starting address
	//------------------------------------------------------------

	enum CtlRegOffset {

	  // Accessing this register latches the current time and
	  // status in the TIME0-TIME1 registers

	  CTL_OFF_TIMEREQ   =  0x0,

	  // Accessing this register latches the current time and
	  // status in the EVENT0-EVENT1 registers

	  CTL_OFF_EVENTREQ  =  0x4,

	  // Accessing this register release the EVENTx time capture
	  // lockout function if it has been enabled, allowing the
	  // event input or periodic output to capture a new time.
	  // The event time capture lockout is enabled using bit 1 of
	  // the CONTROL register, below

	  CTL_OFF_UNLOCK    =  0x8,

	  // Reserved

	  CTL_OFF_RESERVED1 =  0xC,

	  // The control register

	  CTL_OFF_CONTROL   = 0x10,

	  // The ACK register is used to prevent DPRAM contention when
	  // the same address is accessed simultaneously from both
	  // sides

	  CTL_OFF_ACK       = 0x14,

	  // Bits 0-4 of the MASK register correspond to interrupt
	  // sources

	  CTL_OFF_MASK      = 0x18,

	  // Same bit definitions as the MASK register, indicating
	  // which interrupt source was activated

	  CTL_OFF_INTSTAT   = 0x1C,

	  // These registers hold the programmed Time Coincidence
	  // Strobe time. The contents of these registers depend on
	  // the time format selected. The Strobe time is programmable
	  // from hours through microseconds in the decimal time
	  // format. When the time format is set to binary, only the
	  // 22 least significant bits of the major time are used (in
	  // addition to microseconds), this allows the user to
	  // program the Strobe to become activated up to 48 days
	  // beyond the current time.

	  CTL_OFF_MINSTRB   = 0x20,
	  CTL_OFF_MAJSTRB   = 0x24,

	  // Reserved

	  CTL_OFF_RESERVED2 = 0x28,
	  CTL_OFF_RESERVED3 = 0x2C,

	  // These registers hold time captured by an access of the
	  // TIMEREQ register. The contents of these registers depend
	  // on the time format selected.

	  CTL_OFF_TIME0     = 0x30,
	  CTL_OFF_TIME1     = 0x34,

	  // These registers hold time captured when the EVENTREQ
	  // register is accessed by an Event Input (if enabled), or a
	  // PPO is generated (if enabled). The contents of these
	  // registers depend on the time format selected

	  CTL_OFF_EVENT0    = 0x38,
	  CTL_OFF_EVENT1    = 0x3C
	};

	//------------------------------------------------------------
	// CONTROL register bit definitions
	//------------------------------------------------------------

	// The EVSOURCE bit selects one of two signal sources for
	// capturing time in the EVENTx registers; either the Event
	// Input signal from the Signal I/O connector or the
	// Programmable Periodic Output. When the PPO is selected as
	// the Event Source, the PPO and Event Input are internally
	// or'd, eliminating the need for an external physical
	// connection. The EVSOURCE bit does not affect the Event
	// Interrupt, as the Event Input signal is the only source for
	// the Event Interrupt. This allows the Event Input to
	// generate a PCI interrupt (without time capture) when the
	// board is configured to use the Programmable Periodic Output
	// (PPO) for EVENTx time capture (see Chapter 4)
	//
	// The EVENTEN bit is used to enable an external signal
	// capture of time into the EVENTx registers. This bit
	// controls the PPO and Event Input for event time
	// capture.Enabling the Lockout function via the LOCKEN bit
	// allows only the first instance of the selected signal
	// source to latch time in the EVENTx registers, locking out
	// any subsequent events, external or PPO.  Use the UNLOCK
	// register to re-arm the circuit

	enum {

	  // EVENTx capture lockout enable

	  CTL_LOCKEN_BIT   = 0, 

	  // EVENTx Time Capture Register Source Select:
	  // 0 - Event Input (Select Active edge with EVSENSE)
	  // 1 - Programmable Periodic (rising edge active only)

	  CTL_EVSOURCE_BIT = 1, 

	  // Reserved

	  CTL_RESERVED_BIT = 2,

	  // Event Capture register enable:
	  // 0 - disable
	  // 1 - enable (use EVSOURCE to select event source)

	  CTL_EVENTEN_BIT  = 3,

	  // Time Coincidence strobe output enable
	  // 0 - disable (strobe output is held low)
	  // 1 - enable 

	  CTL_STREN_BIT    = 4,

	  // Time coincidence strobe mode
	  // 0 - Use major and minor time for strobe function
	  // 1 - Use minor time only for strobe function

	  CTL_STRMODE_BIT  = 5,

	  // Output Frequency select
	  // 00 = 10 MHz
	  // 01 =  5 MHz
	  // 1X =  1 MHz

	  CTL_FREQSEL0_BIT = 6,
	  CTL_FREQSEL1_BIT = 7,
	};

	//------------------------------------------------------------
	// ACK register bit definitions
	//------------------------------------------------------------

	enum {

	  // Set by the TFP to acknowledge receipt of a user command
	  // from the DPRAM input area.  User can clear this bit by
	  // writing to the ACK register with bit 0 set

	  ACK_CMD_BIT = 0, 

	  // Set by the TFP to acknowledge that a GPS packet is
	  // available in the DPRAM GPS packet area. User can clear
	  // this bit by writing to the ACK register with bit 2 set

	  ACK_GPS_BIT = 2, 

	  // User write to the ACK register with bit 7 set to tell the
	  // TFP to read a command from the DPRAM input area

	  ACK_READ_BIT = 7, 
	};

	enum {
	  READ_CMD = 0x81 // ACK bit 1 + ACK bit 7
	};

	//------------------------------------------------------------
	// MASK register definitions
	//------------------------------------------------------------

	enum {
	  MASK_EVENT_BIT    = 0, // Event input has occurred
	  MASK_PERIODIC_BIT = 1, // Periodic output has occurred
	  MASK_TIME_BIT     = 2, // Time coincidence strobe has occurred
	  MASK_1PPS_BIT     = 3, // 1PPS output has occurred
	  MASK_GPS_BIT      = 4  // A GPS data packet is available
	};

	//------------------------------------------------------------
	// DPRAM command codes
	//------------------------------------------------------------

	enum DpramCmdId {
	  DP_CMD_TIME_MODE           = 0x10,
	  DP_CMD_TIME_FORMAT         = 0x11,
	  DP_CMD_SET_MAJOR           = 0x12,
	  DP_CMD_SET_YEAR            = 0x13,
	  DP_CMD_SET_PERIODIC_OUTPUT = 0x14,
	  DP_CMD_SET_INP_FORMAT      = 0x15,
	  DP_CMD_SET_INP_MOD         = 0x16,
	  DP_CMD_SET_PROP_DELAY      = 0x17,
	  DP_CMD_REQ_DATA            = 0x19,
	  DP_CMD_SOFT_RESET          = 0x1A,
	  DP_CMD_SET_OUT_FORMAT      = 0x1B,
	  DP_CMD_GEN_OFFSET          = 0x1C,
	  DP_CMD_LCL_OFFSET          = 0x1D,
	  DP_CMD_SET_CLK_SRC         = 0x20,
	  DP_CMD_GPS_FMT             = 0x33,
	  DP_CMD_SET_GPS_MODE_FLAG   = 0x34,
	  DP_CMD_YEAR_AUTO_INC       = 0x42,
	  DP_CMD_REQ_ASSY            = 0xf4,
	  DP_CMD_REQ_MODEL           = 0xf6
	};

	enum GpsFormat {
	  FORMAT_UTC       = 0x0,
	  FORMAT_GPS       = 0x1,
	};

	enum TimeFormat {
	  FORMAT_BCD       = 0x00,
	  FORMAT_UNIX      = 0x01
	};

	enum TimeCodeFormat {
	  FORMAT_IRIGA  = 0x41,
	  FORMAT_IRIGB  = 0x42,
	  FORMAT_IEEE   = 0x49,
	  FORMAT_NASA36 = 0x4E,
	};

	enum InputTimeCodeModType {
	  INP_MOD_AMP       = 'M', // Amplitude modulated signal
	  INP_MOD_PULSE     = 'D', // Pulse-code modulated signal
	};

	enum ClockSource {
	  CLK_INTERNAL  = 'I', // Amplitude modulated signal
	  CLK_EXTERNAL  = 'E', // Pulse-code modulated signal
	};

	enum OutputFreq {
	  FREQ_10MHZ = 0x0,
	  FREQ_5MHZ  = 0x1,
	  FREQ_1MHZ  = 0x2
	};

	enum TimeMode {
	  MODE_TIMECODE   = 0x00,
	  MODE_FREERUN    = 0x01,
	  MODE_1PPS       = 0x02,
	  MODE_REAL_CLOCK = 0x03,
	  MODE_GPS        = 0x06
	};

	enum {
	  NONE         =  0x00,
	  INT_EVENT    =  0x01,
	  INT_PERIODIC =  0x02,
	  INT_TIME     =  0x04,
	  INT_1PPS     =  0x08,
	  INT_GPS      =  0x10,
	  ALL = INT_EVENT | INT_PERIODIC | INT_TIME | INT_1PPS | INT_GPS
	};

	// The name by which the device is accessible from user-space

	static const std::string devName_;

	
	// The size of the CTL region of the card
	//
	// This appears as a 4096 byte array

	static const unsigned ctlSizeInBytes_   = 0x1000;

	
	// Offset of the control registers from the start of the
	// card's memory space

	static const unsigned ctlOffsetInBytes_ = 0x0;

	
	// The size of the DPRAM region of the card
	//
	// This appears as a 4096 byte array

	static const unsigned dpramSizeInBytes_   = 0x1000;

	// Offset of the dpram registers from the start of the
	// card's memory space
	
	static const unsigned dpramOffsetInBytes_ = 0x1000;

	
	// Constructor.

	Tfp(bool simulate=false);
	
	
	// Destructor.

	virtual ~Tfp();
	
	void open();
	void close();
	void readBcdTime(gcp::util::TimeVal& tVal);
	void readUnixTime(gcp::util::TimeVal& tVal);

	void setYear(unsigned short year);
	void setMajorTime(unsigned int seconds);
	void setMajorTime(unsigned short year, unsigned short dayNo, 
			  unsigned char hours, unsigned char minutes, 
			  unsigned char seconds);

	void setTimeMode(TimeMode mode);
	void setTimeFormat(TimeFormat format);
	void setInputTimeCodeFormat(TimeCodeFormat format);
	void setOutputTimeCodeFormat(TimeCodeFormat format);
	void setInputTimeCodeModType(InputTimeCodeModType mod);
	void setFrequencyOutput(bool syncToPps, double dutyCyclePerc, double freqInHz);
	void setPeriodicOutput(bool syncToPps, unsigned short n1, unsigned short n2);
	void setOutputClockFreq(OutputFreq freq);
	unsigned short readYear();
	void requestModelId();
	void requestAssemblyPartNo();
	void softReset();

	void setGeneratorTimeOffset(short offset, bool halfHour=false);
	void setLocalTimeOffset(short offset, bool halfHour=false);
	void setPropagationDelay(int delayIn100ns);
	void setClockSource(ClockSource src);
	void setGpsFormat(GpsFormat format);
	void enableGpsModeFlag(bool enable);
	void enableYearAutoIncrement(bool enable);

	void defaultSetUp();
	void specificSetUp();

	// Utility functions for reading and writing 

	static void testBits(unsigned int iVal);
	static void printBits(unsigned char c);
	static void printBits(unsigned short s);
	static void printBits(unsigned int i);
	static unsigned short constructShort(unsigned char msb, unsigned char lsb);
	static unsigned constructInt(unsigned char msb, unsigned char smsb, 
				     unsigned tmsb, unsigned char lsb);

	void enableInterrupt(unsigned mask, bool enable);

	void getDate(gcp::util::RegDate& date);
	void getDate(gcp::util::TimeVal& timeVal);

      private:

	bool simulate_;

	struct DpramCmd {
	  DpramCmd(DpramCmdId cmd);
	  void packVal(unsigned char val);
	  void packVal(unsigned short val);
	  void packVal(unsigned int val);

	  std::vector<unsigned char> data_;
	};

	int fd_;

	// Members for DPRAM access

	void* dpramPtr_;                 // mmap'd pointer to DPRAM
	unsigned dpramPtrLength_;        // Length in bytes, of the
					 // memory pointed to by
					 // dpramPtr_
	unsigned short dpramInOffset_;   // offset of the input area,
					 // relative to the start of
					 // dpram
	unsigned short dpramOutOffset_;  // offset of the output
					 // area, relative to the
					 // start of dpram
	unsigned short dpramGpsOffset_;  // offset of the gps area,
					 // relative to the start of
					 // dpram
	unsigned short dpramYearOffset_; // offset of the year area,
					 // relative to the start of
					 // dpram
	
	unsigned char* dpramInPtr_;
	unsigned char* dpramOutPtr_;
	unsigned char* dpramGpsPtr_;
	unsigned char* dpramYearPtr_;

	// Members for Control register access

	void* ctlPtr_;                    // mmap'd pointer to DPRAM
	unsigned ctlPtrLength_;           // Length in bytes, of the
					  // memory pointed to by
					  // ctlPtr_

	unsigned* ctlAckPtr_;
	unsigned* ctlCtlPtr_;
	unsigned* ctlTime0Ptr_;
	unsigned* ctlTime1Ptr_;
	unsigned* ctlTimeReqPtr_;

	void initialize();
	void resetCtlPointers();
	void resetDpramPointers();

	void mapCtlPtr(unsigned byteOffset=0);
	void mapDpramPtr(unsigned byteOffset=0);
	void map();
	void unmap();
	unsigned char* ctlPtr();
	unsigned char* dpramPtr();
	void sendDpramCmd(DpramCmd& cmd, bool wait=true);
	void waitForAck();
	void setBit(unsigned& mask, unsigned short bit, bool enable);

      }; // End class Tfp
      
    } // End namespace control
  } // End namespace antenna
} // End namespace gcp



#endif // End #ifndef GCP_ANTENNA_CONTROL_TFP_H
