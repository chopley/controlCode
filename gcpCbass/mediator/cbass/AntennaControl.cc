#define __FILEPATH__ "mediator/specific/AntennaControl.cc"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gcp/util/specific/Directives.h"

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/Ports.h"

#include "gcp/mediator/specific/AntennaControl.h"
#include "gcp/mediator/specific/Control.h"

using namespace gcp::antenna::control; // AntennaMasterMsg
using namespace gcp::mediator;
using namespace gcp::util;

// We will time out waiting for a greeting from an antenna after 1
// second

#define CONNECTION_TIMEOUT_SEC 1

AntennaControl* AntennaControl::control_ = 0;

/**.......................................................................
 * Constructor.
 */
AntennaControl::AntennaControl(Control* parent) 
{
  parent_   = parent;
  listener_ = 0;
  control_  = this;
  pending_  = 0;
  
  // Set up to listen for tcpip connection requests from Antennas.  We
  // will use TCP/IP connections to log messages back from antennas,
  // and if not using CORBA, for sending commands to antennas.
  
  connectTcpIp();
};

/**.......................................................................
 * Set up for TCP/IP communications
 */
void AntennaControl::connectTcpIp()
{
  // Record the number of antennas we know about
  
  AntNum antnum;
  nAntenna_ = antnum.getAntMax();
  
  // Set up to listen for connection requests from the antenna
  // computers.
  
  listener_ = new TcpListener(TRANS_ANT_CONTROL_PORT, nAntenna_);
  
  // Reserve space for nAntenna_ network communications objects and
  // initialization flags.
  
  connectedHandlers_.reserve(nAntenna_);
  antennaInitState_.reserve(nAntenna_);
  antennaConnectState_.reserve(nAntenna_);
  sendPending_.reserve(nAntenna_);
  
  for(unsigned iant=0; iant < nAntenna_; iant++) {
    
    // Create the vector of command handlers
    
    connectedHandlers_.push_back(new NetCommHandler());
    
    // Initially mark all antennas as unconnected
    
    antennaConnectState_.push_back(DISCONNECTED);
    
    // Initially mark all antennas as uninitialized
    
    antennaInitState_.push_back(UNINITIALIZED);
    
    // Set all pending flags to false
    
    sendPending_.push_back(false);
  }
}

/**.......................................................................
 * Destructor.
 */
AntennaControl::~AntennaControl() 
{
  if(listener_ != 0) {
    delete listener_;
    listener_ = 0;
  }
  
  // Delete any allocated handlers
  
  for(unsigned iant=0; iant < connectedHandlers_.size(); iant++)
    delete connectedHandlers_[iant];
};

/**.......................................................................
 * Process antenna control messages.
 */
void AntennaControl::processMsg(AntennaControlMsg* msg)
{
  // If we are recording an intialization script, push this message
  // onto the initialization stack.
  
  if(recordingInitScript_)
    recordInitMsg(msg);
  
  switch(msg->type) {
    
    //------------------------------------------------------------
    // A message for the AC
    
  case AntennaControlMsg::ANTENNA_MASTER_MSG:
    
    switch (msg->getMasterMsg()->type) {
      
      // A message for the antenna drive task.
      
    case AntennaMasterMsg::DRIVE_MSG:
      DBPRINT(true, Debug::DEBUG8, "A drive message");
      processAntennaDriveMsg(msg);
      break;
    default:
      std::cout << "AntennaControl::processMsg unknown message type " << msg->getMasterMsg()->type << std::endl;
      break;
    }
    break;
    
    //------------------------------------------------------------
    // A message to begin an initialization script
    
  case AntennaControlMsg::BEGIN_INIT:
    beginInitScript();
    break;
    
    //------------------------------------------------------------
    // A message that an initialization script is ending
    
  case AntennaControlMsg::END_INIT:
    endInitScript();
    break;
    
    //------------------------------------------------------------
    // A message to mark an antenna as reachable/unreachable
    
  case AntennaControlMsg::FLAG_ANTENNA:
    break;
    
    //------------------------------------------------------------ 
    // A message that we are starting/finishing receiving an
    // initialization sequence
    
  case AntennaControlMsg::INIT:
    if(msg->body.init.start)
      startRecordingInitScript();
    else
      stopRecordingInitScript();
    break;
    
    //------------------------------------------------------------
    // A message to send an unparsed network command to the AC
    
  case AntennaControlMsg::NETCMD:
    sendRtcNetCmd(msg);
    break;
  default:
    ThrowError("Unknown msg->type " << msg->type);
    break;
  }
  
  // If an initialization is in progress, queue the next init command
  
  if(initInProgress_)
    sendNextInitMsg();
}


