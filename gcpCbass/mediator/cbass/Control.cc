#define __FILEPATH__ "mediator/specific/Control.cc"

#include <sys/socket.h> // Needed for shutdown()

#include "gcp/control/code/unix/libunix_src/common/tcpip.h"
#include "gcp/util/specific/Directives.h"

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include "gcp/mediator/specific/Master.h"
#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/AntennaControl.h"
#include "gcp/mediator/specific/GrabberControl.h"
#include "gcp/mediator/specific/ReceiverControl.h"
#include "gcp/mediator/specific/WxControl.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::mediator;

/**
 * Initialize the static pointer to NULL.
 */
Control* Control::control_ = 0;

/**.......................................................................
 * Constructor for the Control container
 */
Control::Control(Master* parent)
{
  // Initialize the static pointer to ourselves.
  
  control_ = this;
  
  // Sanity check
  
  if(parent == 0) {
    ThrowError("Received NULL parent argument");
  }
  
  // Initialize the control encapsulation
  
  parent_          = parent;
  antennaControl_  = 0;
  grabberControl_  = 0;
  receiverControl_ = 0;
  wxControl_       = 0;
  
  // Initialize the command forwarder
  
  forwarder_      = 0;
  forwarder_ = new TransNetCmdForwarder(this);
  
  // Initialize the array of threads controlled by this task.
  COUT("LET'S TRY TO START SOME THREADS");
  
  threads_.push_back(new Thread(&startAntennaControl, 
				&cleanAntennaControl, 
				&pingAntennaControl, "AntennaControl"));

  threads_.push_back(new Thread(&startGrabberControl, 
  				&cleanGrabberControl, 
  				&pingGrabberControl, "GrabberControl"));

  threads_.push_back(new Thread(&startReceiverControl, 
  				&cleanReceiverControl, 
  				&pingReceiverControl, "ReceiverControl"));

  if(!parent_->sim()) {
    threads_.push_back(new Thread(&startWxControl, 
				  &cleanWxControl, 
				  &pingWxControl,      "WxControl"));
  }

  if(parent_->sim()) {
    connectControl(true);
  }

  // Run threads managed by this task.
  
  COUT("Abotu to start threads...");
  startThreads(this);
  COUT("Abotu to start threads... DONE");
}

/**.......................................................................
 * Destructor function for Control
 */
Control::~Control()
{
  disconnect();
  
  if(forwarder_) {
    delete forwarder_;
    forwarder_ = 0;
  }
}

/*.......................................................................
 * Connect to the controller port of the control program.
 */
bool Control::connect()				 
{
  // Terminate any existing connection.
  
  disconnect();
  
  // Attempt to open a non-blocking connection to the server
  
  if(client_.connectToServer(parent_->ctlHost(), CP_RTC_PORT, true) < 0) {
    ErrorDef(err, "Control::connect: Error in tcp_connect().\n");
    return false;
  }
  
  // Once we've successfully achieved a connection, reconfigure the
  // socket for non-blocking I/O
  
  client_.setBlocking(false);
  
  // Attach the network I/O streams to the new client socket.
  
  netCommHandler_.attach(client_.getFd());
  
  // Attach a callback to be invoked when a NetCmd has been read
  
  netCommHandler_.getNetCmdHandler()->
    installReadHandler(readNetCmdHandler, this);
  
  // And an error handler
  
  netCommHandler_.getNetCmdHandler()->
    installErrorHandler(networkErrorHandler, this);
  
  // Attach a callback to be invoked when a NetMsg has been sent
  
  netCommHandler_.getNetMsgHandler()->
    installSendHandler(sendNetMsgHandler, this);
  
  // And an error handler
  
  netCommHandler_.getNetMsgHandler()->
    installErrorHandler(networkErrorHandler, this);
  
  // And register the socket to be watched for readability
  
  fdSet_.registerReadFd(client_.getFd());
  
  // If we have a connection to the ACC, install log message handlers
  
  Logger::setPrefix("Mediator: ");
  Logger::installLogHandler(sendLogMsg);
  Logger::installErrHandler(sendErrMsg);
  
  return true;
}

/**.......................................................................
 * Disconnect the connection to the control-program control port.
 */
