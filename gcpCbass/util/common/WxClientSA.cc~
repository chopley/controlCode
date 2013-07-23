#include "gcp/util/common/WxClient40m.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
WxClient40m::WxClient40m(bool spawnThread, std::string host,
			   unsigned port) :
  Client(spawnThread, host, port)
{
  bytes_.resize(wxData_.size());
  setReadBufSize(wxData_.size() + 8);
  setSendBufSize(wxData_.size() + 8);
}

/**.......................................................................
 * Destructor.
 */
WxClient40m::~WxClient40m() {}

void WxClient40m::readServerData(NetHandler& handler)
{
  int size;

  handler.getReadStr()->startGet(&size);

  if(size != bytes_.size())
    ThrowError("Received incompatible message size");

  handler.getReadStr()->getChar(size, &bytes_[0]);
  handler.getReadStr()->endGet();

  wxData_.deserialize(bytes_);

  COUT("Just read data_" << wxData_);

  // Call our process method, if any

  processServerData();
}
