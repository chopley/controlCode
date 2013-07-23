#include "gcp/util/common/NetDat.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include <unistd.h>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
NetDat::NetDat()
{
  maxSize_ = 0;
}

/**.......................................................................
 * Copy constructors
 */
NetDat::NetDat(const NetDat& netDat)
{
  if(maxSize_ != netDat.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netDat.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netDat.maxSize_;
  bytes_   = netDat.bytes_;
}

NetDat::NetDat(NetDat& netDat)
{
  if(maxSize_ != netDat.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netDat.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netDat.maxSize_;
  bytes_   = netDat.bytes_;
}

/**.......................................................................
 * Copy constructors
 */
void NetDat::operator=(const NetDat& netDat)
{
  if(maxSize_ != netDat.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netDat.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netDat.maxSize_;
  bytes_   = netDat.bytes_;
}

void NetDat::operator=(NetDat& netDat)
{
  if(maxSize_ != netDat.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netDat.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netDat.maxSize_;
  bytes_   = netDat.bytes_;
}

/**.......................................................................
 * Destructor.
 */
NetDat::~NetDat() {}

/**.......................................................................
 * Size in bytes
 */
unsigned NetDat::size() {}

/**.......................................................................
 * Max size in bytes
 */
unsigned NetDat::maxSize()
{
  return maxSize_;
}

/**.......................................................................
 * Increment the size
 */
void NetDat::resize(unsigned size)
{
  maxSize_ += size;
  bytes_.resize(maxSize_);
}

/**.......................................................................
 * Return a reference to serialized data
 */
std::vector<unsigned char>& NetDat::getSerializedData()
{
  serialize();
  return bytes_;
}

/**.......................................................................
 * Return a pointer to serialized data
 */
void NetDat::packSerializedData(unsigned char* dest)
{
  unsigned char* src = getSerializedDataPtr();

  for(unsigned i=0; i < size(); i++)
    *dest++ = *src++;
}

/**.......................................................................
 * Return a pointer to serialized data
 */
unsigned char* const NetDat::getSerializedDataPtr()
{
  serialize();

  if(bytes_.size() > 0)
    return &bytes_[0];

  return 0;
}

/**.......................................................................
 * Check the size of an array against our size
 */
void NetDat::checkSize(const std::vector<unsigned char>& bytes)
{
  if(bytes.size() != size())
    ThrowError("Data array has the wrong size for this object");

  if(bytes.size() == 0)
    ThrowError("Encountered zero-sized object");
}

/**.......................................................................
 * Deserialize data into this object
 */
void NetDat::deserialize(const std::vector<unsigned char>& bytes) {}

/**.......................................................................
 * Serialize the data
 */
void NetDat::serialize() {};

/**.......................................................................
 * Private deserialization method
 */
void NetDat::deserialize(const unsigned char* bytes) {}

/**.......................................................................
 * Send this object over a socket
 */
bool NetDat::send(int fd)
{
  unsigned char* buf = getSerializedDataPtr();
  int bytesSent=0, bytesTotal=size(), status=0;
	
  while(bytesSent < bytesTotal) {
    status = 
      ::write(fd, buf+bytesSent, bytesTotal-bytesSent);
	  
    if(status <= 0) {
      ThrowSysError("write()");
      return false;
    }
	  
    bytesSent += status;
  }
}

/**.......................................................................
 * Receive this object over a socket
 */
bool NetDat::recv(int fd)
{	
  unsigned char* buf = getSerializedDataPtr();
  int bytesRead=0, bytesTotal=size(), status=0;
	
  while(bytesRead < bytesTotal) {
    status = 
      ::read(fd, buf+bytesRead, bytesTotal-bytesRead);

    if(status <= 0) {

      if(status == 0)
	ThrowError("Connection was lost");

      if(status < 0)
	ThrowSysError("read()");

      return false;
    }
	  
    bytesRead += status;
  }

  // Once all data have been read, deserialize into our data members
	
  deserialize(buf);
  return true;
}

