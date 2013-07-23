#define __FILEPATH__ "mediator/specific/AntennaConsumerNormal.cc"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/Profiler.h"
#include "gcp/util/common/TcpListener.h"
#include "gcp/util/common/NetAntennaDataFrameHandler.h"

#include "gcp/mediator/specific/AntennaConsumerNormal.h"
#include "gcp/mediator/specific/Scanner.h"

using namespace gcp::util;
using namespace gcp::mediator;

// We will time out waiting for a greeting from an antenna after 1
// second

#define CONNECTION_TIMEOUT_SEC 1

AntennaConsumerNormal* AntennaConsumerNormal::consumer_ = 0;

/**.......................................................................
 * Constructor.
 */
AntennaConsumerNormal::AntennaConsumerNormal(Scanner* parent) :
  AntennaConsumer(parent),
  parent_(parent), listener_(0)
{
  // Set the static class pointer pointing to ourselves
  
  consumer_ = this;
  
  // Set up to listen for tcpip connection requests from
  // Antennas. TCP/IP connections will be used to receive data frames
  // from antennas.
  
  AntNum antnum;
  nAntenna_ = antnum.getAntMax();
  
  // Set up to listen for connection requests from the antenna
  // computers.
  
  listener_ = new TcpListener(TRANS_ANT_SCANNER_PORT, nAntenna_);
  
  // Reserve space for nAntenna_ data frame handlers
  
  connectedHandlers_.reserve(nAntenna_);
  
  for(unsigned iant=0; iant < nAntenna_; iant++) {
    
    // Create the vector of data frame handlers
    
    connectedHandlers_.push_back(new NetAntennaDataFrameHandler());
  }
}

/**.......................................................................
 * Destructor.
 */
AntennaConsumerNormal::~AntennaConsumerNormal() 
{
  if(listener_ != 0) {
    delete listener_;
    listener_ = 0;
  }
  
  // Delete any allocated handlers
  
  for(unsigned iant=0; iant < connectedHandlers_.size(); iant++)
    delete connectedHandlers_[iant];
}

/**.......................................................................
 * Service our message queue.
 */
void AntennaConsumerNormal::serviceMsgQ()
{
  bool stop   = false;
  int  nready = 0;
  int  msgqFd = msgq_.fd();
  
  // Initially our select loop will check the msgq file descriptor for
  // readability, and the server socket for connection requests.  Once
  // connections are establihed, the select loop will block until
  // either a message is received on the task message queue, or on a
  // port to an established antenna.
  
  fdSet_.registerReadFd(msgqFd);
  
  // And listen for connection requests from antennas.
  
  listen(true);
  
  while(!stop) {
    
    nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
		  NULL, timeOut_);
    
    // A message on our message queue?
    
    DBPRINT(true, Debug::DEBUG6, "Checking fd (msgFd): " << msgqFd);
    
    if(fdSet_.isSetInRead(msgqFd)) {
      DBPRINT(true, Debug::DEBUG6, "msgq fd is set");
      processTaskMsg(&stop);
    }
    
    // A connection request from an antenna?
    
    DBPRINT(true, Debug::DEBUG6, "Checking fd (lstener): " 
	    << listener_->getFd());
    
    if(fdSet_.isSetInRead(listener_->getFd())) {
      DBPRINT(true, Debug::DEBUG6, "listener fd is set");
      initializeConnection();
    }
    
    // Are we sending a greeting message?
    
    DBPRINT(true, Debug::DEBUG6, "Checking fd: (temp)" 
	    << temporaryHandler_.getSendFd());
    
    if(fdSet_.isSetInWrite(temporaryHandler_.getNetMsgHandler()->getSendFd())) {
      DBPRINT(true, Debug::DEBUG6, "temporary send fd is set");
      temporaryHandler_.sendNetMsg();
    }
    
    // Are we waiting for a response from a newly connected antenna?    
    
    DBPRINT(true, Debug::DEBUG6, "Checking fd: (temp)" 
	    << temporaryHandler_.getReadFd());
    
    if(temporaryHandler_.getNetMsgHandler()->getReadFd() >= 0) {
      
      // Waiting for an intialization message?
      
      DBPRINT(true, Debug::DEBUG6, "Checking fd: (temp)" 
	      << temporaryHandler_.getReadFd());
      
      if(fdSet_.isSetInRead(temporaryHandler_.getNetMsgHandler()->getReadFd())) {
	DBPRINT(true, Debug::DEBUG6, "temporary read fd is set");
	temporaryHandler_.readNetMsg();
      }
      
      // If the descriptor wasn't read, see if we timed out waiting
      // for a response.  If so, terminate the temporary connection
      // and start listening again on the server socket.
      
      else if(timedOut()) {
	terminateConnection(&temporaryHandler_);
	listen(true);
      }
    }
    
    // Check descriptors associated with established connections.
    
    for(unsigned iant=0; iant < nAntenna_; iant++) {
      NetAntennaDataFrameHandler* str = connectedHandlers_[iant];
      
      // Check for partially read data frames from established antennas.
      
      if(fdSet_.isSetInRead(str->getReadFd())) {
	DBPRINT(true, Debug::DEBUG3, "reading data frame");
	str->read();
	DBPRINT(true, Debug::DEBUG3, "Done reading data frame");
      }
    }
  }
}

