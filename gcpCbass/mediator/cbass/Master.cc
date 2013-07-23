#define __FILEPATH__ "mediator/specific/Master.cc"

#include <cmath>        // Needed for modf()
#include <csignal>      // Needed for SIGPIPE, etc.
#include <setjmp.h>     // Needed for jmp_buf, etc.
#include <unistd.h>     // Needed for close()
#include <string.h>

#include "gcp/mediator/specific/Master.h"
#include "gcp/util/common/Debug.h"

// ACC shared code includes

#include "gcp/control/code/unix/libunix_src/common/astrom.h"
#include "gcp/control/code/unix/libunix_src/common/optcam.h"

// AC includes

#include "gcp/util/common/Exception.h"

#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/Scanner.h"

using namespace std;
using namespace gcp::mediator;
using namespace gcp::util;

#define PRT(a) fprintf(stdout, "%s = %d\n",#a,a);

//=======================================================================
// Master class methods
//=======================================================================

/**
 * Initialize the static pointer to NULL.
 */
Master* Master::master_ = 0;

/**.......................................................................
 * Create the resource object of the Master object.
 *
 * Input:
 *  host             char *   The IP address of the computer on which
 *                            the control program is running.
 */
Master::Master(std::string ctlHost,
	       std::string wxHost,
	       bool sim, 
	       bool simRx) :
  sim_(sim), simRx_(simRx)
{
  // Initialize the static pointer to ourselves.
  
  master_ = this;
  
  // Before attempting any operation that might fail, initialize
  // container at least up to the point at which it can safely be
  // passed to delete
  
  control_    = 0;
  scanner_    = 0;
  
  // Initialize the array of threads managed by this task.
  
  threads_.push_back(new Thread(&startControl, &cleanControl, &pingControl, 
      				"Control"));
  
  threads_.push_back(new Thread(&startScanner, &cleanScanner, &pingScanner, 
   				"Scanner"));
  
  threads_.push_back(new Thread(&startSignal,  &cleanSignal,             0, 
				"Signal"));
  
  // Keep copies of relevant passed arguments
  
  ctlHost_    = ctlHost;
  wxHost_     = wxHost;

  COUT("WXHOST: " << wxHost_);
  // Start up the threads.
  
  startThreads(this);
  
  // Install Signals
  
  installSignals();
  
  // Install timers.
  
  installTimers();
  
  // And start our message queue.  This will block.

  serviceMsgQ();
}

/**.......................................................................
 * Delete the resource object of the program.
 *
 * Input:
 *  sup     Supplier *  The object to be deleted.
 * Output:
 *  return  Supplier *  The deleted object (always NULL).
 */
Master::~Master() 
{
  // GenericTask destructor will call destructors for all objects in
  // threads_, and cancel them as well.
}

//-----------------------------------------------------------------------
// Static initializations

/**.......................................................................
 * Control thread startup function
 */
THREAD_START(Master::startControl)
{
  Master* trans = (Master*) arg;
  Thread* thread = 0;

  thread = trans->getThread("Control");
    
  // Instantiate the subsystem object
    
  trans->control_ = 
    new gcp::mediator::Control(trans);
    
  // Set our internal thread pointer pointing to the AntennaRx thread
    
  trans->control_->thread_ = thread;
    
  // Let other threads know we are ready
    
  thread->broadcastReady();
    
  // Finally, block, running our select loop
    
  DBPRINT(true, Debug::DEBUG10, "About to call run");
    
  trans->control_->run();
    
  DBPRINT(true, Debug::DEBUG10, "Leaving");
  
  return 0;
}

/**.......................................................................
 * Control thread startup function
 */
THREAD_CLEAN(Master::cleanControl)
{
  Master* master = (Master*) arg;
  
  if(master->control_ != 0) {
    delete master->control_;
    master->control_ = 0;
  }
  
  master->sendStopMsg();
}

/**.......................................................................
 * Scanner thread startup function
 */
THREAD_START(Master::startScanner)
{
  Master* master = (Master*) arg;
  Thread* thread = 0;
  
  try {
    // Get a reference to the Thread object which will mange this
    // thread.
    
    thread = master->getThread("Scanner");
    
    // Instantiate the subsystem object
    
    master->scanner_ = 
      new gcp::mediator::Scanner(master);
    
    // Set our internal thread pointer pointing to the AntennaRx thread
    
    master->scanner_->thread_ = thread;
    
    // Let other threads know we are ready
    
    thread->broadcastReady();
    
    // Finally, block, running our select loop
    
    DBPRINT(true, Debug::DEBUG10, "About to call run");
    
    master->scanner_->run();
    
    DBPRINT(true, Debug::DEBUG10, "Leaving");
  } catch(Exception& err) {
    cout << err.what() << endl;
    throw err;
  }
  
  return 0;
}

