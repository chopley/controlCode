#include "gcp/util/common/SolidAngle.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
SolidAngle::SolidAngle() 
{
  initialize();
}

SolidAngle::SolidAngle(const Steradians& units, double sr)
{
  setSr(sr);
}

/**.......................................................................
 * Destructor.
 */
SolidAngle::~SolidAngle() {}

void SolidAngle::initialize()
{
  setSr(0.0);
}

void SolidAngle::setSr(double sr)
{
  sr_ = sr;
}
