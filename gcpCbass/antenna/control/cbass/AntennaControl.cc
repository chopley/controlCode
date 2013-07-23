#define __FILEPATH__ "antenna/control/specific/AntennaControl.cc"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Logger.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/Ports.h"

#include "gcp/antenna/control/specific/AntennaMaster.h"
#include "gcp/antenna/control/specific/AntennaControl.h"
#include "gcp/antenna/control/specific/AntennaGpib.h"
#include "gcp/antenna/control/specific/DlpTempSensors.h"
#include "gcp/antenna/control/specific/LnaBiasMonitor.h"
#include "gcp/antenna/control/specific/AdcMonitor.h"

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

using namespace gcp::util;
using namespace gcp::antenna::control;

/**
 * Initialize the static pointer to NULL.
 */
AntennaControl* AntennaControl::control_ = 0;

/**.......................................................................
 * Constructor to be used if connecting to an array control
 * host via TCP/IP.
 */
AntennaControl::AntennaControl(AntennaMaster* parent, bool simGpib, bool simDlp, bool simLna, bool simAdc) :
  SpecificTask(), gcp::util::GenericTask<AntennaControlMsg>::GenericTask(),
  parent_(parent)
{
  control_   = this;
  
  gpibTask_  = 0;
  dlp_       = 0;
  lnaTask_   = 0;
  adcTask_   = 0;

  if(parent_)
    share_ = parent_->getShare();

  forwarder_ = 0;
  forwarder_ = new AntNetCmdForwarder(parent);

  // I used to attempt to connect to the control program at this
  // point, but this is quite dangerous, since AntennaMaster is
  // waiting for us to exit the constructor before proceeding.  If we
  // were to connect, and receive a slew of initialization commands,
  // which we would forward to the master, we might fill the master's
  // message queue, causing us to block, which would cause a deadlock,
  // since the master is waiting for the thread which instantiated us
  // to call broadcastReady(), which can only happen when we exit this
  // constructor.  Instead, we must rely on the master to tell us when
  // it's ok to connect by sending us a connect message.
  COUT("simgpib: " << simGpib);
  COUT("simlna: " << simLna);

  
  if(!simGpib) {
    threads_.push_back(new Thread(&startGpib, &cleanGpib, 
				  &pingGpib,  "Gpib", 0, 0));
  }

  // Start threads managed by this task

  startThreads(this);

  // Instantiate the temperature sensor object

  if(!simDlp) {
    dlp_ = new DlpTempSensors(share_, "thermal", true);
    dlp_->spawn();
  }

  if(!simLna) {
    lnaTask_ = new LnaBiasMonitor(share_, "receiver", true);
    lnaTask_->spawn();
  }

  if(!simAdc) {
    adcTask_ = new AdcMonitor(share_, "koekblik", true);
    adcTask_->spawn();
  }

}

/**.......................................................................
 * Destructor.
 */
AntennaControl::~AntennaControl() 
{
  disconnect();

  if(forwarder_) {
    delete forwarder_;
    forwarder_ = 0;
  }

  if(dlp_){
    delete dlp_;
    dlp_ = 0;
  };

  if(lnaTask_){
    delete lnaTask_;
    lnaTask_ = 0;
  };

  if(adcTask_){
    delete adcTask_;
    adcTask_ = 0;
  };

};

/*.......................................................................
 * Connect to the controller port of the control program.
 */
bool AntennaControl::connect()				 
{
  // Terminate any existing connection.
  
  disconnect();

  // Attempt to open a non-blocking connection to the server
  
  if(client_.connectToServer(parent_->host(), 
			     TRANS_ANT_CONTROL_PORT, true) < 0) {
    return false;
  }

  // Once we've successfully achieved a connection, reconfigure the
  // socket for non-blocking I/O

  client_.setBlocking(false);

  // Attach the NetMsg I/O streams to the new client socket.  The
  // NetCmd streams will be attached when the connection has been
  // proven

  netCommHandler_.getNetMsgHandler()->attach(client_.getFd());
  
  // Register a handler to be called when a message has been sent

  netCommHandler_.getNetMsgHandler()->
    installSendHandler(netMsgSentHandler, this);

  // Register a handler to be called when a message has been read

  netCommHandler_.getNetMsgHandler()->
    installReadHandler(netMsgReadHandler, this);

  // Install a handler to be called if any errors occur while
  // communicating with the control program

  netCommHandler_.getNetMsgHandler()->
    installErrorHandler(netErrorHandler, this);

  // Register the socket to be watched for input

  fdSet_.registerReadFd(client_.getFd());
  
  // Send a greeting message to the controller.

  sendAntennaIdMsg();

  return true;
}

