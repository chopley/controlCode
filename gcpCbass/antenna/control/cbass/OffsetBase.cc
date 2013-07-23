#define __FILEPATH__ "antenna/control/specific/OffsetBase.cc"

#include <iostream>
#include <cmath>

#include "gcp/util/common/Exception.h"

#include "gcp/antenna/control/specific/OffsetBase.h"

using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Do-nothing constructor and destructor
 */
OffsetBase::OffsetBase() {};
OffsetBase::~OffsetBase() {};

// Stub these one out so that only offsets with a relevant angle member
// have to define it

/**.......................................................................
 * Apply the offsets to the pointing model.
 */
void OffsetBase::apply(PointingCorrections* f) {};

/**.......................................................................
 * Install new offsets received from the control program.
 */
void OffsetBase::set(OffsetMsg msg) {};

/**.......................................................................
 * Install new offsets received from the control program, with
 * sequence number.
 */
void OffsetBase::set(OffsetMsg msg, unsigned seq) {};

/**.......................................................................
 * Set an angle.
 */
void OffsetBase::setAngle(double angle) {};

/**.......................................................................
 * Round an angle into the range -pi..pi.
 *
 * Input:
 *  angle    double   The angle to be rounded (radians).
 *
 * Output:
 *  return   double   An angle in the range:  -pi <= angle < pi.
 */
double OffsetBase::wrapPi(double angle)
{
  double a = wrap2pi(angle);
  return a < pi ? a : (a-twopi);
}

/**.......................................................................
 * Round an angle into the range 0-2.pi. Note that fmod() is not used
 * because it was found that the version of fmod() that comes with the
 * 68060 and other Motorola CPU's is implemented as a while loop which
 * takes seconds (!) to finish if the divisor is much smaller than the
 * numerator.
 *
 * Input:
 *  angle    double   The angle to be rounded (radians).
 *
 * Output:
 *  return   double   An angle in the range:  0 <= angle < 2.pi.
 */
double OffsetBase::wrap2pi(double angle)
{
  return (angle >= 0.0) ? (angle - twopi * floor(angle/twopi)) :
			  (angle + twopi * ceil(-angle/twopi));
}
