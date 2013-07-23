#include <iostream>

#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

#include "gcp/util/common/Debug.h"     // DBPRINT
#include "gcp/util/specific/Directives.h"
#include "gcp/util/common/Exception.h" // Error()

#include "gcp/antenna/control/specific/AntennaControl.h"
#include "gcp/antenna/control/specific/AntennaDrive.h"
#include "gcp/antenna/control/specific/AntennaMaster.h"
#include "gcp/antenna/control/specific/AntennaMonitor.h"
#include "gcp/antenna/control/specific/AntennaRx.h"
#include "gcp/antenna/control/specific/AntennaRoach.h"
#include "gcp/antenna/control/specific/DlpTempSensors.h"


using namespace std;
using namespace gcp::util;

using namespace gcp::antenna::control;

/**.......................................................................
 * Initialize the static AntennaMaster pointer to NULL.  This will be
 * used inside static functions (ie, signal handlers)
 */
AntennaMaster* AntennaMaster::master_ = 0;

/**.......................................................................
 * Constructor method.  This should start up all subsystem threads 
 *
 * Input:
 *
 *  host                    string  The name of the host we are running on.
 *  simDrive                bool    true if ACU should be simulated
 */
AntennaMaster::AntennaMaster(string host, 
			     bool simPmac,
			     bool simGpib,
			     bool simDlp,
			     bool argSimBackend,
			     bool argUsePrio, 
			     bool simLna,
			     bool simAdc,
			     bool simRoach)
{
  // Before we do anything that might throw an exception, initialize
  // data members to the point where ~AntennaMaster() can safely be
  // called.

  // Keep a static pointer to ourselves; we will need this for access
  // to the AntennaMaster resources in signal handlers

  master_       = this;

  simPmac_      = simPmac;
  simGpib_      = simGpib;
  simDlp_       = simDlp;
  simBackend_   = argSimBackend;
  simLna_       = simLna;
  simAdc_       = simAdc;
  simRoach_     = simRoach;
  usePrio_      = argUsePrio;

  antNum_       = 0;
  controlTask_  = 0;
  driveTask_    = 0;
  monitorTask_  = 0;
  rxTask_       = 0;
  roachTask_    = 0;
  share_        = 0;


  // Store copies of relevant strings.

  host_         = host;

  // Initialize the array of threads managed by this task.

  // We start the communications task first, since AntennaMonitor will
  // need its resources.  Commands received before other tasks have
  // started will simply be pushed onto the Master message queue and
  // will not be parsed until the call to serviceMsgQ() below, so even
  // though we start the control task first, we are not in danger of
  // losing messages while other tasks are starting.

  threads_.push_back(new Thread(&startAntennaControl, &cleanAntennaControl,
  				&pingAntennaControl, "AntennaControl",  0, 4));

  threads_.push_back(new Thread(&startAntennaDrive,   &cleanAntennaDrive, 
  				&pingAntennaDrive,    "AntennaDrive",   1, 3));
						
  if(usePrio()) {

    static const int MONITOR_PRIORITY = 45; // Lower than tracker,

    threads_.push_back(new Thread(&startAntennaMonitor, &cleanAntennaMonitor, 
				  &pingAntennaMonitor,  "AntennaMonitor", 2, 2, 
				  MONITOR_PRIORITY));
  } else {
    threads_.push_back(new Thread(&startAntennaMonitor, &cleanAntennaMonitor, 
				  &pingAntennaMonitor,  "AntennaMonitor", 2, 2));
  }

 
  if(!simRoach_) {
    threads_.push_back(new Thread(&startAntennaRoach,   &cleanAntennaRoach, 
    				  &pingAntennaRoach,    "AntennaRoach",   3, 1));
  }


  threads_.push_back(new Thread(&startAntennaSignal,  &cleanAntennaSignal,
  				  0,                    "AntennaSignal",  4, 0));
  
  // Initialize the antenna enumerator
  
  antNum_ = new AntNum(AntNum::ANT0);

  // Initialize the resource object which will be shared between
  // threads spawned by this task

  share_  = new SpecificShare(host);

  if(share_ == 0)
    ThrowError("AntennaMaster::AntennaMaster: share is NULL.\n");


  // Install signals of interest to us.

  installSignals();
  
  // Install timers of interest to us.  Do this before we start up our
  // threads, that way we ensure all timers are defined before a
  // thread may send a message to disable or enable one.

  installTimers();
  
  // Finally, start up the threads.

  startThreads(this);

  // Start up our message queue
  COUT("here1b");

  run();

  COUT("here1c");

}

/**.......................................................................
 * AntennaMaster destructor method.  This should cause all subsystem
 * threads to shut down.
 */
AntennaMaster::~AntennaMaster() 
{
  DBPRINT(true, Debug::DEBUG7, "About to delete share");

  // Delete the dynamically allocated shared resource object

  if(share_ != 0) {
    delete share_;
    share_ = 0;
  }

  DBPRINT(true, Debug::DEBUG7, "About to delete antNum_");

  // Delete the dynamically allocated antenna enumerator

  if(antNum_ != 0) {
    delete antNum_;
    antNum_ = 0;
  }
}

