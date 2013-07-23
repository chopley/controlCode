#define __FILEPATH__ "antenna/control/specific/DriveAxis.cc"

#include "gcp/antenna/control/specific/DriveAxis.h"

using namespace std;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor method
 */
DriveAxis::DriveAxis()
{
  reset();
}

/**.......................................................................
 * Reset method
 */
void DriveAxis::reset()
{
  count_ = 0;
  rate_  = 0;
}

/**.......................................................................
 * Return the encoder count
 */
signed DriveAxis::getCount()
{
  return count_;
}

/**.......................................................................
 * Return the encoder rate
 */
signed DriveAxis::getRate()
{
  return rate_;
}

/**.......................................................................
 * Set the count
 */
void DriveAxis::setCount(signed count)
{
  count_  = count;
}

/**.......................................................................
 * Set the rate
 */
void DriveAxis::setRate(signed rate)
{
  rate_ = rate;
}


