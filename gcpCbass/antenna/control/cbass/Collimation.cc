#include "gcp/antenna/control/specific/Collimation.h"
#include "gcp/util/common/Debug.h"

using namespace std;

using namespace gcp::antenna::control;

/**.......................................................................
* Constructor.
*/
Collimation::Collimation() {}

/**.......................................................................
* Destructor.
*/
Collimation::~Collimation() {}

bool Collimation::isUsable()
{
  return usable_;
}

void Collimation::setUsable(bool usable)
{
  usable_ = usable;
}

void Collimation::reset()
{
  setUsable(false);
}
void Collimation::print()
{
  DBPRINT(false, gcp::util::Debug::DEBUGANY, "Usable_ = " << usable_ << std::endl);
}
