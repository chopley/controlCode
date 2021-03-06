#ifndef ANTENNAMASTER_H
#define ANTENNAMASTER_H

/**
 * @file AntennaMaster.h
 * 
 * Tagged: Thu Nov 13 16:53:28 UTC 2003
 * 
 * @author Erik Leitch
 */

// C++ includes

#include <string>

// C includes

#include <signal.h>

// Util includes

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/GenericMasterTask.h"
#include "gcp/util/common/SignalTask.h"

#include "gcp/util/specific/Directives.h"

// Antenna includes

#include "gcp/antenna/control/specific/AntennaTask.h"
#include "gcp/antenna/control/specific/AntennaMasterMsg.h"
#include "gcp/antenna/control/specific/SpecificShare.h"
#include "gcp/antenna/control/specific/SpecificTask.h"

#define ANTENNA_HEARTBEAT_SIGNAL  SIGRTMIN
#define ANTENNA_MONITOR_SIGNAL    SIGRTMIN+1
#define ANTENNA_SERVO_CMD_SIGNAL  SIGRTMIN+2
#define ANTENNA_SERVO_READ_SIGNAL SIGRTMIN+3
#define CONNECT_SIGNAL            SIGRTMIN+4
#define ANTENNA_GPIB_SIGNAL       SIGRTMIN+5
#define ANTENNA_RX_SIGNAL         SIGRTMIN+6
#define ANTENNA_RX_READ_SIGNAL    SIGRTMIN+7
#define ANTENNA_LNA_READ_SIGNAL   SIGRTMIN+8
#define ANTENNA_ROACH_SIGNAL      SIGRTMIN+9
#define ANTENNA_ROACH_READ_SIGNAL SIGRTMIN+10
#define ANTENNA_ADC_READ_SIGNAL SIGRTMIN+11

//------------------------------------------------------------
// This timer will cause parent tasks to send a heartbeat request to
// their children
//------------------------------------------------------------

#define ANTENNA_HEARTBEAT_SEC    5  
       
//-------------------------------------------------------------
// We will read out the entire shared memory object every second, on
// the absolute second boundary
//------------------------------------------------------------

#define ANTENNA_MONITOR_SEC     1 // 1Hz data pulse
#define ANTENNA_MONITOR_NSEC    0

//------------------------------------------------------------
// We will command the servo once a second, on the three-quarter-second
//------------------------------------------------------------

#define ANTENNA_SERVO_CMD_SEC   1
#define ANTENNA_SERVO_CMD_NSEC  750000000 // 0.75 seconds after the
					  // 0.absolute second
					  // 0.boundary

//------------------------------------------------------------
// We will read back positions from the servo box once a second, on
// the quarter second, so that the previous positions are in shared
// memory by the time the 0.5-second command 1pps arrives
//------------------------------------------------------------

#define ANTENNA_SERVO_READ_SEC   1
#define ANTENNA_SERVO_READ_NSEC  250000000 // 0.25 seconds after the
					   // 0.absolute second
					   // 0.boundary

//------------------------------------------------------------
// If we are disconnected from the mediator, we will attempt to
// reconnect every 2 seconds
//------------------------------------------------------------

#define CONNECT_SEC         2         // Connect timer
#define CONNECT_NSEC        750000000 // 0.25 seconds after the
				      // 0.absolute second boundary

//------------------------------------------------------------
// We will read out the GPIB network every 200 ms, offset from the
// second boundary
//------------------------------------------------------------

#define ANTENNA_GPIB_SEC       1
#define ANTENNA_GPIB_NSEC       200000000
#define ANTENNA_GPIB_DELAY_NSEC  50000000 // start timer 50 ms after
					  // the absolute
					  // second boundary

//------------------------------------------------------------ 
// We will
// probe the roach boards 10x/s, 
//------------------------------------------------------------

