#define __FILEPATH__ "mediator/specific/GrabberControl.cc"

#include <cmath>
#include <sys/socket.h> // Needed for shutdown()

#include "gcp/mediator/specific/Master.h"
#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/GrabberControl.h"

#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

using namespace std;
using namespace gcp::mediator;
using namespace gcp::util;

//=======================================================================
// GrabberControl class
//=======================================================================

/**.......................................................................
 * Constructor.
 */
GrabberControl::GrabberControl(Control* parent) :
  parent_(parent)
{
  // Set up to listen for connection requests from the frame grabber
  
  listener_ = 0;
  listener_ = new TcpListener(TRANS_GRABBER_CONTROL_PORT, 1);
}

/**.......................................................................
 * Destructor.
 */
GrabberControl::~GrabberControl() 
{
  if(listener_ != 0) {
    delete listener_;
    listener_ = 0;
  }
}

/**.......................................................................
 * Process control messages.
 */
void GrabberControl::processMsg(GrabberControlMsg* msg)
{
  switch(msg->type) {
    
    // A network message to be forwarded to the frame grabber
    
  case GrabberControlMsg::NETCMD:
    sendRtcNetCmd(msg);
    break;
  default:
    ThrowError("Unrecognized message type");
    break;
  }
}

/**.......................................................................
 * Forward a network command to a set of antennas.
 */
void GrabberControl::
sendRtcNetCmd(GrabberControlMsg* msg)
{
  // Pack the network command into the stream buffer.
  
  netHandler_.packRtcNetCmd(&msg->body.netCmd.cmd, msg->body.netCmd.opcode);
  
  // And register to be told when this descriptor is writable
  
  fdSet_.registerWriteFd(netHandler_.getNetCmdHandler()->getSendFd());
}

/**.......................................................................
 * Service our message queue.
 */
void GrabberControl::serviceMsgQ()
{
  bool stop   = false;
  int  nready = 0;
  int  msgqFd = msgq_.fd();
  
  // Start listening for connection requests.
  
  listen(true);
  
  // Initially our select loop will check the msgq file descriptor for
  // readability, and the server socket for connection requests.  Once
  // connections are establihed, the select loop will block until
  // either a message is received on the message queue, on the control
  // port, or on a port to an established antenna.
  
  fdSet_.registerReadFd(msgqFd);
  
  while(!stop) {
    nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
		  NULL, NULL);
    
    // A message on our message queue?
    
    if(fdSet_.isSetInRead(msgqFd))
      processTaskMsg(&stop);
    
    // A connection request from the frame grabber?
    
    if(fdSet_.isSetInRead(listener_->getFd()))
      connectGrabber();
    
    // Check for partially read messages from the frame grabber
    
    if(fdSet_.isSetInRead(netHandler_.getNetMsgHandler()->getReadFd()))
      netHandler_.readNetMsg();
    
    // Check for partially sent commands to the frame grabber
    
    if(fdSet_.isSetInWrite(netHandler_.getNetCmdHandler()->getSendFd()))
      netHandler_.sendNetCmd();
  }
}

/**.......................................................................
 * Act upon receipt of a network message from one of our antennas.
 */
void GrabberControl::
processNetMsg()
{
  NetMsg* netMsg = netHandler_.getLastReadNetMsg();
  
  switch(netMsg->type) {
  case NetMsg::LOG:
    bufferLogMsg(netMsg);
    break;
  default:                 // Forward all messages to the control task
			   // for now
    forwardNetMsg(netMsg); 
    break;
  }
}

/**.......................................................................
 * Act upon receipt of a network message from one of our antennas.
 */
void GrabberControl::bufferLogMsg(NetMsg* msg)
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
 * Forward a network message received from the frame grabber.
 */
void GrabberControl::
forwardNetMsg(NetMsg* msg)
{
  ControlMsg controlMsg;
  NetMsg* netMsg = controlMsg.getNetMsg();
  
  *netMsg = *msg;
  
  parent_->forwardControlMsg(&controlMsg);
}

/**.......................................................................
 * This function is called to accept or reject a connection request
 * from the real-time controller.
 */
void GrabberControl::
connectGrabber()
{
  DBPRINT(true, Debug::DEBUG7, "Got a connection request");
  
  // Terminate any connections we may already have to the frame grabber
  
  disconnectGrabber();
  
  // Allow the caller to connect.
  
  int fd = -1;
  fd = listener_->acceptConnection();
  
  // Attach this file descriptor to the first empty slot in the vector
  // of descriptors from which we are waiting for responses.
  
  netHandler_.attach(fd);
  
  // Attach a handler to be called when a message is read from the
  // frame grabber
  
  netHandler_.getNetMsgHandler()->installReadHandler(netMsgReadHandler, this);
  netHandler_.getNetMsgHandler()->installErrorHandler(netErrorHandler, this);
  
  // Attach a handler to be called when a command is sent to the frame
  // grabber
  
  netHandler_.getNetCmdHandler()->installSendHandler(netCmdSentHandler, this);
  netHandler_.getNetCmdHandler()->installErrorHandler(netErrorHandler, this);
  
  // Stop listening for connection requests
  
  listen(false);
  
  // And register the descriptor to be watched for input. 
  
  fdSet_.registerReadFd(netHandler_.getNetMsgHandler()->getReadFd());
}

/**.......................................................................
 * This function is called when an ID message has been received on an
 * unestablished antenna connection.
 */
void GrabberControl::
disconnectGrabber()
{
  int fd = netHandler_.getFd();
  
  if(fd >= 0) {
    fdSet_.clear(fd);
    shutdown(fd, 2);
    close(fd);
    netHandler_.attach(-1);
  }
  
  // And start listening for connections again
  
  listen(true);
}

/**.......................................................................
 * Call this function to start or stop listening on our server socket
 */
void GrabberControl::
listen(bool listenVar)
{
  // If listening, register the server socket to be watched for
  // connection requests, and set the timeout to NULL.
  
  if(listenVar) {
    fdSet_.registerReadFd(listener_->getFd());
  } else {
    
    // Else clear the server socket fd from the set to be watched, and
    // set the timeout.
    
    fdSet_.clearFromReadFdSet(listener_->getFd());
  }
}

//-----------------------------------------------------------------------
// Handlers for network communications
//-----------------------------------------------------------------------

/**.......................................................................
 * A handler to be called when a message has been completely read from
 * the frame grabber.
 */
NET_READ_HANDLER(GrabberControl::netMsgReadHandler)
{
  GrabberControl* grabber =  (GrabberControl*)arg;
  
  DBPRINT(true, Debug::DEBUG7, 
	  "Recevied a network message from the frame grabber");
  
  grabber->processNetMsg();
}

/**.......................................................................
 * A handler to be called when command has been completely sent.
 */
NET_SEND_HANDLER(GrabberControl::netCmdSentHandler)
{
  GrabberControl* grabber =  (GrabberControl*)arg;
  
  // Stop watching the file descriptor
  
  grabber->fdSet_.clearFromWriteFdSet(grabber->netHandler_.getNetCmdHandler()->getSendFd());
}

/**.......................................................................
 * A handler to be called when an error occurs in communication
 */
NET_ERROR_HANDLER(GrabberControl::netErrorHandler)
{
  GrabberControl* grabber =  (GrabberControl*)arg;
  
  DBPRINT(true, Debug::DEBUG7, "Inside netErrorHandler");
  
  grabber->disconnectGrabber();
  
  // Log a message to the control program
  
  LogStream logStr;
  logStr.initMessageSimple(true);
  logStr << "Lost control connection to the frame grabber";
  logStr.log();
}