//-----------------------------------------------------------------------
// AntennaDrive subsystem

/**.......................................................................
 * AntennaDrive subsystem accessor
 */
AntennaDrive* AntennaMaster::AntennaDrive() 
{
  return driveTask_;
};

/**.......................................................................
 * AntennaDrive thread startup function
 */
THREAD_START(AntennaMaster::startAntennaDrive)
{
  bool waserr=false;
  AntennaMaster* ant = (AntennaMaster*) arg;
  Thread* thread = 0;

  DBPRINT(true, Debug::DEBUG11, "starting drive thread");

  try {
    // Get a reference to the Thread object which will manage this
    // thread.
    
    thread = ant->getThread("AntennaDrive");
    
    // Instantiate the subsystem object
    
    ant->driveTask_ = new gcp::antenna::control::AntennaDrive(ant);
    
    // Set our internal thread pointer pointing to the AntennaDrive thread
    
    ant->driveTask_->thread_ = thread;
    
    // Let other threads know we are ready
    
    thread->broadcastReady();
    
    // Finally, block, running our message service:
    
    ant->driveTask_->run();
  } catch(Exception& err) {
    DBPRINT(true, Debug::DEBUGANY, "====== Exception: ====== " << err.what());
    err.report();
    throw err;
  }

  return 0;  
};

/**.......................................................................
 * A cleanup handler for the AntennaDrive thread.
 */
THREAD_CLEAN(AntennaMaster::cleanAntennaDrive) 
{
  AntennaMaster* master = (AntennaMaster*) arg;

  if(master->driveTask_ != 0) {
    delete master->driveTask_;
    master->driveTask_ = 0;
  }
  
  master->sendStopMsg();
}

/**.......................................................................
 * A function by which we will ping the AntennaDrive thread
 */
THREAD_PING(AntennaMaster::pingAntennaDrive)
{
  bool waserr=0;
  AntennaMaster* ant = (AntennaMaster*) arg;

  if(ant == 0)
    throw Error("AntennaMaster::pingAntennaDrive: NULL argument.\n");

  ant->driveTask_->sendHeartBeatMsg();
}

//-----------------------------------------------------------------------
// AntennaMonitor subsystem

/**.......................................................................
 * Accessor method for AntennaMonitor subsystem
 */
AntennaMonitor* AntennaMaster::AntennaMonitor() 
{
  return monitorTask_;
};

/**.......................................................................
 * AntennaMonitor thread startup function
 */
THREAD_START(AntennaMaster::startAntennaMonitor)
{
  bool waserr=false;
  AntennaMaster* ant = (AntennaMaster*) arg;
  Thread* thread = 0;

  try {
    DBPRINT(true, Debug::DEBUG11, "starting monitor thread");
    
    // Get a reference to the Thread object which will manage this
    // thread.
    
    thread = ant->getThread("AntennaMonitor");
    
    // Instantiate the subsystem object
    
    DBPRINT(true, Debug::DEBUG3, "startMonitor:: about to call new");
    
    ant->monitorTask_ = new 
      gcp::antenna::control::AntennaMonitor(ant);
    
    // Set the internal thread pointer pointing to the AntennaMonitor thread
    
    DBPRINT(true, Debug::DEBUG3, "startMonitor::  about to set thread");
    
    ant->monitorTask_->thread_ = thread;
    
    // Let other threads know we are ready
    
    DBPRINT(true, Debug::DEBUG3, "startMonitor:: Just about to broadcast ready");
    
    thread->broadcastReady();
    
    // Finally, block, running our message service:
    
    ant->monitorTask_->run();
  } catch(Exception& err) {
    DBPRINT(true, Debug::DEBUGANY, "====== Exception: ====== " << err.what());
  }

  return 0;    
};

/**.......................................................................
 * A cleanup handler for the AntennaMonitor thread.
 */
THREAD_CLEAN(AntennaMaster::cleanAntennaMonitor) 
{
  AntennaMaster* master = (AntennaMaster*) arg;

  if(master->monitorTask_ != 0) {
    delete master->monitorTask_;
    master->monitorTask_ = 0;
  }
  
  master->sendStopMsg();
}

/**.......................................................................
 * A function by which we will ping the AntennaMonitor thread
 */
THREAD_PING(AntennaMaster::pingAntennaMonitor)
{
  bool waserr=0;
  AntennaMaster* ant = 0;

  ant = (AntennaMaster*) arg;

  if(ant == 0)
    throw Error("AntennaMaster::pingAntennaMonitor: NULL argument.\n");

  ant->monitorTask_->sendHeartBeatMsg();
}

//-----------------------------------------------------------------------
// AntennaSignal subsystem

/**.......................................................................
 * AntennaSignal thread startup function
 */