/**.......................................................................
 * Scanner thread startup function
 */
THREAD_CLEAN(Master::cleanScanner)
{
  Master* master = (Master*) arg;
  
  if(master->scanner_ != 0) {
    delete master->scanner_;
    master->scanner_ = 0;
  }
  
  master->sendStopMsg();
}

/**.......................................................................
 * Signal thread startup function
 */
THREAD_START(Master::startSignal)
{
  Master* master = (Master*) arg;
  Thread* thread = 0;
  
  try {
    // Get a reference to the Thread object which will mange this
    // thread.
    
    thread = master->getThread("Signal");
    
    // Instantiate the subsystem object
    
    DBPRINT(true, Debug::DEBUG10, "About to instantiate Signal");
    
    master->signal_ = 
      new gcp::util::SignalTask();
    
    // Let other threads know we are ready
    
    thread->broadcastReady();
    
    // Finally, block.
    
    DBPRINT(true, Debug::DEBUG10, "About to call run");
    
    master->signal_->run();
    
    DBPRINT(true, Debug::DEBUG10, "Leaving");
  } catch(Exception& err) {
    std::cout << err.what() << std::endl;
    throw err;
  }
  
  return 0;
}

/**.......................................................................
 * Signal thread startup function
 */
THREAD_CLEAN(Master::cleanSignal)
{
  Master* master = (Master*) arg;
  
  if(master->signal_ != 0) {
    delete master->signal_;
    master->signal_ = 0;
  }
  
  master->sendStopMsg();
}

/**.......................................................................
 * A function by which we will ping the Control thread
 */
THREAD_PING(Master::pingControl)
{
  Master* master = (Master*) arg;
  
  if(master == 0)
    throw Error("Master::pingControl: NULL argument.\n");
  
  if(master->control_ != 0) 
    master->control_->sendHeartBeatMsg();
}

/**.......................................................................
 * A function by which we will ping the Scanner thread
 */
THREAD_PING(Master::pingScanner)
{
  Master* master = (Master*) arg;
  
  if(master == 0)
    throw Error("Master::pingScanner: NULL argument.\n");
  
  if(master->scanner_ != 0) 
    master->scanner_->sendHeartBeatMsg();
}

/**.......................................................................
 * Send a heartbeat request to all threads.  
 */
void Master::sendHeartBeat()
{
  if(threadsAreRunning()) {
    pingThreads(this);
  } else {
    sendRestartMsg();
  } 
};

/**.......................................................................
 * Process a message received on our message queue.
 */
void Master::processMsg(MasterMsg* msg)
{
  switch (msg->type) {
    
  case MasterMsg::SEND_HEARTBEAT:
    sendHeartBeat();
    break;
  case MasterMsg::CONTROL_CONNECTED:
    sendAddHandlerMsg("connect", &sendControlConnectMsg, 
		      !msg->body.controlConnected.connected);
    break;
  case MasterMsg::SCANNER_CONNECTED:
    sendAddHandlerMsg("connect", &sendScannerConnectMsg, 
		      !msg->body.scannerConnected.connected);
    break;
  case MasterMsg::SCANNER_MSG:
    DBPRINT(true, Debug::DEBUG10, "Got a scanner message");
    forwardScannerMsg(msg);
    break;
  case MasterMsg::CONTROL_MSG:
    DBPRINT(false, Debug::DEBUG6, "Got a control message");
    forwardControlMsg(msg);
    break;
  default:
    ThrowError("Unknown msg->type " << msg->type);
    break;
  }
}

/**.......................................................................
 * Static function to send a message to the master thread that it's
 * time to send a heartbeat.  
 *
 * This will be passed as a callback when the heartbeat timer signals
 * the Signal thread.
 */
SIGNALTASK_HANDLER_FN(Master::sendSendHeartBeatMsg)
{
  MasterMsg msg;
  
  msg.packSendHeartBeatMsg();
  
  master_->sendTaskMsg(&msg);
}

/**.......................................................................
 * Send a message to the Scanner task to start a new data frame.
 */
SIGNALTASK_HANDLER_FN(Master::sendStartDataFrameMsg)
{
  MasterMsg masterMsg;
  ScannerMsg* scannerMsg = masterMsg.getScannerMsg();
  static unsigned wxCounter=0;
  
  scannerMsg->packStartDataFrameMsg();
  
  master_->forwardScannerMsg(&masterMsg);
}

