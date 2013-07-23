#define __FILEPATH__ "util/common/NetMsgHandler.cc"

#include "gcp/util/common/NetMsg.h"
#include "gcp/util/common/NetMsgHandler.h"
#include "gcp/util/common/NewRtcNetMsg.h"
#include "gcp/util/common/NewNetMsg.h"


#include "gcp/control/code/unix/libunix_src/common/netobj.h"
#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"
#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

#include "gcp/util/common/Debug.h"

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
NetMsgHandler::NetMsgHandler() :
  NetHandler()
{
  arraymap_ = 0;

  if((arraymap_ = new_ArrayMap())==0) {
    ThrowError("Unable to allocate array map");
  }

  // Set the user handlers to NULL

  userReadHandler_  = 0;
  userReadArg_      = 0;
  userSendHandler_  = 0;
  userSendArg_      = 0;
  userErrorHandler_ = 0;
  userErrorArg_     = 0;
  
  // Set up the network buffers

  gcp::control::RtcNetMsg netMsg;
  
  // Get the minimum buffer size

  unsigned bufferSize = NET_PREFIX_LEN + netMsg.size();

  setReadBuffer(NULL, bufferSize);
  setSendBuffer(NULL, bufferSize);

  // Install our handlers as the default handlers

  NetHandler::installReadHandler(readHandler, this);
  NetHandler::installReadErrorHandler(errorHandler, this);
  NetHandler::installSendHandler(sendHandler, this);
  NetHandler::installSendErrorHandler(errorHandler, this);
}

/**.......................................................................
 * Destructor.
 */
NetMsgHandler::~NetMsgHandler() 
{
  if(arraymap_ != 0)
    arraymap_ = del_ArrayMap(arraymap_);
  arraymap_ = 0;
}

/**.......................................................................
 * Send a message to a socket described by a previously attached fd.
 */
NetSendStr::NetSendId NetMsgHandler::sendNetMsg(NetMsg* msg)
{
  LogStream ls;

  if(nss_->state() != NetSendStr::NET_SEND_DONE) {
    ls.appendMessage(true, "Last message wasn't completely sent");
    throw Error(ls);
  }

  // Pack the message in our buffer and send it.

  packNetMsg(msg);
  return NetHandler::send();
}

/**.......................................................................
 * Send a message to a socket
 */
NetSendStr::NetSendId NetMsgHandler::sendNetMsg(int fd, NetMsg* msg)
{
  LogStream ls;

  if(nss_->state() != NetSendStr::NET_SEND_DONE) {
    ls.appendMessage(true, "Last message wasn't completely sent");
    throw Error(ls);
  }

  // Pack the message in our buffer and send it.

  packNetMsg(msg);
  return NetHandler::send(fd);
}

/**.......................................................................
 * Pack a network message into our send buffer.
 */
void NetMsgHandler::packNetMsg(NetMsg* msg)
{
  DBPRINT(true, Debug::DEBUG6, "Packing: type = " << msg->type
	  << " antenna = " << msg->body.antenna);

  // Copy it into our last sent buffer

  lastSentNetMsg_ = *msg;

  // And pack the message into the network buffer.

  nss_->startPut(msg->type);
  nss_->putInt(1, &msg->body.antenna);
  nss_->putObj(&rtc_msg_table, msg->type, (void*)&msg->body.msg);
  nss_->endPut();
}

/**.......................................................................
 * Pack a new-style network message into our send buffer.
 */
bool NetMsgHandler::packNewRtcNetMsg(NetMsg* msg)
{
  NewRtcNetMsg rtcNetMsg;
  bool recognized = true;

  // Copy it into our last sent buffer

  lastSentNetMsg_ = *msg;

  switch (msg->type) {
  case NetMsg::LOG:
    rtcNetMsg.msg->setTo(gcp::util::NewNetMsg::NET_LOG_MSG);
    strcpy(rtcNetMsg.msg->log.text, msg->body.msg.log.text);
    rtcNetMsg.msg->log.bad = msg->body.msg.log.bad;
    rtcNetMsg.msg->log.seq = msg->body.msg.log.seq;
    rtcNetMsg.msg->log.end = msg->body.msg.log.end;
    break;
  case NetMsg::DRIVE_DONE:
    rtcNetMsg.msg->setTo(gcp::util::NewNetMsg::NET_DRIVE_DONE_MSG);
    rtcNetMsg.msg->drive_done.seq = msg->body.msg.drive_done.seq;
    break;
  case NetMsg::BENCH_DONE:
    rtcNetMsg.msg->setTo(gcp::util::NewNetMsg::NET_BENCH_DONE_MSG);
    rtcNetMsg.msg->bench_done.seq = msg->body.msg.bench_done.seq;
    break;
  case NetMsg::SCAN_DONE:
    rtcNetMsg.msg->setTo(gcp::util::NewNetMsg::NET_SCAN_DONE_MSG);
    rtcNetMsg.msg->scan_done.seq = msg->body.msg.scan_done.seq;
    break;
  case NetMsg::SCRIPT_DONE:
    rtcNetMsg.msg->setTo(gcp::util::NewNetMsg::NET_SCRIPT_DONE_MSG);
    rtcNetMsg.msg->scriptDone.seq = msg->body.msg.scriptDone.seq;
    break;
  case NetMsg::SOURCE_SET:
    rtcNetMsg.msg->setTo(gcp::util::NewNetMsg::NET_SOURCE_SET_MSG);
    rtcNetMsg.msg->source_set.seq = msg->body.msg.source_set.seq;
    break;
  case NetMsg::NAV_UPDATE:
    rtcNetMsg.msg->setTo(gcp::util::NewNetMsg::NET_NAV_UPDATE_MSG);
    break;
  default:
    recognized = false;
    break;
  }

  if(recognized) {
    
    // And pack the message into the network buffer.
    
    std::vector<unsigned char>data = rtcNetMsg.getSerializedData();
    
    nss_->startPut(data.size());
    nss_->putChar(data.size(), &data[0]);
    nss_->endPut();
  }

  return recognized;
}