THREAD_START(AntennaMaster::startAntennaSignal)
{
  bool waserr=false;
  AntennaMaster* ant = (AntennaMaster*) arg;
  Thread* thread = 0;

  try {
    DBPRINT(true, Debug::DEBUG11, "startSignal:: Inside");
    
    // Get a reference to the Thread object managing this thread.
    
    thread = ant->getThread("AntennaSignal");
    
    // Instantiate the subsystem object
    
    ant->signal_ = new 
      gcp::util::SignalTask();
    
    // Let other threads know we are ready
    
    thread->broadcastReady();
    
    // Finally, block in SignalTask::run(), which services all handled
    // signals.
    
    DBPRINT(true, Debug::DEBUG3, "About to call signalTask: run()");
    
    ant->signal_->run();
  } catch(Exception& err) {
    DBPRINT(true, Debug::DEBUGANY, "====== Exception: ====== " << err.what());
    throw err;
  }

  return 0;  
}

/**.......................................................................
 * A cleanup handler for the AntennaSignal thread.
 */
THREAD_CLEAN(AntennaMaster::cleanAntennaSignal) 
{
  AntennaMaster* master = (AntennaMaster*) arg;

  DBPRINT(true, Debug::DEBUG7, "Inside cleanSignal");

  if(master->signal_ != 0) {
    delete master->signal_;
    master->signal_ = 0;
  }
  
  master->sendStopMsg();

  DBPRINT(true, Debug::DEBUG7, "Leaving cleanSignal");
}

//-----------------------------------------------------------------------
// AntennaControl subsystem
//-----------------------------------------------------------------------

/**.......................................................................
 * Accessor method for the AntennaControl subsystem
 */
AntennaControl* AntennaMaster::AntennaControl() 
{
  return controlTask_;
};

/**.......................................................................
 * AntennaControl thread startup function
 */
THREAD_START(AntennaMaster::startAntennaControl)
{
  bool waserr=false;
  AntennaMaster* master = (AntennaMaster*) arg;
  Thread* thread = 0;

  try {
    DBPRINT(true, Debug::DEBUG11, "startAntennaControl:: Inside");
    
    // Get a reference to the Thread object which will manage this
    // thread.
    
    thread = master->getThread("AntennaControl");
    
    // Initialize communications with the outside world
    
    master->controlTask_  = new gcp::antenna::control::AntennaControl(master, master->simGpib(), master->simDlp(), master->simLna(), master->simAdc());
    
    // Set our internal thread pointer pointing to the AntennaControl thread
    
    master->controlTask_->thread_ = thread;
    
    // Let other threads know we are ready
    
    thread->broadcastReady();
    
    // Block in the communications run method.
    
    master->controlTask_->run();
  } catch(Exception& err) {
    DBPRINT(true, Debug::DEBUGANY, "====== Exception: ====== " << err.what());
    throw err;
  }

  return 0;  
}

/**.......................................................................
 * AntennaControl thread startup function
 */
THREAD_CLEAN(AntennaMaster::cleanAntennaControl) 
{
  AntennaMaster* master = (AntennaMaster*) arg;

  DBPRINT(true, Debug::DEBUG7, "About to delete controlTask");

  if(master->controlTask_ != 0) {
    delete master->controlTask_;
    master->controlTask_ = 0;
  }
  
  master->sendStopMsg();
}

/**.......................................................................
 * A function by which we will ping the AntennaControl thread
 */
THREAD_PING(AntennaMaster::pingAntennaControl)
{
  bool waserr=0;
  AntennaMaster* master = (AntennaMaster*) arg;

  if(master == 0)
    ThrowError("NULL argument.");

  master->controlTask_->sendHeartBeatMsg();
}

//-----------------------------------------------------------------------
// AntennaRx subsystem
//-----------------------------------------------------------------------

/**.......................................................................
 * Accessor method for the AntennaRx subsystem
 */
AntennaRx* AntennaMaster::AntennaRx() 
{
  return rxTask_;
};

/**.......................................................................
 * AntennaRx thread startup function
 */
THREAD_START(AntennaMaster::startAntennaRx)
{
  bool waserr=false;
  AntennaMaster* master = (AntennaMaster*) arg;
  Thread* thread = 0;

  try {
    DBPRINT(true, Debug::DEBUG11, "startAntennaRx:: Inside");

    // Get a reference to the Thread object which will manage this
    // thread.
    
    thread = master->getThread("AntennaRx");

    // Initialize communications with the outside world
    master->rxTask_  = new gcp::antenna::control::AntennaRx(master);
    
    // Set our internal thread pointer pointing to the AntennaRx thread
    master->rxTask_->thread_ = thread;
    
    // Let other threads know we are ready
    thread->broadcastReady();
    
    // Block in the communications run method.
    master->rxTask_->run();
  } catch(Exception& err) {
    DBPRINT(true, Debug::DEBUGANY, "====== Exception: ====== " << err.what());
    throw err;
  }

  return 0;  
}

/**.......................................................................
 * AntennaRx thread startup function
 */
THREAD_CLEAN(AntennaMaster::cleanAntennaRx) 
{
  AntennaMaster* master = (AntennaMaster*) arg;

  DBPRINT(true, Debug::DEBUG7, "About to delete rxTask");

  if(master->rxTask_ != 0) {
    delete master->rxTask_;
    master->rxTask_ = 0;
  }
  
  master->sendStopMsg();
}

/**.......................................................................
 * A function by which we will ping the AntennaRx thread
 */
