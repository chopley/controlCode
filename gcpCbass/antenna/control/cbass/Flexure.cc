#define __FILEPATH__ "antenna/control/specific/Flexure.cc"

#include "gcp/antenna/control/specific/Flexure.h"
#include "gcp/antenna/control/specific/PointingCorrections.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Debug.h"

#include <cmath>

using namespace std;

using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Flexure::Flexure() 
{
  reset();
}

void Flexure::reset()
{
  usable_ = false;
  sFlexure_ = 0.0;
  cFlexure_ = 0.0;
}

/**.......................................................................
 * Destructor.
 */
Flexure::~Flexure() {}

/**.......................................................................
 * Set the coefficient of the sin(elevation) flexure term
 */
void Flexure::setSineElFlexure(double sFlexure)
{
  DBPRINT(true, Debug::DEBUG3, "sFlexure = " << sFlexure);
  sFlexure_ = sFlexure;
}

/**.......................................................................
 * Set the coefficient of the cos(elevation) flexure term
 */
void Flexure::setCosElFlexure(double cFlexure)
{
  DBPRINT(true, Debug::DEBUG3, "cFlexure = " << cFlexure);
  cFlexure_ = cFlexure;
}

/**.......................................................................
 * Apply the flexure terms to the model
 */
void Flexure::apply(PointingCorrections* f)
{
  DBPRINT(true, Debug::DEBUG3, "Applying flexure: sFlexure = " << sFlexure_ << ", cFlexure = "
	  << cFlexure_);
  f->el -= (sFlexure_ * f->sin_el + cFlexure_ * f->cos_el);
  f->sin_el = sin(f->el);
  f->cos_el = cos(f->el);
}

/**.......................................................................
 * Set whether or not the flexure terms in this object are currently
 * usable
 */
void Flexure::setUsable(bool usable)
{
   usable_ = usable;
}

/**.......................................................................
 * Return true if the flexure terms in this object are currently
 * usable
 */
bool Flexure::isUsable()
{
  return usable_;
}

void Flexure::pack(signed* s_elements)
{
  DBPRINT(true, Debug::DEBUG3, "Packing flexure: sFlexure = " << sFlexure_ << ", cFlexure = "
	  << cFlexure_);

  s_elements[0] = static_cast<signed>(sFlexure_ * gcp::util::Angle::masPerRad_);
  s_elements[1] = static_cast<signed>(cFlexure_ * gcp::util::Angle::masPerRad_);
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::antenna::control::operator<<(ostream& os, Flexure& flex)
{
  os << "sFlexure_ = " << flex.sFlexure_ << std::endl
     << "cFlexure_ = " << flex.cFlexure_ << std::endl
     << "usable_   = " << flex.usable_   << std::endl;

  return os;
}
