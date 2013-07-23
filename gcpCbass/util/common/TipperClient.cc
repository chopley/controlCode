#include "gcp/util/common/TipperClient.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
TipperClient::TipperClient(bool spawnThread, std::string host,
			   unsigned port) :
  Client(spawnThread, host, port)
{
  bytes_.resize(tipperData_.size());
  setReadBufSize(tipperData_.size() + 8);
  setSendBufSize(tipperData_.size() + 8);
}

/**.......................................................................
 * Destructor.
 */
TipperClient::~TipperClient() {}

void TipperClient::readServerData(NetHandler& handler)
{
  int size;

  handler.getReadStr()->startGet(&size);

  if(size != bytes_.size())
    ThrowError("Received incompatible message size");

  handler.getReadStr()->getChar(size, &bytes_[0]);
  handler.getReadStr()->endGet();

  tipperData_.deserialize(bytes_);

  COUT("Tipper tau is: " << tipperData_.tau_);
}
