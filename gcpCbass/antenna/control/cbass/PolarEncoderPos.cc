#define __FILEPATH__ "antenna/control/specific/PolarEncoderPos.cc"

#include "gcp/antenna/control/specific/PolarEncoderPos.h"

using namespace std;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor.
 */
PolarEncoderPos::PolarEncoderPos()
{
  // Initialize these to something impossible

  right_ = UNSET;
  left_  = UNSET;
}
