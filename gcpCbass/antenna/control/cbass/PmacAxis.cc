#include "gcp/antenna/control/specific/PmacAxis.h"

using namespace std;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor method
 */
PmacAxis::PmacAxis()
{
  reset();
}

/**.......................................................................
 * Reset method
 */
void PmacAxis::reset()
{
  count_ = 0;
  rate_  = 0;
}

/**.......................................................................
 * Return the encoder count
 */
signed PmacAxis::getCount()
{
  return count_;
}

/**.......................................................................
 * Return the encoder rate
 */
signed PmacAxis::getRate()
{
  return rate_;
}

/**.......................................................................
 * Set the count
 */
void PmacAxis::setCount(signed count)
{
  count_  = count;
}

/**.......................................................................
 * Set the rate
 */
void PmacAxis::setRate(signed rate)
{
  rate_ = rate;
}

/**.......................................................................
 * Set the position as an angle
 */
void PmacAxis::setAngle(gcp::util::Angle& angle)
{
  angle_ = angle;
}

void PmacAxis::setRadians(double radians)
{
  angle_.setRadians(radians);
}

/**.......................................................................
 * Return the position as an angle
 */
gcp::util::Angle PmacAxis::getAngle()
{
  return angle_;
}