/**.......................................................................
 * This function is called to accept or reject a connection request
 * from the real-time controller.
 */
void AntennaConsumerNormal::initializeConnection()
{
  int fd = -1;
  
  // Allow the caller to connect.
  
  fd = listener_->acceptConnection();
  
  DBPRINT(true, Debug::DEBUG6, "Initializing connection: fd = " << fd);
  
  // Attach this file descriptor to the temporary connection handler
  
  temporaryHandler_.getNetMsgHandler()->attach(fd);
  
  // Register a handler to be called when a network message is
  // received.  On receipt, we will send a greeting message with the
  // register map revision and byte count.
  
  temporaryHandler_.getNetMsgHandler()->
    installReadHandler(netMsgReadHandler, 0);
  
  // Register a handler to be called if an error occurs. 
  
  temporaryHandler_.getNetMsgHandler()->
    installErrorHandler(netMsgErrorHandler, 0);
  
  // And register the descriptor to be watched for input.  The
  // antenna should send us its ID after the connection is made.
  
  fdSet_.registerReadFd(fd);
  
  // Stop listening for further connections until this antenna has
  // responded.
  
  listen(false);
}

/**.......................................................................
 * This function is called when an ID message has been received on an
 * unestablished antenna connection.
 */
void AntennaConsumerNormal::
terminateConnection(NetCommHandler* str)
{
  terminateConnection(str->getNetMsgHandler());
  terminateConnection(str->getNetCmdHandler());
}

/**.......................................................................
 * This function is called when an ID message has been received on an
 * unestablished antenna connection.
 */
void AntennaConsumerNormal::
terminateConnection(NetHandler* str)
{
  DBPRINT(true, Debug::DEBUG6, "Inside terminateConnection");
  
  shutdownConnection(str->getReadFd());
  shutdownConnection(str->getSendFd());
  str->attach(-1);
}

/**.......................................................................
 * Call this function to start or stop listening on our server socket
 */