THREAD_PING(AntennaMaster::pingAntennaRx)
{
  bool waserr=0;
  AntennaMaster* master = (AntennaMaster*) arg;

  if(master == 0)
    ThrowError("NULL argument.");

  master->rxTask_->sendHeartBeatMsg();
}

//-----------------------------------------------------------------------
// AntennaRoach subsystem
//-----------------------------------------------------------------------

/**.......................................................................
 * Accessor method for the AntennaRoach subsystem
 */
AntennaRoach* AntennaMaster::AntennaRoach() 
{
  return roachTask_;
};

/**.......................................................................
 * AntennaRoach thread startup function
 */
THREAD_START(AntennaMaster::startAntennaRoach)
{
  bool waserr=false;
  AntennaMaster* master = (AntennaMaster*) arg;
  Thread* thread = 0;

  try {
    DBPRINT(true, Debug::DEBUG11, "startAntennaRoach:: Inside");

    // Get a reference to the Thread object which will manage this
    // thread.
    
    thread = master->getThread("AntennaRoach");

    // Initialize communications with the outside world
    master->roachTask_  = new gcp::antenna::control::AntennaRoach(master, master->simBackend());
    
    // Set our internal thread pointer pointing to the AntennaRoach thread
    master->roachTask_->thread_ = thread;
    
    // Let other threads know we are ready
    thread->broadcastReady();
    
    // Block in the communications run method.
    master->roachTask_->run();
  } catch(Exception& err) {
    COUT("ROACH TASK RUNNING FAIL!!!");
    DBPRINT(true, Debug::DEBUGANY, "====== Exception: ====== " << err.what());
    throw err;
  }
  COUT("something should have happened!!!");
  return 0;  
}

/**.......................................................................
 * AntennaRoach thread startup function
 */
THREAD_CLEAN(AntennaMaster::cleanAntennaRoach) 
{
  AntennaMaster* master = (AntennaMaster*) arg;

  DBPRINT(true, Debug::DEBUG7, "About to delete rxTask");

  if(master->rxTask_ != 0) {
    delete master->rxTask_;
    master->rxTask_ = 0;
  }
  
  master->sendStopMsg();
}

/**.......................................................................
 * A function by which we will ping the AntennaRoach thread
 */
THREAD_PING(AntennaMaster::pingAntennaRoach)
{
  bool waserr=0;
  AntennaMaster* master = (AntennaMaster*) arg;

  if(master == 0)
    ThrowError("NULL argument.");

  master->rxTask_->sendHeartBeatMsg();
}

/**.......................................................................
 * Restart subsystem threads
 */
void AntennaMaster::restartServices()
{
  // Forcibly stop all threads

  cancelThreads();

  // Restart the threads.

  startThreads(this);

  // Install signals.

  DBPRINT(false, Debug::DEBUG31, "Just about to re-install signals");

  installSignals();

  // Restart timers.

  DBPRINT(false, Debug::DEBUG31, "Just about to re-install timers");

  installTimers();

  // And service our message queue

  serviceMsgQ();
};

/**.......................................................................
 * Send a heartbeat request to all threads.  
 */
void AntennaMaster::sendHeartBeat()
{
  if(threadsAreRunning()) {
    pingThreads(this);
  } else {
    cout << "Sending restart" << endl;
    sendRestartMsg(0, NULL);
  } 
};

//-----------------------------------------------------------------------
// Communications methods
//-----------------------------------------------------------------------

/**.......................................................................
 * Process a message specific to the AntennaMaster.
 */