void Control::disconnect()
{
  // Clear the client fd from the set to be watched in select()
  
  if(client_.isConnected())
    fdSet_.clear(client_.getFd());
  
  // Disconnect from the server socket.
  
  client_.disconnect();
  
  // Detach network handlers
  
  netCommHandler_.attach(-1);
  
  // Detach logging handlers
  
  Logger::installLogHandler(0);
  Logger::installErrHandler(0);
}

/**.......................................................................
 * Service our message queue.
 */
void Control::serviceMsgQ()
{
  bool stop   = false;
  int  nready = 0;
  int  msgqFd = msgq_.fd();
  
  if(msgqFd < 0)
    throw Error("GenericTask::serviceMsgQ: "
		"Received NULL file descriptor");
  
  // Register the msgq to be watched for readability  
  
  fdSet_.registerReadFd(msgqFd);
  
  // Initially our select loop will only check the msgq file
  // descriptor for readability, but attempt to connect to the control
  // port every two seconds.  Once a connection is achieved, the
  // select loop will block until either a message is received on the
  // message queue, or on the control port.
  
  while(!stop) {
    
    nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
		  NULL, NULL);
    
    // A message on our message queue?
    
    if(fdSet_.isSetInRead(msgqFd)) {
      processTaskMsg(&stop);
    }
    
    // If we are connected to the controller, check the controller fd
    
    if(client_.isConnected()) {
      // Command input from the controller?
      
      if(fdSet_.isSetInRead(client_.getFd())) {
	netCommHandler_.readNetCmd();
      }
      
      // Send a pending message to the control program
      
      if(fdSet_.isSetInWrite(client_.getFd())) {
	netCommHandler_.sendNetMsg();
      }
    }
  }
}

/**.......................................................................
 * Return true if the scanner connection is open
 */
bool Control::isConnected() 
{
  return client_.isConnected();
}

/**.......................................................................
 * Process a message specific to the Task
 */
void Control::processMsg(ControlMsg* msg)
{
  COUT("Inside Control::processMsg id = " << msg->type);

  // Now parse the message

  switch (msg->type) {
  case ControlMsg::CONNECT:
    
    // Attempt to connect to the control port.  On success, disable
    // the control_connect timer.
    
    connectControl(false);
    break;
    
  case ControlMsg::INIT:   // An initialization message
    sendAntennaInitMsg(msg->body.init.start);
    break;
  case ControlMsg::NETMSG: // A message to be sent to the ACC
    packNetMsg(msg);
    break;
    
    // Messages for spawned threads
    
  case ControlMsg::ANTENNA_CONTROL_MSG:
    forwardAntennaControlMsg(msg);
    break;
  case ControlMsg::GRABBER_CONTROL_MSG:
    forwardGrabberControlMsg(msg);
    break;
  case ControlMsg::WX_CONTROL_MSG:
    forwardWxControlMsg(msg);
    break;
  case ControlMsg::RECEIVER_CONTROL_MSG:
    forwardReceiverControlMsg(msg);
    break;
  default:
    ThrowError("Unrecognized message type");
    break;
  }

  COUT("Inside Control::processMsg id = " << msg->type << " done");
}

/**.......................................................................
 * A method to send a connection status message to the
 * Master object.
 */
void Control::sendControlConnectedMsg(bool connected)
{
  MasterMsg msg;
  
  msg.packControlConnectedMsg(connected);
  
  parent_->forwardMasterMsg(&msg);
}

/**.......................................................................
 * A method to send the antenna control task an init message.
 */
void Control::sendAntennaInitMsg(bool start)
{
  ControlMsg msg;
  AntennaControlMsg* antennaMsg = msg.getAntennaControlMsg();
  
  antennaMsg->packInitMsg(start);
  
  forwardAntennaControlMsg(&msg);
}

//-----------------------------------------------------------------------
// Thread startup functions
//-----------------------------------------------------------------------

/**.......................................................................
 * Thread startup function for the Antenna Control task.
 */
THREAD_START(Control::startAntennaControl)
{  
  Control* parent = (Control*) arg;
  Thread* thread = 0;
  
  thread = parent->getThread("AntennaControl");
  
  // Instantiate the subsystem object
  
  parent->antennaControl_ = new gcp::mediator::AntennaControl(parent);
  
  // Set our internal thread pointer pointing to the AntennaControl
  // thread
  
  parent->antennaControl_->thread_ = thread;
  
  // Let other threads know we are ready
  
  thread->broadcastReady();
  
  // Finally, block, running our select loop
  
  parent->antennaControl_->run();
  
  return 0;
}

