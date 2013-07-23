#define __FILEPATH__ "util/common/AxisRange.cc"

#include "gcp/util/common/AxisRange.h"

using namespace std;
using namespace gcp::util;

#define TEST_BLOCK(block) \
 { \
    if(block == 0) \
      std::cout << "block is NULL" << std::endl; \
    else \
      std::cout << "block is NOT NULL" << std::endl; \
 }

/**.......................................................................
 * Constructor.
 */
AxisRange::AxisRange(CoordAxes axes, CoordRange range) 
{
  setTo(&axes, &range);
}

/**.......................................................................
 * Constructor.
 */
AxisRange::AxisRange(CoordAxes* axes, CoordRange* range) 
{
  setTo(axes, range);
}

/**.......................................................................
 * Constructor.
 */
AxisRange::AxisRange(CoordAxes& axes, CoordRange* range) 
{
  setTo(&axes, range);
}

/**.......................................................................
 * Constructor for a simplistic axis consisting of nEl
 * consecutive elements
 */
AxisRange::AxisRange(unsigned nEl)
{
  setTo(nEl);
}

/**.......................................................................
 * Bare constructor
 */
AxisRange::AxisRange() 
{
  reset();
}

/**.......................................................................
 * Constructor from a block and range
 */
AxisRange::AxisRange(RegMapBlock* block, CoordRange* range) 
{
  if(block == 0)
    ThrowError("Block is NULL");

  setTo(block->axes_, range);

  reset();
}

/**.......................................................................
 * Set the axis range
 */
void AxisRange::setTo(unsigned nEl)
{
#if 0 // These two options make no difference in execution time!
  axes_.setAxis(0, nEl);

  nEl_ = axes_.nEl();

  // Get the ranges

  ranges_ = axes_.getRanges();
#else
  CoordAxes axes(nEl);

  ranges_ = axes.getRanges();
#endif

  // And reset to the beginning

  reset();
}

/**.......................................................................
 * Set the axis range
 */
void AxisRange::setTo(CoordAxes* axes, CoordRange* range)
{
  if(axes==0)
    ThrowError("NULL argument(s)");

  // Get the range of indices

  if(!axes->rangeIsValid(range)) {
    COUT("Range: " << *range << " is invalid for axes: " << *axes);
    ThrowError("Range: " << *range << " is invalid for axes: " << *axes);
  }

  // Get the ranges

  ranges_ = axes->getRanges(range);

  // Store the number of elements corresponding to this range

  nEl_ = axes->nEl(range);

  // Keep a copy of the axes

  axes_ = *axes;

  // And reset to the beginning

  reset();
}

/**.......................................................................
 * Set the axis range
 */
void AxisRange::setToDc(CoordAxes* axes, CoordRange* range)
{
  if(axes==0)
    ThrowError("NULL argument(s)");

  // Get the range of indices

  if(!axes->rangeIsValid(range)) {
    COUT("Here ?!");
    ThrowError("Range: " << *range << " is invalid for axes: " << *axes);
  }

  // Get the ranges

  ranges_ = axes->getRanges(range);

  // Store the number of elements corresponding to this range

  nEl_ = axes->nEl(range);

  // Keep a copy of the axes

  axes_ = *axes;

  // And reset to the beginning

  reset();
}

/**.......................................................................
 * Destructor.
 */
AxisRange::~AxisRange() {}


/**.......................................................................
 * Reset internal iterators to correspond to the starting byte of this
 * range.
 */
void AxisRange::reset() 
{
  iRange_     = ranges_.begin();

  if(iRange_ != ranges_.end())
    iElCurrent_ = (*iRange_).start();
  else 
    iElCurrent_ = 0;

  iter_ = 0;
}

/**.......................................................................
 * Return true if we are at the end of an iteration
 */
bool AxisRange::isEnd()
{
  return iRange_ == ranges_.end();
}


/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::util::operator<<(ostream& os, AxisRange& range)
{
  os << "Current element = " << range.iElCurrent_ << endl;
  return os;
}

/**.......................................................................
 * Return the current coordinate
 */
Coord AxisRange::currentCoord()
{
  return axes_.coordOf(currentElement());
}

/**.......................................................................
 * Return the number of elements corresponding to this object
 */
unsigned AxisRange::nEl()
{
  return nEl_;
}

void AxisRange::operator=(AxisRange& obj)
{
  iter_       = obj.iter_;
  iElCurrent_ = obj.iElCurrent_;
  ranges_     = obj.ranges_;
  iRange_     = obj.iRange_;
  axes_       = obj.axes_;
  nEl_        = obj.nEl_;
}

