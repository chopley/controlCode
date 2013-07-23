#define __FILEPATH__ "antenna/control/specific/AxisPositions.cc"

#include "gcp/util/common/Exception.h"

#include "gcp/antenna/control/specific/AxisPositions.h"

#include "gcp/control/code/unix/libunix_src/common/const.h"

using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor initializes the AxisPos members.
 */
AxisPositions::AxisPositions() : 
  az_(Axis::AZ), el_(Axis::EL), pa_(Axis::PA) {};

/**.......................................................................
 * Return a pointer to the requested axis position
 */
gcp::antenna::control::AxisPos* 
AxisPositions::AxisPos(Axis::Type type)
{
  switch (type) {
  case Axis::AZ:
    return &az_;
    break;
  case Axis::EL:
    return &el_;
    break;
  case Axis::PA:
    return &pa_;
    break;
  default:
    throw Error("AxisPositions::AxisPos: Unrecognized axis.\n");
    break;
  }
}

/**.......................................................................
 * Pack the topocentric positions for archival in the register
 * database
 */
void AxisPositions::pack(double* d_elements)
{
  d_elements[0] = Angle(Angle::Radians(), az_.topo_).radians();
  d_elements[1] = Angle(Angle::Radians(), el_.topo_).radians();
  d_elements[2] = Angle(Angle::Radians(), pa_.topo_).radians();
}
