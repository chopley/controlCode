#include "gcp/util/common/Client.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/TimeVal.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Client::Client(bool spawnThread, std::string host, unsigned port, 
	       unsigned readBufSize, unsigned sendBufSize) :
  Runnable(spawnThread, runFn)
{
  initMembers(host, port, readBufSize, sendBufSize);
  spawn(this);
}

void Client::initMembers(std::string host, unsigned port, 
			 unsigned readBufSize, unsigned sendBufSize)
{
  tcp_.setHost(host);
  tcp_.setPort(port);

  handler_.installReadHandler(readHandler, (void*)this);
  handler_.installSendHandler(sendHandler, (void*)this);

  handler_.installReadErrorHandler(errHandler, (void*)this);
  handler_.installSendErrorHandler(errHandler, (void*)this);

  stop_ = false;

  timeOut_.setSeconds(0);

  timeOutPtr_ = timeOut_.timeVal();
}

void Client::disconnect()
{
  tcp_.disconnect();

  fdSet_.clearFromReadFdSet(handler_.getReadFd());
  fdSet_.clearFromWriteFdSet(handler_.getSendFd());

  handler_.attach(-1);

  // And set a timer to reconnect!
  
  timeOutPtr_ = timeOut_.timeVal();
}

void Client::connect()
{
  if(tcp_.connectToServer(true) > 0) {
    handler_.attach(tcp_.getFd());
    fdSet_.registerReadFd(handler_.getReadFd());
    timeOutPtr_ = NULL;
    COUT("Client connection succeeded");
  } else {
    COUT("Client connection to " << tcp_.getHost() << ":" << tcp_.getPort() << " failed");
    timeOut_.reset();
  }
}

/**.......................................................................
 * Destructor.
 */
Client::~Client() 
{
  disconnect();
}

/**.......................................................................
 * Send data to the server
 */
void Client::sendServerData(NetDat& dat)
{
  std::vector<unsigned char>data = dat.getSerializedData();

  handler_.getSendStr()->startPut(data.size());
  handler_.getSendStr()->putChar(data.size(), &data[0]);
  handler_.getSendStr()->endPut();
  
  // And register this client fd to be watched for writability

  fdSet_.registerWriteFd(handler_.getSendFd());
}

/**.......................................................................
 * Static method to be called when a message is fully read from the server
 */
NET_READ_HANDLER(Client::readHandler)
{
  Client* client = (Client*)arg;
  client->readServerData(client->handler_);
}

/**.......................................................................
 * Static method to be called when a message is fully sent to the server
 */
NET_SEND_HANDLER(Client::sendHandler)
{
  Client* client = (Client*)arg;
  client->fdSet_.clearFromWriteFdSet(client->handler_.getSendFd());
}

/**.......................................................................
 * Static method to be called when an error occurs reading or sending
 * data
 */
NET_ERROR_HANDLER(Client::errHandler)
{
  Client* client = (Client*)arg;
  client->disconnect();
}

/**.......................................................................
 * Block in select
 */
void Client::run()
{
  int nready=0;

  do {

    // Block in select() until one or more file descriptors are readable

    if((nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
		      NULL, timeOutPtr_)) < 0) {
      ThrowSysError("select()");
    }

    if(nready > 0) {

      // read data received over the socket connection
      
      if(fdSet_.isSetInRead(handler_.getReadFd()))
	handler_.read();

      // send data over the socket connection
      
      if(fdSet_.isSetInWrite(handler_.getSendFd()))
	handler_.send();
   
    } else {
      connect();
    }

  } while(!stop_);
}

/**.......................................................................
 * A blocking run method
 */
RUN_FN(Client::runFn)
{
  Client* runnable = (Client*) arg;
  runnable->run();
}

void Client::setReadBufSize(unsigned size)
{
  handler_.setReadBuffer(0, size);
}

void Client::setSendBufSize(unsigned size)
{
  handler_.setSendBuffer(0, size);
}
