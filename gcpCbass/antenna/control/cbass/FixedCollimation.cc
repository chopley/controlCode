#include <cmath>
#include <iostream>

#include "gcp/util/common/Debug.h"

#include "gcp/antenna/control/specific/FixedCollimation.h"

using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor trivially calls reset() method, below.
 */
FixedCollimation::FixedCollimation() : SkyOffset() 
{
  FixedCollimation::reset();
};

/**.......................................................................
 * Update the x offset associated with this collimation correction
 */
void FixedCollimation::setXOffset(Angle x)
{
  setXInRadians(x.radians());
}

/**.......................................................................
 * Update the elevation offset associated with this collimation correction
 */
void FixedCollimation::setYOffset(Angle y)
{
  setYInRadians(y.radians());
}

/**.......................................................................
 * Update the x offset associated with this collimation correction
 */
void FixedCollimation::incrXOffset(Angle x)
{
  incrXInRadians(x.radians());
}

/**.......................................................................
 * Update the elevation offset associated with this collimation correction
 */
void FixedCollimation::incrYOffset(Angle y)
{
  incrYInRadians(y.radians());
}

void FixedCollimation::reset()
{
  Collimation::reset();
  SkyOffset::reset();
}

void FixedCollimation::apply(PointingCorrections* f, TrackerOffset& offset)
{
  SkyOffset::apply(f);
}

void FixedCollimation::pack(signed* s_elements)
{
  SkyOffset::pack(s_elements);
}

void FixedCollimation::print()
{
  Collimation::print();
  SkyOffset::print();
}
