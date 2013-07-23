#define __FILEPATH__ "antenna/control/specific/AxisPos.cc"

#include "gcp/util/common/Exception.h"
#include "gcp/antenna/control/specific/AxisPos.h"

using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor
 */
AxisPos::AxisPos(Axis::Type type) : axis_(type)
{
  reset();
}

/**.......................................................................
 * Reset internal members
 */
void AxisPos::reset()
{
  topo_  = 0.0;
  count_ = 0;

  // Complain if the axis does not represent a single axis

  if(!axis_.isValidSingleAxis())
    throw Error("AxisPos::reset: Invalid axis.\n");
}
