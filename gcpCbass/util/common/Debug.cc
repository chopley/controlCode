#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Debug::Debug() {};

// Create the static instance

Debug::Debug();

// Initialize static variables

Debug::Level Debug::level_ = DEBUGANY;
Mutex Debug::mutex_;

/**.......................................................................
 * Public method to change the debugging level
 */
void Debug::setLevel(Level level)
{
  level_ = level;
}

/**.......................................................................
 * Public method to change the debugging level
 */
void Debug::addLevel(Level level)
{
  unsigned lmask = static_cast<unsigned>(level_);
  unsigned rmask = static_cast<unsigned>(level);
  level_ = static_cast<Level>(lmask | rmask);
}

/**.......................................................................
 * Public method to change the debugging level
 */
void Debug::remLevel(Level level)
{
  unsigned lmask = static_cast<unsigned>(level_);
  unsigned rmask = static_cast<unsigned>(level);
  level_ = static_cast<Level>(lmask & ~rmask);
}

/**.......................................................................
 * Public method to change the debugging level.
 */
void Debug::setLevel(unsigned int level)
{
  if(level==0)
    level_ = DEBUGNONE;
  else
    level_ = (Level) (1<<(level-1));
}

/**.......................................................................
 * Public method to query the debugging state
 */
bool Debug::debugging(Level level)
{
  return level_ & level;
}

void Debug::lock()
{
  mutex_.lock();
}

void Debug::unlock()
{
  mutex_.unlock();
}

