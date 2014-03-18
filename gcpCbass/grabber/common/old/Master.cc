#include "gcp/grabber/common/Control.h"
#include "gcp/grabber/common/Master.h"
#include "gcp/grabber/common/Scanner.h"

#include "gcp/util/common/Debug.h"

// ACC shared code includes

#include "gcp/control/code/unix/libunix_src/common/astrom.h"
#include "gcp/control/code/unix/libunix_src/common/optcam.h"
#include "gcp/control/code/unix/libunix_src/common/scanner.h"

// AC includes

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/Ports.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::grabber;

//=======================================================================
// Master class methods
//=======================================================================

Master* Master::master_ = 0;

/**.......................................................................
 * Create the resource object of the Master object.
 *
 * Input:
 *  host             char *   The IP address of the computer on which
 *                            the control program is running.
 */
Master::Master(string ctlHost, std::string imHost, 
	       bool simulate, unsigned channel) : 
  ctlHost_(ctlHost), imHost_(imHost), simulate_(simulate), channel_(channel)
{
  LogStream errStr;

  master_  = this;
  control_ = 0;
  scanner_ = 0;

  // Initialize the array of threads controlled by this task.

  threads_.push_back(new Thread(&startControl, &cleanControl, 0,
    				"Control"));

  threads_.push_back(new Thread(&startScanner, &cleanScanner, 0, 
				"Scanner"));

  threads_.push_back(new Thread(&startSignal,  &cleanSignal,  0, 
				"Signal"));

  // Start up the threads.
  
  startThreads(this);

  // Install Signals

  installSignals();

  // Install timers.

  installTimers();

  // And run this object

  run();
}

/**.......................................................................
 * Delete the resource object of the program.
 *
 * Input:
 *  sup     Master *  The object to be deleted.
 * Output:
 *  return  Master *  The deleted object (always NULL).
 */
Master::~Master() {}

/**.......................................................................
 * Method by which tasks can query the control host.
 */
string Master::ctlHost()
{
  return ctlHost_;
}

/**.......................................................................
 * Method by which tasks can query the image host.
 */
string Master::imHost()
{
  return imHost_;
}

//-----------------------------------------------------------------------
// Static initializations

/**.......................................................................
 * Control thread startup function
 */
THREAD_START(Master::startControl)
{
  Master* master = (Master*) arg;
  Thread* thread = 0;
  
  // Get a reference to the Thread object which will manage this thread.

  thread = master->getThread("Control");
    
  // Instantiate the subsystem object

  DBPRINT(true, Debug::DEBUG7, "About to instantiate Control");

  master->control_ = 
    new gcp::grabber::Control(master);

  // Set our internal thread pointer pointing to the AntennaRx thread
    
  master->control_->thread_ = thread;

  // Let other threads know we are ready
  
  thread->broadcastReady();

  // Finally, block, running our select loop
  
  DBPRINT(true, Debug::DEBUG7, "About to call run");

  master->control_->run();

  DBPRINT(true, Debug::DEBUG7, "Leaving");

  return 0;
}

/**.......................................................................
 * Control thread startup function
 */
THREAD_CLEAN(Master::cleanControl)
{
  Master* master = (Master*) arg;

  DBPRINT(true, Debug::DEBUG7, "Inside cleanControl");

  if(master->control_ != 0) {
    delete master->control_;
    master->control_ = 0;
  }

  master->sendStopMsg();

  DBPRINT(true, Debug::DEBUG7, "Leaving cleanControl");

}

/**.......................................................................
 * Scanner thread startup function
 */
THREAD_START(Master::startScanner)
{
  Master* master = (Master*) arg;
  Thread* thread = 0;
  
  // Get a reference to the Thread object which will mange this
  // thread.

  thread = master->getThread("Scanner");
    
  // Instantiate the subsystem object

  DBPRINT(true, Debug::DEBUG7, "About to instantiate Scanner");

  master->scanner_ = 
    new gcp::grabber::Scanner(master, master->simulate());

  // Set our internal thread pointer pointing to the AntennaRx thread
  
  master->scanner_->thread_ = thread;
  
  // Let other threads know we are ready
  
  thread->broadcastReady();
    
  // Finally, block, running our select loop
    
  DBPRINT(true, Debug::DEBUG7, "About to call run");

  master->scanner_->run();
  
  DBPRINT(true, Debug::DEBUG7, "Leaving");

  return 0;
}

/**.......................................................................
 * Scanner thread startup function
 */
