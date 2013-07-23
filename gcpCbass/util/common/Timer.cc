#include "gcp/util/common/Timer.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Timer::Timer() {}

/**.......................................................................
 * Destructor.
 */
Timer::~Timer() {}

void Timer::start()
{
  start_.setToCurrentTime();
}

void Timer::stop()
{
  stop_.setToCurrentTime();
  diff_ = stop_ - start_;
}

double Timer::deltaInSeconds()
{
  return diff_.getTimeInSeconds();
}