/**.......................................................................
 * Pack a greeting message to be sent to the controller.
 */
void AntennaControl::sendAntennaIdMsg()
{
  DBPRINT(true, Debug::DEBUG6, "Inside sendAntennaIdMsg()");

  // Pack the message into our network buffer.

  netCommHandler_.getNetMsgHandler()->
    packAntennaIdMsg(parent_->getAnt()->getIntId());

  // And set the fd to be watched for writability

  fdSet_.registerWriteFd(client_.getFd());
}

/**.......................................................................
 * Disconnect the connection to the control-program control port.
 */
void AntennaControl::disconnect()
{
  if(client_.isConnected()) 
    fdSet_.clear(client_.getFd());

  // Disconnect from the server socket.

  DBPRINT(true, Debug::DEBUG6, "About to disconnect from the server socket");
  client_.disconnect();

  // Detach network handlers

  netCommHandler_.attach(-1);
}

/**.......................................................................
 * Service our message queue.
 */
void AntennaControl::serviceMsgQ()
{
  bool stop   = false;
  int  nready = 0;
  LogStream errStr;
  
  // Our select loop will always check the msgq file descriptor for
  // readability, and the control port, when we have successfully
  // achieved a connection.
  
  fdSet_.registerReadFd(msgq_.fd());

  while(!stop) {

    nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
		  NULL, NULL);

    // A message on our message queue?

    if(fdSet_.isSetInRead(msgq_.fd())) {
      processTaskMsg(&stop);
    }
    
    // Command input from the controller?  Note that the check for
    // commands should come before the check for messages, as receipt
    // of a greeting message may change the state of the NetCmdHandler
    // fd.

    if(fdSet_.isSetInRead(netCommHandler_.getNetCmdHandler()->getReadFd())) {
      netCommHandler_.readNetCmd();
    }
    
    // Are we waiting for a message from the controller?
    
    if(fdSet_.isSetInRead(netCommHandler_.getNetMsgHandler()->getReadFd())) {
      netCommHandler_.readNetMsg();
    }
    
    // Are we waiting to send a message to the controller?
    
    if(fdSet_.isSetInWrite(netCommHandler_.getNetMsgHandler()->getSendFd())) {
      netCommHandler_.sendNetMsg();
    }
  }
}

/**.......................................................................
 * Return true if the scanner connection is open
 */
bool AntennaControl::isConnected() 
{
  return client_.isConnected();
}

/**.......................................................................
 * Process a message received on the AntennaControl message queue
 *
 * Input:
 *
 *   msg AntennaControlMsg* The message received on the AntennaControl 
 *                   message queue.
 */
void AntennaControl::processMsg(AntennaControlMsg* msg)
{
  switch (msg->type) {
  case AntennaControlMsg::CONNECT:
    connectControl(false);
    break;
  case AntennaControlMsg::NETMSG:    // A message to be sent to the ACC
    packNetMsg(msg);
    break;
  case AntennaControlMsg::GPIB_MSG:  // A message to be sent to the
				     // GPIB thread
    forwardGpibMsg(msg);
    break;
  case AntennaControlMsg::LNA_MSG:  // A message to be sent to the
				     // LNA thread
    if(lnaTask_)
      lnaTask_->sendVoltRequest();
    break;

  case AntennaControlMsg::ADC_VOLTS:  // A message to be sent to the
				     // ADC thread
    if(adcTask_)
      adcTask_->sendVoltRequest();
    break;

  case AntennaControlMsg::DLP_TEMPS: // A message to query the
				     // temperatures from the Dlp Usb
				     // device
    if(dlp_)
      dlp_->sendTempRequest();

    break;

  default:
    ReportError("unknown msg->type " << msg->type);
    break;
  }
}

/**.......................................................................
 * Pack a network message intended for the ACC
 */
void AntennaControl::packNetMsg(AntennaControlMsg* msg)
{
  // Pack the message into our network buffer.

  netCommHandler_.packNetMsg(msg->getNetMsg());

  // Arrange to be told when the control program socket is ready for
  // the message. Meanwhile since only one message can be queued to
  // the control program at a time, arrange not to listen for
  // controller message queue messages until this message is sent.
  
  fdSet_.registerWriteFd(client_.getFd());
  fdSet_.clearFromReadFdSet(msgq_.fd());
}

/**.......................................................................
 * Attempt to connect to the host
 */
