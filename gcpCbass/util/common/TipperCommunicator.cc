#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/IoLock.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeOut.h"
#include "gcp/util/common/TipperCommunicator.h"

#include <fstream>

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Initialize pertinent members of this class to sensible defaults
 */
void TipperCommunicator::initialize(std::string host)
{
  client_    = 0;
  ftpClient_ = 0;
  fdSetPtr_  = 0;

  // Connect to the FTP port on smmtipper

  client_ = new TcpClient(host, 21);

  // This is necessary or else the returned string may contain
  // characters like '\r' which will cause subsequent string
  // comparisons by the likes of strstr(), etc., to fail.

  client_->stripUnprintable(true);
  client_->dontStrip("\n\t");

  // And will append CR to everything we send to the strip's port.

  client_->append("\n");
}

/**.......................................................................
 * Constructor.
 */
TipperCommunicator::TipperCommunicator(FdSet* fdSet, std::string host) :
  SpawnableTask<TipperCommunicatorMsg>(false)
{
  initialize(host);
  fdSetPtr_ = fdSet;
}

/**.......................................................................
 * Constructor.
 */
TipperCommunicator::TipperCommunicator(std::string host, unsigned timeOutIntervalInSeconds) :
  SpawnableTask<TipperCommunicatorMsg>(true)
{
  initialize(host);
  fdSetPtr_ = &fdSet_;

  if(timeOutIntervalInSeconds > 0) {
    timeOut_.setIntervalInSeconds(timeOutIntervalInSeconds);
    timeOut_.activate(true);
  } else {
    timeOut_.activate(false);
  }

}

/**.......................................................................
 * Destructor.
 */
TipperCommunicator::~TipperCommunicator() 
{
  if(client_)
    delete client_;

  if(ftpClient_)
    delete ftpClient_;
}

/**.......................................................................
 * Initiate sending commands to the server.
 */
void TipperCommunicator::
initiateGetTipperLogCommSequence()
{
  // Compile the command/response stack associated with this command

  compileGetTipperLogStateMachine();
  
  if(client_->connectToServer(true) < 0) {
    ThrowError("Unable to initiate connection to the ftp server");
  }

  // And register the fd to be watched for readability

  fdSetPtr_->registerReadFd(client_->getFd());
}

/**.......................................................................,
 * React to a failure to reply
 */
void TipperCommunicator::registerTimeOut()
{
  // Register our failure to communicate

  LogMessage(true, "Timed out waiting for a response from the horizons server");
  terminateCommSequence(true);
}

/**.......................................................................
 * Terminate a command sequence to the server.
 */
void TipperCommunicator::terminateCommSequence(bool error)
{
  Communicator::terminateCommSequence(error);
}

/**.......................................................................
 * Compile the state machine we will use to get the tipper file
 */
void TipperCommunicator::
compileGetTipperLogStateMachine()
{
  // Clear the stack of sent commands and expected responses

  sentStrings_.clear();
  rcvdStrings_.clear();

  rcvdStrings_.push_back(RcvdStr("220 Microsoft FTP Service", sendNextString, this));
  sentStrings_.push_back("user anonymous");

  rcvdStrings_.push_back(RcvdStr("password", sendNextString, this));
  sentStrings_.push_back("pass sptdaq@sptnet");

  rcvdStrings_.push_back(RcvdStr("logged in", sendNextString, this));
  sentStrings_.push_back("pasv");

  rcvdStrings_.push_back(RcvdStr("Entering Passive Mode", parsePortNumber, this));
  sentStrings_.push_back("retr SmmTip.log");

  rcvdStrings_.push_back(RcvdStr("Transfer complete", quitFromServer, this));

  // And set the iterators pointing to the head of the stack
  
  sentStringIter_ = sentStrings_.begin();
  rcvdStringIter_ = rcvdStrings_.begin();
}

/**.......................................................................
 * Static parser method to respond to disconnect from the FTP server
 */
COMM_PARSER_FN(TipperCommunicator::quitFromServer)
{
  TipperCommunicator* comm = (TipperCommunicator*) arg;
  comm->quitFromServer();
}

/**.......................................................................
 * Method to disconnect from the FTP server
 */
void TipperCommunicator::quitFromServer()
{
  std::string str("quit");
  fdSetPtr_->clearFromReadFdSet(client_->getFd());
  client_->writeString(str);
  client_->disconnect();
}

/**.......................................................................
 * Static parser method to respond to a port number message from the
 * FTP server
 */
