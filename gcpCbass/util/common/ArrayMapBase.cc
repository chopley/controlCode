#include "gcp/util/common/ArrayMapBase.h"

using namespace std;
using namespace gcp::util;

// Construct the static singleton object

ArrayMapPtr ArrayMapBase::arrayMapPtr_;

/**.......................................................................
 * Constructor.
 */
ArrayMapBase::ArrayMapBase() 
{
  if(arrayMapPtr_.arrayMap_ == 0) {
    arrayMapPtr_.allocateArrayMap();
  }
}

/**.......................................................................
 * Destructor.
 */
ArrayMapBase::~ArrayMapBase() 
{
}

/**.......................................................................
 * Copy constructor
 */
ArrayMapBase::ArrayMapBase(const ArrayMapBase& arrayMapBase)
{
  *this = arrayMapBase;
}

/**.......................................................................
 * Copy constructor
 */
ArrayMapBase::ArrayMapBase(ArrayMapBase& arrayMapBase)
{
  *this = arrayMapBase;
}

void ArrayMapBase::operator=(const ArrayMapBase& arrayMapBase) 
{
  *this = (ArrayMapBase&)arrayMapBase;
}

/**.......................................................................
 * Do nothing -- this class manages a singleton object
 */
void ArrayMapBase::operator=(ArrayMapBase& arrayMapBase) 
{
}
