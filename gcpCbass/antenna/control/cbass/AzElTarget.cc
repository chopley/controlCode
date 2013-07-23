#define __FILEPATH__ "antenna/control/specific/AzElTarget.cc"

#include "gcp/util/common/Exception.h"

#include "gcp/antenna/control/specific/AzElTarget.h"

using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Return a pointer to the requested DriveAxis container
 *
 * @throws Exception
 */
gcp::antenna::control::DriveAxis* 
AzElTarget::DriveAxis(Axis::Type axis)
{
  switch (axis) {
  case Axis::AZ:
    return &az_;
    break;
  case Axis::EL:
    return &el_;
    break;
  default:
    throw Error("AzElTarget::DriveAxis: Received unrecognized axis type.\n");
    break;
  };
}

/**.......................................................................
 * Return the current mode
 */
DriveMode::Mode AzElTarget::getMode()
{
  return mode_;
}

/**.......................................................................
 * Set the current mode
 */
void AzElTarget::setMode(DriveMode::Mode mode)
{
  mode_ = mode;
}

/**.......................................................................
 * Pack the current mode for archival in the register database.
 */
void AzElTarget::packMode(unsigned* u_elements)
{
  u_elements[0] = mode_;
}

/**
 * Pack the raw azimuth and elevation in radians
 */
void AzElTarget::packRawAngles(double* d_elements)
{
  d_elements[0] = az_.getRawAngle().radians();
  d_elements[1] = el_.getRawAngle().radians();
}

/**.......................................................................
 * Pack the encoder rates for archival in the register database.
 */
void AzElTarget::packRawRates(signed* s_elements)
{
  s_elements[0] = static_cast<signed>(az_.getRawRate().mas());
  s_elements[1] = static_cast<signed>(el_.getRawRate().mas());
}

