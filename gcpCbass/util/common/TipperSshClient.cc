#include "gcp/util/common/TipperSshClient.h"
#include "gcp/util/common/SshTunnel.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructors
 */
TipperSshClient::
TipperSshClient(bool spawnThread, std::string gateway, std::string host,
		unsigned short port) :
  SshClient(spawnThread, gateway, host, port)
{
  bytes_.resize(tipperData_.size());
  setReadBufSize(tipperData_.size() + 8);
  setSendBufSize(tipperData_.size() + 8);
}

/**.......................................................................
 * Initialize network container sizes and start up this thread if
 * requested to
 */
void TipperSshClient::initialize(bool spawnThread) 
{
  bytes_.resize(tipperData_.size());
  setReadBufSize(tipperData_.size() + 8);
  setSendBufSize(tipperData_.size() + 8);
}

/**.......................................................................
 * Destructor.
 */
TipperSshClient::~TipperSshClient() {}

// Method called when data have been completely read from the server

void TipperSshClient::readServerData(NetHandler& handler) 
{
  int size;

  handler.getReadStr()->startGet(&size);

  if(size != bytes_.size())
    ThrowError("Received incompatible message size");

  handler.getReadStr()->getChar(size, &bytes_[0]);
  handler.getReadStr()->endGet();

  tipperData_.deserialize(bytes_);

  // Call our process method, if any

  processServerData();
}
