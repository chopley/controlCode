#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/SignalTask.h"
#include "gcp/util/common/SerialClient.h"
#include "gcp/util/common/String.h"
#include "gcp/util/common/StringUtils.h"
#include "gcp/util/common/TcpClient.h"
#include "gcp/util/common/TcpListener.h"
#include "gcp/util/common/Server.h"
#include "gcp/util/common/Vector.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor for a serial connection
 */
Server::Server(bool spawnThread, int listenPort, unsigned readBufSize, 
	       unsigned sendBufSize) : 
  Runnable(spawnThread, runFn)
{
  initMembers(listenPort, readBufSize, sendBufSize);
}

/**.......................................................................
 * Initialize members
 */
void Server::initMembers(int listenPort, unsigned readBufSize, unsigned sendBufSize)
{
  listener_    =  0;
  signalTask_  =  0;
  stop_        =  false;
  readBufSize_ = readBufSize;
  sendBufSize_ = sendBufSize;

  // We will set the timeout to 1 second for our select() loop

  setTimeOutSeconds(1);

  // Finally, spawn a thread for managing signal handling

  signalTask_ = new SignalTask(true);

  // And install a signal handler for SIGINT

  signalTask_->sendInstallSignalMsg(SIGINT, &shutDown, this);

  // Start listening on the requested port

  listen(listenPort);
}

/**.......................................................................
 * Listen for connection requests on the specified port
 */
void Server::listen(unsigned port, unsigned nClients) 
{
  // Listen on the requested socket.
  
  if(nClients > 0) {
    listener_ = new TcpListener(port, nClients);
    fdSet_.registerReadFd(listener_->getFd());
  }
}

/**.......................................................................
 * Destructor.
 */
Server::~Server() 
{
  // Shut down the signal task

  if(signalTask_ != 0) {
    delete signalTask_;
    signalTask_ = 0;
  }

  // Free any memory allocated in this class

  for(std::list<ServerConnection*>::iterator iclient=clients_.begin();
      iclient != clients_.end(); iclient++) {
    delete *iclient;
  }

  // Free the listener

  if(listener_ != 0) {
    delete listener_;
    listener_ = 0;
  }

}

/**.......................................................................
 * Block in select
 */
void Server::run()
{
  int nready=0;

  // On entry to the loop, timeout immediately

  setTimeOutSeconds(0);

  do {

    // Block in select() until one or more file descriptors are readable
  
    if((nready=select(fdSet_.size(), fdSet_.readFdSet(), 
		      fdSet_.writeFdSet(), NULL, 
		      timeOut_.tVal())) < 0) {
      ThrowSysError("select()");
    }

    if(nready > 0) {

      // Service requests received over the socket connection
      
      if(listener_ != 0 && fdSet_.isSetInRead(listener_->getFd())) {
	acceptConnection();
      }
      
      // Check connected clients for data
      
      checkClientsForReadableData();

      // Check connected clients for sendable data
      
      checkClientsForWritability();

      // Check other fds

      checkOtherFds();

    } else {
      
      // Do whatever it is we are supposed to do on timeout

      timeOutAction();

      // And reset the timeout

      timeOut_.reset();
    }

  } while(!stop_);
}

/**.......................................................................
 * Respond to a connection request from a client
 */
void Server::acceptConnection()
{
  int fd = -1;
  
  // Allow the caller to connect.  The fd returned will be configured
  // for blocking I/O.
  
  fd = listener_->acceptConnection(true);
  
  // Insert a new connection into the list

  ServerConnection* conn = 
    new ServerConnection(fd, readBufSize_, sendBufSize_, this);

  clients_.insert(clients_.begin(), conn);
  
  // And register the descriptor to be watched for input
  
  fdSet_.registerReadFd(fd);
  
  // Do anything inheritors define when a client connects

  acceptClientAction();
}

/**.......................................................................
 * Check clients for writability
 */
void Server::checkClientsForWritability()
{
  for(std::list<ServerConnection*>::iterator iClient=clients_.begin();
      iClient != clients_.end(); iClient++) 
  {
    ServerConnection* client = *iClient;

    if(fdSet_.isSetInWrite(client->handler_.getSendFd()))
      client->handler_.send();
  }
}

