#include "gcp/receiver/specific/ReceiverConfigConsumer.h"
#include "gcp/receiver/specific/FpgaBoardManager.h"
#include "gcp/receiver/specific/SquidBoardManager.h"

#include "gcp/mediator/specific/Scanner.h"

#include "Utilities/HardwareManager_CommandFactory.h"

#include<iostream>

#include <iostream>
#include <sstream>

using namespace std;
using namespace gcp::receiver;
using namespace MuxReadout;

/**.......................................................................
 * Constructor.
 */
ReceiverConfigConsumer::
ReceiverConfigConsumer(gcp::mediator::Scanner* scanner, 
		       std::string host,
		       unsigned short port,
		       bool send_connect_request) :
  XMLConsumer(0, scanner, "receiverConfig", host, port, 
	      send_connect_request)
{
  haveBoardInfo_   =  false;

  seqId_           = -1;

  getBoardsCmd_    =  0;
  getSquidsCmd_    =  0;
  getSeqIdCmd_     =  0;
  getRegsCmd_      =  0;

  fpgaBm_          = 0;
  squidBm_         = 0;

  // True when we have sent a request for data from the HWM but have
  // not yet received a response

  type_ = TYPE_NONE;

  // And initialize the getdata command

  HardwareManager_CommandFactory cf;

  // Get Boards command

  getBoardsCmd_ = 
    new MuxReadout::HardwareManagerClient::Cmd(cf.GetBoards());

  // Get Squid channels command

  getSquidsCmd_ = 
    new MuxReadout::HardwareManagerClient::Cmd(cf.GetSQChannels());

  // A command container we will re-use for sending the
  // GetSeqId command to the HWM

  getSeqIdCmd_ = 
    new MuxReadout::HardwareManagerClient::Cmd(cf.GetSeqId());

  // A command container we will re-use for sending the
  // GetAllRegisters command to the HWM

  getRegsCmd_ = 
    new MuxReadout::HardwareManagerClient::Cmd(cf.GetAllRegisters());

  fpgaBm_     = new FpgaBoardManager();
  squidBm_    = new SquidBoardManager();
}

/**.......................................................................
 * Destructor.
 */
ReceiverConfigConsumer::~ReceiverConfigConsumer() 
{
  if(getBoardsCmd_) {
    delete getBoardsCmd_;
    getBoardsCmd_ = 0;
  }

  if(getSquidsCmd_) {
    delete getSquidsCmd_;
    getSquidsCmd_ = 0;
  }

  if(getSeqIdCmd_) {
    delete getSeqIdCmd_;
    getSeqIdCmd_ = 0;
  }

  if(getRegsCmd_) {
    delete getRegsCmd_;
    getRegsCmd_ = 0;
  }

  if(fpgaBm_) {
    delete fpgaBm_;
    fpgaBm_ = 0;
  }

  if(squidBm_) {
    delete squidBm_;
    squidBm_ = 0;
  }
}

/**.......................................................................
 * Get the next set of data from the server
 */
bool ReceiverConfigConsumer::getData()
{
  //COUT("Calling getData() for receiver config consumer"); 

  bool success=false;

  // If we don't yet have any info about what boards are in the
  // system attempt to get it from the hardware manager
  
  if(!haveBoardInfo_) {
    success |= requestBoards();
  }
  
  // Only read the configuration data if anything has changed.
  // Here we only send the command -- we will read the response
  // when (and if) the hardware manager responds.
  
  success |= requestHardwareStatus();

  //COUT("Calling getData() for receiver config consumer done, success = " << success); 

  return success;
}

/**.......................................................................
 * Request the list of boards
 */
bool ReceiverConfigConsumer::requestBoards()
{
  return sendCommand(getBoardsCmd_, TYPE_GETBOARDS);
}

/**.......................................................................
 * Request the list of boards
 */
bool ReceiverConfigConsumer::requestSquids()
{
  return sendCommand(getSquidsCmd_, TYPE_GETSQUIDS);
}

/**.......................................................................
 * Request the hardware configuration status (has anything changed?)
 */
