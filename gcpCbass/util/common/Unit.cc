#include "gcp/util/common/Unit.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Unit::Unit() {}

/**.......................................................................
 * Destructor.
 */
Unit::~Unit() {}

/**.......................................................................
 * Return true if the passed name is a recognized name for this
 * unit
 */
bool Unit::isThisUnit(std::string unitName)
{
  addNames();

  for(unsigned iName=0; iName < names_.size(); iName++) {
    if(unitName == names_[iName])
      return true;
  }
  return false;
}

/**.......................................................................
 * Add a name to this list
 */
void Unit::addName(std::string name)
{
  names_.push_back(name);
}

void Unit::addNames()
{
  COUT("Calling Unit::addNames() stub");
}
