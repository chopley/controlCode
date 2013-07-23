#include "gcp/util/common/NetStruct.h"
#include "gcp/util/common/NetVar.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
NetStruct::NetStruct() {}

/**.......................................................................
 * Copy constructors - we don't want to copy members_, as this
 * contains pointers to private memory locations in each object
 */
NetStruct::NetStruct(const NetStruct& netStruct) 
{
  //COUT("Inside NetStruct const copy constructor");

  if(maxSize_ != netStruct.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netStruct.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netStruct.maxSize_;
  bytes_   = netStruct.bytes_;
}

NetStruct::NetStruct(NetStruct& netStruct) 
{
  //COUT("Inside NetStruct copy constructor");
  if(maxSize_ != netStruct.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netStruct.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netStruct.maxSize_;
  bytes_   = netStruct.bytes_;
}

void NetStruct::operator=(const NetStruct& netStruct) 
{
  //COUT("Inside NetStruct const assignment operator");

  if(maxSize_ != netStruct.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netStruct.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netStruct.maxSize_;
  bytes_   = netStruct.bytes_;
}

void NetStruct::operator=(NetStruct& netStruct) 
{
  //COUT("Inside NetStruct assignment operator");

  if(maxSize_ != netStruct.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netStruct.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netStruct.maxSize_;
  bytes_   = netStruct.bytes_;
}

/**.......................................................................
 * Destructor.
 */
NetStruct::~NetStruct() 
{
  // Delete any objects which were allocated by this object

  for(unsigned i=0; i < members_.size(); i++) {
    if(members_[i].alloc_) {
      delete members_[i].datPtr_;
      members_[i].datPtr_ = 0;
    }
  }
}

/**.......................................................................
 * Add a member
 */
void NetStruct::addMember(NetDat* netDat, bool alloc)
{
  members_.push_back(NetDat::Info(netDat, alloc));
  resize(netDat->maxSize());
}

/**.......................................................................
 * Serialize the data in this struct
 */
void NetStruct::serialize()
{
  unsigned iByte=0;

  if(size() > 0) {
    unsigned char* dest = &bytes_[0];

    // Iterate over members, serializing each one

    for(unsigned i=0; i < members_.size(); i++, iByte++) {

      const unsigned char* src = members_[i].datPtr_->getSerializedDataPtr();

      // Iterate over the size of this member

      for(unsigned iEl=0; iEl < members_[i].datPtr_->size(); iEl++) {
	*dest++ = *src++;
      }
    }
  }
}

/**.......................................................................
 * De-serialize data into this struct
 */
void NetStruct::deserialize(const std::vector<unsigned char>& bytes)
{
  checkSize(bytes);

  deserialize(&bytes[0]);
}

void NetStruct::deserialize(const unsigned char* bytes)
{
  // Iterate over members, de-serializing into each one

  const unsigned char* src = &bytes[0];
  
  for(unsigned iMem=0; iMem < members_.size(); iMem++) {
    members_[iMem].datPtr_->deserialize(src);
    src += members_[iMem].datPtr_->size();
  }
}

/**.......................................................................
 * Check the size of an array against our size
 */
void NetStruct::checkSize(const std::vector<unsigned char>& bytes)
{
  if(bytes.size() == 0)
    ThrowError("Encountered zero-sized object");
}

/**.......................................................................
 * Add a variable to the iternal vector of members
 */
void NetStruct::addVar(gcp::util::DataType::Type type, void* vPtr, unsigned nEl)
{
  NetVar* netVar = new NetVar(type, vPtr, nEl);
  addMember(netVar, true);
}

/**.......................................................................
 * Return the size of this object.
 */
unsigned NetStruct::size()
{
  unsigned size=0;
  for(unsigned iMem=0; iMem < members_.size(); iMem++) {
    size += members_[iMem].datPtr_->size();
  }
  return size;
}