void AntennaMaster::processMsg(AntennaMasterMsg* msg)
{
  switch (msg->type) {
  case AntennaMasterMsg::CONTROL_MSG:    // Message for the
					 // AntennaControl thread
    forwardControlMsg(msg);
    break;
  case AntennaMasterMsg::DRIVE_MSG:      // Message for the
					 // AntennaDrive thread
    forwardDriveMsg(msg);
    break;

  case AntennaMasterMsg::RX_MSG:         // Message for the
					 // AntennaRx thread
    forwardRxMsg(msg);
    break;

  case AntennaMasterMsg::RX_CONNECTED:   // Add or remove handler to
					 // the connect timer in
					 // response to an update of
					 // the host connection status
    DBPRINT(true, Debug::DEBUG3, "Got a receiver connected message: "
	    << msg->body.rxConnected.connected);

    sendAddHandlerMsg("connect", &sendConnectRxMsg, 
		      !msg->body.rxConnected.connected);
    break;

  case AntennaMasterMsg::ROACH_MSG:         // Message for the
					 // AntennaRoach thread
    forwardRoachMsg(msg);
    COUT("Roach Message forwarded");
    break;

  case AntennaMasterMsg::ROACH_CONNECTED:   // Add or remove handler to
					 // the connect timer in
					 // response to an update of
					 // the host connection status
    DBPRINT(true, Debug::DEBUG3, "Got a receiver connected message: "
	    << msg->body.roachConnected.connected);

    COUT("Roach Connected Message");
    sendAddHandlerMsg("connect", &sendConnectRoachMsg, 
		      !msg->body.roachConnected.connected);
    break;

  case AntennaMasterMsg::ROACH_TIMER:   // Turn Receiver timers on/off
    DBPRINT(true, Debug::DEBUG3, "Got a receiver timer message: "
	    << msg->body.roachTimer.on);

    COUT("roach timers turning on");
    
    sendAddHandlerMsg("roach", &sendReadoutRoachMsg,  
		      msg->body.roachConnected.connected);
    sendAddHandlerMsg("roachWrit", &sendWriteRoachMsg, 
		      msg->body.roachConnected.connected);
    break;

  case AntennaMasterMsg::RX_TIMER:   // Turn Receiver timers on/off
    DBPRINT(true, Debug::DEBUG3, "Got a receiver timer message: "
	    << msg->body.rxTimer.on);
    
    sendAddHandlerMsg("rx", &sendReadoutRxMsg,  
		      msg->body.rxConnected.connected);
    sendAddHandlerMsg("rxWrite", &sendWriteRxMsg, 
		      msg->body.rxConnected.connected);
    break;

  case AntennaMasterMsg::CONTROL_CONNECTED: // Add or remove a handler to
					 // the connect timer in      
					 // response to an update of  
					 // the host connection status
    DBPRINT(true, Debug::DEBUG3, "Got a control connected message: "
	    << msg->body.controlConnected.connected);

    sendAddHandlerMsg("connect", &sendConnectControlMsg, 
		      !msg->body.controlConnected.connected);
    break;
  case AntennaMasterMsg::SCANNER_CONNECTED: // Add or remove a handler to
					 // the connect timer in      
					 // response to an update of  
					 // the host connection status
    DBPRINT(true, Debug::DEBUG3, "Got a scanner connected message: "
	    << msg->body.scannerConnected.connected);

    // If the scanner successfully connected, remove the handler from
    // the connect timer.

    if(msg->body.scannerConnected.connected)
      sendAddHandlerMsg("connect", &sendConnectScannerMsg, false);
    // Else reenable handlers for both the scanner and the control
    // process.  Otherwise if we've lost our connection to the control
    // program, the control thread will never know until it tries to
    // send something
    else {
      sendAddHandlerMsg("connect", &sendConnectScannerMsg, true);
      sendAddHandlerMsg("connect", &sendConnectControlMsg, true);
    }

    break;
  case AntennaMasterMsg::MONITOR_MSG:    // Message for the
					 // AntennaMonitor thread
    forwardMonitorMsg(msg);
    break;
  case AntennaMasterMsg::DRIVE_CONNECTED: // Add or remove a handler to
					 // the connect timer in      
					 // response to an update of  
					 // the pmac connection status
    DBPRINT(true, Debug::DEBUG5, "Got a pmac connected message: " 
	    << msg->body.driveConnected.connected);

    sendAddHandlerMsg("connect", &sendConnectDriveMsg, 
		      !msg->body.driveConnected.connected);
    break;
  case AntennaMasterMsg::SEND_HEARTBEAT: // Message to send a
					 // heartbeat request to all
					 // pingable threads
    sendHeartBeat();
    break;
  default:
    {
      cout << "AntennaMaster::processMsg: Unrecognized message type: "
	 << msg->type << "." << endl;
      ostringstream os;
      os << "AntennaMaster::processMsg: Unrecognized message type: "
	 << msg->type << "." << endl << ends;
      throw Error(os.str());
    }
    break;
  }

}

//-----------------------------------------------------------------------
// Signal handlers