COMM_PARSER_FN(TipperCommunicator::parsePortNumber)
{
  TipperCommunicator* comm = (TipperCommunicator*) arg;
  comm->parsePortNumber();
}

/**.......................................................................
 * Parse a data port number received from the FTP server
 */
void TipperCommunicator::parsePortNumber()
{
  String str(os_.str());

  str.findNextInstanceOf("(", ",");
  str.findNextInstanceOf(",", ",");
  str.findNextInstanceOf(",", ",");
  str.findNextInstanceOf(",", ",");

  unsigned p1 = str.findNextInstanceOf(",", ",").toInt();
  unsigned p2 = str.findNextInstanceOf(",", ")").toInt();

  // Now connect to the smm tipper host on the port it specified

  ftpClient_ = new TcpClient("smmtipper", p1*256+p2);

  if(ftpClient_->connectToServer(true) < 0) {
    ThrowError("Unable to initiate connection to the ftp server");
  }

  // And register it to be watched for readability

  fdSetPtr_->registerReadFd(ftpClient_->getFd());

  // Finally, send the next string to the server.  This will be the
  // command to send the data

  sendNextString();
}

/**.......................................................................
 * Process a tipper log received from a remote server
 */
void TipperCommunicator::processTipperLog()
{
  os_.str("");

  // Get the file from the remote FTP server

  ftpClient_->concatenateString(os_);

  // Now clear from the read fd set

  fdSetPtr_->clearFromReadFdSet(ftpClient_->getFd());

  delete ftpClient_;
  ftpClient_ = 0;

  // Save the submm tipper log to a file

  std::ofstream fout("SmmTip.log", ios::out);
  fout << os_.str();
  fout.close();
}

/**.......................................................................
 * Main Task event loop: when this is called, the task blocks forever
 * in select(), or until a stop message is received.
 */
void TipperCommunicator::serviceMsgQ()
{
  try {
  bool stop=false;
  int nready; // number of file descriptors ready for reading

  if(msgq_.fd() < 0) {
    ThrowError("Received NULL file descriptor");
  }

  while(!stop) {

    nready = select(fdSet_.size(), fdSet_.readFdSet(), 
		    NULL, NULL, timeOut_.tVal());

    switch (nready) {
      // If no file descriptors were ready, it is time to contact the
      // tipper ftp server and retrieve the tipper log

    case 0:
      initiateGetTipperLogCommSequence();
      timeOut_.reset();
      break;

    case -1:
      stop = true;
      ThrowSysError("select()");
      break;

    default:

      // If a message is waiting to be read, process it now

      if(fdSet_.isSetInRead(msgq_.fd())) {
	processTaskMsg(&stop);
      }
	 
      // If this communicator's read fd was set, read the next line
      // from the FTP server
	 
      if(fdSet_.isSetInRead(getFtpCommFd())) {
	processClientMessage();
      }
      
      // If data are ready to be ready on the FTP data connection,
      // process the log file now

      if(getFtpDataFd() > 0 && fdSet_.isSetInRead(getFtpDataFd())) {
	processTipperLog();
      }

      break;
    }
  }
  } catch(Exception& err) {
    COUT("Caught an exception: " << err.what());
  }

  COUT("Exiting tipper serviceMsgQ");
}

/**.......................................................................
 * Return a reference to the fd corresonding to the comms link with
 * the FTP server
 */
int TipperCommunicator::getFtpCommFd()
{
  return getFd();
}

/**.......................................................................
 * Return a reference to the fd corresonding to the data link with
 * the FTP server
 */
int TipperCommunicator::getFtpDataFd()
{
  if(ftpClient_) {
    return ftpClient_->getFd();
  } else {
    return -1;
  }
}

/**.......................................................................
 * Tell this object to retrieve the tipper log
 */
void TipperCommunicator::getTipperLog()
{
  TipperCommunicatorMsg msg;
  msg.genericMsgType_ = GenericTaskMsg::TASK_SPECIFIC;
  msg.type = TipperCommunicatorMsg::GET_FILE;
  sendTaskMsg(&msg);
}

void TipperCommunicator::processMsg(TipperCommunicatorMsg* msg)
{
  switch (msg->type) {
  case TipperCommunicatorMsg::GET_FILE:
    initiateGetTipperLogCommSequence();
    break;
  default:
    ThrowError("Unrecognized message type: " << msg->type);
    break;
  }
}
