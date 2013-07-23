#define __FILEPATH__ "util/common/NetTermHandler.cc"

#include "gcp/util/common/NetTermHandler.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Define the maximum size of a network buffer.  This should be large
 * enough to send the longest string, plus a long int specifying the
 * length of the string
 */
const unsigned NetTermHandler::LINE_LEN = 100;
const unsigned NetTermHandler::BUF_SIZE = LINE_LEN + sizeof(int) + 1;

/**.......................................................................
 * Constructor.
 */
NetTermHandler::NetTermHandler() :
  NetHandler()
{
  nrs_->setBuffer(NULL, BUF_SIZE);
  nss_->setBuffer(NULL, BUF_SIZE);

  // And register our handler to be called when a message has been
  // completely read

  nrs_->installReadHandler(NetTermHandler::readMsg, this);
}


/**.......................................................................
 * Constructor.
 */
NetTermHandler::NetTermHandler(int fd) :
  NetHandler(fd)
{
  nrs_->setBuffer(NULL, BUF_SIZE);
  nss_->setBuffer(NULL, BUF_SIZE);

  // And register our handler to be called when a message has been
  // completely read

  nrs_->installReadHandler(readMsg, this);
}

/**.......................................................................
 * Destructor.
 */
NetTermHandler::~NetTermHandler() {}

/**.......................................................................
 * A handler to be called when a command has been completely read.
 */
NET_READ_HANDLER(NetTermHandler::readMsg)
{
  NetTermHandler* handler = (NetTermHandler*)arg;
  NetReadStr* nrs = handler->nrs_;
  int unsigned nchar;
  unsigned char line[LINE_LEN+1];

  // Read the message type and unpack the corresponding message
  
  nrs->startGet((int *)&handler->lastMsgType_);
  nrs->getInt(1, &nchar);
  nrs->getChar(nchar, line);
  nrs->endGet();

  // Stuff the message just read into our internal string buffer

  handler->lastLine_ = (const char*)line;
}

NetTermHandler::MsgType NetTermHandler::getLastMsgType()
{
  return lastMsgType_;
}

std::string NetTermHandler::getLastLine()
{
  return lastLine_;
}

void NetTermHandler::sendLine(std::string& line)
{
  LogStream errStr;
  unsigned int size = line.size();

  if(line.size() > LINE_LEN) {
    errStr.appendMessage(true, "Line is too long");
    throw Error(errStr);
  }

  nss_->startPut(LINE);
  nss_->putInt(1, &size);
  nss_->putChar(size, (unsigned char*)line.c_str());

  if(nss_->send() != NetSendStr::NET_SEND_DONE) {
    errStr.appendMessage(true, "Error sending line");
    throw Error(errStr);
  }
}