/**.......................................................................
 * A no-op signal handler for signals we wish to explicitly disable.
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::doNothing) {}

/**.......................................................................
 * Send a message to initiate a shutdown
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendShutDownMsg)
{
  DBPRINT(true, Debug::DEBUGANY, "Sending shutdown message");
  
  // Tell drive to stop talking to ACU
  
  master_->sendDriveShutDownMessage();
  
  // Wait 100 milliseconds to be sure drive interrupts are disabled
  
  gcp::util::TimeVal shutdownWait(0, 100000, 0);
  nanosleep(shutdownWait.timeSpec(), NULL);
  
  master_->sendStopMsg();
}

/**.......................................................................
 * Send a message to restart
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendRestartMsg)
{
  master_->GenericTask<AntennaMasterMsg>::sendRestartMsg();
}

/**.......................................................................
 * Send a message to initiate a heartbeat.
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendSendHeartBeatMsg)
{
  AntennaMasterMsg msg;

  msg.packSendHeartBeatMsg();

  master_->sendTaskMsg(&msg);
}

/**.......................................................................
 * Send a message that the 1pps tick has arrived
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendDriveTickMsg)
{
  TimeVal currTime;
  currTime.setToCurrentTime();

  //  COUT("in sendDriveTickMsg.  currTime: " << currTime.getUtcString()); 

  AntennaMasterMsg masterMsg;
  AntennaDriveMsg* driveMsg = masterMsg.getDriveMsg();
  TrackerMsg* trackerMsg = driveMsg->getTrackerMsg();

  trackerMsg->packTickMsg(currTime);

  master_->forwardDriveMsg(&masterMsg);
};
/**.......................................................................
 * Send a message to read out the DLp temp sensors
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendDlpTempRequestMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaControlMsg* controlMsg = masterMsg.getControlMsg();

  controlMsg->packDlpMsg();

  master_->forwardControlMsg(&masterMsg);
};


/**.......................................................................
 * Send a message to read out the Labjack ADC
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendReadAdcMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaControlMsg* controlMsg = masterMsg.getControlMsg();

  controlMsg->packAdcMsg();

  master_->forwardControlMsg(&masterMsg);
};

/**.......................................................................
 * Send a message that the 1pps read signal has arrived
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendDriveReadMsg)
{
  TimeVal currTime;
  currTime.setToCurrentTime();

  AntennaMasterMsg masterMsg;
  AntennaDriveMsg* driveMsg = masterMsg.getDriveMsg();
  TrackerMsg* trackerMsg = driveMsg->getTrackerMsg();

  trackerMsg->packStrobeMsg(currTime);

  master_->forwardDriveMsg(&masterMsg);
};

/**.......................................................................
 * Send a message to the monitor task to send a data frame back to the ACC.
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendPackDataFrameMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaMonitorMsg* msg = masterMsg.getMonitorMsg();

  msg->packPackDataFrameMsg();

  master_->forwardMonitorMsg(&masterMsg);
};

/**.......................................................................
 * Send a message to the tracker task to connect to the pmac
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendConnectDriveMsg)
{
  AntennaMasterMsg masterMsg;
  TrackerMsg* msg = masterMsg.getDriveMsg()->getTrackerMsg();

  DBPRINT(true, Debug::DEBUG8, "Inside sendConnectDriveMsg");

  msg->packConnectDriveMsg();

  master_->forwardDriveMsg(&masterMsg);
};

/**.......................................................................
 * Send a message to the control task to connect to the ACC.
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendConnectControlMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaControlMsg* msg = masterMsg.getControlMsg();

  DBPRINT(true, Debug::DEBUG3, "Inside sendConnectHostMsg");

  msg->packConnectMsg();

  master_->forwardControlMsg(&masterMsg);
};

/**.......................................................................
 * Send a message to the receiver task to connect to the ACC.
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendConnectRxMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaRxMsg* msg = masterMsg.getRxMsg();

  msg->packConnectMsg();

  DBPRINT(true, Debug::DEBUG3, "Inside sendConnectRxMsg");

  master_->forwardRxMsg(&masterMsg);

};

/**.......................................................................
 * Send a message to the receiver task to connect to the ACC.
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendConnectRoachMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaRoachMsg* msg = masterMsg.getRoachMsg();

  msg->packConnectMsg();

  DBPRINT(true, Debug::DEBUG3, "Inside sendConnectRoachMsg");

  master_->forwardRoachMsg(&masterMsg);

    COUT("Roach sendConnectRoachMsg");

};

/**.......................................................................
 * Send a message to the control task to connect to the ACC.
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendConnectScannerMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaMonitorMsg* msg = masterMsg.getMonitorMsg();

  DBPRINT(true, Debug::DEBUG3, "Inside sendConnectScannerMsg");

  msg->packConnectMsg();

  master_->forwardMonitorMsg(&masterMsg);
};

/**.......................................................................
 * Send a message to the gpib task to read data back from the GPIB
 * network
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendReadoutGpibMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaGpibMsg* msg = masterMsg.getControlMsg()->getGpibMsg();

  msg->packReadoutMsg();

  master_->forwardControlMsg(&masterMsg);
};

/**.......................................................................
 * Send a message to the receiver task to disconnect to the ACC.
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendDisconnectRxMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaRxMsg* msg = masterMsg.getRxMsg();

  msg->packDisconnectMsg();

  DBPRINT(true, Debug::DEBUG3, "Inside sendDisconnectRxMsg");

  master_->forwardRxMsg(&masterMsg);
};
/**.......................................................................
 * Send a message to the receiver task to disconnect to the ACC.
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendDisconnectRoachMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaRoachMsg* msg = masterMsg.getRoachMsg();

  msg->packDisconnectMsg();

  DBPRINT(true, Debug::DEBUG3, "Inside sendDisconnectRoachMsg");
  COUT("Inside sendDisconnectRoachMsg");

  master_->forwardRoachMsg(&masterMsg);
};

/**.......................................................................
 * Send a message to the rx task to read data back from the receiver
 * power supply
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendReadLnaMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaControlMsg* controlMsg = masterMsg.getControlMsg();
  AntennaLnaMsg* msg = controlMsg->getLnaMsg();

  msg->packReadDataMsg();

  master_->forwardControlMsg(&masterMsg);
};

/**.......................................................................
 * Send a message to the rx task to read data back from the receiver
 * backend
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendReadoutRoachMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaRoachMsg* msg = masterMsg.getRoachMsg();

  msg->packReadDataMsg();

  master_->forwardRoachMsg(&masterMsg);
};

/**.......................................................................
 * Send a message to the rx task to read data back from the receiver
 * backend
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendReadoutRxMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaRxMsg* msg = masterMsg.getRxMsg();

  msg->packReadDataMsg();

  master_->forwardRxMsg(&masterMsg);
};


/**.......................................................................
 * Send a message to the rx task to write the data it has stored into
 * the share object
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendWriteRxMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaRxMsg* msg = masterMsg.getRxMsg();
  gcp::util::TimeVal currTime;
  currTime.setToCurrentTime();
  msg->packWriteDataMsg(currTime);

  master_->forwardRxMsg(&masterMsg);
};

/**.......................................................................
 * Send a message to the roach task to write the data it has stored into
 * the share object
 */
