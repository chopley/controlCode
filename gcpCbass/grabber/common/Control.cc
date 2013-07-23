#define __FILEPATH__ "grabber/Control.cc"

#include <sys/socket.h> // Needed for shutdown()

#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Directives.h"
#include "gcp/util/common/Exception.h"

#include "gcp/util/common/SpecificName.h"

#include "gcp/grabber/common/Master.h"
#include "gcp/grabber/common/Control.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::grabber;

/**
 * Initialize the static pointer to NULL.
 */
Control* Control::control_ = 0;

/**.......................................................................
 * Constructor for the Control container
 */
Control::Control(Master* parent) :
  parent_(parent)
{
  control_   = 0;
  forwarder_ = 0;

  // Initialize the static pointer to ourselves.

  control_ = this;

  // Sanity check
  
  if(parent == 0) {
    ThrowError("Received NULL parent argument");
  }
  
  // Allocate the new forwarder

  forwarder_ = new GrabberNetCmdForwarder(parent);
}

/**.......................................................................
 * Destructor function for Control
 */
Control::~Control()
{
  disconnect();

  if(forwarder_ != 0) {
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
  
  if(client_.connectToServer(parent_->ctlHost(), 
			     TRANS_GRABBER_CONTROL_PORT, true) < 0) {

    ReportMessage("Couldn't connect to " 
		  << gcp::util::SpecificName::mediatorName()
		  << " at " << parent_->ctlHost() << ":" 
		  << TRANS_GRABBER_CONTROL_PORT
		  << " (to receive commands)");

    return false;
  }
  
  // Once we've successfully achieved a connection, reconfigure the
  // socket for non-blocking I/O

  client_.setBlocking(false);

  // Attach the network I/O streams to the new client socket.
  
  netCommHandler_.attach(client_.getFd());

  // Attach a callback to be invoked when a NetCmd has been read

  netCommHandler_.getNetCmdHandler()->
    installReadHandler(netCmdReadHandler, this);

  // And an error handler

  netCommHandler_.getNetCmdHandler()->
    installErrorHandler(netErrorHandler, this);

  // Attach a callback to be invoked when a NetMsg has been sent

  netCommHandler_.getNetMsgHandler()->
    installSendHandler(netMsgSentHandler, this);

  // And an error handler

  netCommHandler_.getNetMsgHandler()->
    installErrorHandler(netErrorHandler, this);

  // And register the socket to be watched for readability

  fdSet_.registerReadFd(client_.getFd());
  
  // If we have a connection to the ACC, install log message handlers

  Logger::setPrefix("Grabber: ");
  Logger::installLogHandler(sendLogMsg);
  Logger::installErrHandler(sendErrMsg);

  if(Debug::debugging(Debug::DEBUG7)) {
    LogStream testStr;
    testStr.appendMessage(true, "This is a test");
    testStr.log();
  }

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
    ThrowError("Received NULL file descriptor");
  
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
    
    if(fdSet_.isSetInRead(msgqFd))
      processTaskMsg(&stop);
    
    // If we are connected to the controller, check the controller fd
   
    if(client_.isConnected()) {
      
      // Command input from the controller?
      
      if(fdSet_.isSetInRead(client_.getFd()))
	netCommHandler_.readNetCmd();

      // Send a pending message to the control program

      if(fdSet_.isSetInWrite(client_.getFd()))
	netCommHandler_.sendNetMsg();
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
  // Now parse the message

  switch (msg->type) {
  case ControlMsg::CONNECT:

    // Attempt to connect to the control port.  On success, disable
    // the control_connect timer.

    connectControl(false);
    break;
  case ControlMsg::NETMSG: // A message to be sent to the
				     // ACC
    packNetMsg(msg);
    break;
  default:
    throw Error("Control::processMsg: Unrecognized message type.\n");
    break;
  }
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
 * Pack a network message intended for the ACC
 */
void Control::packNetMsg(ControlMsg* msg)
{
  // If we have no connection to the control program, just drop the message

  if(client_.getFd() < 0)
    return;

  // Pack the message into our network buffer.

  netCommHandler_.packNetMsg(msg->getNetMsg());

  DBPRINT(true, Debug::DEBUG3, "Packed a network message intended for the ACC");

  // Arrange to be told when the control program socket is ready for
  // the message. Meanwhile since only one message can be queued to
  // the control program at a time, arrange not to listen for
  // controller message queue messages until this message is sent.
  
  fdSet_.registerWriteFd(client_.getFd());
  fdSet_.clearFromReadFdSet(msgq_.fd());
}

/**.......................................................................
 * Read a command from the control program.
 */
NET_READ_HANDLER(Control::netCmdReadHandler)
{
  Control* control = (Control*)arg;

  // Forward the NetCmd just read to the appropriate destination

  DBPRINT(true, Debug::DEBUG7, "Got a net command from the control program");

  control->forwarder_->
    forwardNetCmd(control->netCommHandler_.getLastReadNetCmd());
}

/**.......................................................................
 * Send a message to the control program.
 */
NET_SEND_HANDLER(Control::netMsgSentHandler)
{
  Control* control = (Control*)arg;

  // Now that the message ha been sent, remove the file descriptor
  // from the set to be watched, and re-register the message queue
  // descriptor to be watched for more messages

  control->fdSet_.clearFromWriteFdSet(control->client_.getFd());
  control->fdSet_.registerReadFd(control->msgq_.fd());
}

/**.......................................................................
 * Call this function if an error occurs while communicating with the
 * ACC
 */
NET_ERROR_HANDLER(Control::netErrorHandler)
{
  Control* control = (Control*)arg;

  // Terminate our connection to the control program, and attempt to
  // reconnect

  control->disconnectControl();
}

/**.......................................................................
 * Send a message to the control thread
 */
void Control::sendNetMsg(std::string& logStr, bool isErr)
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

    message = control_->logMsgHandler_.getNextMessageSubstr(seq, netMsg->maxMsgLen(), isLast);
    netMsg->packLogMsg(message, isErr, seq, isLast);
    control_->sendTaskMsg(&msg);

  } while(!isLast);
}

/**.......................................................................
 * Send a log string to be logged back to the control program.
 */
LOG_HANDLER_FN(Control::sendLogMsg)
{
  sendNetMsg(logStr, false);
}

/**.......................................................................
 * Send an error string to be logged back to the control program.
 */
LOG_HANDLER_FN(Control::sendErrMsg)
{
  sendNetMsg(logStr, true);
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