/**.......................................................................
 * Process antenna drive control messages.
 */
void AntennaControl::
processAntennaDriveMsg(AntennaControlMsg* antennaMsg)
{
  AntennaDriveMsg* msg = antennaMsg->getMasterMsg()->getDriveMsg();
  unsigned int antennas = antennaMsg->antennas;
  
  switch(msg->type) {
    
  case AntennaDriveMsg::TRACKER_MSG:
    processTrackerMsg(antennaMsg);
    break;
  default:
    ReportSimpleError("Unknown msg->type " << msg->type);
    break;
  }
}

/**.......................................................................
 * Process antenna drive system tracker control messages.
 */
void AntennaControl::
processTrackerMsg(AntennaControlMsg* antennaMsg)
{
  bool init = antennaMsg->init_;
  
  TrackerMsg* msg = antennaMsg->getMasterMsg()->getDriveMsg()->getTrackerMsg();
  AntNum::Id antennas = antennaMsg->antennas;
}

/**.......................................................................
 * Return true if we get a control connection to this antenna
 */
bool AntennaControl::haveControlConnection(unsigned short iant) 
{
  return (connectedHandlers_[iant]->getFd() >= 0);
}

/**.......................................................................
 * Forward a network command to a set of antennas.
 */
void AntennaControl::
sendRtcNetCmd(AntennaControlMsg* msg)
{
  AntNum antSet(msg->antennas);
  unsigned iant=0;
  bool init = msg->init_;
  bool validAntenna = false;

  // Write the command into the network buffers of requested antennas.
  
  for(unsigned iant=0; iant < nAntenna_; iant++) {
    NetCommHandler* str = connectedHandlers_[iant];
    
    // Skip this antenna if the command wasn't intended for it
    
    if(!antSet.isSet(iant))
      continue;

    validAntenna = true;

    // Skip this antenna if it is disconnected
    
    if(antennaConnectState_[iant] == DISCONNECTED)
      continue;
    
    // Execute this command only if:
    // 
    //    this antenna is initializing, and this _is_ an init command, OR
    //    this antenna is initialized, and this is _not_ an init command
    
    if(!((init && antennaInitState_[iant]==INITIALIZING) ||
	 (!init && antennaInitState_[iant]==INITIALIZED)))
      continue;
    
    // Pack the network command into the stream buffer.
    
    str->packRtcNetCmd(&msg->body.netCmd.cmd, msg->body.netCmd.opcode);

    // And register to be told when this descriptor is writable
    
    fdSet_.registerWriteFd(str->getFd());
    
    // Set the pending flag for this antenna
    
    setPending(iant, true);
  }

  // If this command wasn't forwarded to any antenna, report it as an
  // error.  Probably means someone hasn't set the antenna mask

  if(!validAntenna)
    ReportError("Forwarded a command not marked for any antenna opcode = " << msg->body.netCmd.opcode);
}

/**.......................................................................
 * A handler to be called when a command is sent to an antenna.  Just
 * removes the descriptor from the set to be watched in select().
 */
NET_SEND_HANDLER(AntennaControl::netCmdSentHandler)
{
  NetCommHandler* netHandler = (NetCommHandler*)arg;

  // Unset the pending flag for this antenna
  
  control_->setPending(netHandler->getIntId(), false);
  
  // And remove this antennas descriptor from the set to be watched
  // for writability
  
  control_->
    fdSet_.clearFromWriteFdSet(netHandler->getNetCmdHandler()->getSendFd());
}

/**.......................................................................
 * A handler to be called when a command is sent to an antenna.  Just
 * removes the descriptor from the set to be watched in select().
 */
NET_ERROR_HANDLER(AntennaControl::netErrorHandler)
{
  NetCommHandler* handler = (NetCommHandler*)arg;
  
  // Terminate the connection to this antenna
  
  control_->terminateConnection(handler);
}

/**.......................................................................
 * Increment the count of commands pending to antennas.
 */
