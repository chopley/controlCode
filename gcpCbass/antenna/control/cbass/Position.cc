#define __FILEPATH__ "antenna/control/specific/Position.cc"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/antenna/control/specific/Position.h"

using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor just calls reset() to intialize
 */
Position::Position()
{
  reset();
}

/**.......................................................................
 * Initialize data members to something sensible
 */
void Position::reset() 
{
  az_ = 0.0;
  el_ = 0.0;
  pa_ = 0.0;
}

/**.......................................................................
 * Set the data members
 */
void Position::set(Axis::Type axis, double val)
{
  switch (axis) {
  case Axis::AZ:
    az_ = val;
    break;
  case Axis::EL:
    el_ = val;
    break;
  case Axis::PA:
    pa_ = val;
    break;
  default:
    ErrorDef(err, "Position::set: Unrecognized axis.\n");
    break;
  };
}

/**.......................................................................
 * Set the data members
 */
void Position::set(double az, double el, double pa)
{
  az_ = az;
  el_ = el;
  pa_ = pa;
}

/**.......................................................................
 * Get the data members
 */
double Position::get(Axis::Type axis)
{
  switch (axis) {
  case Axis::AZ:
    return az_;
    break;
  case Axis::EL:
    return el_;
    break;
  case Axis::PA:
    return pa_;
    break;
  default:
    ErrorDef(err, "Position::get: Unrecognized axis.\n");
    break;
  };
}

/**.......................................................................
 * Increment the requested position with mount offsets
 */
void Position::increment(Axis::Type axis, double val)
{
  switch(axis) {
  case Axis::AZ:
    az_  = OffsetBase::wrap2pi(az_ + val);
    break;
  case Axis::EL:
    el_ += OffsetBase::wrapPi(val);
    break;
  case Axis::PA:
    pa_  = OffsetBase::wrapPi(pa_ + val);
    break;
  default:
    ReportError("Unrecognized axis.");
    break;
  };
}

/**.......................................................................
 * Increment the requested position with sky offsets
 */
void Position::increment(SkyOffset* offset)
{
  PointingCorrections f(az_, el_, pa_);
  offset->apply(&f);

  az_  = f.az;
  el_  = f.el;
  pa_  = f.pa;
}

/**.......................................................................
 * Increment the requested position with tilt meter corrections
 */
void Position::increment(AzTiltMeter* offset)
{
  PointingCorrections f(az_, el_, pa_);
  offset->apply(&f);

  az_ = f.az;
  el_ = f.el;
  pa_ = f.pa;
}

/**.......................................................................
 * Increment the requested position with mount offsets
 */
void Position::increment(MountOffset* offset)
{
  az_  = offset->wrap2pi(az_ + offset->getAz());
  el_ += offset->getEl();
  //pa_  = offset->wrapPi(pa_ + offset->getPa());
}

/**.......................................................................
 * Pack this position for archival in the register database.
 */
void Position::pack(signed* s_elements)
{
  s_elements[0] = static_cast<signed>(az_ * rtomas);
  s_elements[1] = static_cast<signed>(el_ * rtomas);
  //s_elements[2] = static_cast<signed>(pa_ * rtomas);
}
/**.......................................................................
 * Pack this position for archival in the register database.
 */
void Position::pack(double* array)
{
  array[0] = az_;
  array[1] = el_;
  //array[2] = pa_;
}
void Position::applyCollimation(Model& model, TrackerOffset& offset)
{
  PointingCorrections f(az_, el_, pa_);
  model.applyCollimation(&f, offset);

  az_  = f.az;
  el_  = f.el;
  //pa_  = f.pa;
}