SIGNALTASK_HANDLER_FN(AntennaMaster::sendWriteRoachMsg)
{
  AntennaMasterMsg masterMsg;
  AntennaRoachMsg* msg = masterMsg.getRoachMsg();
  gcp::util::TimeVal currTime;
  currTime.setToCurrentTime();
  msg->packWriteDataMsg(currTime);

  master_->forwardRoachMsg(&masterMsg);
};

//-----------------------------------------------------------------------
// Miscellaneous methods
//-----------------------------------------------------------------------

/**.......................................................................
 * Public method to get access to our shared resource object
 */
SpecificShare* AntennaMaster::getShare()
{
  return share_;
}

/**.......................................................................
 * Public method to get access to our antenna enumerator.
 */
AntNum* AntennaMaster::getAnt()
{
  return antNum_;
}

/**.......................................................................
 * Install signals of interest to us.
 */
void AntennaMaster::installSignals()
{
  sendInstallSignalMsg(SIGINT, &sendShutDownMsg);
}

/**.......................................................................
 * Install timers of interest to us.
 */
void AntennaMaster::installTimers()
{
  //------------------------------------------------------------
  // Install and enable the heartbeat timer.
  //------------------------------------------------------------
  sendInstallTimerMsg("heartbeat", ANTENNA_HEARTBEAT_SIGNAL,
		      ANTENNA_HEARTBEAT_SEC, 0, 
		      &sendSendHeartBeatMsg);

  sendEnableTimerMsg("heartbeat", false);

  //------------------------------------------------------------
  // Install and enable the connect signal.
  //
  // sendConnectControlMsg sends a message to the control thread to
  // connect to the mediator process
  //------------------------------------------------------------

  sendInstallTimerMsg("connect", CONNECT_SIGNAL, 
		      0, CONNECT_NSEC, 
		      CONNECT_SEC, 0,
		      &sendConnectControlMsg);

  // Add another handler to tell the scanner process to connect to the
  // mediator process

  sendAddHandlerMsg("connect", &sendConnectScannerMsg, true);
  sendAddHandlerMsg("connect", &sendConnectRxMsg, true);
  sendAddHandlerMsg("connect", &sendConnectRoachMsg, true);



  // Finally, enable the timer

  sendEnableTimerMsg("connect", true);
  
  //------------------------------------------------------------
  // Install a timer to generate the 1-second tick which will drive
  // the tracking loop

  sendInstallTimerMsg("tick", ANTENNA_SERVO_CMD_SIGNAL,
		      0, ANTENNA_SERVO_CMD_NSEC,
		      ANTENNA_SERVO_CMD_SEC, 0,
		      &sendDriveTickMsg);
  sendAddHandlerMsg("tick", &sendDlpTempRequestMsg, true);
  sendAddHandlerMsg("tick", &sendReadAdcMsg, true);
  sendEnableTimerMsg("tick", true);

  //------------------------------------------------------------
  // Install a timer to generate the 1-second tick which will cause us
  // to read back positions from the servo

  sendInstallTimerMsg("read", ANTENNA_SERVO_READ_SIGNAL,
		      0, ANTENNA_SERVO_READ_NSEC,
		      ANTENNA_SERVO_READ_SEC, 0,
		      &sendDriveReadMsg);
                     
  sendEnableTimerMsg("read", true);

  //------------------------------------------------------------
  // Install a timer to generate the 1-second tick which will cause us
  // to pack a data frame

  sendInstallTimerMsg("monitor", ANTENNA_MONITOR_SIGNAL,
		      0, ANTENNA_MONITOR_NSEC,
		      ANTENNA_MONITOR_SEC, 0,
		      &sendPackDataFrameMsg);
                     
  sendEnableTimerMsg("monitor", true);

  //------------------------------------------------------------
  // Install a timer to generate the 200 ms tick which will cause us
  // to read out the GPIB network

  sendInstallTimerMsg("gpib", ANTENNA_GPIB_SIGNAL,
		      0, ANTENNA_GPIB_DELAY_NSEC,
		      0, ANTENNA_GPIB_NSEC,
		      //		      ANTENNA_GPIB_SEC, 0
		      &sendReadoutGpibMsg);
                     
  sendEnableTimerMsg("gpib", true);

  //------------------------------------------------------------
  // Install a timer to generate the 50 ms tick which will cause us
  // to read out the receiver backend

  sendInstallTimerMsg("roach", ANTENNA_ROACH_SIGNAL,
		      0, ANTENNA_ROACH_DELAY_NSEC,
		      0, ANTENNA_ROACH_NSEC,
		      &sendReadoutRoachMsg);
                     
  sendEnableTimerMsg("roach", true);

  //------------------------------------------------------------
  //Install a timer so that every second at 300ms the data from the
  //backend gets writen out to disk.

  sendInstallTimerMsg("roachWrit", ANTENNA_ROACH_READ_SIGNAL,
		      0, ANTENNA_ROACH_READ_DELAY_NSEC,
		      0, ANTENNA_ROACH_READ_NSEC,
		      &sendWriteRoachMsg);
                     
  sendEnableTimerMsg("roachWrit", true);

  //------------------------------------------------------------
  // Install a timer to generate the 50 ms tick which will cause us
  // to read out the receiver backend

  sendInstallTimerMsg("rx", ANTENNA_RX_SIGNAL,
		      0, ANTENNA_RX_DELAY_NSEC,
		      0, ANTENNA_RX_NSEC,
		      &sendReadoutRxMsg);
                     
  sendEnableTimerMsg("rx", true);

  //------------------------------------------------------------
  //Install a timer so that every second at 300ms the data from the
  //backend gets writen out to disk.

  sendInstallTimerMsg("rxWrite", ANTENNA_RX_READ_SIGNAL,
		      0, ANTENNA_RX_READ_DELAY_NSEC,
		      0, ANTENNA_RX_READ_NSEC,
		      &sendWriteRxMsg);
                     
  sendEnableTimerMsg("rxWrite", true);

  //------------------------------------------------------------
  //Install a timer so that every second at 50ms the LNA gets read out
  //and written to disk

  sendInstallTimerMsg("lnaRead", ANTENNA_LNA_READ_SIGNAL,
		      0, ANTENNA_LNA_READ_DELAY_NSEC,
		      0, ANTENNA_LNA_READ_NSEC,
		      &sendReadLnaMsg);
                     
  sendEnableTimerMsg("lnaRead", true);

  //------------------------------------------------------------
  // Install a timer to generate the 1-second tick which will drive
  // the tracking loop

#if(0)
  sendInstallTimerMsg("adcRead", ANTENNA_ADC_READ_SIGNAL,
		      0, ANTENNA_ADC_READ_DELAY_NSEC,
		      ANTENNA_ADC_READ_NSEC, 0,
		      &sendReadAdcMsg);
  sendEnableTimerMsg("adcRead", true);
#endif

}

