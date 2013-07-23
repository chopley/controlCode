#ifndef GCP_ANTENNA_CONTROL_ANTENNACONTROL_H
#define GCP_ANTENNA_CONTROL_ANTENNACONTROL_H

/**
 * @file AntennaControl.h
 * 
 * Started: Wed Feb 25 16:40:05 PST 2004
 * 
 * @author Erik Leitch
 */
#include <string>
#include <sys/socket.h>

// Shared control code includes

#include "gcp/util/specific/Directives.h"
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/Logger.h"
#include "gcp/util/common/LogMsgHandler.h"
#include "gcp/util/common/NetCommHandler.h"
#include "gcp/util/common/TcpClient.h"

#include "gcp/antenna/control/specific/AntennaControlMsg.h"
#include "gcp/antenna/control/specific/AntNetCmdForwarder.h"
#include "gcp/antenna/control/specific/FrameSender.h"
#include "gcp/antenna/control/specific/SpecificTask.h"

#define ANTENNACONTROL_TASK_FWD_FN(fn) void (fn)(AntennaControlMsg* msg)

namespace gcp {
  namespace antenna {
    namespace control {

      /**
       * Incomplete type specifications.
       */
      class AntennaMaster;
      class AntennaGpib;
      class LnaBiasMonitor;
      class AdcMonitor;
      class DlpTempSensors;

      class AntennaControl :
	public SpecificTask,
	public gcp::util::GenericTask<AntennaControlMsg> {
	
	public:
	
	/**
	 * Constructor.
	 */
	  AntennaControl(AntennaMaster* parent, bool simGpib, bool simDlp, bool simLna, bool simAdc);
	
	/**
	 * Destructor.
	 */
	virtual ~AntennaControl();
	
	/**
	 * Public method to get a reference to the FrameSender object.
	 */
	FrameSender* getSender();

	/**
	 * Public method to get a reference to our shared resource
	 * object.
	 */
	SpecificShare* getShare();

	void sendGpibConnectedMsg(bool connected);

      private:

	/**
	 * We declare AntennaMaster a friend because its
	 * forwardCommsMsg() method will call our sendTaskMsg() method
	 */
	friend class AntennaMaster;

	/**
	 * A static pointer to ourselves for use in static functions.
	 */
	static AntennaControl* control_;

	/**
	 * The parent of this task.
	 */
	AntennaMaster* parent_;

	// The task that will be used to communicate with GPIB devices

	AntennaGpib* gpibTask_;


	// The task that will be used to communicate with LNA power supply

	LnaBiasMonitor* lnaTask_;


	// The task that will be used to communicate with the ADC labjack

	AdcMonitor* adcTask_;

	/**
	 * An object encapsulating conversion between net commands and
	 * task messages.
	 */
	AntNetCmdForwarder* forwarder_;

	/** 
	 * the object that gets the temperature data
	 */
	DlpTempSensors* dlp_;

	/**
	 * A send stream for handling messages to be sent back to the
	 * control program
	 */
	gcp::util::NetCommHandler netCommHandler_;
	
	/**
	 * An object for managing our connection to the host machine
	 */
	gcp::util::TcpClient client_;

	/**
	 * An object for sending data back to the control program.
	 */
	FrameSender sender_;

	/**
	 * An object for managing log messages
	 */
	gcp::util::LogMsgHandler logMsgHandler_;
	
	/**
	 * Attempt to open a connection to the control host.
	 */
	bool connect();

	/**
	 * True when connected to the RTC control port
	 */
	bool isConnected(); 

	/**
	 * Close a connection to the control host.
	 */
	void disconnect();

	/**
	 * Attempt to connect to the host
	 */
	void connectControl(bool reEnable);
	  
	/**
	 * Attempt to connect to the host
	 */
	void disconnectControl();
	  
	/**
	 * Overwrite the base-class method.
	 */
	void serviceMsgQ();

	/**
	 * Send a message to the parent about the connection status of
	 * the host
	 */
	void sendControlConnectedMsg(bool connected);

	/**
	 * Send a string to be logged back to the control program.
	 */
	static LOG_HANDLER_FN(sendLogMsg);
	
	/**
	 * Send an error string to be logged back to the control
	 * program.
	 */
	static LOG_HANDLER_FN(sendErrMsg);
	
	static void sendNetMsg(std::string& logStr, bool isErr);

	/**
	 * Overwrite the base-class method.
	 */
	void processMsg(AntennaControlMsg* msg);

	/**
	 * Pack a network message intended for the ACC
	 */
	void packNetMsg(AntennaControlMsg* msg);

	/**
	 * Send a greeting message
	 */
	void sendGreeting();

	/**
	 * A handler called when a command has been read from the
	 * control program.
	 */
	static NET_READ_HANDLER(netCmdReadHandler);
	
	/**
	 * A handler called when a message has been read from the
	 * control program.
	 */
	static NET_SEND_HANDLER(netMsgReadHandler);

	/**
	 * A handler called when a message has been sent to the
	 * control program.
	 */
	static NET_SEND_HANDLER(netMsgSentHandler);
	
	/**
	 * Call this function if an error occurs while communicating
	 * with the ACC
	 */
	static NET_ERROR_HANDLER(netErrorHandler);
	
	/**
	 * Send an antenna ID message to the control program
	 * decide what to do
	 */
	void sendAntennaIdMsg();

	/**
	 * Read a greeting message from the control program, and
	 * decide what to do
	 */
	void parseGreetingMsg(gcp::util::NetMsg* msg);

	//------------------------------------------------------------
	// Thread management functions.
	//------------------------------------------------------------

	static THREAD_START(startGpib);
	
	//------------------------------------------------------------
	// Thread cleanup handlers methods
	
	static THREAD_CLEAN(cleanGpib);
	
	//------------------------------------------------------------
	// Ping routines for the (pingable) subsystem threads

	static THREAD_PING(pingGpib);

	ANTENNACONTROL_TASK_FWD_FN(forwardGpibMsg);

      }; // End class AntennaControl
      
    } // End namespace control
  } // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 



