#include "gcp/receiver/specific/XMLConsumer.h"

#include "gcp/mediator/specific/Scanner.h"

#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/Debug.h"

#include "Utilities/MuxXMLDatacard.h"

#include <sstream>
#include <fstream>
#include <string>
#include <popt.h>

using namespace gcp::receiver;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
XMLConsumer::XMLConsumer(XMLDataFrameManager* dataFrame, 
			 gcp::mediator::Scanner* parent,
			 std::string regMapName, 
			 std::string host, 
			 unsigned short port, 
			 bool send_connect_request) :
  dataFrame_(dataFrame), parent_(parent), regMapName_(regMapName), 
  host_(host), port_(port), send_connect_request_(send_connect_request), 
  client_(0), nConnAttempt_(0)
{
  connected_      =  false;
  clientFd_       = -1;
  commandPending_ =  false;

  try {
    client_ = new MuxReadout::HardwareManagerClient(host_, port_, true);
  } catch(...) {
  }

  if(!client_)    
    ThrowError("Unable to allocate new client");
}

/**.......................................................................
 * Destructor.
 */
XMLConsumer::~XMLConsumer() 
{
  if(client_) {
    delete client_;
    client_ = 0;
  }
};

void XMLConsumer::sendDispatchDataFrameMsg()
{
  gcp::mediator::ScannerMsg msg;
  msg.packDispatchDataFrameMsg();
  sendTaskMsg(&msg);
}

/**.......................................................................
 * Check that we have a valid client connection
 */
bool XMLConsumer::checkClient() 
{
  // Check if we are connected, or if not, if a connection attempt
  // succeeds

  if(client_->isConnected() || client_->connect(false)) {

    // A re-zero the counter of failed connection attempts
	  
    nConnAttempt_ = 0;
    
    // If we weren't connected before, register that we now are
    
    if(!connected_) {
      reportSuccess();
      connected_ = true;

      // And register the client fd to be watched for input

      clientFd_ = client_->getFd();

      fdSet_.registerReadFd(clientFd_);
    }

    return true;
  }

  return false;
}

/**.......................................................................
 * Process a message specific to this task.
 */
void XMLConsumer::processMsg(gcp::mediator::ScannerMsg* msg) 
{
  switch (msg->type) {

    // We only expect one message -- to query our server for data

  case gcp::mediator::ScannerMsg::DISPATCH_DATAFRAME:

    // Catch all errors here

    try {

      if(checkClient()) {

	if(!getData()) {
	  registerError();
	}

      } else {
	registerError();
      }

    } catch(Exception& err) {
      registerError();
    } catch(...) {
      registerError();
    }
    
    break;

  default:
    ThrowError("Unknown message type: " << msg->type);
    break;
  }
}

/**.......................................................................
 * Register an error communicating with the server
 */
void XMLConsumer::registerError()
{
  // If we were previously connected, report that we are now
  // disconnected

  if(connected_) {
    ReportSimpleError("Scanner connection to the " << regMapName_ 
		      << " server at: " << host_ << ":" << port_
		      << std::endl << " was dropped");
    
    connected_ = false;
  } 

  // Remove the client fd from the list to be watched

  fdSet_.clearFromReadFdSet(clientFd_);

  // Reset the client on error
  
  client_->registerError();
  
  clientFd_ = -1;

  // And cancel any pending command

  commandPending_ = false;

  // See if it is time to notify someone that we can't connect
  
  if(nConnAttempt_ == 0) 
    reportError();
  
  ++nConnAttempt_ %= nConnAttemptBeforeReport_;
}

/**.......................................................................
 * Get the next set of data from the server
 */
bool XMLConsumer::getData()
{
  return false;
}

/**.......................................................................
 * Report an error
 */
void XMLConsumer::reportError()
{
  ReportSimpleError("Unable to get scanner connection to " 
		    << regMapName_ << " server at: "
		    << host_ << ":" << port_);
}

/**.......................................................................
 * Report a successfull connection
 */
void XMLConsumer::reportSuccess()
{
  ReportMessage("Successfully connected to the " 
		<< regMapName_ << " server at: " 
		<< host_ << ":" << port_);
}