/**.......................................................................
 * Check clients for data to be read
 */
void Server::checkClientsForReadableData() 
{
  std::vector<ServerConnection*> disconnectedClients_;

  for(std::list<ServerConnection*>::iterator iClient=clients_.begin();
      iClient != clients_.end(); iClient++) 
  {
    ServerConnection* client = *iClient;

    if(fdSet_.isSetInRead(client->handler_.getReadFd())) {

      client->handler_.read();

      // If after processing messages from this client, the client is
      // disconneted, mark it for removal

      if(client->handler_.getReadFd() < 0)
	disconnectedClients_.push_back(client);
    }
  }

  // Finally, remove any clients that were disconnected after reading

  for(std::vector<ServerConnection*>::iterator 
	iClient=disconnectedClients_.begin();
      iClient != disconnectedClients_.end(); iClient++) 
  {
    ServerConnection* client = *iClient;
    clients_.remove(client);
    delete client;
  }
}

/**.......................................................................
 * Method to send data to all connected clients
 */
void Server::sendClientData(NetDat& dat)
{
  for(std::list<ServerConnection*>::iterator iClient=clients_.begin();
      iClient != clients_.end(); iClient++) 
  {
    ServerConnection* client = *iClient;

    std::vector<unsigned char>data = dat.getSerializedData();

    client->handler_.getSendStr()->startPut(data.size());
    client->handler_.getSendStr()->putChar(data.size(), &data[0]);
    client->handler_.getSendStr()->endPut();

    // And register this client fd to be watched for writability

    fdSet_.registerWriteFd(client->handler_.getSendFd());
  }
}

/**.......................................................................
 * A shutdown method
 */
SIGNALTASK_HANDLER_FN(Server::shutDown)
{
  exit(1);
}

/**.......................................................................
 * A blocking run method
 */
RUN_FN(Server::runFn)
{
  Server* runnable = (Server*) arg;
  runnable->run();
}

/**.......................................................................
 * Static method to be called when a message is fully read from a client
 */
NET_ERROR_HANDLER(Server::errHandler)
{
  ServerConnection* conn = (ServerConnection*)arg;

  conn->parent_->fdSet_.clearFromReadFdSet(conn->handler_.getReadFd());
  conn->parent_->fdSet_.clearFromWriteFdSet(conn->handler_.getSendFd());
  conn->handler_.attach(-1);
}

/**.......................................................................
 * Static method to be called when a message is fully read from a client
 */
NET_READ_HANDLER(Server::readHandler)
{
  ServerConnection* conn = (ServerConnection*)arg;
  conn->parent_->readClientData(conn->handler_);
}

/**.......................................................................
 * Static method to be called when a message is fully sent to a client
 */
NET_SEND_HANDLER(Server::sendHandler)
{
  ServerConnection* conn = (ServerConnection*)arg;
  conn->parent_->fdSet_.clearFromWriteFdSet(conn->handler_.getFd());
}

/**.......................................................................
 * Constructor for ServerConnection class
 */
Server::ServerConnection::
ServerConnection(int fd, unsigned readBufSize, 
		 unsigned sendBufSize, Server* parent)
{
  handler_.attach(fd);
  handler_.setReadBuffer(0, readBufSize);
  handler_.setSendBuffer(0, sendBufSize);
  parent_ = parent;

  handler_.installReadHandler(Server::readHandler, (void*)this);
  handler_.installSendHandler(Server::sendHandler, (void*)this);

  handler_.installReadErrorHandler(Server::errHandler, (void*)this);
  handler_.installSendErrorHandler(Server::errHandler, (void*)this);
}

/**.......................................................................
 * Destructor for ServerConnection class
 */
Server::ServerConnection::~ServerConnection()
{
  if(handler_.getFd() > 0) {
    close(handler_.getFd());
    
    handler_.attach(-1);

    ::close(handler_.getFd());
  }
}

void Server::setReadBufSize(unsigned size)
{
  readBufSize_ = size;
}

void Server::setSendBufSize(unsigned size)
{
  sendBufSize_ = size;
}

void Server::setTimeOutSeconds(unsigned int seconds)
{
  timeOut_.setIntervalInSeconds(seconds);
  timeOut_.activate(true);
}