/**.......................................................................
 * Send a message to the Scanner task to connect to the archiver port.
 */
SIGNALTASK_HANDLER_FN(Master::sendScannerConnectMsg)
{
  MasterMsg masterMsg;
  ScannerMsg* msg = masterMsg.getScannerMsg();
  msg->packConnectMsg();
  
  master_->forwardScannerMsg(&masterMsg);
}

/**.......................................................................
 * Send a message to the Control task to connect to the control port.
 */
SIGNALTASK_HANDLER_FN(Master::sendControlConnectMsg)
{
  MasterMsg masterMsg;
  ControlMsg* msg = masterMsg.getControlMsg();
  msg->packConnectMsg();
  
  master_->forwardControlMsg(&masterMsg);
}

/**.......................................................................
 * Send a message to the master thread to restart its threads.
 */
SIGNALTASK_HANDLER_FN(Master::sendSendRestartMsg)
{
  master_->sendRestartMsg();
}

/**.......................................................................
 * Send a message to the master thread to shutdown.
 */
SIGNALTASK_HANDLER_FN(Master::sendShutDownMsg)
{
  DBPRINT(true, Debug::DEBUG1, "Inside");
  master_->sendStopMsg();
}

/**.......................................................................
 * Trap segmentation fault signal
 */
SIGNALTASK_HANDLER_FN(Master::trapSegv)
{
  ReportError("Detected a segmentation fault");
}

/**.......................................................................
 * Install signals of interest to us.
 */
void Master::installSignals()
{
  sendInstallSignalMsg(SIGINT,  &sendShutDownMsg);
  sendInstallSignalMsg(SIGSEGV, &trapSegv);
  sendInstallSignalMsg(SIGBUS,  &trapSegv);
}

/**.......................................................................
 * Install timers of interest to us.
 */
void Master::installTimers()
{
  // Receipt of this signal will cause the master task to send a
  // heartbeat request to all pingable threads.
  
  sendInstallTimerMsg("heartbeat", HEARTBEAT_SIGNAL,
		      HEARTBEAT_SEC, 0, &sendSendHeartBeatMsg);
  
  sendEnableTimerMsg("heartbeat", true);

  // Receipt of this signal will cause the scanner task to send a data
  // frame back to the archiver port.
  
  sendInstallTimerMsg("dataframe", DATAFRAME_SIGNAL,
		      0, DATAFRAME_NSEC, &sendStartDataFrameMsg);
  
  // If this is the expstub code, don't enable the dataframe tick -- the
  // data heartbeat will come from the receipt of a frame from the
  // antenna

  sendEnableTimerMsg("dataframe", true);

  // Receipt of this signal will cause the scanner task to attempt to
  // connect to the archiver port.  
  
  sendInstallTimerMsg("connect", CONNECT_SIGNAL,
		      0,0,
		      CONNECT_SEC, 0, &sendControlConnectMsg);

  // We enable the connect timer, to tell the control process to
  // connect only after we have begun listening to our message queue.
  
  sendEnableTimerMsg("connect", true);
}

/**.......................................................................
 * Reinitialize the resources of this task.
 */
void Master::restart()
{
  cout << "Inside restart" << endl;
  
  if(signal_ != 0)
    signal_->stopTimers();
  
  cancelThreads();
  
  startThreads(this);
  
  installSignals();
  installTimers();
}

/**.......................................................................
 * Forward a message to the master task
 */
MASTER_TASK_FWD_FN(Master::forwardMasterMsg)
{
  sendTaskMsg(msg);
}

/**.......................................................................
 * Forward a message to the scanner task
 */
MASTER_TASK_FWD_FN(Master::forwardScannerMsg)
{
  DBPRINT(true, Debug::DEBUG7, "scanner_ is " << ((scanner_==0) ? "NULL" : "not NULL"));
  
  if(scanner_ != 0)
    scanner_->sendTaskMsg(msg->getScannerMsg());
}

/**.......................................................................
 * Forward a message to the control task
 */
MASTER_TASK_FWD_FN(Master::forwardControlMsg)
{
  if(control_ != 0)
    control_->sendTaskMsg(msg->getControlMsg());
}

#if DIR_HAVE_OPTCAM
/**.......................................................................
 * Forward a message to the optcam task
 */
MASTER_TASK_FWD_FN(Master::forwardOptCamMsg)
{
  if(optcam_ != 0)
    optcam_->sendTaskMsg(msg->getOptCamMsg());
}
#endif

gcp::util::ArrayRegMapDataFrameManager& Master::getArrayShare()
{
  return arrayShare_;
}