void AntennaControl::setPending(unsigned iant, bool pending)
{
  unsigned prevPending = pending_;
  
  // Increment or Decrement the counter of pending writes
  
  if(pending)                   // If a command is pending for this
				// antenna, increment the counter
    pending_++;
  else if(sendPending_[iant]) { // Else decrement the counter if a
				// send was pending for this antenna
    pending_--;
  }
  
  // And change the state of this antenna
  
  sendPending_[iant] = pending;
  
  // When the pending count rises becomes non-zero, stop listening to
  // the message queue
  
  if(pending_ > 0 && prevPending == 0) {
    fdSet_.clearFromReadFdSet(msgq_.fd());
  }
  
  // When the pending count drops to zero, register the message
  // queue to be watched for new commands
  
  if(pending_ == 0 && prevPending > 0) {
    fdSet_.registerReadFd(msgq_.fd());
  }
}

/**.......................................................................
 * Service our message queue.
 */
void AntennaControl::serviceMsgQ()
{
  bool stop   = false;
  int  nready = 0;
  int  msgqFd = msgq_.fd();
  
  // Initially our select loop will check the msgq file descriptor for
  // readability, and the server socket for connection requests.  Once
  // connections are establihed, the select loop will block until
  // either a message is received on the message queue, on the control
  // port, or on a port to an established antenna.
  
  fdSet_.registerReadFd(msgqFd);
  
  // Start listening for connection requests from antennas.
  
  listen(true);
  
  while(!stop) {
    
    nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
    		  NULL, timeOut_);
    
    // A message on our message queue?
    
    if(fdSet_.isSetInRead(msgqFd)) {
      processTaskMsg(&stop);
    }
    
    // A connection request from an antenna?
    
    if(fdSet_.isSetInRead(listener_->getFd())) {
      initializeConnection();
    }
    
    // Are we in the process of communicating with a newly connected
    // antenna?
    
    if(temporaryHandler_.getNetMsgHandler()->getReadFd() >= 0) {
      
      // Waiting for an intialization message?
      
      if(fdSet_.isSetInRead(temporaryHandler_.getNetMsgHandler()->getReadFd())) {
	temporaryHandler_.readNetMsg();
      }
      
      // Sending a greeting message?
      
      if(fdSet_.isSetInWrite(temporaryHandler_.getNetMsgHandler()->getSendFd())) {
	temporaryHandler_.sendNetMsg();
      }
      
      // If the descriptor wasn't ready, see if we timed out waiting
      // for a response.  If so, terminate the temporary connection
      // and start listening again on the server socket.
      
      else if(timedOut()) {
	terminateConnection(&temporaryHandler_);
	listen(true);
      }
    }
    
    // Check descriptors associated with established connections.
    
    for(unsigned iant=0; iant < nAntenna_; iant++) {
      NetCommHandler* handler = connectedHandlers_[iant];
      NetMsgHandler* msgHandler = handler->getNetMsgHandler();
      NetCmdHandler* cmdHandler = handler->getNetCmdHandler();
      
      // Check for partially sent commands to established antennas.
      
      if(fdSet_.isSetInWrite(cmdHandler->getSendFd())) {
	handler->sendNetCmd();
      }
      
      // Check for partially read messages from established antennas.
      
      if(fdSet_.isSetInRead(msgHandler->getReadFd())) {
	handler->readNetMsg();
      }
      
      // Check for partially sent messages to established antennas.
      
      if(fdSet_.isSetInWrite(msgHandler->getSendFd())) {
	handler->sendNetMsg();
      }
    }
  }
}

/**.......................................................................
 * Act upon receipt of a network message from an antenna with which we
 * are in the process of establishing a connection.
 */
NET_READ_HANDLER(AntennaControl::processTemporaryNetMsg)
{
  NetMsg* netMsg = control_->temporaryHandler_.getLastReadNetMsg();
  
  switch(netMsg->type) {
    
    // If the antenna sent us an ID, respond by sending a greeting message
    
  case NetMsg::ID:
    control_->finalizeConnection();
    break;
    
    // We shouldn't receive any other messages from this antenna until
    // we have responded, so receipt of any other message is an error.
    
  default:    
    ReportSimpleError("Unknown netMsg->type " << netMsg->type);
    control_->terminateConnection(&control_->temporaryHandler_);
    break;
  }
}

/**.......................................................................
 * Send a greeting message to an antenna
 */
void AntennaControl::sendGreetingMsg(NetCommHandler* netHandler)
{
  netHandler->getNetMsgHandler()->packGreetingMsg(netHandler->getIntId());
  fdSet_.registerWriteFd(netHandler->getFd());
}