bool ReceiverConfigConsumer::requestHardwareStatus()
{
  return sendCommand(getSeqIdCmd_, TYPE_GETSEQID);
}

/**.......................................................................
 * Request configuration data fgrom the HardwareManager
 */
bool ReceiverConfigConsumer::requestHardwareConfiguration()
{
  return sendCommand(getRegsCmd_, TYPE_GETREGS);
}

/**.......................................................................
 * Generic method to send a command to the HWM
 */
bool ReceiverConfigConsumer::
sendCommand(MuxReadout::HardwareManagerClient::Cmd* cmd, 
	    ReceiverConfigConsumer::Command type)
{
  // Do nothing if we are waiting for a response to a previously
  // issued command

  if(commandPending_) {
    return true;
  }

  bool success=false;
  
  cmd->reset();

  try {

    client_->sendOnly(*cmd);
    success = cmd->cmdWasSuccessfullySent_;

    type_ = type;
    commandPending_ = true;

  } catch(...) {
    success = false;
  }

  return success;
}

/**.......................................................................
 * Read configuration data from the HardwareManager
 */
bool ReceiverConfigConsumer::readResponse()
{
  bool success=false;
  MuxReadout::MuxXMLFile* xml = NULL;

  // Set the command pending flag to false

  commandPending_ = false;
    
  // Get the last sent command

  MuxReadout::HardwareManagerClient::Cmd* cmd = getLastSentCommand();
  
  if(cmd) {
    
    // Read the response
    
    client_->recvOnly(*cmd);
    
    if(cmd->succeeded()) {
      
      switch(type_) {
      case TYPE_GETBOARDS:
	return readBoardInfo(cmd);
	break;
      case TYPE_GETSQUIDS:
	return readSquidInfo(cmd);
	break;
      case TYPE_GETSEQID:
	return readHardwareStatus(cmd);
	break;
      case TYPE_GETREGS:
	return readHardwareConfiguration(cmd);
	break;
      default:
	return false;
	break;
      }
    }
  }

  return false;
}

/**.......................................................................
 * Return the last command we sent
 */
MuxReadout::HardwareManagerClient::Cmd* 
ReceiverConfigConsumer::getLastSentCommand()
{
  switch(type_) {
  case TYPE_GETBOARDS:
    return getBoardsCmd_;
    break;
  case TYPE_GETSQUIDS:
    return getSquidsCmd_;
    break;
  case TYPE_GETSEQID:
    return getSeqIdCmd_;
    break;
  case TYPE_GETREGS:
    return getRegsCmd_;
    break;
  default:
    return 0;
    break;
  }
}

bool ReceiverConfigConsumer::
readBoardInfo(MuxReadout::HardwareManagerClient::Cmd* cmd)
{
  // Set the return value if the command succeeded

  std::vector<unsigned> boards;

  boards = 
    client_->parseBoards(cmd->response_->get_string("Val"));

  // Intialize our array of boards

  for(unsigned iBoard=0; iBoard < boards.size(); iBoard++) {
    Board brd(boards[iBoard], SQUID_NONE);
    boards_.push_back(brd);
  }

  // Now that we have the list of boards, request the list of squids,
  // so we can determine which boards have squid controllers

  return requestSquids();
}

bool ReceiverConfigConsumer::
readSquidInfo(MuxReadout::HardwareManagerClient::Cmd* cmd)
{
  unsigned nSquid=0;

  // Get the list of squid channels from the HWM

  std::vector<string> squids = 
    client_->parseList(cmd->response_->get_string("Val"), " ");

  // Now for each board, iterate through the squid channels looking
  // for a match

  std::ostringstream osa, osb;
  for(unsigned iBoard=0; iBoard < boards_.size(); iBoard++) {

    osa.str("");
    osb.str("");

    osa << std::hex << boards_[iBoard].id_ << ":sa";
    osb << std::hex << boards_[iBoard].id_ << ":sb";

    // Iterate over the list of squids, looking for a match

    Board& brd = boards_[iBoard];
    for(unsigned iSquid=0; iSquid < squids.size(); iSquid++) {

      if(strncmp(osa.str().c_str(), squids[iSquid].c_str(), 5)==0) {
	brd.squid_ = SQUID_A;
	++nSquid;
	break;
      } else if(strncmp(osa.str().c_str(), squids[iSquid].c_str(), 5)==0) {
	brd.squid_ = SQUID_B;
	++nSquid;
	break;
      } 

    }
  }

  ReportMessage("Hardware manager reports there are: " << boards_.size() 
		<< " boards and " << nSquid << " squid controllers");

  haveBoardInfo_ = true;

  // If this is the first time we have board information avaiilable,
  // request the hardware status

  return requestHardwareStatus();
}

