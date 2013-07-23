#include "gcp/util/common/Power.h"

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Power::Power() 
{
  initialize();
}

Power::Power(const dBm& units, double dBmPow)
{
  setdBm(dBmPow);
}

/**.......................................................................
 * Destructor.
 */
Power::~Power() {}

void Power::initialize()
{
  setdBm(0.0);
}

