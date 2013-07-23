#ifndef DLPTEMPSENSORS_H
#define DLPTEMPSENSORS_H
/**
 * @file LsThermal.h
 * 
 * Tagged: Thu Oct 18 15:49:12 PDT 2007
 * 
 * @author GCP data acquisition
 */
#include "gcp/util/common/DlpUsbThermal.h"
#include "gcp/antenna/control/specific/Board.h"
#include "gcp/control/code/unix/libunix_src/common/tcpip.h"
#include "gcp/control/code/unix/libunix_src/common/regmap.h"
#include "gcp/util/common/GenericTaskMsg.h"


namespace gcp { 
  namespace antenna {
    namespace control {
      
      // define a class to hold the message
      class DlpTempSensorsMsg : public gcp::util::GenericTaskMsg {
      public:

	enum MsgType {
	  CONNECT,
	  DISCONNECT, 
	  GET_TEMPS,
	};

	// A type for this message

	MsgType type_;
      };


      // Class to read data back from the DlpUsbThermal device
      
      class SpecificShare;
      
      class DlpTempSensors : public Board, public gcp::util::SpawnableTask<DlpTempSensorsMsg> { 
	
      public:
	
	/**
	 * Constructor.
	 */
	DlpTempSensors();
	DlpTempSensors(SpecificShare* share, std::string name, bool spawn);
	
	/**
	 * Destructor.
	 */
	virtual ~DlpTempSensors();

	/**
	 * Way to process the message to figure out what to do.
	 */
	void processMsg(DlpTempSensorsMsg* msg);

	// Pointer to resources.
	gcp::util::DlpUsbThermal* dlp_;

	// connects:
	bool connect();
	// disconnects:
	bool disconnect();

	// is the response valid?
	bool isValid_;

	// values from the previous reading
	float prevTempVals_[NUM_DLP_TEMP_SENSORS];

	/** 
	 * send a message to request temperatures
	 */ 
	void sendTempRequest();

	
	/**
	 * Requests all Temperatures
	 */
	void requestAllTemperatures();

	/**
	 * Requests all Voltages
	 */
	void requestAllVoltages();
	
	/**
	 *  Data registers we store
	 */
	RegMapBlock* dlpTempSensors_;  

      }; // End class DlpTempSensors
    } // End namespace control
  } // End namespace antenna
} // End namespace gcp
#endif // End #ifndef DLPTEMPSENSORS_H