void AntennaControl::connectControl(bool reEnable)
{
  DBPRINT(true, Debug::DEBUG6, "In connetControl, client is: "
	  << (client_.isConnected() ? "connected" : "not connected"));

  // Ignore any further connection requests if we are in the process
  // of handshaking with the control program, or if we are already
  // connected (there may be a delay between the message getting
  // through to the master process that we have connected, and receipt
  // of another prod to connect.

  if(client_.isConnected())
    return;

  // Else attempt to connect

  if(connect()) {
    sendControlConnectedMsg(true);
  }

  // Only send a message to reenable the connect timer if it was
  // previously disabled.

  else if(reEnable)
    sendControlConnectedMsg(false);
}

/**.......................................................................
 * Disconnect from the host.
 */
void AntennaControl::disconnectControl()
{
  // Disconnect and let the parent know it should re-enable the
  // connect timer.

  disconnect();

  // And send a message to reenable the connect timer.

  sendControlConnectedMsg(false);
}

/**.......................................................................
 * Respond to a network error
 */
NET_ERROR_HANDLER(AntennaControl::netErrorHandler)
{
  AntennaControl* control = (AntennaControl*) arg;

  control->disconnectControl();

  // And remove the handlers

  Logger::installLogHandler(NULL);
  Logger::installErrHandler(NULL);
}

/**.......................................................................
 * Send a message to the parent about the connection status of the
 * host
 */
void AntennaControl::sendControlConnectedMsg(bool connected) 
{
  AntennaMasterMsg msg;

  msg.packControlConnectedMsg(connected);

  parent_->forwardMasterMsg(&msg);
}

/**.......................................................................
 * Send a message to the parent about the connection status of the
 * host
 */
void AntennaControl::sendGpibConnectedMsg(bool connected) 
{
}

/**.......................................................................
 * Public method to get a reference to the FrameSender object.
 */
FrameSender* AntennaControl::getSender()
{
  return &sender_;
}

/**.......................................................................
 * Send a message to the control thread
 */
void AntennaControl::sendNetMsg(std::string& logStr, bool isErr)
{
  AntennaControlMsg msg;
  NetMsg* netMsg = msg.getNetMsg();

  // Iterate sending the message in segments of size
  // NetMsg::maxMsgLen() until the whole thing is sent

  unsigned seq = control_->logMsgHandler_.nextSeq();
  control_->logMsgHandler_.append(seq, logStr);

  std::string message;
  bool isLast = false;

  do {

    message = control_->logMsgHandler_.getNextMessageSubstr(seq, netMsg->maxMsgLen(), isLast);
    netMsg->packLogMsg(message, isErr, seq, isLast);
    control_->sendTaskMsg(&msg);

  } while(!isLast);
}

/**.......................................................................
 * Send a log string to be logged back to the control program.
 */
LOG_HANDLER_FN(AntennaControl::sendLogMsg)
{
  sendNetMsg(logStr, false);
}

/**.......................................................................
 * Send an error string to be logged back to the control program.
 */
LOG_HANDLER_FN(AntennaControl::sendErrMsg)
{
  sendNetMsg(logStr, true);
}

/**.......................................................................
 * A method called when a message has been sent to the control program
 */
NET_SEND_HANDLER(AntennaControl::netMsgSentHandler)
{
  AntennaControl* control = (AntennaControl*)arg;
  NetCommHandler* handler = &control_->netCommHandler_;
  
  DBPRINT(true, Debug::DEBUG7, "");

  // Remove the fd from the set to be watched

  control_->
    fdSet_.clearFromWriteFdSet(handler->getNetMsgHandler()->getSendFd());

  // And make sure we are listening to our message queue.  Apart from
  // the initial id message, sending of all further messages to the
  // control program causes us to stop listening to our message queue
  // until they are sent.

  control_->fdSet_.registerReadFd(control_->msgq_.fd());
}

/**.......................................................................
 * A method called when a message has been read from the control program
 */
NET_READ_HANDLER(AntennaControl::netMsgReadHandler)
{
  AntennaControl* control = (AntennaControl*)arg;
  NetCommHandler* handler = &control_->netCommHandler_;
  NetMsg* msg = handler->getNetMsgHandler()->getLastReadNetMsg();

  // See what the message was.

  switch(msg->type) {

    // After we have sent an ID message to the control program, it
    // should respond with a greeting message

  case NetMsg::GREETING:
    {
      control_->parseGreetingMsg(msg);
    }
    break;

    // Any else is an error.  Once we have received a greeting
    // message, we should receive no furter messages from the control
    // program -- only commands.

  default:
    ReportError("Unknown msg->type " << msg->type);
    control_->disconnectControl();
    break;
  }
}

