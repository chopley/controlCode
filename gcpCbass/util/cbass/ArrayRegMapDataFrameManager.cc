#define __FILEPATH__ "util/specific/ArrayRegMapDataFrameManager.cc"

#include "gcp/util/common/ArrayRegMapDataFrameManager.h"
#include "gcp/util/common/DataFrameNormal.h"

#include "gcp/control/code/unix/libunix_src/common/scanner.h"
#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
ArrayRegMapDataFrameManager::ArrayRegMapDataFrameManager(bool archivedOnly,
						     DataFrame* dataFrame) 
{
  initialize(archivedOnly, dataFrame);
}

/**.......................................................................
 * Destructor.
 */
ArrayRegMapDataFrameManager::~ArrayRegMapDataFrameManager() {}

/**.......................................................................
 * Constructor with no resizing of the initially zero-length DataFrame
 * buffer. This constructor doesn't initialize the antenna number
 * associated with this manager object.
 */
void ArrayRegMapDataFrameManager::initialize(bool archivedOnly,
					   DataFrame* frame) 
{
  if((regMap_ = new_ArrayRegMap())==0)
    ThrowError("Unable to allocate register map");

  // Set the flag if this frame only contains archived registers

  archivedOnly_ = archivedOnly;

  // If we are only recording archived registers, frame size is the
  // size of the archived register map

  unsigned frameSize = SCAN_BUFF_BYTE_SIZE(regMap_->nByte(archivedOnly));

  frame_ = new DataFrameNormal(frameSize);

  // Initialize the nBuffer variable

  nBuffer_ = frameSize;

  // Initialize from the passed frame, if any was passed.

  if(frame != 0)
    *frame_ = *frame;
}
