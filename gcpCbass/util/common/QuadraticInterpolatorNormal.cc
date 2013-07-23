#define __FILEPATH__ "util/common/QuadraticInterpolatorNormal.cc"

#include "gcp/util/common/QuadraticInterpolatorNormal.h"

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
QuadraticInterpolatorNormal::
QuadraticInterpolatorNormal(double emptyValue) 
{
  type_ = QP_NORMAL;
  setEmptyValue(emptyValue);
}