/**.......................................................................
 * Act upon receipt of a network message from one of our antennas.
 */
NET_READ_HANDLER(AntennaControl::processNetMsg)
{
  NetCommHandler* str = (NetCommHandler*) arg;
  NetMsg* netMsg = str->getLastReadNetMsg();
  
  switch(netMsg->type) {
    
    // We shouldn't get this message after this connection has been
    // established
    
  case NetMsg::ID:
    control_->terminateConnection(str);
    break;
    
    // Buffer log messages intended for the control task
    
  case NetMsg::LOG:
    control_->bufferLogMsg(netMsg); 
    break;

    // Forward all other messages to the control task

  default:   
    control_->forwardNetMsg(netMsg); 
    break;
  }
}

/**.......................................................................
 * Act upon receipt of a network message from one of our antennas.
 */
void AntennaControl::forwardNetMsg(NetMsg* msg)
{
  ControlMsg controlMsg;
  NetMsg* netMsg = controlMsg.getNetMsg();
  
  *netMsg = *msg;
  
  parent_->forwardControlMsg(&controlMsg);
}

/**.......................................................................
 * Act upon receipt of a network message from one of our antennas.
 */
void AntennaControl::bufferLogMsg(NetMsg* msg)
{
  // Append network messages to the message handler buffer until we
  // reach the end of a message

  unsigned seq = msg->body.msg.log.seq;
  bool isErr   = msg->body.msg.log.bad;
  bool isLast  = msg->body.msg.log.end;

  logMsgHandler_.append(seq, msg->body.msg.log.text);

  // If we've reached the end of the current message, send the message
  // in its entirety via the parents method

  if(isLast) 
    parent_->sendNetLogMsg(logMsgHandler_.getMessage(seq), isErr);
}

/**.......................................................................
 * This function is called to accept or reject a connection request
 * from the real-time controller.
 */
void AntennaControl::initializeConnection()
{
  int fd = -1;

  // Allow the caller to connect.
  
  fd = listener_->acceptConnection();
  
  // Attach this file descriptor to the temporary connection handler
  
  temporaryHandler_.getNetMsgHandler()->attach(fd);
  
  // Register a handler to be called when a network message is
  // received
  
  temporaryHandler_.getNetMsgHandler()->
    installReadHandler(processTemporaryNetMsg, 0);
  
  // Register a handler to be called if an error occurs. 
  
  temporaryHandler_.getNetMsgHandler()->
    installErrorHandler(netErrorHandler, &temporaryHandler_);
  
  // And register the descriptor to be watched for input.  The
  // antenna should send us its ID after the connection is made.
  
  fdSet_.registerReadFd(fd);
  
  // Stop listening for further connections until this antenna has
  // responded.
  
  listen(false);
}

/**.......................................................................
 * This function is called to terminate a connection to an antenna.
 */
void AntennaControl::terminateConnection(unsigned short iant, bool shutdown)
{
  terminateConnection(connectedHandlers_[iant], shutdown);
}

/**.......................................................................
 * This function is called to terminate a connection to an antenna.
 */
void AntennaControl::terminateConnection(NetCommHandler* handler, bool shutdown)
{
  NetMsgHandler* msgHandler = handler->getNetMsgHandler();
  NetCmdHandler* cmdHandler = handler->getNetCmdHandler();
  
  if(Debug::debugging(Debug::DEBUG7)) {
    DBPRINT(true, Debug::DEBUG7, "Terminating: " << msgHandler->getReadFd());
    DBPRINT(true, Debug::DEBUG7, "Terminating: " << msgHandler->getSendFd());
    DBPRINT(true, Debug::DEBUG7, "Terminating: " << cmdHandler->getReadFd());
    DBPRINT(true, Debug::DEBUG7, "Terminating: " << cmdHandler->getSendFd());
  }
  
  // If shutting down the connection, shut down, else just clear the
  // descriptor from the set to be watched.
  
  if(shutdown) {
    shutdownConnection(msgHandler->getReadFd());
    shutdownConnection(msgHandler->getSendFd());
    shutdownConnection(cmdHandler->getReadFd());
    shutdownConnection(cmdHandler->getSendFd());
  } else {
    fdSet_.clearFromReadFdSet(msgHandler->getReadFd());
    fdSet_.clearFromWriteFdSet(msgHandler->getSendFd());
    fdSet_.clearFromReadFdSet(cmdHandler->getReadFd());
    fdSet_.clearFromWriteFdSet(cmdHandler->getSendFd());
  }
  
  // Detach all network streams of this handler
  
  handler->attach(-1);
  
  // If this handler was associated with an antenna, mark the antenna
  // as now disconnected.
  
  if(handler->getId() != AntNum::ANTNONE) {
    
    DBPRINT(true, Debug::DEBUG7, "Marking antenna: " 
	    << handler->getIntId() << " as disconnected");
    
    // Mark the antenna as disconnected
    
    antennaConnectState_[handler->getIntId()] = DISCONNECTED;
    
    // Set the pending flag for this antenna to false
    
    setPending(handler->getIntId(), false);
    
    // And log a message to the control program
    
    ReportSimpleError("Lost control connection to Antenna" << handler->getIntId());
    
    // Set the antenna this handler is associated with to none
    
    handler->setId(AntNum::ANTNONE);
  }
}

