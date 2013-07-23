#include "gcp/receiver/specific/DioClient.h"
#include "gcp/receiver/specific/DioConsumer.h"
#include "gcp/mediator/specific/Scanner.h"

#include <sstream>
#include <fstream>
#include <string>
#include <popt.h>

using namespace gcp::receiver;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
DioConsumer::
DioConsumer(gcp::mediator::Scanner* parent, 
		  std::string dioHost, unsigned short dioPort,
		  std::string hwHost,  unsigned short hwPort) :
  parent_(parent), 
  dioHost_(dioHost), dioPort_(dioPort), 
  hwHost_(hwHost),   hwPort_(hwPort)
{
  clearTimeout();

  // DIO client must be instantiated by inheritors
  
  dioClient_ = 0;
}

/**.......................................................................
 * Destructor.
 */
DioConsumer::~DioConsumer() 
{
  if(dioClient_) {
    delete dioClient_;
    dioClient_ = 0;
  }
}

/**.......................................................................
 * Service our message queue.
 */
void DioConsumer::serviceMsgQ()
{
  bool stop  = false;
  int nready = 0;
  int msgqFd = msgq_.fd();

  // Check that the dioClient has been allocated

  if(dioClient_ == 0)
    ThrowError("DIO Client is NULL");

  // Register the message queue to be watched for readability

  fdSet_.registerReadFd(msgqFd);

  while(!stop) {

    int nready = select(fdSet_.size(), fdSet_.readFdSet(), NULL, NULL, timeOut_);

    // A message on our message queue?

    if(fdSet_.isSetInRead(msgqFd)) {
      processTaskMsg(&stop);
    }

    checkForDioData();

  }
}

/**.......................................................................
 * Check for from the DIO Client
 */
void DioConsumer::checkForDioData() 
{
  // If the dioClient is not connected, try to connect.  On success,
  // we will disable the timeout, since the dioClient class does its
  // own blocking reads.
  //
  // If we fail to connect, we will reset the timeout, and print an
  // message.


  if(!dioClient_->isConnected()) {

    if(dioClient_->connect()) {
      clearTimeout();
    } else {
      COUT("Connect failed: resetting timeout");
      resetTimeout();
    }

  }

  // If we are connected, get the next sample from the DIO server.
  // When buffered data are ready, pack the buffered data into the
  // frame
  
  if(dioClient_->isConnected()) {

    if(dioClient_->bufferNextSample()) {

      if(dioClient_->dataAreReady()) {
	packFrame();
      }

    } else {
      COUT("bufferNextSample failed - resetting timeout");
      resetTimeout();
    }

  }
}

/**.......................................................................
 * Set the timeout until the next connection attempt
 */
void DioConsumer::resetTimeout()
{
  // Set up a 10-second timer
    
  timer_.setTime(10, 0);
  timeOut_ = timer_.timeVal();
}

/**.......................................................................
 * Set the timeout until the next connection attempt
 */
void DioConsumer::clearTimeout()
{
  // Set a 0-second timeout to timeout immediately
    
  timer_.setTime(0, 0);
  timeOut_ = timer_.timeVal();
}

/**.......................................................................
 * Return the currently-readable frame
 */
RegMapDataFrameManager* DioConsumer::grabReadFrame()
{
  return dioClient_->grabReadFrame(); 
}

/**.......................................................................
 * Release the read frame
 */
void DioConsumer::releaseReadFrame()
{
  return dioClient_->releaseReadFrame(); 
}

/**.......................................................................
 * Copy stored values of persistent items into the register
 * frame
 */
void DioConsumer::
copyPersistentRegs(gcp::util::ArrayMapDataFrameManager* frame)
{
  dioClient_->copyPersistentRegs(frame);
}