bool ReceiverConfigConsumer::
readHardwareStatus(MuxReadout::HardwareManagerClient::Cmd* cmd)
{
  std::vector<int> seqId;
  seqId = cmd->response_->get_int_vec("Val");

  // If we successfully read a sequence id from this response, check
  // it to see if anything has changed.  If it has, send a request for
  // the register map

  if(seqId.size() == 1) {

    // If the sequence id is negative (means we haven't yet read a
    // hardware configuration from the HWM), or the sequence id has
    // changed, send a request for all registers.
    //
    // Note, we can't just check if the sequence number is
    // incrementing (ie, if seqId[0] > seqId_), because if the HWM is
    // restarted, its sequence number will reset to 0!

    if(seqId_ < 0 || seqId[0] != seqId_) {

      if(seqId[0] < seqId_) {
	ReportSimpleError("The HWM was restarted. Note that the HWM can take "
			  "up to 5 minutes " << std::endl 
			  << "to respond to a configuration request after "
			  << "a reboot -- be patient!");
      }

      seqId_ = seqId[0];
      return requestHardwareConfiguration();
    } else {
      return true;
    }
  }

  return false;
}

bool ReceiverConfigConsumer::
readHardwareConfiguration(MuxReadout::HardwareManagerClient::Cmd* cmd)
{
  std::istringstream istr(cmd->response_->get_xml_text());

  MuxReadout::MuxXMLFile* xml = 0;

  xml = new MuxReadout::MuxXMLFile(istr, "");

  fpgaBm_->setTo(xml, boards_);
  squidBm_->setTo(xml, boards_);

  // Pack the boards now

  if(parent_) {
    parent_->packBoard(fpgaBm_);
    parent_->packBoard(squidBm_);
  }

  if(xml) {
    delete xml;
  }
  
  return true;
}
  


/**.......................................................................
 * Report an error
 */
void ReceiverConfigConsumer::reportError()
{
  ReportSimpleError("Unable to get scanner connection to the HardwareManager "
		    << "daemon at" << std::endl
		    << host_ << ":" << port_);
}

/**.......................................................................
 * Report a successfull connection
 */
void ReceiverConfigConsumer::reportSuccess()
{
  ReportMessage("Successfully connected to the HardwareManager daemon at " << host_ << ":" << port_);
}

ostream& gcp::receiver::operator<<(ostream& os, 
				   const ReceiverConfigConsumer::Board& brd)
{
  os << std::hex << brd.id_ << ":s";

  switch (brd.squid_) {
  case ReceiverConfigConsumer::SQUID_A:
    os << "a";
    break;
  case ReceiverConfigConsumer::SQUID_B:
    os << "b";
    break;
  case ReceiverConfigConsumer::SQUID_NONE:
    os << "none";
    break;
  default:
    break;
  }

  return os;
}

/**.......................................................................
 * Service our message queue.
 */
void ReceiverConfigConsumer::serviceMsgQ()
{
  bool stop   = false;
  int  nready = 0;
  int  msgqFd = msgq_.fd();
  
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

    // A response from the hardware client?

    if(fdSet_.isSetInRead(clientFd_)) {

      //COUT("About to call receiverconfigconsumer readResponse()");

      if(!readResponse())
	registerError();

      //COUT("About to call receiverconfigconsumer readResponse() done");
    }

  }

}
