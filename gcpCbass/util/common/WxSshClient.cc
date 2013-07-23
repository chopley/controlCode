#include "gcp/util/common/WxSshClient.h"
#include "gcp/util/common/SshTunnel.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructors
 */
WxSshClient::
WxSshClient(bool spawnThread, std::string gateway, std::string host,
		unsigned short port) :
  SshClient(spawnThread, gateway, host, port)
{
  bytes_.resize(wxData_.size());
  setReadBufSize(wxData_.size() + 8);
  setSendBufSize(wxData_.size() + 8);
}

/**.......................................................................
 * Initialize network container sizes and start up this thread if
 * requested to
 */
void WxSshClient::initialize(bool spawnThread) 
{
  bytes_.resize(wxData_.size());
  setReadBufSize(wxData_.size() + 8);
  setSendBufSize(wxData_.size() + 8);
}

/**.......................................................................
 * Destructor.
 */
WxSshClient::~WxSshClient() {}

// Method called when data have been completely read from the server

void WxSshClient::readServerData(NetHandler& handler) 
{
  int size;

  handler.getReadStr()->startGet(&size);

  if(size != bytes_.size())
    ThrowError("Received incompatible message size");

  handler.getReadStr()->getChar(size, &bytes_[0]);
  handler.getReadStr()->endGet();

  wxData_.deserialize(bytes_);

  // Call our process method, if any

  processServerData();
}
