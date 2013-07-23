#ifndef LABJACKMONITOR_H
#define LABJACKMONITOR_H
/**
 * @file LsThermal.h
 * 
 * Tagged: Thu Oct 18 15:49:12 PDT 2007
 * 
 * @author GCP data acquisition
 */

#include "gcp/util/common/Labjack.h"
#include "gcp/util/common/LabjackU3.h"
#include "gcp/antenna/control/specific/Board.h"
#include "gcp/control/code/unix/libunix_src/common/tcpip.h"
#include "gcp/control/code/unix/libunix_src/common/regmap.h"
#include "gcp/util/common/GenericTaskMsg.h"

#define NUM_TOTAL_VOLTAGES 12
#define NUM_DEVICES_EXPECTED 2
#define LNA_LABJACK_SERIAL_NUMBER 320050844
#define ADC_LABJACK_SERIAL_NUMBER 320050869

namespace gcp { 
  namespace antenna {
    namespace control {
      
      // define a class to hold the message
      class LabjackMonitorMsg : public gcp::util::GenericTaskMsg {
      public:

	enum MsgType {
	  CONNECT,
	  DISCONNECT, 
	  GET_VOLTAGE,
	};

	// A type for this message

	MsgType type_;
      };


      // Class to read data back from the DlpUsbThermal device
      
      class SpecificShare;
      
      class LabjackMonitor : public Board, public gcp::util::SpawnableTask<LabjackMonitorMsg> { 
	
      public:
	
	/**
	 * Constructor.
	 */
	LabjackMonitor();
	LabjackMonitor(SpecificShare* share, std::string name, bool spawn);
	
	/**
	 * Destructor.
	 */
	virtual ~LabjackMonitor();

	/**
	 * Way to process the message to figure out what to do.
	 */
	void processMsg(LabjackMonitorMsg* msg);

	// Pointer to resources.
	gcp::util::Labjack* labjack_;

	// connects:
	void connect();
	// disconnects:
	bool disconnect();

	bool connected_;
	int reConnectCounter_;
	// is the response valid?
	bool isValid_;

	// values from the previous reading
	float prevDrainCurrentVals_[NUM_RECEIVER_AMPLIFIERS];
	float prevDrainVoltageVals_[NUM_RECEIVER_AMPLIFIERS];
	float prevGateVoltageVals_[NUM_RECEIVER_AMPLIFIERS];
	
	/**
	 *  request voltage command issued from AntennaControl
	 **/
	void sendVoltRequest();

	/**
	 * Requests all Voltages
	 */
	void requestAllVoltages();
	
	/**
	 *  Data registers we store
	 */
	/**
         * The data registers we will store
         */
	RegMapBlock* drainCurrent_;
	RegMapBlock* drainVoltage_;
	RegMapBlock* gateVoltage_;

      }; // End class LabjackMonitor
    } // End namespace control
  } // End namespace antenna
} // End namespace gcp
#endif // End #ifndef LABJACKMONITOR_H