#define ANTENNA_ROACH_SEC                0
#define ANTENNA_ROACH_NSEC       100000000
#define ANTENNA_ROACH_DELAY_NSEC  10000000 // start timer 10 ms after the
				       // absolute second boundary
//------------------------------------------------------------ 
// We will
// write the receiver data once a second at the 0.3ms mark
//------------------------------------------------------------

#define ANTENNA_ROACH_READ_SEC                0
#define ANTENNA_ROACH_READ_NSEC      1000000000
#define ANTENNA_ROACH_READ_DELAY_NSEC 300000000 // start timer 0.3 s after the
				             // absolute second boundary

//------------------------------------------------------------ 
// We will
// probe 1 roach board 10x/s, but have them offset from each other by 50ms.
//------------------------------------------------------------

#define ANTENNA_RX_SEC                0
#define ANTENNA_RX_NSEC       100000000
#define ANTENNA_RX_DELAY_NSEC  10000000 // start timer 10 ms after the
				       // absolute second boundary
//------------------------------------------------------------ 
// We will
// write the receiver data once a second at the 0.3ms mark
//------------------------------------------------------------

#define ANTENNA_RX_READ_SEC                0
#define ANTENNA_RX_READ_NSEC      1000000000
#define ANTENNA_RX_READ_DELAY_NSEC 300000000 // start timer 0.3 s after the


//------------------------------------------------------------ 
// We will
// write the LNA data every second at the 150ms mark
//------------------------------------------------------------

#define ANTENNA_LNA_READ_SEC                1
#define ANTENNA_LNA_READ_NSEC      1000000000
#define ANTENNA_LNA_READ_DELAY_NSEC  150000000 // start timer 150 ms after the
				             // absolute second boundary

#define ANTENNA_ADC_READ_SEC                10
#define ANTENNA_ADC_READ_NSEC      5000000000
#define ANTENNA_ADC_READ_DELAY_NSEC  550000000 // start timer 150 ms after the
				             // absolute second boundary

#define ANTENNAMASTER_TASK_FWD_FN(fn) void (fn)(AntennaMasterMsg* msg)

namespace gcp {
  namespace antenna {
    namespace control {
      
      /**
       * Forward declarations so we don't have to include the header
       * files.
       */
      class AntennaDrive;
      class AntennaControl;
      class AntennaMonitor;
      class AntennaRx;
      class AntennaRoach;
      