/**.......................................................................
 * GrabberControl thread startup function
 */
THREAD_START(Control::startGrabberControl)
{
  Control* parent = (Control*) arg;
  Thread* thread = 0;
  
  // Get a reference to the Thread object which will mange this
  // thread.
  
  thread = parent->getThread("GrabberControl");
  
  // Instantiate the subsystem object
  
  parent->grabberControl_ = 
    new gcp::mediator::GrabberControl(parent);
  
  // Set our internal thread pointer pointing to the AntennaRx thread
  
  parent->grabberControl_->thread_ = thread;
  
  // Let other threads know we are ready
  
  thread->broadcastReady();
  
  // Finally, block, running our select loop
  
  parent->grabberControl_->run();

  return 0;
}

/**.......................................................................
 * ReceiverControl thread startup function
 */
THREAD_START(Control::startReceiverControl)
{
  Control* parent = (Control*) arg;
  Thread* thread = 0;
  
  // Get a reference to the Thread object which will mange this
  // thread.
  
  thread = parent->getThread("ReceiverControl");
  
  // Instantiate the subsystem object
  
  parent->receiverControl_ = 
    new gcp::mediator::ReceiverControl(parent);
  
  // Set our internal thread pointer pointing to the AntennaRx thread
  
  parent->receiverControl_->thread_ = thread;
  
  // Let other threads know we are ready
  
  thread->broadcastReady();
  
  // Finally, block, running our select loop
  
  parent->receiverControl_->run();

  return 0;
}

/**.......................................................................
 * WxControl thread startup function
 */
THREAD_START(Control::startWxControl)
{
  Control* parent = (Control*) arg;
  Thread* thread = 0;
  
  // Get a reference to the Thread object which will mange this
  // thread.
  
  thread = parent->getThread("WxControl");
  
  // Instantiate the subsystem object
  
  parent->wxControl_ = 
    new gcp::mediator::WxControl(parent);

  COUT("mediator::control:: did we make it here?");
  
  // Set our internal thread pointer pointing to the AntennaRx thread
  
  parent->wxControl_->thread_ = thread;
  
  // Let other threads know we are ready
  
  thread->broadcastReady();
  
  // Finally, block, running our select loop
  
  parent->wxControl_->run();

  return 0;
}

//-----------------------------------------------------------------------
// Thread clean functions
//-----------------------------------------------------------------------

/**.......................................................................
 * AntennaControl thread cleanup function
 */
THREAD_CLEAN(Control::cleanAntennaControl)
{
  Control* parent = (Control*) arg;
  
  if(parent->antennaControl_ != 0) {
    delete parent->antennaControl_;
    parent->antennaControl_ = 0;
  }
  
  parent->sendStopMsg();
}

/**.......................................................................
 * Grabber thread cleanup function
 */
THREAD_CLEAN(Control::cleanGrabberControl)
{
  Control* parent = (Control*) arg;
  
  if(parent->grabberControl_ != 0) {
    delete parent->grabberControl_;
    parent->grabberControl_ = 0;
  }
}

/**.......................................................................
 * Receiver thread cleanup function
 */
THREAD_CLEAN(Control::cleanReceiverControl)
{
  Control* parent = (Control*) arg;
  
  if(parent->receiverControl_ != 0) {
    delete parent->receiverControl_;
    parent->receiverControl_ = 0;
  }
}

/**.......................................................................
 * Wx thread cleanup function
 */
THREAD_CLEAN(Control::cleanWxControl)
{
  Control* parent = (Control*) arg;

  if(parent->wxControl_ != 0) {
    delete parent->wxControl_;
    parent->wxControl_ = 0;
  }
}

//-----------------------------------------------------------------------
// Thread ping functions
//-----------------------------------------------------------------------

/**.......................................................................
 * A function by which we will ping the AntennaControl thread
 */
