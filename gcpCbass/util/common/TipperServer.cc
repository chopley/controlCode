#include "gcp/util/common/TipperServer.h"
#include "gcp/util/common/Date.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
TipperServer::TipperServer(std::string ftpHost, bool spawnThread) : 
  Server(spawnThread, TIPPER_SERVER_PORT), tipperComm_(ftpHost)
{
  setReadBufSize(data_.size() + 8);
  setSendBufSize(data_.size() + 8);

  timeOutCounter_ = 0;
  first_ = true;

  tipperComm_.spawn();
}

/**.......................................................................
 * Destructor.
 */
TipperServer::~TipperServer() {}

/**.......................................................................
 * What do we do on timeout?
 */
void TipperServer::timeOutAction() 
{
  // If this is not the first timeout we've encountered, read the data
  // from the end of the tipper log, and send it to any connected
  // clients

  if(!first_) {
    getMostRecentData();
    sendClientData(data_);
    CTOUT(data_);
  }

  // If this is the first, or it's time to retrieve a new file from
  // the FTP server, do it now

  if(first_ || timeOutCounter_ % 10 == 0) {
    first_ = false;
    tipperComm_.getTipperLog();
  }

  timeOutCounter_++;
  setTimeOutSeconds(15);
}

void TipperServer::acceptClientAction()
{
  sendClientData(data_);
}