      /**
       * Define a class to encapsulate the entire Antenna control
       * system.  Instantiating this object will spawn all subsystem
       * threads of the Antenna control system, which currently
       * include a receiver task, a drive system task and a
       * monitoring task.
       */
      class AntennaMaster : 
	public SpecificTask,
	public gcp::util::GenericMasterTask<AntennaMasterMsg> {
	
	public:
	
	/**
	 * Constructor
	 * 
	 * @throws Exception;
	 */
	AntennaMaster(std::string host, 
		      bool simPmac=false,
		      bool simGpib=false,
		      bool simDlp=false,
		      bool priv=true,
		      bool simLna=false,
		      bool simAdc=false,
		      bool simRoach1=false,
		      bool simRoach2=false);
	
	/**
	 * Destructor
	 *
	 * @throws Exception
	 */
	~AntennaMaster();
	
	/**
	 * Return a pointer to the Control task resources
	 */
	gcp::antenna::control::AntennaControl* AntennaControl();
	
	/**
	 * Return a pointer to the Drive control task resources
	 */
	gcp::antenna::control::AntennaDrive*   AntennaDrive();
	
	/**
	 * Return a pointer to the Monitor task resources
	 */
	gcp::antenna::control::AntennaMonitor* AntennaMonitor();
	
	/**
	 * Return a pointer to the Receiver task resources
	 */
	gcp::antenna::control::AntennaRx*      AntennaRx();

	/**
	 * Return a pointer to the Receiver task resources
	 */
	gcp::antenna::control::AntennaRoach*      AntennaRoach();

	/**
	 * Public interface to startThreads()
	 *
	 * @throws Exception	
	 */
	void restartServices();
	
	//------------------------------------------------------------
	// Public methods for sending messages to this task.
	//------------------------------------------------------------
	
	/**
	 * A no-op signal handler for signals we wish to explicitly
	 * disable.
	 */
	SIGNALTASK_HANDLER_FN(doNothing);
	
	/**
	 * Send a message to the AntennaMaster to tell it to send out
	 * a heartbeat request to all subordinate threads.
	 *
	 * @throws Exception (via MsgQ::sendMsg)
	 */
	static SIGNALTASK_HANDLER_FN(sendSendHeartBeatMsg);
	
	/**
	 * Send a message to the AntennaMaster to tell it to shut down
	 *
	 * @throws Exception (via MsgQ::sendMsg)
	 */
	static SIGNALTASK_HANDLER_FN(sendShutDownMsg);
	
	/**
	 * Send a message to the monitor task that it's time to pack a
	 * data frame for transmission back to the ACC.
	 */
	static SIGNALTASK_HANDLER_FN(sendPackDataFrameMsg);
	
	/**
	 * Send message to drive telling it to shut down ACU interface
	 */
	void sendDriveShutDownMessage();
	
	/**
	 * Tell the Tracker task that the 1-second servo command pulse
	 * has arrived.
	 */
	static SIGNALTASK_HANDLER_FN(sendDriveTickMsg);
	
	/**
	 * Tell the Tracker task that the 1-second servo read pulse
	 * has arrived.
	 */
	static SIGNALTASK_HANDLER_FN(sendDriveReadMsg);

	/**
	 * Read those damn Dlp temp sensors
	 */
	static SIGNALTASK_HANDLER_FN(sendDlpTempRequestMsg);

	/**
	 * Tell the Control task to attempt to connect to the host.
	 */
	static SIGNALTASK_HANDLER_FN(sendConnectControlMsg);

	/**
	 * Tell the Tracker to attempt to connect to the servo
	 */
	static SIGNALTASK_HANDLER_FN(sendConnectDriveMsg);
	
	/**
	 * Tell the Control task to attempt to connect to the host.
	 */
	static SIGNALTASK_HANDLER_FN(sendConnectScannerMsg);
	
	/*
	 * Send a message to the gpib task to read data back from the
	 * GPIB network
	 */
	static SIGNALTASK_HANDLER_FN(sendReadoutGpibMsg);

	/**
	 * Tell the Receiver task to attempt to connect to the host.
	 */
	static SIGNALTASK_HANDLER_FN(sendConnectRxMsg);

	/**
	 * Tell the Receiver task to attempt to connect to the host.
	 */
	static SIGNALTASK_HANDLER_FN(sendConnectRoachMsg);

	/**
	 * Tell the Receiver task to disconnect from the host
	 */
	static SIGNALTASK_HANDLER_FN(sendDisconnectRxMsg);

	/**
	 * Tell the Receiver task to disconnect from the host
	 */
	static SIGNALTASK_HANDLER_FN(sendDisconnectRoachMsg);

	/*
	 * Send a message to the rx task to read data back from the
	 * receiver backend
	 */
	static SIGNALTASK_HANDLER_FN(sendReadoutRoachMsg);

	/*
	 * Send a message to the rx task to write the data it has stored into
	 * the share object
	 */
	static SIGNALTASK_HANDLER_FN(sendWriteRoachMsg);

	/*
	 * Send a message to the rx task to read data back from the
	 * receiver backend
	 */
	static SIGNALTASK_HANDLER_FN(sendReadoutRxMsg);

	/*
	 * Send a message to the rx task to write the data it has stored into
	 * the share object
	 */
	static SIGNALTASK_HANDLER_FN(sendWriteRxMsg);

	/*
	 * Send a message to the lna task to read/write the data it has stored into
	 * the share object
	 */
	static SIGNALTASK_HANDLER_FN(sendReadLnaMsg);

	/*
	 * Send a message to the adc task to read/write the data it has stored into
	 * the share object
	 */
	static SIGNALTASK_HANDLER_FN(sendReadAdcMsg);

	/**
	 * Method by which other tasks can ask us for control of
	 * boards.
	 */
	void sendAdoptBoardMsg(unsigned short, AntennaTask::Id taskId);
	
	//------------------------------------------------------------
	// Communications methods
	//------------------------------------------------------------
	
	/**
	 * Public method by which other tasks can forward message to us.
	 */
	static ANTENNAMASTER_TASK_FWD_FN(forwardMasterMsg);
	
	//------------------------------------------------------------
	// Public accessor methods
	//------------------------------------------------------------
	
	/**
	 * Public method to get a reference to our shared resource
	 * object.
	 */
	SpecificShare* getShare();
	
	/**
	 * Public method to get a reference to our antenna enumerator.
	 */
	gcp::util::AntNum* getAnt();
	
	/**
	 * A copy of the host to which we will attach
	 */
	inline std::string host() {return host_;}

	/**
	 * A copy of the boolean flag specifying whether or not to use
	 * thread prioritizing
	 */
	bool usePrio();

	// Methods to determine if we are simulating devices
	
	bool simPmac();
	bool simGpib();
	bool simLna();
	bool simAdc();
	bool simDlp();
	bool simRoach1();
	bool simRoach2();


	private:

	/**
	 * True if simulating the drive controllers
	 */
	bool simPmac_;

	// True if simulating a GPIB network

	bool simGpib_;

	// True if simulating a LNA task

	bool simLna_;

	// True if simulating a ADC task

	bool simAdc_;

	// True if simulating a temperature sensor bus

	bool simDlp_;

	// True if simulating a roach

	bool simRoach1_;
	bool simRoach2_;

	/**
	 * True if using thread priorities
	 */
	bool usePrio_;

	/**
	 * A static pointer to ourselves for use in static functions
	 */
	static AntennaMaster* master_;
	
	/**
	 * An enumerator specifying which antenna we are.
	 */
	gcp::util::AntNum* antNum_;
	
	/**
	 * A copy of the mediator host to which we will attach
	 */
	std::string host_;

	//------------------------------------------------------------
	// Thread management methods
	//------------------------------------------------------------
	
	/**
	 * Startup routine for the Drive task thread
	 */
	static THREAD_START(startAntennaDrive);
	
	/**
	 * Startup routine for the Monitor task thread
	 */
	static THREAD_START(startAntennaMonitor);
	
	/**
	 * Startup routine for the control task thread
	 */
	static THREAD_START(startAntennaControl);
	
	/**
	 * Startup routine for the Receiver task thread
	 */
	static THREAD_START(startAntennaRx);

	/**
	 * Startup routine for the Receiver task thread
	 */
	static THREAD_START(startAntennaRoach);
	
	/**
	 * Startup routine for the signal task thread
	 */
	static THREAD_START(startAntennaSignal);

	
	//------------------------------------------------------------
	// Thread cleanup handlers methods
	
	/**
	 * Cleanup routine for the Drive task thread
	 */
	static THREAD_CLEAN(cleanAntennaDrive);
	
	/**
	 * Cleanup routine for the Monitor task thread
	 */
	static THREAD_CLEAN(cleanAntennaMonitor);
	
	/**
	 * Cleanup routine for the communications task thread
	 */
	static THREAD_CLEAN(cleanAntennaControl);
	
	/**
	 * Cleanup routine for the Receiver task thread
	 */
	static THREAD_CLEAN(cleanAntennaRx);

	/**
	 * Cleanup routine for the Receiver task thread
	 */
	static THREAD_CLEAN(cleanAntennaRoach);
	
	/**
	 * Cleanup routine for the signal task thread
	 */
	static THREAD_CLEAN(cleanAntennaSignal);

	//------------------------------------------------------------
	// Ping routines for the (pingable) subsystem threads
	
	/**
	 * Ping routine for the Control task thread
	 *
	 * @throws Exception
	 */
	static THREAD_PING(pingAntennaControl);

	/**
	 * Ping routine for the Drive task thread
	 *
	 * @throws Exception
	 */
	static THREAD_PING(pingAntennaDrive);
	
	/**
	 * Ping routine for the Monitor task thread
	 *
	 * @throws Exception
	 */
	static THREAD_PING(pingAntennaMonitor);
	
	/**
	 * Ping routine for the Receiver task thread
	 *
	 * @throws Exception
	 */
	static THREAD_PING(pingAntennaRx);

	/**
	 * Ping routine for the Receiver task thread
	 *
	 * @throws Exception
	 */
	static THREAD_PING(pingAntennaRoach);
	
	//------------------------------------------------------------
	// Pointers to the subsystem resources.  These pointers are
	// initialized by the subsystem threads on startup
	
	// Pointer to the Control resources

	gcp::antenna::control::AntennaControl*    controlTask_;
	
	// Pointer to the Drive system resources

	gcp::antenna::control::AntennaDrive*      driveTask_; 
	
	// Pointer to the Receiver Task resources

	gcp::antenna::control::AntennaRx*         rxTask_; 

	// Pointer to the Receiver Task resources

	gcp::antenna::control::AntennaRoach*         roachTask_; 
	
	// Pointer to the Monitor Task resources

	gcp::antenna::control::AntennaMonitor*    monitorTask_; 
	
	//------------------------------------------------------------
	// Members pertaining to communication with this task
	
	/**
	 * Process a message received on our message queue.  Processes
	 * all messages intended for tasks spawned by the
	 * AntennaMaster.
	 */
	void processMsg(AntennaMasterMsg* taskMsg);
	
	//------------------------------------------------------------
	// Message forwarding methods.  These are private, as they
	// should not be used directly by tasks to send messages to
	// each other, or they will run the risk of a message getting
	// dropped if these methods are called before the recipient's
	// queue exists.
	//
	// Messages to all tasks should be sent to the master, whose
	// message queue is guaranteed to exist before any other tasks
	// are spawned, and which will not service messages until all
	// spawned tasks have allocated their resources.  This
	// guarantees that no messages will be lost, but simply queued
	// until the master calls its serviceMsgQ() method.
	
	/**
	 * Method for forwarding a message to the Control task. 
	 */
	static ANTENNAMASTER_TASK_FWD_FN(forwardControlMsg);
	
	/**
	 * Method for forwarding a message to the Drive task. 
	 */
	static ANTENNAMASTER_TASK_FWD_FN(forwardDriveMsg);
	
	/**
	 * Method for forwarding a message to the Monitor task.
	 */
	static ANTENNAMASTER_TASK_FWD_FN(forwardMonitorMsg);
	
	/**
	 * Method for forwarding a message to the Receiver task.
	 */
	static ANTENNAMASTER_TASK_FWD_FN(forwardRxMsg);

	/**
	 * Method for forwarding a message to the Receiver task.
	 */
	static ANTENNAMASTER_TASK_FWD_FN(forwardRoachMsg);
	
	//------------------------------------------------------------
	// Private send methods for messages intended for the
	// AntennaMaster
	//------------------------------------------------------------
	
	/**
	 * Send a message to the AntennaMaster to restart threads
	 */
	static SIGNALTASK_HANDLER_FN(sendRestartMsg);
	
	//------------------------------------------------------------
	// Methods pertaining to timers managed by this task
	//------------------------------------------------------------

	/**
	 * Send a heartbeat to all threads managed by this task.
	 */
	void sendHeartBeat();
	
	/**
	 * Install timers we are interested in.
	 */
	void installTimers();
	
	/**
	 * Install signals we are interested in.
	 */
	void installSignals();
	
      }; // End class AntennaMaster
      
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp

#endif
