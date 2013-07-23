#define __FILEPATH__ "antenna/control/specific/OpticalBench.cc"

#include "gcp/util/common/Exception.h"
#include "gcp/antenna/control/specific/OpticalBench.h"

using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor function just intializes the date fields by calling
 * reset(), below.
 */
OpticalBench::OpticalBench()
{
  theta_ = gcp::util::Angle(gcp::util::Angle::Degrees(), 41.47);
  setZeroPosition(25, 25, 25, 25, 25, 25);
  setOffset(0, 0, 0, 0, 0, 0);
  setFocus(0);
  lowLimit_.y1 = 11;
  lowLimit_.y2 = 11;
  lowLimit_.y3 = 11;
  lowLimit_.x4 = 11;
  lowLimit_.x5 = 11;
  lowLimit_.z6 = 11;
  highLimit_.y1 = 47;
  highLimit_.y2 = 47;
  highLimit_.y3 = 47;
  highLimit_.x4 = 47;
  highLimit_.x5 = 47;
  highLimit_.z6 = 47;
}

OpticalBench::~OpticalBench()
{
}

void OpticalBench::setZeroPosition(double y1, double y2, double y3, double x4, double x5, double z6) {
  zeroPosition_.y1 = y1; 
  zeroPosition_.y2 = y2; 
  zeroPosition_.y3 = y3; 
  zeroPosition_.x4 = x4;
  zeroPosition_.x5 = x5;
  zeroPosition_.z6 = z6;
}
  
void OpticalBench::setOffset(double y1, double y2, double y3, double x4, double x5, double z6) {
  offset_.y1 = y1; 
  offset_.y2 = y2; 
  offset_.y3 = y3; 
  offset_.x4 = x4;
  offset_.x5 = x5;
  offset_.z6 = z6;
}

void OpticalBench::setUseBrakes(bool use_brakes) {
  useBrakes_ = use_brakes;
}

void OpticalBench::setAcquiredThreshold(double threshold) {
  acquiredThreshold_ = threshold;
}

void OpticalBench::setFocus(double focus) {
  focus_ = focus;
}

double OpticalBench::getDeadBand() {
  if(useBrakes_)
  {
    return acquiredThreshold_;
  } else {
    return 0.0;
  }
}

double OpticalBench::enforceY1Limits(double y1)
{
  if(y1 < lowLimit_.y1)
  {
    return lowLimit_.y1;
  } else if(y1 > highLimit_.y1) {
    return highLimit_.y1;
  } else {
    return y1;
  }
}

double OpticalBench::enforceY2Limits(double y2)
{
  if(y2 < lowLimit_.y2)
  {
    return lowLimit_.y2;
  } else if(y2 > highLimit_.y2) {
    return highLimit_.y2;
  } else {
    return y2;
  }
}

double OpticalBench::enforceY3Limits(double y3)
{
  if(y3 < lowLimit_.y3)
  {
    return lowLimit_.y3;
  } else if(y3 > highLimit_.y3) {
    return highLimit_.y3;
  } else {
    return y3;
  }
}

double OpticalBench::enforceX4Limits(double x4)
{
  if(x4 < lowLimit_.x4)
  {
    return lowLimit_.x4;
  } else if(x4 > highLimit_.x4) {
    return highLimit_.x4;
  } else {
    return x4;
  }
}

double OpticalBench::enforceX5Limits(double x5)
{
  // Don't allow x4 and x5 to differ by more than this
  static const double twistLimit = 5.0; // 5 millimeters

  if(x5 > (getExpectedX4() + twistLimit))
  {
    x5 = getExpectedX4() + twistLimit;
  } else if(x5 < (getExpectedX4() - twistLimit))
  {
    x5 = getExpectedX4() - twistLimit;
  }

  if(x5 < lowLimit_.x5)
  {
    return lowLimit_.x5;
  } else if(x5 > highLimit_.x5) {
    return highLimit_.x5;
  } else {
    return x5;
  }
}

double OpticalBench::enforceZ6Limits(double z6)
{
  if(z6 < lowLimit_.z6)
  {
    return lowLimit_.z6;
  } else if(z6 > highLimit_.z6) {
    return highLimit_.z6;
  } else {
    return z6;
  }
}

double OpticalBench::getExpectedY1() {
  return enforceY1Limits(zeroPosition_.y1 + offset_.y1 + (focus_ * sin(theta_.radians())));
}

double OpticalBench::getExpectedY2() {
  return enforceY2Limits(zeroPosition_.y2 + offset_.y2 + (focus_ * sin(theta_.radians())));
}

double OpticalBench::getExpectedY3() {
  return enforceY3Limits(zeroPosition_.y3 + offset_.y3 + (focus_ * sin(theta_.radians())));
}

double OpticalBench::getExpectedX4() {
  return enforceX4Limits(zeroPosition_.x4 + offset_.x4 + (focus_ * cos(theta_.radians())));
}

double OpticalBench::getExpectedX5() {
  return enforceX5Limits(zeroPosition_.x5 + offset_.x5 + (focus_ * cos(theta_.radians())));
}

double OpticalBench::getExpectedZ6() {
  return enforceZ6Limits(zeroPosition_.z6 + offset_.z6);
}
