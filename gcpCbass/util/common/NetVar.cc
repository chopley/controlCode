#include "gcp/util/common/NetVar.h"

using namespace std;

using namespace gcp::util;
using namespace gcp::util;

/**
 * Constructor.
 */
NetVar::NetVar(gcp::util::DataType::Type type, void* vPtr, unsigned nEl)
{
  type_ = type;
  vPtr_ = vPtr;
  nEl_  = nEl;

  resize(DataType::sizeOf(type_) * nEl_);
}

/**.......................................................................
 * Copy constructor
 */
NetVar::NetVar(const NetVar& netVar)
{
  type_ = netVar.type_;
  nEl_  = netVar.nEl_;
}

/**.......................................................................
 * Copy constructor
 */
NetVar::NetVar(NetVar& netVar)
{
  type_ = netVar.type_;
  nEl_  = netVar.nEl_;
}

/**.......................................................................
 * Destructor.
 */
NetVar::~NetVar() {}

/**.......................................................................
 * Resize
 */
void NetVar::resize(unsigned size)
{
  size_ = size;
  NetDat::resize(size);
}

/**.......................................................................
 * Serialize the data
 */
void NetVar::serialize()
{
  // For now, just copy 1-1 (no conversion to network byte order)

  if(size() > 0) {
    unsigned char* src  = (unsigned char*)vPtr_;
    unsigned char* dest = &bytes_[0];
    
    for(unsigned i=0; i < size(); i++) {
      *dest++ = *src++;
    }
  }
}

/**.......................................................................
 * Method for deserialization of variables
 */
void NetVar::deserialize(const std::vector<unsigned char>& bytes) 
{
  // If this vector doesn't have the same size, stop

  checkSize(bytes);
  deserialize(&bytes[0]);
}

/**.......................................................................
 * Private deserialization method
 */
void NetVar::deserialize(const unsigned char* src) 
{
  unsigned char* dest = (unsigned char*) vPtr_;

  for(unsigned i=0; i < size(); i++)
    *dest++ = *src++;
}

unsigned NetVar::size()
{
  return size_;
}