/**.......................................................................
 * Pack a greeting message into our send buffer.
 */
void NetMsgHandler::packGreetingMsg(unsigned int antenna)
{
  AntNum antNum(antenna);
  std::string regMapName = antNum.getAntennaName();

  RegMap* regmap = arraymap_->findRegMap(regMapName);

  if(regmap==0) {
    LogStream errStr;
    errStr.initMessage(true);
    errStr << "No such register map: " << antNum.getAntennaName() << std::endl;
    throw Error(errStr);
  }

  NetMsg netMsg;
  netMsg.packGreetingMsg(antenna, REGMAP_REVISION, 
			 regmap->nreg_, regmap->nByte_);
  packNetMsg(&netMsg);
}

/**.......................................................................
 * Pack an antenna ID message into our send buffer.
 */
void NetMsgHandler::packAntennaIdMsg(unsigned int antenna)
{
  NetMsg netMsg;
  netMsg.packAntennaIdMsg(antenna);
  packNetMsg(&netMsg);
}

/**.......................................................................
 * Return the last message read
 */
gcp::util::NetMsg* NetMsgHandler::getLastReadNetMsg()
{
  return &lastReadNetMsg_;
}

/**.......................................................................
 * Return the last message sent
 */
gcp::util::NetMsg* NetMsgHandler::getLastSentNetMsg()
{
  return &lastSentNetMsg_;
}

/**.......................................................................
 * Read a net message out of the network buffer
 */
void NetMsgHandler::readNetMsg()
{
  // Read the message type and unpack the corresponding message

  nrs_->startGet((int*)&lastReadNetMsg_.type);
  nrs_->getInt(1, &lastReadNetMsg_.body.antenna);
  nrs_->getObj(&rtc_msg_table, lastReadNetMsg_.type, 
	      &lastReadNetMsg_.body.msg);
  nrs_->endGet();
}

/**.......................................................................
 * A handler to be called when a message has been completely read.
 */
NET_READ_HANDLER(NetMsgHandler::readHandler)
{
  NetMsgHandler* netHandler = (NetMsgHandler*) arg;

  netHandler->readNetMsg();

  if(netHandler->userReadHandler_ != 0)
    netHandler->userReadHandler_(netHandler->userReadArg_);
}

/**.......................................................................
 * A handler to be called when a message has been completely send.
 */
NET_SEND_HANDLER(NetMsgHandler::sendHandler)
{
  NetMsgHandler* netHandler = (NetMsgHandler*) arg;

  if(netHandler->userSendHandler_ != 0)
    netHandler->userSendHandler_(netHandler->userSendArg_);
}

/**.......................................................................
 * A handler to be called when an error has occurred
 */
NET_ERROR_HANDLER(NetMsgHandler::errorHandler)
{
  NetMsgHandler* netHandler = (NetMsgHandler*) arg;

  if(netHandler->userErrorHandler_ != 0)
    netHandler->userErrorHandler_(netHandler->userErrorArg_);
}

/**.......................................................................
 * Methods to install user-defined handlers
 */
void NetMsgHandler::installReadHandler(NET_READ_HANDLER(*handler), void* arg)
{
  userReadHandler_ = handler;
  userReadArg_ = arg;
}

/**.......................................................................
 * Methods to install user-defined handlers
 */
void NetMsgHandler::installSendHandler(NET_SEND_HANDLER(*handler), void* arg)
{
  userSendHandler_ = handler;
  userSendArg_ = arg;
}

/**.......................................................................
 * Methods to install user-defined handlers
 */
void NetMsgHandler::installErrorHandler(NET_READ_HANDLER(*handler), void* arg)
{
  userErrorHandler_ = handler;
  userErrorArg_ = arg;
}