/**.......................................................................
 * Call this function to start or stop listening on our server socket
 */
void AntennaControl::
listen(bool listenVar)
{
  // If listening, register the server socket to be watched for
  // connection requests, and set the timeout to NULL.
  
  if(listenVar) {
    fdSet_.registerReadFd(listener_->getFd());
    temporaryHandler_.attach(-1);
    timeOut_ = NULL;
  } else {
    
    // Else clear the server socket fd from the set to be watched, and
    // set the timeout.
    
    fdSet_.clearFromReadFdSet(listener_->getFd());
    
    // Set up a 1-second timer
    
    timer_.setTime(CONNECTION_TIMEOUT_SEC, 0);
    
    // This will be used as a timeout in select
    
    timeOut_ = timer_.timeVal();
    
    // And register the current time.
    
    startTime_.setToCurrentTime();
  }
}

/**.......................................................................
 * This function is called when an ID message has been received on an
 * unestablished antenna connection.
 */
void AntennaControl::finalizeConnection()
{
  NetMsg* netMsg = temporaryHandler_.getLastReadNetMsg();
  unsigned int antennaId = netMsg->body.antenna;
  AntNum antNum(antennaId);
  int fd = temporaryHandler_.getFd();
  
  // Only proceed if the response was valid, otherwise terminate the
  // temporary connection
  
  if(antNum.isValidSingleAnt()) {
    
    // We will terminate the temporary connection in either event.  But
    // don't shutdown the connection, because we will simply reattach
    
    terminateConnection(&temporaryHandler_, false);
    
    DBPRINT(true, Debug::DEBUG6, "Finalizing connection to antenna " 
	    << antennaId);
    
    // Get the handler we will use for communicating with this antenna
    
    NetCommHandler* handler = connectedHandlers_[antennaId];
    
    // Terminate any connection we may already have to this antenna
    
    terminateConnection(handler);
    
    // Set the antenna id of this handler
    
    handler->setId(antNum.getId());
    
    //------------------------------------------------------------
    // Install the new descriptor as the connection to this antenna.
    //------------------------------------------------------------
    
    // We will maintain a read-write connection for messages
    
    handler->getNetMsgHandler()->attachReadStream(fd);
    handler->getNetMsgHandler()->attachSendStream(fd);
    
    // Install message handlers to be called when communicating with
    // this antenna.
    //
    // Once the connection has been establihed, we want to send
    // initialization commands to this antenna.
    
    handler->getNetMsgHandler()->
      installSendHandler(netMsgSentHandler, handler);
    
    // Install a handler to be called when a message is received from
    // this antenna
    
    handler->getNetMsgHandler()->
      installReadHandler(processNetMsg, handler);
    
    // Register this file descriptor to be watched for input
    
    fdSet_.registerReadFd(fd);
    
    // Install a handler to be called when an error occurs
    // communicating with this antenna
    
    handler->getNetMsgHandler()->
      installErrorHandler(netErrorHandler, handler);
    
    // Send a greeting message to this antenna.  Note this is the last
    // message we will ever send to it.  From now on, there will only
    // be commands sent, and messages received.
    
    sendGreetingMsg(handler);
    
    DBPRINT(true, Debug::DEBUG6, "Cmd handler fd is: " 
	    << handler->getNetCmdHandler()->getSendFd());
    
    // Mark the antenna as uninitialized
    
    antennaInitState_[antennaId] = UNINITIALIZED;
    
    // If we have a control connection to the antenna (redundant if we
    // are using TCP/IP for communications, not if we are using
    // CORBA), mark it as connected
    
    if(haveControlConnection(antennaId)) 
      antennaConnectState_[antennaId] = CONNECTED;
    
    // Set the timeout back to NULL
    
    timeOut_ = NULL;
    
  } else  { 
    
    // Else close the connection for real
    
    terminateConnection(&temporaryHandler_, true);
    
    // Start listening again if an error occurred.
    
    listen(true);
  }
}

