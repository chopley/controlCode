#include "gcp/pgutil/common/MultipleImagePlotter.h"

#include "gcp/grabber/common/Channel.h"

#include "gcp/util/common/Exception.h"

#include<iostream>

using namespace std;

using namespace gcp::grabber;

/**.......................................................................
 * Constructor.
 */
MultipleImagePlotter::MultipleImagePlotter() 
{
  images_.resize(gcp::grabber::Channel::nChan_);
}

/**.......................................................................
 * Destructor.
 */
MultipleImagePlotter::~MultipleImagePlotter() {}

void MultipleImagePlotter::setCurrentImage(unsigned short imageIndex)
{
  imageIndex_ = imageIndex;
}

unsigned short MultipleImagePlotter::getCurrentChannel()
{
  return imageIndex_;
}

ImagePlotter& MultipleImagePlotter::currentImage() 
{
  if(imageIndex_ < 0 || imageIndex_ > gcp::grabber::Channel::nChan_-1)
    ThrowError("Iinvalid image index: " << imageIndex_);

  return images_[imageIndex_];
}

ImagePlotter& MultipleImagePlotter::getImage(unsigned index) 
{
  if(index < 0 || imageIndex_ > gcp::grabber::Channel::nChan_-1)
    ThrowError("Iinvalid image index: " << index);

  return images_[index];
}

//-----------------------------------------------------------------------
// ImagePlotter's interface
//-----------------------------------------------------------------------

void MultipleImagePlotter::open(char* device)
{
  currentImage().open(device);

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {
    images_[iChan].setPgplotId(currentImage().pgplotId());
  }

}

void MultipleImagePlotter::draw(bool cmapUpdate)
{
  currentImage().draw(cmapUpdate);
}

void MultipleImagePlotter::drawDouble()
{
  currentImage().drawDouble();
}

void MultipleImagePlotter::setGridInterval(gcp::util::Angle& interval)
{
  currentImage().setGridInterval(interval);
}

void MultipleImagePlotter::installNewImage(unsigned short channel, 
					   unsigned short* image)
{
  setCurrentImage(channel);
  currentImage().installNewImage(image);
}

void MultipleImagePlotter::testDraw()
{
  currentImage().testDraw();
}

void MultipleImagePlotter::pixelToWorld(unsigned ix, unsigned iy,
					double& x, double& y)
{
  currentImage().pixelToWorld(ix, iy, x, y);
}

void MultipleImagePlotter::worldToPixel(double x, double y, 
					unsigned& ix, unsigned& iy, 
					bool truncate)
{
  currentImage().worldToPixel(x, y, ix, iy, truncate);
}

void MultipleImagePlotter::worldToSkyOffset(double x, double y, 
					    gcp::util::Angle& xsky, 
					    gcp::util::Angle& ysky)
{
  currentImage().worldToSkyOffset(x, y, xsky, ysky);
} 

void MultipleImagePlotter::setPlotBoundaries()
{
  currentImage().setPlotBoundaries();
}
      
void MultipleImagePlotter::calculatePlotLimits()
{
  currentImage().calculatePlotLimits();
}

void MultipleImagePlotter::setPeak(unsigned ix, unsigned iy)
{
  currentImage().setPeak(ix, iy);
}

bool& MultipleImagePlotter::doGrid()
{
  currentImage().doGrid();
}
bool& MultipleImagePlotter::doCompass()
{
  currentImage().doCompass();
}
bool& MultipleImagePlotter::doBullseye()
{
  currentImage().doBullseye();
}
bool& MultipleImagePlotter::doCrosshair()
{
  currentImage().doCrosshair();
}

void MultipleImagePlotter::setContrast(double contrast)
{
  currentImage().setContrast(contrast);
}

void MultipleImagePlotter::setBrightness(double brightness)
{
  currentImage().setBrightness(brightness);
}

void MultipleImagePlotter::installColormap(std::string cmap)
{
  currentImage().installColormap(cmap);
}