/**.......................................................................
 * Forward a message to ourselves
 */
ANTENNAMASTER_TASK_FWD_FN(AntennaMaster::forwardMasterMsg)
{
  master_->sendTaskMsg(msg);
}

/**.......................................................................
 * Forward a message to the control task
 */
ANTENNAMASTER_TASK_FWD_FN(AntennaMaster::forwardControlMsg)
{
  if(master_->controlTask_ == 0)
    return;

  AntennaControlMsg* controlMsg = msg->getControlMsg();

  master_->controlTask_->sendTaskMsg(controlMsg);
}

/**.......................................................................
 * Forward a message to the rx task
 */
ANTENNAMASTER_TASK_FWD_FN(AntennaMaster::forwardRxMsg)
{
  if(master_->rxTask_ == 0)
    return;

  AntennaRxMsg* rxMsg = msg->getRxMsg();

  master_->rxTask_->sendTaskMsg(rxMsg);
}

/**.......................................................................
 * Forward a message to the roach task
 */
ANTENNAMASTER_TASK_FWD_FN(AntennaMaster::forwardRoachMsg)
{
  if(master_->roachTask_ == 0)
    return;

  AntennaRoachMsg* roachMsg = msg->getRoachMsg();

  master_->roachTask_->sendTaskMsg(roachMsg);
}

void AntennaMaster::sendDriveShutDownMessage()
{  
  AntennaMasterMsg masterMsg;
  AntennaDriveMsg* driveMsg = masterMsg.getDriveMsg();
  driveMsg->packShutdownDriveMsg();
  
  master_->driveTask_->sendTaskMsg(driveMsg);
}

/**.......................................................................
 * Forward a message to the drive task
 */
ANTENNAMASTER_TASK_FWD_FN(AntennaMaster::forwardDriveMsg)
{
  if(master_->driveTask_ == 0)
    return;

  AntennaDriveMsg* driveMsg = msg->getDriveMsg();

  master_->driveTask_->sendTaskMsg(driveMsg);
}

/**.......................................................................
 * Forward a message to the monitor task
 */
ANTENNAMASTER_TASK_FWD_FN(AntennaMaster::forwardMonitorMsg)
{
  if(master_->monitorTask_ == 0)
    return;

  AntennaMonitorMsg* monitorMsg = msg->getMonitorMsg();

  master_->monitorTask_->sendTaskMsg(monitorMsg);
}

bool AntennaMaster::usePrio()
{
  return usePrio_;
}

bool AntennaMaster::simPmac()
{
  return simPmac_;
}

bool AntennaMaster::simGpib()
{
  return simGpib_;
}

bool AntennaMaster::simLna()
{
  return simLna_;
}

bool AntennaMaster::simAdc()
{
  return simAdc_;
}

bool AntennaMaster::simRoach()
{
  return simRoach_;
}

bool AntennaMaster::simDlp()
{
  return simDlp_;
}

bool AntennaMaster::simBackend()
{
  return simBackend_;
}