void AntennaConsumerNormal::
listen(bool listenVar)
{
  // If listening, register the server socket to be watched for
  // connection requests, and set the timeout to NULL.
  
  if(listenVar) {
    fdSet_.registerReadFd(listener_->getFd());
    DBPRINT(true, Debug::DEBUG3, "Setting temp handler fd to -1");
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
 * Check if we timed out waiting for a response from a newly connected
 * antenna
 */
bool AntennaConsumerNormal::timedOut()
{
  TimeVal currentTime, diff;
  
  currentTime.setToCurrentTime();
  
  diff = currentTime - startTime_;
  
  return (diff.getTimeInSeconds() >= CONNECTION_TIMEOUT_SEC);
}

/**.......................................................................
 * Act upon receipt of a network message from an antenna with which we
 * are in the process of establishing a connection.
 */
NET_READ_HANDLER(AntennaConsumerNormal::netMsgReadHandler)
{
  NetMsg* netMsg = consumer_->temporaryHandler_.getLastReadNetMsg();
  
  switch(netMsg->type) {
    
    // If the antenna sent us an ID, respond by sending a greeting message
    
  case NetMsg::ID:
    consumer_->finalizeConnection();
    break;
    
    // We shouldn't receive any other messages from this antenna until
    // we have responded, so receipt of any other message is an error.
    
  default:    
    consumer_->terminateConnection(&consumer_->temporaryHandler_);
    break;
  }
}

/**.......................................................................
 * A handler to be called when an error occurs reading or sending a
 * message to an antenna.
 */
NET_ERROR_HANDLER(AntennaConsumerNormal::netMsgErrorHandler)
{
  NetCommHandler* handler = &consumer_->temporaryHandler_;
  NetMsg* lastMsg = handler->getNetMsgHandler()->getLastSentNetMsg();
  
  DBPRINT(true, Debug::DEBUG6, "");
  
  // Terminate the connection to this antenna
  
  consumer_->terminateConnection(handler);
  
  // Log a message to the control program
  
  ReportSimpleError("Error reading or sending: Lost scanner connection to Antenna" << lastMsg->body.antenna);
}

/**.......................................................................
 * A handler to be called when a frame is completely read from an antenna
 */
NET_READ_HANDLER(AntennaConsumerNormal::netAntennaDataFrameReadHandler)
{
  NetAntennaDataFrameHandler* str = (NetAntennaDataFrameHandler*) arg;
  consumer_->parent_->packAntennaFrame(str->getFrame());
}

/**.......................................................................
 * A handler to be called when an error occurs reading a data frame
 * from an antenna.
 */
NET_ERROR_HANDLER(AntennaConsumerNormal::netAntennaDataFrameErrorHandler)
{
  NetAntennaDataFrameHandler* handler = (NetAntennaDataFrameHandler*)arg;
  
  DBPRINT(true, Debug::DEBUG6, "");
  
  // Terminate the connection to this antenna
  
  consumer_->terminateConnection(handler);
  
  // Log a message to the control program
  
  ReportSimpleError("Error reading a data frame: Lost scanner connection to Antenna" << handler->getAnt());
}

/**.......................................................................
 * This function is called when an ID message has been received on an
 * unestablished antenna connection.
 */
void AntennaConsumerNormal::finalizeConnection()
{
  NetMsg* netMsg = temporaryHandler_.getLastReadNetMsg();
  unsigned int antennaId = netMsg->body.antenna;
  AntNum antNum(antennaId);
  int fd = temporaryHandler_.getFd();
  
  // Only proceed if the response was valid, otherwise terminate the
  // temporary connection
  
  if(antNum.isValidSingleAnt()) {
    
    DBPRINT(true, Debug::DEBUG3, "Finalizing connection to antenna " 
	    << antennaId);
    
    // Install a handler to be called when the greeting message has
    // been completely sent
    
    temporaryHandler_.getNetMsgHandler()->
      installSendHandler(netMsgSentHandler, 0);
    
    // Send a greeting message to this antenna.  Note this is the last
    // message we will ever send to it.  From now on, there will only
    // be commands sent, and messages received.
    
    sendGreetingMsg();
    
  } else // Terminate the temporary connection
    terminateConnection(&temporaryHandler_);
}

/**.......................................................................
 * A handler called after a greeting message has been sent to an antenna
 */
NET_SEND_HANDLER(AntennaConsumerNormal::netMsgSentHandler)
{
  DBPRINT(true, Debug::DEBUG6, "Calling netMsgSentHandler");
  
  NetMsg* netMsg = 
    consumer_->temporaryHandler_.getNetMsgHandler()->getLastSentNetMsg();
  AntNum antNum(netMsg->body.antenna);
  
  // Get the handler we will use for communicating with this antenna
  
  NetAntennaDataFrameHandler* handler = 
    consumer_->connectedHandlers_[antNum.getIntId()];
  
  // Terminate any old connection we already have to this antenna
  
  consumer_->terminateConnection(handler);
  
  // Install the new descriptor as the connection to this antenna.
  
  handler->attachReadStream(consumer_->temporaryHandler_.getFd());
  
  // Install a handler to be called when a data frame has ben
  // completely read
  
  handler->installReadHandler(netAntennaDataFrameReadHandler, handler);
  
  // Install a handler to be called if an error occurs reading a data
  // frame
  
  handler->installReadErrorHandler(netAntennaDataFrameErrorHandler, handler);
  
  // Set the antenna number associated with this handler
  
  handler->setAnt(antNum.getId());
  
  // Clear the write fd and detach the temporary handler
  
  consumer_->
    fdSet_.clearFromWriteFdSet(consumer_->temporaryHandler_.getSendFd());
  
  consumer_->temporaryHandler_.attachSendStream(-1);
  
  // Start listening again on the server socket.
  
  consumer_->listen(true);
  
  // Log a message that we have accepted a connection
  
  ReportMessage("Scanner connection established to Antenna" << handler->getAnt());
}

/**.......................................................................
 * Send a greeting message to an antenna
 */
void AntennaConsumerNormal::sendGreetingMsg()
{
  NetMsg* netMsg = temporaryHandler_.getLastReadNetMsg();
  unsigned int antennaId = netMsg->body.antenna;
  
  DBPRINT(true, Debug::DEBUG3, "Sending Greeting message to antenna"
	  << antennaId);
  
  temporaryHandler_.getNetMsgHandler()->packGreetingMsg(antennaId);
  fdSet_.registerWriteFd(temporaryHandler_.getFd());
}