THREAD_CLEAN(Master::cleanScanner)
{
  Master* master = (Master*) arg;

  DBPRINT(true, Debug::DEBUG7, "Inside cleanScanner");

  if(master->scanner_ != 0) {
    delete master->scanner_;
    master->scanner_ = 0;
  }

  master->sendStopMsg();

  DBPRINT(true, Debug::DEBUG7, "Leaving cleanScanner");
}

/**.......................................................................
 * Signal thread startup function
 */
THREAD_START(Master::startSignal)
{
  Master* master = (Master*) arg;
  Thread* thread = 0;

  // Get a reference to the Thread object which will mange this
  // thread.
  
  thread = master->getThread("Signal");
  
  // Instantiate the subsystem object
  
  DBPRINT(true, Debug::DEBUG7, "About to instantiate Signal");

  master->signal_ = 
    new gcp::util::SignalTask();
  
  // Let other threads know we are ready
  
  thread->broadcastReady();
  
  // Finally, block.
  
  DBPRINT(true, Debug::DEBUG7, "About to call run");

  master->signal_->run();

  DBPRINT(true, Debug::DEBUG7, "Leaving");

  return 0;
}

/**.......................................................................
 * Signal thread startup function
 */
THREAD_CLEAN(Master::cleanSignal)
{
  Master* master = (Master*) arg;

  DBPRINT(true, Debug::DEBUG7, "Inside cleanSignal");

  if(master->signal_ != 0) {
    delete master->signal_;
    master->signal_ = 0;
  }

  master->sendStopMsg();

  DBPRINT(true, Debug::DEBUG7, "Leaving cleanSignal");
}

/**.......................................................................
 * Install signals of interest to us.
 */
void Master::installSignals()
{
  sendInstallSignalMsg(SIGINT, &sendShutDownMsg);
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
 * Send a message to connect to the grabber control port
 */
SIGNALTASK_HANDLER_FN(Master::sendConnectControlMsg)
{
  MasterMsg msg;
  ControlMsg* controlMsg = msg.getControlMsg();

  controlMsg->packConnectMsg();

  master_->sendTaskMsg(&msg);
}

/**.......................................................................
 * Send a message to connect to the grabber scanner port
 */
SIGNALTASK_HANDLER_FN(Master::sendConnectScannerMsg)
{
  MasterMsg msg;
  ScannerMsg* scannerMsg = msg.getScannerMsg();

  scannerMsg->packConnectMsg();

  master_->sendTaskMsg(&msg);
}

/**.......................................................................
 * Send a message that the grabber is connected
 */
 void Master::sendControlConnectedMsg(bool connected)
{
  MasterMsg msg;

  msg.packControlConnectedMsg(connected);

  master_->sendTaskMsg(&msg);
}

/**.......................................................................
 * Send a message that the grabber is connected
 */
 void Master::sendScannerConnectedMsg(bool connected)
{
  MasterMsg msg;

  msg.packScannerConnectedMsg(connected);

  master_->sendTaskMsg(&msg);
}

/**.......................................................................
 * Install timers of interest to us.
 */
void Master::installTimers()
{
  // Receipt of this signal will cause the control and/or scanner task
  // to attempt to connect to the control/archiver ports.

  sendInstallTimerMsg("connect", CONNECT_SIGNAL,
		      CONNECT_SEC, 0, &sendConnectControlMsg);

  // We enable the connect timer, to tell the control task to attempt
  // to connect to the control progra,

  sendEnableTimerMsg("connect", true);
}

/**.......................................................................
 * Process a message received on our message queue.
 */
void Master::processMsg(MasterMsg* msg)
{
  switch (msg->type) {

  case MasterMsg::CONTROL_CONNECTED:
    sendAddHandlerMsg("connect", &sendConnectControlMsg, 
		      !msg->body.controlConnected.connected);
    break;
  case MasterMsg::SCANNER_CONNECTED:
    sendAddHandlerMsg("connect", &sendConnectScannerMsg, 
		      !msg->body.scannerConnected.connected);
    break;
  case MasterMsg::CONTROL_MSG:
    forwardControlMsg(msg);
    break;
  case MasterMsg::SCANNER_MSG:
    forwardScannerMsg(msg);
    break;
  default:
    {
      LogStream errStr;
      errStr.initMessage(true);
      errStr << "Unrecognized message type: " << msg->type << endl;
      throw Error(errStr);
    }
    break;
  }
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
