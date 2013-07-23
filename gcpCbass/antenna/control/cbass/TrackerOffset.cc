#include "gcp/antenna/control/specific/TrackerOffset.h"

using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor doesn't need to call reset methods of subclasses, since
 * their constructors will already do this
 */
TrackerOffset::TrackerOffset() {} 

/** .......................................................................
 * A public function to request an offset by enumerated type
 */
OffsetBase* TrackerOffset::Offset(OffsetMsg::Type type)
{
  switch (type) {
  case OffsetMsg::MOUNT:
    return &mount_;
    break;
  case OffsetMsg::EQUAT:
    return &equat_;
    break;
  case OffsetMsg::SKY:
    return &sky_;
    break;
  case OffsetMsg::TV:
    return &tv_;
    break;
  }
}

/**.......................................................................
 * Reset all offset parameters
 */
void TrackerOffset::reset()
{
  mount_.reset();
  equat_.reset();
  sky_.reset();
  tv_.reset();
}

/*.......................................................................
 * Update the az and el pointing offsets to include any new offsets measured
 * by the user from the TV monitor of the optical-pointing telescope.
 *
 * Input:
 *  f   PointingCorrections *  The corrected az,el and latitude.
 */
void TrackerOffset::mergeTvOffset(PointingCorrections *f)
{
  double daz, del;

  tv_.apply(f, &daz, &del);
  mount_.increment(daz, del);
}

/**.......................................................................
 * Pack equatorial offsets for archival in the register database.
 */
void TrackerOffset::packEquatOffset(signed* s_elements)
{
  equat_.pack(s_elements);
}

/**.......................................................................
 * Pack mount horizon offsets for archival in the register database.
 */
void TrackerOffset::packHorizOffset(signed* s_elements)
{
  mount_.pack(s_elements);
}

/**.......................................................................
 * Pack mount horizon offsets for archival in the register database.
 */
void TrackerOffset::packHorizOffset(double* array)
{
  mount_.pack(array);
}

/**.......................................................................
 * Pack sky offsets for archival in the register database.
 */
void TrackerOffset::packSkyOffset(signed* s_elements)
{
  sky_.pack(s_elements);
}
