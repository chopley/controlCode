#define __FILEPATH__ "util/common/QuadPath.cc"

#include <stdlib.h>
#include <math.h>

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/QuadPath.h"

using namespace std;
using namespace gcp::util;


/**.......................................................................
 * Create a new QuadPath object.
 *
 * Input:
 *  empty_value  double    The value to return until at least one
 *                         coordinate pair has been added.
 *  type       QuadType    The type of ordinate being interpolated.
 *                          QP_NORMAL         - A continuous function.
 *                          QP_SIGNED_ANGLE   - Angles defined modulo
 *                                              2.pi between -pi <= v < pi.
 *                          QP_POSITIVE_ANGLE - Angles defined modulo
 *                                              2.pi between 0 <= v < 2.pi.
 */
QuadPath::QuadPath(double empty_val, QuadType type)
{
  // Initialize this to NULL first, so the destructor can be safely
  // called on error

  quadpath_ = 0;

  gcp::control::QuadType quadtype;

  switch (type) {
  case QP_NORMAL:
    quadtype = gcp::control::QP_NORMAL;
    break;
  case QP_SIGNED_ANGLE:
    quadtype = gcp::control::QP_SIGNED_ANGLE;
    break;
  case QP_POSITIVE_ANGLE:
    quadtype = gcp::control::QP_POSITIVE_ANGLE;
    break;
  default:
    throw Error("QuadPath::QuadPath: Received unrecognized QuadType.\n");
    break;
  };

  quadpath_ = gcp::control::new_QuadPath(empty_val, quadtype);

  if(quadpath_ == 0)
    throw Error("QuadPath::QuadPath: "
		"Insufficient memory to allocate quadpath.\n");
}

/*.......................................................................
 * Delete a QuadPath object.
 */
QuadPath::~QuadPath()
{
  if(quadpath_ != 0)
    quadpath_ = gcp::control::del_QuadPath(quadpath_);
}

/*.......................................................................
 * Empty the coordinate table of a QuadPath object.
 *
 * After this call the value of the empty_value argument that was
 * presented to QuadPath::QuadPath() will be returned until
 * extend() is next called.
 */
void QuadPath::empty()
{
  gcp::control::empty_QuadPath(quadpath_);
}

/*.......................................................................
 * Evaluate the quadratic equation that joins the last 3 coordinate
 * pairs that were added to the path.
 *
 * Input:
 *  x         double    The X-axis value to evaluate the quadratic at.
 * Output:
 *  return    double    The Y-axis value that corresponds to 'x'.
 */
double QuadPath::eval(double x)
{
  return gcp::control::eval_QuadPath(quadpath_, x);
}
/*.......................................................................
 * Evaluate the gradient of the quadratic equation that joins the last 3
 * coordinate pairs that were added to the path.
 *
 * Input:
 *  x         double    The X-axis value to evaluate the quadratic at.
 * Output:
 *  return    double    The gradient that corresponds to 'x'. 
 */
double QuadPath::grad(double x)
{
  return gcp::control::grad_QuadPath(quadpath_, x);
}

/*.......................................................................
 * Append or prepend an x,y coordinate pair to the three-entry
 * circular table of a quadratic interpolation object. Entries are
 * kept in ascending order of x, so if the new x value is larger than
 * any currently in the table, it will be appended, and if it is
 * smaller it will be prepended. If there are already three entries in
 * the table the one at the other end of the table will be discarded
 * and the table rotated over it to make room for the new sample.
 *
 * If the new x value is within the range of x values already covered
 * by the table, the interpolator will be left unchanged.
 *
 * Each time a new entry is added, the three quadratic polynomial
 * coefficients a,b,c (ie. a.x^2+b.x+c) are recomputed for use by
 * eval().
 *
 * The coefficients are initialized according to the number of entries
 * in the interpolation table. After just one coordinate pair has been
 * entered via this function, eval() returns its y-value
 * irrespective of the target x-value. After a second point has been
 * added, the coefficients implement linear interpolation of the two
 * coordinate pairs. After 3 or more points have been added, the three
 * coefficients implement a quadratic interpolation of the last three
 * points entered.
 *
 * Note that calls to this function and set_QuadPath() can be interleaved.
 * In fact this function itself calls set_QuadPath().
 *
 * Input:
 *  x,y       double    The X,Y coordinate to add.
 */
void QuadPath::extend(double x, double y)
{
  if(gcp::control::extend_QuadPath(quadpath_, x, y)==1)
    throw Error("QuadPath::extend: Error in extend_QuadPath().\n");
}

/*.......................................................................
 * Given two angles A and B within the same 2.pi interval, return
 * whichever of B-2.pi, B, B+2.pi is within pi of A.
 */
double QuadPath::extendAngle(double a, double b)
{
  return gcp::control::extend_angle(a, b);
}

/*.......................................................................
 * Wrap an angle into the range -pi <= v < pi.
 *
 * Input:
 *  angle   double  The angle to be wrapped (radians).
 *
 * Output:
 *  return  double  The angle adjusted into the range -pi <= v < pi.
 */
QP_ANGLE_FN(QuadPath::angle_around_zero)
{
  return gcp::control::angle_around_zero(angle);
}

/*.......................................................................
 * Wrap an angle into the range 0 <= v < 2.pi.
 *
 * Input:
 *  angle   double  The angle to be wrapped (radians).
 * Output:
 *  return  double  The angle adjusted into the range 0 <= v < 2.pi.
 */
QP_ANGLE_FN(QuadPath::angle_around_pi)
{
  return gcp::control::angle_around_pi(angle);
}

/*.......................................................................
 * Query the contents of a QuadPath object.
 *
 * Input/Output
 *  data       QuadData *  On input pass a pointer to a return container.
 *                         On output the contents of *quad will be assigned
 *                         to *data. Note that the X-axis values are
 *                         guaranteed to be in ascending order, and if the
 *                         Y-axis values are angles, then they will be returned
 *                         wrapped into the range prescribed by the 'type'
 *                         argument of QuadPath::QuadPath().
 */
void QuadPath::get(QuadData *data)
{
  if(gcp::control::get_QuadPath(quadpath_, &data->quaddata_)==1)
     throw Error("QuadPath::get: Error in get_QuadPath().\n");
}

/*.......................................................................
 * Set the contents of a QuadPath object.
 *
 * Each time a new set of samples is received by this function, the
 * three quadratic polynomial coefficients a,b,c (ie. a.x^2+b.x+c) are
 * recomputed for use by eval().
 *
 * The coefficients are initialized according to the number of entries
 * in the interpolation table. If just one coordinate pair has been
 * entered via this function, eval() returns its y-value
 * irrespective of the target x-value. If two samples have been
 * entered, the coefficients implement linear interpolation of the two
 * coordinate pairs. If all 3 points are entered, the three
 * coefficients implement a quadratic interpolation.
 *
 * Input:
 *  data       QuadData *  The data to assign to the interpolator. See
 *                         quad.h for details. Note that the quad copy of
 *                         data->s will be sorted into ascending order of
 *                         x, and that if there are duplicate values of x,
 *                         all but one of them will be removed, and npt
 *                         will be decremented accordingly.
 */
void QuadPath::set(QuadData *data)
{
  if(gcp::control::set_QuadPath(quadpath_, &data->quaddata_)==1)
     throw Error("QuadPath::set: Error in set_QuadPath().\n");
}

/**.......................................................................
 * Empty a QuadData object and set all of its sample values to 0.
 */
void QuadPath::QuadData::init()
{
  gcp::control::ini_QuadData(&quaddata_);
}