// Fiddle the contrast and brightness of a frame grabber image

void MultipleImagePlotter::fiddleContrast(double xa, double ya)
{
  currentImage().fiddleContrast(xa, ya);
}

void MultipleImagePlotter::setDisplayedRange(double xa, double xb, 
					     double ya, double yb)
{
  currentImage().setDisplayedRange(xa, xb, ya, yb);
}

void MultipleImagePlotter::setDisplayedRange()
{
  currentImage().setDisplayedRange();
}

void MultipleImagePlotter::initialize()
{
  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++)
    images_[iChan].initialize();
}

void MultipleImagePlotter::getGreyscale()
{
  currentImage().getGreyscale();
}

void MultipleImagePlotter::drawImage(bool cmapUpdate)
{
  currentImage().drawImage(cmapUpdate);
}

void MultipleImagePlotter::drawGrid()
{
  currentImage().drawGrid();
}

void MultipleImagePlotter::drawBullseye()
{
  currentImage().drawBullseye();
}

void MultipleImagePlotter::drawCompass()
{
  currentImage().drawCompass();
}

void MultipleImagePlotter::drawCrosshair()
{
  currentImage().drawCrosshair();
}

//-----------------------------------------------------------------------
// ImageHandler's interface
//-----------------------------------------------------------------------

void MultipleImagePlotter::
setDeckAngleRotationSense(gcp::control::RotationSense sense)
{
  currentImage().setDeckAngleRotationSense(sense);
}

// Public method to set the number of images to combine

void MultipleImagePlotter::setNCombine(unsigned ncombine)
{
  currentImage().setNCombine(ncombine);
}

// Public method to set the channel

void MultipleImagePlotter::setChannel(unsigned channel)
{
  currentImage().setChannel(channel);
}
      
// Public method to set the flatfield

 void MultipleImagePlotter::setFlatfieldType(unsigned flatfieldType)
{
  currentImage().setFlatfieldType(flatfieldType);
}

// Public method to set the optical camera image directions

void MultipleImagePlotter::setXImDir(gcp::control::ImDir dir)
{
  currentImage().setXImDir(dir);
}
      
// Public method to set the optical camera image directions

void MultipleImagePlotter::setYImDir(gcp::control::ImDir dir)
{
  currentImage().setYImDir(dir);
}

// Public method to reset the optical camera FOV

void MultipleImagePlotter::setFov(const gcp::util::Angle& fov)
{
  currentImage().setFov(fov);
}
      
// Public method to set the optical camera FOV

void MultipleImagePlotter::setAspectRatio(double aspectRatio)
{
  currentImage().setAspectRatio(aspectRatio);
}
      
// Public method to set the optical camera collimation

void MultipleImagePlotter::setRotationAngle(const gcp::util::Angle& rotationAngle)
{
  currentImage().setRotationAngle(rotationAngle);
}

// Public method to set the deck angle

void MultipleImagePlotter::setDeckAngle(const gcp::util::Angle& deckAngle)
{
  currentImage().setDeckAngle(deckAngle);
}

gcp::util::ImageHandler::ImStat 
MultipleImagePlotter::getStats()
{
  currentImage().getStats();
}

gcp::util::ImageHandler::ImStat 
MultipleImagePlotter::getStats(unsigned ixmin, unsigned iymin, 
			       unsigned ixmax, unsigned iymax)
{
  currentImage().getStats(ixmin, iymin, ixmax, iymax);
}

void MultipleImagePlotter::getOffsetInfo(gcp::util::Angle& horiz, 
					 gcp::util::Angle& vert, 
					 unsigned& ipeak, unsigned&  jpeak)
{
  currentImage().getOffsetInfo(horiz, vert, ipeak, jpeak);
}

void MultipleImagePlotter::getPeakInfo(double& peak, double& snr)
{
  currentImage().getPeakInfo(peak, snr);
}