THREAD_PING(Control::pingAntennaControl)
{
  LogStream errStr;
  
  Control* parent = (Control*) arg;
  
  if(parent == 0) {
    errStr.appendMessage(true, "NULL argument");
    throw Error(errStr);
  }
  
  if(parent->antennaControl_ != 0)
    parent->antennaControl_->sendHeartBeatMsg();
}

/**.......................................................................
 * A function by which we will ping the GrabberControl thread
 */
THREAD_PING(Control::pingGrabberControl)
{
  Control* parent = (Control*) arg;
  
  if(parent == 0)
    throw Error("Control::pingGrabberControl: NULL argument.\n");
  
  if(parent->grabberControl_ != 0)
    parent->grabberControl_->sendHeartBeatMsg();
}

/**.......................................................................
 * A function by which we will ping the ReceiverControl thread
 */
THREAD_PING(Control::pingReceiverControl)
{
  Control* parent = (Control*) arg;
  
  if(parent == 0)
    throw Error("Control::pingReceiverControl: NULL argument.\n");
  
  if(parent->receiverControl_ != 0)
    parent->receiverControl_->sendHeartBeatMsg();
}

/**.......................................................................
 * A function by which we will ping the WxControl thread
 */
THREAD_PING(Control::pingWxControl)
{
  Control* parent = (Control*) arg;
  
  if(parent == 0)
    throw Error("Control::pingWxControl: NULL argument.\n");
  
  if(parent->wxControl_ != 0)
    parent->wxControl_->sendHeartBeatMsg();
}

/**.......................................................................
 * Send a heartbeat request to all threads managed by this task.
 */
void Control::sendHeartBeat()
{
  if(threadsAreRunning()) {
    pingThreads(this);
  } else 
    sendRestartMsg();
}

/**.......................................................................
 * We will piggyback on the heartbeat signal received from the parent
 * to send a heartbeat to our own tasks.
 */
void Control::respondToHeartBeat()
{
  // Respond to the parent heartbeat request
  
  if(thread_ != 0)
    thread_->setRunState(true);
  
  // And ping any threads we are running.
  
  sendHeartBeat();
}

/**.......................................................................
 * Forward a message to the AntennaControl task.
 */
void Control::forwardAntennaControlMsg(ControlMsg* msg)
{
  LogStream errStr;
  static int counter=0;
  
  if(antennaControl_) 
    antennaControl_->sendTaskMsg(msg->getAntennaControlMsg());
  
  counter++;
}

/**.......................................................................
 * Public method to forward a message to this task.
 */
void Control::forwardControlMsg(ControlMsg* msg)
{
  sendTaskMsg(msg);
}

/**.......................................................................
 * Forward a message to the master task.
 */
void Control::forwardMasterMsg(MasterMsg* msg)
{
  LogStream errStr;
  
  if(parent_ == 0) {
    errStr.appendMessage(true, "Parent task pointer is NULL");
    throw Error(errStr);
  }
  
  parent_->forwardMasterMsg(msg);
}

/**.......................................................................
 * Forward a message to the grabber control task.
 */
void Control::forwardGrabberControlMsg(ControlMsg* msg)
{
  if(grabberControl_ != 0) 
    grabberControl_->sendTaskMsg(msg->getGrabberControlMsg());
}

/**.......................................................................
 * Forward a message to the receiver control task.
 */
void Control::forwardReceiverControlMsg(ControlMsg* msg)
{
  if(receiverControl_ != 0) 
    receiverControl_->sendTaskMsg(msg->getReceiverControlMsg());
}

/**.......................................................................
 * Forward a message to the wx control task.
 */
void Control::forwardWxControlMsg(ControlMsg* msg)
{
  LogStream errStr;
  
  if(wxControl_ != 0) 
    wxControl_->sendTaskMsg(msg->getWxControlMsg());
}

/**.......................................................................
 * Pack a network message intended for the ACC
 */
void Control::packNetMsg(ControlMsg* msg)
{
  // If we have no connection to the control program, just drop the message
  
  if(client_.getFd() < 0)
    return;
  
  // Pack the message into our network buffer.
  
  bool recognized = netCommHandler_.packNewRtcNetMsg(msg->getNetMsg());
  
  DBPRINT(true, Debug::DEBUG9, "Packed a network message intended for the ACC");
  
  // Arrange to be told when the control program socket is ready for
  // the message. Meanwhile since only one message can be queued to
  // the control program at a time, arrange not to listen for
  // controller message queue messages until this message is sent.
  
  DBPRINT(true, Debug::DEBUG9, "client fd is: " << client_.getFd());
  
  // Ignore unrecognized messages

  if(recognized) {
    fdSet_.registerWriteFd(client_.getFd());
    fdSet_.clearFromReadFdSet(msgq_.fd());
  }
}

