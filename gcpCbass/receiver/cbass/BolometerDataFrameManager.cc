#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/AxisRange.h"
#include "gcp/util/common/DataFrameNormal.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Directives.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/FrameFlags.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/receiver/specific/BolometerDataFrameManager.h"

#include "gcp/control/code/unix/libunix_src/common/scanner.h"

#include <iomanip>

using namespace gcp::util;
using namespace gcp::receiver;
using namespace std;

/**.......................................................................
 * Constructor with no resizing of the initially zero-length DataFrame
 * buffer.  This constructor doesn't intialize the antenna number
 * associated with this manager object.
 */
void BolometerDataFrameManager::
initialize(bool archivedOnly, gcp::util::DataFrame* frame) 
{
  regMap_ = 0;

  if((regMap_ = new_ReceiverRegMap())==0)
    ThrowError("Unable to allocate regMap_");
  
  archivedOnly_ = archivedOnly;

  // Make the frame large enough to accomodate the register map for all bands
  
  unsigned frameSize = SCAN_BUFF_BYTE_SIZE(regMap_->nByte(archivedOnly));

  // Allocate a frame for this manager

  frame_ = new gcp::util::DataFrameNormal(frameSize);
  
  // Initialize the nBuffer variable

  nBuffer_ = frameSize;

  // Initialize from the passed frame, if any was passed

  if(frame != 0)
    *frame_ = *frame;
}

/**.......................................................................
 * Copy constructor
 */
BolometerDataFrameManager::BolometerDataFrameManager(BolometerDataFrameManager& fm)
{
  initialize(fm.archivedOnly_, fm.frame_);
}

/**.......................................................................
 * Constructor with initialization from a DataFrame object.
 */
BolometerDataFrameManager::
BolometerDataFrameManager(bool archivedOnly, 
			   gcp::util::DataFrame* frame)
{
  initialize(archivedOnly, frame);
}

/**.......................................................................
 * Destructor.
 */
BolometerDataFrameManager::~BolometerDataFrameManager() 
{
}