/**.......................................................................
 * A handler called wen a command has been read from the control
 * program.
 */
NET_READ_HANDLER(AntennaControl::netCmdReadHandler)
{
  AntennaControl* control = (AntennaControl*)arg;

  control->forwarder_->
    forwardNetCmd(control->netCommHandler_.getLastReadNetCmd());
}

/**.......................................................................
 * Read a greeting message from the control program, and decide what
 * to do
 */
void AntennaControl::parseGreetingMsg(NetMsg* msg)
{
  DBPRINT(true, Debug::DEBUG6, *msg);

  if((msg->body.msg.greeting.revision != REGMAP_REVISION) ||
     (msg->body.msg.greeting.nReg     != parent_->getShare()->getNreg()) ||
     (msg->body.msg.greeting.nByte    != parent_->getShare()->getNbyte())) {

    // If there is a register mismatch, we cannot continue.

    DBPRINT(true, Debug::DEBUGANY, "Register map mismatch wrt host");
    throw Error("Register map mismatch wrt host");

    // Else set up for normal control operations

  } else {

    // Install logging handlers by which other tasks can send messages
    // to the control program

    Logger::setPrefix(parent_->getAnt()->getLoggerPrefix());
    Logger::installLogHandler(AntennaControl::sendLogMsg);
    Logger::installErrHandler(AntennaControl::sendErrMsg);

    NetMsgHandler* msgHandler = netCommHandler_.getNetMsgHandler();
    NetCmdHandler* cmdHandler = netCommHandler_.getNetCmdHandler();
    unsigned fd = msgHandler->getReadFd();

    // Once the greeting message has been received, we shouldn't get
    // any more messages

    fdSet_.clearFromReadFdSet(msgHandler->getReadFd());
    msgHandler->attachReadStream(-1);

    // Now that the greeting message has been received, attach the
    // command handler stream to the control fd, and (re)register the
    // read fd.

    cmdHandler->attachReadStream(fd);
    fdSet_.registerReadFd(fd);

    // And register handlers to be called when a command has been
    // read, or if an error occurs while communicating.

    netCommHandler_.getNetCmdHandler()->
      installReadHandler(netCmdReadHandler, this);

    netCommHandler_.getNetCmdHandler()->
      installErrorHandler(netErrorHandler, this);
  }
}

/**.......................................................................
 * Public method to get access to our shared resource object
 */
SpecificShare* AntennaControl::getShare()
{
  return share_;
}

/**.......................................................................
 * Gpib thread startup function
 */
THREAD_START(AntennaControl::startGpib)
{
  bool waserr=false;
  AntennaControl* control = (AntennaControl*) arg;
  Thread* thread = 0;

  try {

    // Get the Thread object which will manage this thread.
    
    thread = control->getThread("Gpib");
    
    // Instantiate the subsystem object
    
    control->gpibTask_ = new gcp::antenna::control::AntennaGpib(control, "/dev/ttyGpib");
    
    // Set our internal thread pointer pointing to the Gpib thread
    
    control->gpibTask_->thread_ = thread;
    
    // Let other threads know we are ready
    
    thread->broadcastReady();
    
    // Finally, block, running our message service:
    
    control->gpibTask_->run();

  } catch(Exception& err) {
    DBPRINT(true, Debug::DEBUGANY, "====== Exception: ====== " << err.what());
    throw err;
  }

  return 0;
}

/**.......................................................................
 * A cleanup handler for the Gpib thread.
 */
THREAD_CLEAN(AntennaControl::cleanGpib)
{
  AntennaControl* control = (AntennaControl*) arg;

  // Call the destructor function explicitly for the gpibTask
    
  if(control->gpibTask_ != 0) {
    delete control->gpibTask_;
    control->gpibTask_ = 0;
  }
}

/**.......................................................................
 * A function by which we will ping the Gpib thread
 */
THREAD_PING(AntennaControl::pingGpib)
{
  bool waserr=0;
  AntennaControl* control = (AntennaControl*) arg;

  if(control == 0)
    throw Error("AntennaControl::pingGpib: NULL argument.\n");

  control->gpibTask_->sendHeartBeatMsg();
}

/**.......................................................................
 * Forward a message to the gpib task
 */
ANTENNACONTROL_TASK_FWD_FN(AntennaControl::forwardGpibMsg)
{
  if(control_->gpibTask_ == 0)
    return;

  AntennaGpibMsg* gpibMsg = msg->getGpibMsg();

  control_->gpibTask_->sendTaskMsg(gpibMsg);
}


