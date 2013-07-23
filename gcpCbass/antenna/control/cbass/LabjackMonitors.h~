#ifndef LNABIASMONITOR_H
#define LNABIASMONITOR_H
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
namespace gcp { 
  namespace antenna {
    namespace control {
      
      // define a class to hold the message
      class LnaBiasMonitorMsg : public gcp::util::GenericTaskMsg {
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
      
      class LnaBiasMonitor : public Board, public gcp::util::SpawnableTask<LnaBiasMonitorMsg> { 
	
      public:
	
	/**
	 * Constructor.
	 */
	LnaBiasMonitor();
	LnaBiasMonitor(SpecificShare* share, std::string name, bool spawn);
	
	/**
	 * Destructor.
	 */
	virtual ~LnaBiasMonitor();

	/**
	 * Way to process the message to figure out what to do.
	 */
	void processMsg(LnaBiasMonitorMsg* msg);

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

      }; // End class LnaBiasMonitor
    } // End namespace control
  } // End namespace antenna
} // End namespace gcp
#endif // End #ifndef LNABIASMONITOR_H