/**.......................................................................
 * Check if we timed out waiting for a response from a newly connected
 * antenna
 */
bool AntennaControl::timedOut()
{
  TimeVal currentTime, diff;
  
  currentTime.setToCurrentTime();
  
  diff = currentTime - startTime_;
  
  return (diff.getTimeInSeconds() >= CONNECTION_TIMEOUT_SEC);
}

//=======================================================================
// Initialization methods
//=======================================================================

/**.......................................................................
 * Mark subsequent commands as belonging to the initialization script.
 */
void AntennaControl::startRecordingInitScript()
{
  
  DBPRINT(true, Debug::DEBUG2, "Starting to record init script");
  
  // Clear the list of initialization commands
  
  initScript_.clear();
  
  // Set the recording flag to true
  
  recordingInitScript_ = true;
  
  // And mark any currently connected antennas as initializing
  
  DBPRINT(true, Debug::DEBUG9, "About to set stat to UNINITIALIZED");
  
  setInitState(UNINITIALIZED);
  
  DBPRINT(true, Debug::DEBUG9, "About to set stat to INITIALIZING");
  
  setInitState(INITIALIZING);
}

/**.......................................................................
 * Finish recording initialization commands
 */
void AntennaControl::stopRecordingInitScript()
{
  DBPRINT(true, Debug::DEBUG2, "Ceasing to record init script");
  
  // Push an END_INIT message onto the stack
  
  AntennaControlMsg msg;
  msg.packEndInitMsg();
  initScript_.push_back(msg);
  
  // Set the recording flag to false
  
  recordingInitScript_ = false;
  
  // Mark any antennas which were previously initializing as initialized
  
  setInitState(INITIALIZED);
  
  // If any antennas connected during the initialization phase, queue
  // another initialization sequence
  
  if(haveAntennasReady())
    beginInitScript();
}

/**.......................................................................
 * Record the next command in an initialization script
 */
void AntennaControl::recordInitMsg(AntennaControlMsg* msg)
{
  // Make sure this message is part of an initialization script.
  
  if(msg->init_) 
    initScript_.push_back(*msg);
}

/**.......................................................................
 * Set the initialization state of an antenna
 */
void AntennaControl::setInitState(InitState state)
{
  for(unsigned iant=0; iant < nAntenna_; iant++) {
    switch(state) {
      
      // We can always flag an antenna as uninitialized
      
    case UNINITIALIZED:
      DBPRINT(true, Debug::DEBUG2, "Setting antenna " 
	      << iant << " to " << state);
      antennaInitState_[iant] = state;
      break;
      
      // Only flag an antenna as initialized if it was previously
      // marked as initializing.  This is to protect against an
      // antenna connecting in the middle of an initialization script,
      // in which case the antenna will be flagged as UNINITIALIZED.
      
    case INITIALIZED:
      if(antennaInitState_[iant] == INITIALIZING) {
	DBPRINT(true, Debug::DEBUG2, "Setting antenna " 
		<< iant << " to " << state);
	antennaInitState_[iant] = INITIALIZED;
      }
      break;
    case INITIALIZING:
      
      // If we don't have a connection to this antenna, do nothing.
      // If we do, only mark the antenna as initializing if it was
      // previously flagged as uninitialized.
      
      if(antennaConnectState_[iant] == CONNECTED && 
	 antennaInitState_[iant] == UNINITIALIZED) {
	antennaInitState_[iant] = INITIALIZING;
	DBPRINT(true, Debug::DEBUG2, "Setting antenna " 
		<< iant << " to " << state);
      }
    default:
      break;
    }
  }
}

/**.......................................................................
 * A handler called after a greeting message has been sent to an antenna
 */
