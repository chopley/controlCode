#define __FILEPATH__ "util/common/AntennaFrameBuffer.cc"

#include "gcp/util/common/AntennaFrameBuffer.h"
#include "gcp/util/common/AntennaDataFrameManager.h"

#include "gcp/control/code/unix/libunix_src/common/scanner.h"

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
AntennaFrameBuffer::AntennaFrameBuffer(gcp::util::AntNum* antNum,
				       bool archivedOnly) :
  FrameBuffer(SCAN_MAX_FRAME)
{
  for(unsigned iframe=0; iframe < SCAN_MAX_FRAME; iframe++) {
    
    // Allocate each element of the frame vector

    slots_[iframe].frame_ = new AntennaDataFrameManager(antNum,
							archivedOnly);

    // Clear and skip the network header region of the frame.
  
    slots_[iframe].frame_->reinitialize(); 
  }
}

/**.......................................................................
 * Constructor.
 */
AntennaFrameBuffer::AntennaFrameBuffer(const gcp::util::AntNum& antNum,
				       bool archivedOnly) :
  FrameBuffer(SCAN_MAX_FRAME)
{
  for(unsigned iframe=0; iframe < SCAN_MAX_FRAME; iframe++) {

    // Allocate each element of the frame vector

    slots_[iframe].frame_ = new AntennaDataFrameManager(antNum,
							archivedOnly);

    // Clear and skip the network header region of the frame.
  
    slots_[iframe].frame_->reinitialize(); 
  }
}

/**.......................................................................
 * Constructor.
 */
AntennaFrameBuffer::AntennaFrameBuffer(const gcp::util::AntNum& antNum,
				       unsigned int nFrame,
				       bool archivedOnly) :
				       
  FrameBuffer(nFrame)
{
  for(unsigned iframe=0; iframe < nFrame; iframe++) {

    // Allocate each element of the frame vector

    slots_[iframe].frame_ = new AntennaDataFrameManager(antNum,
							archivedOnly);

    // Clear and skip the network header region of the frame.
  
    slots_[iframe].frame_->reinitialize(); 
  }
}

/**.......................................................................
 * Destructor.
 */
AntennaFrameBuffer::~AntennaFrameBuffer() 
{
  // Delete any allocated memory

  for(unsigned int iframe; iframe < nSlot_; iframe++)
    if(slots_[iframe].frame_ != 0)
      delete slots_[iframe].frame_;
}
