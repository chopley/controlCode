#include "gcp/util/common/CoProc.h"
#include "gcp/util/common/SshClient.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/TimeVal.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
SshClient::SshClient(bool spawnThread, 
		     std::string gateway, std::string host, unsigned port, 
		     unsigned readBufSize, unsigned sendBufSize) :
  Runnable(spawnThread, runFn), tunnel_(gateway, host, port)
{
  initMembers(gateway, host, port, readBufSize, sendBufSize);

  spawn(this);
}

void SshClient::
initMembers(std::string gateway, std::string host, 
	    unsigned port, unsigned readBufSize, unsigned sendBufSize)
{
  gateway_ = gateway;
  host_    = host;
  port_    = port;

  tcp_.setHost("localhost");
  tcp_.setPort(port);

  handler_.installReadHandler(readHandler, (void*)this);
  handler_.installSendHandler(sendHandler, (void*)this);

  handler_.installReadErrorHandler(errHandler, (void*)this);
  handler_.installSendErrorHandler(errHandler, (void*)this);

  stop_ = false;

  timeOut_.setSeconds(1);
  timeOutPtr_ = timeOut_.timeVal();
}

void SshClient::disconnect()
{
  tcp_.disconnect();

  fdSet_.clearFromReadFdSet(handler_.getReadFd());
  fdSet_.clearFromWriteFdSet(handler_.getSendFd());

  handler_.attach(-1);

  // And set a timer to reconnect!
  
  timeOutPtr_ = timeOut_.timeVal();
}

void SshClient::connect()
{
  if(tcp_.connectToServer(true) > 0) {
    handler_.attach(tcp_.getFd());
    fdSet_.registerReadFd(handler_.getReadFd());
    timeOutPtr_ = NULL;
  } else {
    reportError();
    timeOut_.reset();
  }
}

/**.......................................................................
 * Destructor.
 */
SshClient::~SshClient() 
{
  disconnect();
}

/**.......................................................................
 * Send data to the server
 */
void SshClient::sendServerData(NetDat& dat)
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
NET_READ_HANDLER(SshClient::readHandler)
{
  SshClient* client = (SshClient*)arg;
  client->readServerData(client->handler_);
}

/**.......................................................................
 * Static method to be called when a message is fully sent to the server
 */
NET_SEND_HANDLER(SshClient::sendHandler)
{
  SshClient* client = (SshClient*)arg;
  client->fdSet_.clearFromWriteFdSet(client->handler_.getSendFd());
}

/**.......................................................................
 * Static method to be called when an error occurs reading or sending
 * data
 */
NET_ERROR_HANDLER(SshClient::errHandler)
{
  SshClient* client = (SshClient*)arg;
  client->disconnect();
}

/**.......................................................................
 * Block in select
 */
void SshClient::run()
{
  // Now enter our select loop

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
RUN_FN(SshClient::runFn)
{
  SshClient* runnable = (SshClient*) arg;
  runnable->run();
}

void SshClient::setReadBufSize(unsigned size)
{
  handler_.setReadBuffer(0, size);
}

void SshClient::setSendBufSize(unsigned size)
{
  handler_.setSendBuffer(0, size);
}