NET_SEND_HANDLER(AntennaControl::netMsgSentHandler)
{
  NetCommHandler* handler = (NetCommHandler*)arg;
  NetMsgHandler* msgHandler = handler->getNetMsgHandler();
  NetCmdHandler* cmdHandler = handler->getNetCmdHandler();
  
  DBPRINT(true, Debug::DEBUG6, "");
  
  // Now that the greeting message has been sent, attach the command
  // handler stream to the antenna fd
  
  cmdHandler->attachSendStream(msgHandler->getSendFd());
  
  // Install command handlers to be called when communicating with
  // this antenna.
  
  cmdHandler->
    installSendHandler(netCmdSentHandler, handler);
  
  // Install a handler to be called when an error occurs
  // communicating with this antenna
  
  cmdHandler->
    installErrorHandler(netErrorHandler, handler);
  
  // Once the greeting message has been sent, we won't send anymore
  // messages.
  
  control_->fdSet_.clearFromWriteFdSet(msgHandler->getSendFd());  
  msgHandler->attachSendStream(-1);
  
  // Log a message that we have accepted a connection
  
  ReportMessage("Control connection established to Antenna" << handler->getIntId());
  
  // And start the initialization script
  
  control_->beginInitScript();
}

/**.......................................................................
 * Start an initialization script.
 */
void AntennaControl::beginInitScript()
{
  DBPRINT(true, Debug::DEBUG7, "Inside beginInitScript");
  
  // If an initialization is in progress, don't start a new one.  When
  // endInitScript() is called at the end of the current
  // initialization phase, we will check if any new antennas have
  // connected, and queue another call to beginInitScript() then.
  
  if(control_->initInProgress_)
    return;
  
  DBPRINT(true, Debug::DEBUG7, "Init stack is: " 
	  << initScript_.size() << " deep");
  
  // Set the script iterator pointing to the head of the command list
  
  control_->initScriptIter_ = control_->initScript_.begin();
  
  // Mark an initialization as in progress
  
  DBPRINT(true, Debug::DEBUG7, "Setting initInProgress_ to true");
  
  control_->initInProgress_ = true;
  
  // Mark any uninitialized antennas to which we currently have a
  // connection as initializing
  
  control_->setInitState(INITIALIZING);
  
  // Start sending the messages in the init script list
  
  control_->sendNextInitMsg();
}

/**.......................................................................
 * Finish an initialization script.
 */
void AntennaControl::endInitScript() 
{
  DBPRINT(true, Debug::DEBUG7, "Inside endInitScript");
  
  // Mark this initialization as finished
  
  DBPRINT(true, Debug::DEBUG7, "Setting initInProgress_ to false");
  
  initInProgress_ = false;
  
  // Set the iterator back to the beginning of the list
  
  initScriptIter_ = initScript_.begin();
  
  // Mark any previously initializing antennas as initialized
  
  setInitState(INITIALIZED);
  
  // Send a request for an update of ephemeris parameters from the
  // ACC.  These must be re-initialized from above, since they are
  // time-dependent.
  
  sendNavUpdateMsg();
  
  // Start listening again on the server socket.  Note that this
  // function clears the fd of the temporaryHandler_.
  
  listen(true);
}

/**.......................................................................
 * Send the next initialization script command
 */
void AntennaControl::sendNextInitMsg()
{
  // If we are at the end of the initialization command list, mark the
  // initialization as finished
  
  if(initScriptIter_ == initScript_.end()) {
    
    DBPRINT(true, Debug::DEBUG2, "Ending init script");
    endInitScript();
    
  } else {
    AntennaControlMsg msg = *initScriptIter_;
    
    sendTaskMsg(&msg);
    initScriptIter_++;
    
    DBPRINT(true, Debug::DEBUG7, "Done sending next init script command");
  }
}

/**.......................................................................
 * Return true if any antennas have connected but have not yet been
 * initialized
 */
bool AntennaControl::haveAntennasReady()
{
  for(unsigned iant=0; iant < nAntenna_; iant++)
    if(antennaConnectState_[iant] == CONNECTED && 
       antennaInitState_[iant] == UNINITIALIZED)
      return true;
  return false;
}

/**.......................................................................
 * Send a request for an update of ephemeris parameters from the ACC
 */
void AntennaControl::sendNavUpdateMsg()
{
  ControlMsg msg;
  NetMsg* netMsg = msg.getNetMsg();
  
  netMsg->packNavUpdateMsg();
  
  parent_->sendTaskMsg(&msg);
}

