#define __FILEPATH__ "util/common/ArrayFrameBuffer.cc"

#include "gcp/util/common/ArrayDataFrameManager.h"
#include "gcp/util/common/ArrayFrameBuffer.h"

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
ArrayFrameBuffer::ArrayFrameBuffer(unsigned int nSlot, bool archivedOnly) :
FrameBuffer(nSlot) 
{
  for(unsigned iframe=0; iframe < nSlot; iframe++) {
    
    // Allocate each element of the frame vector

    slots_[iframe].frame_ = new ArrayDataFrameManager(archivedOnly);

    // Clear and skip the network header region of the frame.
  
    slots_[iframe].frame_->reinitialize(); 
  }
}

/**.......................................................................
 * Destructor.
 */
ArrayFrameBuffer::~ArrayFrameBuffer() 
{
  // Delete any allocated memory
  
 for(unsigned int iframe; iframe < nSlot_; iframe++)
    if(slots_[iframe].frame_ != 0)
      delete slots_[iframe].frame_;
}
