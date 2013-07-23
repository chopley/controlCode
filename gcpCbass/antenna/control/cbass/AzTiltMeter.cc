#include "gcp/antenna/control/specific/AzTilt.h"
#include "gcp/antenna/control/specific/AzTiltMeter.h"
#include "gcp/antenna/control/specific/PointingCorrections.h"
#include "gcp/util/common/Exception.h"

using namespace gcp::antenna::control;
using namespace gcp::util;

AzTiltMeter::AzTiltMeter()
{
  xAvg_ = new MovingAverage<double>(defaultAvgCount);
  yAvg_ = new MovingAverage<double>(defaultAvgCount);
  avgCount_ = defaultAvgCount;
  meterEnabled_ = false;
  setTheta(Angle(Angle::Degrees(), 15.0));
  setRange(Angle(Angle::Degrees(), 1.0));
  setOffset(Angle(Angle::Degrees(), 0.0), Angle(Angle::Degrees(), 0.0));
} 

AzTiltMeter::~AzTiltMeter()
{
  delete xAvg_;
  delete yAvg_;
}

void AzTiltMeter::enable()
{
  if(!meterEnabled_)
  {
    xAvg_->reset();
    yAvg_->reset();
    meterEnabled_ = true;
  }
}

void AzTiltMeter::disable()
{
  meterEnabled_ = false;
}

void AzTiltMeter::reset()
{
  xAvg_->reset();
  yAvg_->reset();
}

void AzTiltMeter::setMovingAvgSampleCount(unsigned int count)
{
  COUT("AzTiltMeter::setMovingAvgSampleCount = " << count);
  avgCount_ = count;
  xAvg_->setNumSamples(count);
  yAvg_->setNumSamples(count);
}

void AzTiltMeter::setTheta(Angle theta)

{
  COUT("AzTiltMeter theta = " << theta.degrees());
  theta_ = theta;
}

void AzTiltMeter::addSample(gcp::util::Angle x, gcp::util::Angle y)
{
  xAvg_->addSample(x.radians());
  yAvg_->addSample(y.radians());
}

static double constrain(double x, double max)
{
  if(x > max)
  {
    x = max;
  } else if (x < -max) 
  {
    x = -max;
  }
  return x;
}

void AzTiltMeter::apply(PointingCorrections* f)
{
  if(meterEnabled_)
  {
    double x = constrain(*xAvg_, maxAngle_) + xOffset_;
    double y = constrain(*yAvg_, maxAngle_) + yOffset_;
    double theta = theta_.radians();
    AzTilt tilt;
    double latTilt = (x * cos(theta)) - (y * sin(theta)); 
    double haTilt =  (x * sin(theta)) + (y * cos(theta)); 
    tilt.setLatTilt(latTilt);
    tilt.setHaTilt(haTilt);
    tilt.apply(f);
    f->az = wrap2pi(f->az);
  }
}
