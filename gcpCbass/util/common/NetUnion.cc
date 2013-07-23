#include "gcp/util/common/NetUnion.h"
#include "gcp/util/common/NetVar.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
NetUnion::NetUnion() 
{ 
  id_ = NETUNION_UNKNOWN;
}

/**.......................................................................
 * Copy constructors - we don't want to copy members_, as this
 * contains pointers to private memory locations in each object
 */
NetUnion::NetUnion(const NetUnion& netUnion) 
{
  id_ = netUnion.id_;

  if(maxSize_ != netUnion.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netUnion.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netUnion.maxSize_;
  bytes_   = netUnion.bytes_;
}

NetUnion::NetUnion(NetUnion& netUnion) 
{
  id_ = netUnion.id_;

  if(maxSize_ != netUnion.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netUnion.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netUnion.maxSize_;
  bytes_   = netUnion.bytes_;
}

/**.......................................................................
 * Assignment operators
 */
NetUnion& NetUnion::operator=(const NetUnion& netUnion) 
{
  id_ = netUnion.id_;

  if(maxSize_ != netUnion.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netUnion.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netUnion.maxSize_;
  bytes_   = netUnion.bytes_;

  return *this;
}

NetUnion& NetUnion::operator=(NetUnion& netUnion) 
{
  id_ = netUnion.id_;

  if(maxSize_ != netUnion.maxSize_)
    ThrowError("Objects are not the same");

  if(bytes_.size() != netUnion.bytes_.size())
    ThrowError("Objects are not the same");

  maxSize_ = netUnion.maxSize_;
  bytes_   = netUnion.bytes_;

  return *this;
}

/**.......................................................................
 * Destructor.
 */
NetUnion::~NetUnion() 
{
  // Delete any objects which were allocated by this one

  for(std::map<unsigned, NetDat::Info>::iterator i=members_.begin();
      i != members_.end(); i++) {
    if(i->second.alloc_) {
      delete i->second.datPtr_;
      i->second.datPtr_ = 0;
    }
  }
}

/**.......................................................................
 * Increment the maximum size
 */
void NetUnion::resize(unsigned size)
{
  unsigned oldMax = maxSize_;
  unsigned newMax = size + sizeof(id_);

  maxSize_ = (oldMax > newMax) ? oldMax : newMax;
  bytes_.resize(maxSize_);
}

/**.......................................................................
 * Add a member
 */
void NetUnion::addMember(unsigned id, NetDat* netDat, bool alloc)
{

  // Don't let the user create an id corresponding to UNKNOWN

  if(id == NETUNION_UNKNOWN)
    ThrowError("Invalid union id: " << id);

  // And don't let the user install multiple members with the same id

  if(findMember(id) != 0)
    ThrowError("A member with id: " << id << " already exists");

  // Add the new member to the list

  members_[id] = NetDat::Info(netDat, alloc);

  // And increment the size.  Check that netDat is not NULL in case we
  // are adding a "null" member -- ie, a case switch with no
  // associated data

  if(netDat != 0)
    resize(netDat->maxSize());
}

/**.......................................................................
 * Find a member by id
 */
NetDat* const NetUnion::findMember(unsigned id)
{
  std::map<unsigned int, NetDat::Info>::iterator slot = members_.find(id);

  if(slot == members_.end())
    return 0;

  return slot->second.datPtr_;
}

/**.......................................................................
 * Find a member by id
 */
bool NetUnion::memberIsValid(unsigned id)
{
  std::map<unsigned int, NetDat::Info>::iterator slot = members_.find(id);

  if(slot == members_.end())
    return false;

  return true;
}

/**.......................................................................
 * Find a member by id
 */
NetDat* const NetUnion::getMember(unsigned id)
{
  NetDat* netDat = findMember(id);

  if(netDat == 0 && !memberIsValid(id))
    ThrowError("Invalid member requested");
    
  return netDat;
}

/**.......................................................................
 * Set this union to the requested member
 */
void NetUnion::setTo(unsigned id)
{
  // Get the requested member to make sure it exists

  NetDat* netDat = getMember(id);

  // And set the id

  id_   = id;
}

/**.......................................................................
 * Return the size of this object
 */
unsigned NetUnion::size()
{
  // Unions are the size of the requested member, plus the size of the
  // id tag

  return sizeOf(id_) + sizeof(id_);
}

/**.......................................................................
 * Serialize the data in this struct
 */
void NetUnion::serialize()
{
  NetDat* netDat = getMember(id_);

  if(size() > 0) {

    unsigned char* dest = &bytes_[0];
    const unsigned char* src  = (const unsigned char*) &id_;
 
    for(unsigned i=0; i < sizeof(id_); i++) {
      *dest++ = *src++;
    }
      
    // Now serialize the appropriate member
    
    if(netDat != 0) {
      src = netDat->getSerializedDataPtr();

      for(unsigned i=0; i < netDat->size(); i++) {
	*dest++ = *src++;
      }
    }

  }
}

/**.......................................................................
 * De-serialize data into this struct
 */
void NetUnion::deserialize(const std::vector<unsigned char>& bytes)
{
  checkSize(bytes);

  deserialize(&bytes[0]);
}

void NetUnion::deserialize(const unsigned char* src)
{
  unsigned char* dest  = (unsigned char*) &id_;

  for(unsigned i=0; i < sizeof(id_); i++)
    *dest++ = *src++;

  // Get the requested member
   
  NetDat* netDat = getMember(id_);

  // And de-serialize the appropriate member

  if(netDat != 0)    
    netDat->deserialize(src);
}

/**.......................................................................
 * Check the size of an array against our size
 */
void NetUnion::checkSize(const std::vector<unsigned char>& bytes)
{
  if(bytes.size() < sizeof(id_))
    ThrowError("Data array is too small for a NetUnion");

  if(bytes.size() == 0)
    ThrowError("Encountered zero-sized object");
}

/**.......................................................................
 * Check the size of an array against our size
 */
void NetUnion::checkSize(const std::vector<unsigned char>& bytes, unsigned id)
{
  if(bytes.size() != getMember(id)->size())
    ThrowError("Data array has the wrong size for this object");
}

/**.......................................................................
 * Return the size of the requested member
 */
unsigned NetUnion::sizeOf(unsigned id)
{
  NetDat* dat = getMember(id);

  return (dat ? dat->size() : 0);
}

/**.......................................................................
 * Add a variable to the internal vector of members
 */
void NetUnion::addVar(unsigned id, 
		      gcp::util::DataType::Type type, void* vPtr, unsigned nEl)
{
  NetVar* netVar = new NetVar(type, vPtr, nEl);
  addMember(id, netVar, true);
}

/**.......................................................................
 * Add just an id
 */
void NetUnion::addCase(unsigned id)
{
  addMember(id, 0, false);
}

/**.......................................................................
 * Return the message type
 */
unsigned NetUnion::getType()
{
  return id_;
}
