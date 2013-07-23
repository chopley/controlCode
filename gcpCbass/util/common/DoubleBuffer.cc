#include "gcp/util/common/DoubleBuffer.h"
#include "gcp/util/common/Exception.h"

#include<iostream>

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
DoubleBuffer::DoubleBuffer() 
{
  // Initialize the read pointer to point to first object
  // Initialize the write pointer to point to first object

  readBuf_  = &buf1_;
  writeBuf_ = &buf2_;
}


/**.......................................................................
 * Const Assignment Operator.
 */
void DoubleBuffer::operator=(const DoubleBuffer& objToBeAssigned)
{
  *this = (DoubleBuffer&) objToBeAssigned;
}

/**.......................................................................
 * Assignment Operator.
 */
void DoubleBuffer::operator=(DoubleBuffer& objToBeAssigned)
{
  COUT("Warning: calling default DoubleBuffer assignment oeprator");
}


/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::util::operator<<(std::ostream& os, DoubleBuffer& obj)
{
  os << "Default output operator for class: DoubleBuffer" << std::endl;
  return os;
};

/**.......................................................................
 * Destructor.
 */
DoubleBuffer::~DoubleBuffer() 
{
  readBuf_->unlock();
  writeBuf_->unlock();
}

/**.......................................................................
 * Grab the read buffer
 */
void* DoubleBuffer::grabReadBuffer()
{
  readBuf_->lock();
  return readBuf_->data_;
}

/**.......................................................................
 * Grab the write buffer
 */
void* DoubleBuffer::grabWriteBuffer()
{
  writeBuf_->lock();
  return writeBuf_->data_;
}

/**.......................................................................
 * Release the read buffer
 */
void DoubleBuffer::releaseReadBuffer()
{
  readBuf_->unlock();
}

/**.......................................................................
 * Release the write buffer
 */
void DoubleBuffer::releaseWriteBuffer()
{
  writeBuf_->unlock();
}

/**.......................................................................
 * Switch the buffer pointers
 */
void DoubleBuffer::switchBuffers()
{
  try {
    readBuf_->lock();
    writeBuf_->lock();
    
    BufferLock* tmp = readBuf_;
    
    readBuf_  = writeBuf_;
    writeBuf_ = tmp;
    
    readBuf_->unlock();
    writeBuf_->unlock();

  } catch(...) {
    readBuf_->unlock();
    writeBuf_->unlock();
  }
}
