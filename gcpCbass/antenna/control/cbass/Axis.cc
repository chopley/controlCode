#define __FILEPATH__ "antenna/control/specific/Axis.cc"

#include "gcp/antenna/control/specific/Axis.h"

using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor
 */
Axis::Axis(Type type)
{
  type_ = type;
}

/**.......................................................................
 * Return true if the passed enumerator represents a valid single axis
 */
bool Axis::isValidSingleAxis()
{
  if(type_ == AZ || type_ == EL || type_ == PA)
    return true;
  return false;
}
