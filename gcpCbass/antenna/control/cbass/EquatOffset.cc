#define __FILEPATH__ "antenna/control/specific/EquatOffset.cc"

#include "gcp/util/common/Exception.h"

#include "gcp/antenna/control/specific/EquatOffset.h"

using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor just intializes the offsets to zero
 */
EquatOffset::EquatOffset()
{
  reset();
}

/**.......................................................................
 * Reset the offsets to zero
 */
void EquatOffset::reset()
{
  ra_  = 0.0;
  dec_ = 0.0;
}

/**.......................................................................
 * Set the offsets
 */
void EquatOffset::set(OffsetMsg msg)
{
  // Increment the existing offsets?

  switch(msg.mode) {
  case OffsetMsg::ADD:
    if(msg.axes & OffsetMsg::RA)
      ra_  = wrapPi(ra_ + msg.body.equat.ra);
    if(msg.axes & OffsetMsg::DEC)
      dec_ = wrapPi(dec_ + msg.body.equat.dec);
    break;

    // Replace the existing offsets.

  case OffsetMsg::SET:
    if(msg.axes & OffsetMsg::RA)
      ra_  = wrapPi(msg.body.equat.ra);
    if(msg.axes & OffsetMsg::DEC)
      dec_ = wrapPi(msg.body.equat.dec);
    break;
  default:
    ErrorDef(err,"SkyOffset::set: Unrecognized mode.\n");
    break;
  };
}

/**.......................................................................
 * Apply the offsets to the pointing correction.
 */
void EquatOffset::apply(PointingCorrections* f) {}

/**.......................................................................
 * Pack equatorial offsets for archival in the register database.
 */
void EquatOffset::pack(signed* s_elements)
{
  s_elements[0] = static_cast<signed>(ra_ * rtomas);
  s_elements[1] = static_cast<signed>(dec_ * rtomas);
}