void Control::forwardNetCmd(gcp::util::NetCmd* netCmd)
{
  forwarder_->forwardNetCmd(netCmd);
}

/**.......................................................................
 * Read a command from the control program.
 */
NET_READ_HANDLER(Control::readNetCmdHandler)
{
  Control* control = (Control*)arg;
  
  // Forward the NetCmd just read to the appropriate destination
  
  control->forwarder_->
    forwardNetCmd(control->netCommHandler_.getLastReadNetCmd());
}

/**.......................................................................
 * Handler to be called once a message has been sent to the control
 * program.
 */
NET_SEND_HANDLER(Control::sendNetMsgHandler)
{
  Control* control = (Control*)arg;
  
  // Now that the message has been sent, remove the file descriptor
  // from the set to be watched, and re-register the message queue
  // descriptor to be watched for more messages
  
  control->fdSet_.clearFromWriteFdSet(control->client_.getFd());
  control->fdSet_.registerReadFd(control->msgq_.fd());
}

/**.......................................................................
 * Call this function if an error occurs while communicating with the
 * ACC
 */
NET_ERROR_HANDLER(Control::networkErrorHandler)
{
  Control* control = (Control*)arg;
  
  // Terminate our connection to the control program, and attempt to
  // reconnect
  
  control->disconnectControl();
}

/**.......................................................................
 * Send a message to the control thread
 */
void Control::sendNetLogMsg(std::string logStr, bool isErr)
{
  ControlMsg msg;
  NetMsg* netMsg = msg.getNetMsg();

  // Iterate sending the message in segments of size
  // NetMsg::maxMsgLen() until the whole thing is sent

  unsigned seq = control_->logMsgHandler_.nextSeq();
  control_->logMsgHandler_.append(seq, logStr);

  std::string message;
  bool isLast = false;

  do {

    message = control_->
      logMsgHandler_.getNextMessageSubstr(seq, netMsg->maxMsgLen(), isLast);

    netMsg->packLogMsg(message, isErr, seq, isLast);
    control_->sendTaskMsg(&msg);

  } while(!isLast);
}

/**.......................................................................
 * Send a log string to be logged back to the control program.
 */
LOG_HANDLER_FN(Control::sendLogMsg)
{
  sendNetLogMsg(logStr, false);
}

/**.......................................................................
 * Send an error string to be logged back to the control program.
 */
LOG_HANDLER_FN(Control::sendErrMsg)
{
  sendNetLogMsg(logStr, true);
}

/**.......................................................................
 * Attempt to connect to the host
 */
void Control::connectControl(bool reEnable)
{
  // Ignore any further connection requests if we are in the process
  // of handshaking with the control program, or if we are already
  // connected (there may be a delay between the message getting
  // through to the master process that we have connected, and receipt
  // of another prod to connect.
  
  if(client_.isConnected())
    return;
  
  // Else attempt to connect
  
  if(connect())
    sendControlConnectedMsg(true);
  
  // Only send a message to reenable the connect timer if it was
  // previously disabled.
  
  else if(reEnable)
    sendControlConnectedMsg(false);
}

/**.......................................................................
 * Disconnect from the host.
 */
void Control::disconnectControl()
{
  // Disconnect and let the parent know it should re-enable the
  // connect timer.
  
  disconnect();
  
  sendControlConnectedMsg(false);
}

gcp::util::RegMapDataFrameManager* Control::getArrayShare()
{
  return &(parent_->getArrayShare());
}

/**.......................................................................
 * A method to send a receiver script completion message
 */
void Control::sendScriptDoneMsg(unsigned seq)
{
  ControlMsg msg;
  NetMsg* netMsg = msg.getNetMsg();
  netMsg->packScriptDoneMsg(seq);

  sendTaskMsg(&msg);
}

std::string Control::wxHost()
{
  return parent_->wxHost();
}
