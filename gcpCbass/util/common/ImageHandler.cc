#include "gcp/util/common/Exception.h"
#include "gcp/util/common/ImageHandler.h"

#include "gcp/grabber/common/Flatfield.h"

#include<iostream>

using namespace std;

using namespace gcp::util;
using namespace gcp::control;

//-----------------------------------------------------------------------
// Static variable initialization
//-----------------------------------------------------------------------

// The camera Field of View

const gcp::util::Angle ImageHandler::defaultFov_ = 
gcp::util::Angle(gcp::util::Angle::ArcMinutes(), 12.0);

// The Aspect Ratio (y/x)

const double ImageHandler::defaultAspectRatio_  = 0.8;

// The rotation Angle of the camera

const gcp::util::Angle ImageHandler::defaultRotationAngle_ = 
gcp::util::Angle(gcp::util::Angle::Degrees(), 0.0);

// The channel

const gcp::grabber::Channel::FgChannel ImageHandler::defaultChannel_ = 
gcp::grabber::Channel::NONE;

// The flatfielding flag

const unsigned ImageHandler::defaultFlatfieldType_ = 
gcp::grabber::Flatfield::FLATFIELD_NONE;

// The number of frames to combine 

const unsigned int ImageHandler::defaultNcombine_ = 1;

// The sense of the x,y-axis increment

const ImDir ImageHandler::defaultXImDir_ = UPRIGHT;
const ImDir ImageHandler::defaultYImDir_ = UPRIGHT;

// The sense of the deck rotation

const RotationSense ImageHandler::defaultDeckAngleRotationSense_ = CCW;

/**.......................................................................
 * Initialize image parameters to defaults
 */
void ImageHandler::initializeCameraDefaults()
{
  // The optical camera field of view

  fov_ = defaultFov_;

  // The optical camera aspect ratio (y/x)

  aspectRatio_ = defaultAspectRatio_;

  // The optical camera collimation angle

  rotationAngle_ = defaultRotationAngle_;

  // The channel

  channel_ = defaultChannel_;

  // The flatfielding flag

  flatfieldType_ = defaultFlatfieldType_;

  // The number of frames to combine 

  nCombine_ = defaultNcombine_;

  // The sense of the x,y-axis increment

  xImDir_ = defaultXImDir_;
  yImDir_ = defaultYImDir_;

  // The sense of the deck rotation

  deckAngleRotationSense_ = defaultDeckAngleRotationSense_;

  // The size of the image is fixed

  nx_ = GRABBER_XNPIX;
  ny_ = GRABBER_YNPIX;

  imageToArchive_.resize(nx_*ny_);
}

/**.......................................................................
 * Initialize image statistics to defaults
 */
void ImageHandler::initializeImageStats()
{
  // Initialize parameters.  All positions will be computed in pixels.
  // In these units, the first pixel of the image is centered at -N/2 + 0.5.
  
  xa_ = -(double)(GRABBER_XNPIX)/2 + 0.5;
  ya_ = -(double)(GRABBER_YNPIX)/2 + 0.5;
  xb_ =  (double)(GRABBER_XNPIX)/2 - 0.5;
  yb_ =  (double)(GRABBER_YNPIX)/2 - 0.5;
  dx_ = 1.0;
  dy_ = 1.0;

  // And initialize to use all pixels 

  for(unsigned i=0; i < GRABBER_IM_SIZE; i++)
    usePixel_[i] = true;
}

void ImageHandler::initialize()
{
  haveImage_ = false;
  initializeCameraDefaults();
  initializeImageStats();
}

/**.......................................................................
 * Constructor.
 */
ImageHandler::ImageHandler() 
{
  initialize();
}

/**.......................................................................
 * Const Copy Constructor.
 */
ImageHandler::ImageHandler(const ImageHandler& objToBeCopied)
{
  *this = (ImageHandler&)objToBeCopied;
};

/**.......................................................................
 * Copy Constructor.
 */
ImageHandler::ImageHandler(ImageHandler& objToBeCopied)
{
  *this = objToBeCopied;
};

/**.......................................................................
 * Const Assignment Operator.
 */
void ImageHandler::operator=(const ImageHandler& objToBeAssigned)
{
  *this = (ImageHandler&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void ImageHandler::operator=(ImageHandler& obj)
{
  fov_ = obj.fov_;
  aspectRatio_ = obj.aspectRatio_;
  deckAngle_ = obj.deckAngle_;
  rotationAngle_ = obj.rotationAngle_;
  deckAngleRotationSense_ = obj.deckAngleRotationSense_;
  channel_ = obj.channel_;
  flatfieldType_ = obj.flatfieldType_;
  nCombine_ = obj.nCombine_;
  xImDir_ = obj.xImDir_;
  yImDir_ = obj.yImDir_;
  
  imageToArchive_ = obj.imageToArchive_;

  dx_ = obj.dx_;
  dy_ = obj.dy_;
  xa_ = obj.xa_;
  ya_ = obj.ya_;
  xb_ = obj.xb_;
  yb_ = obj.yb_;
  
  nx_ = obj.nx_;
  ny_ = obj.ny_;

  for(unsigned iPix = 0; iPix < GRABBER_IM_SIZE; iPix++) {
    imageToStore_[iPix] = obj.imageToStore_[iPix];
    usePixel_[iPix] = obj.usePixel_[iPix];
  }

  boxes_.resize(obj.boxes_.size());

  for(unsigned i=0; i < obj.boxes_.size(); i++) {
    boxes_[i].ixmin_ = obj.boxes_[i].ixmin_;
    boxes_[i].ixmax_ = obj.boxes_[i].ixmax_;
    boxes_[i].iymin_ = obj.boxes_[i].iymin_;
    boxes_[i].iymax_ = obj.boxes_[i].iymax_;

    boxes_[i].inc_   = obj.boxes_[i].inc_;
  }

  utc_[0] = obj.utc_[0];
  utc_[1] = obj.utc_[1];
};

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::util::operator<<(std::ostream& os, ImageHandler& obj)
{
  os << "Default output operator for class: ImageHandler" << std::endl;
  return os;
};

/**.......................................................................
 * Destructor.
 */
ImageHandler::~ImageHandler() {}

/**.......................................................................
 * Install a new image in the image buffer
 */
void ImageHandler::installNewImage(unsigned short* image)
{
  unsigned int utc[2];
  installNewImage(image, utc, 0, 0, 0);
}


/**.......................................................................
 * Install a new image in the image buffer
 */
void ImageHandler::installNewImage(unsigned short* image, unsigned int utc[2], 
				   Angle* az, Angle* el, Angle* dk)
{
  try {

    // Gain exclusive access to the image buffer.
  
    guard_.lock();
  
    // Store the deck angle

    if(dk)
      deckAngle_ = *dk;

    // Now copy the control program image to the grabber image buffer.
    // Store the max on the fly.
  
    unsigned indFrom, indTo;
    unsigned short val;
    bool first=true;
    unsigned short max=0;

    for(unsigned iy=0; iy < GRABBER_YNPIX; iy++) {
      for(unsigned ix=0; ix < GRABBER_XNPIX; ix++) {
      
	// Flip the image in y
      
	indFrom = iy * GRABBER_XNPIX + ix;
	indTo   = (GRABBER_YNPIX - iy - 1) * GRABBER_XNPIX + ix;
      
	// Write the image into our buffer

	val = image[indFrom];

	imageToStore_[indTo]   = val;
	imageToArchive_[indTo] = val;

	// Store the max on the fly

	if(val > max)
	  max = val;
      }
    }
  
    // Now write a crosshair into the image buffer for archiving.  We
    // will do this by setting a 1-pixel wide row and column to the
    // maximum pixel value found.
  
    unsigned ixmid = GRABBER_XNPIX/2;
    unsigned iymid = GRABBER_YNPIX/2;
  
    // Write out the (1-pixel, for ny odd, 2-pixel for ny even)
    // crosshair in x.  We set this to < max value so that the maximum
    // will be visible at full dynamic range
  
    for(unsigned ix=0; ix < GRABBER_XNPIX; ix++) { 
      unsigned ind = iymid*GRABBER_XNPIX + ix;
      imageToArchive_[ind] = (max > 0 ? max-1 : 0);

      if(ny_%2==0) {
	ind = (iymid-1)*GRABBER_XNPIX + ix;
	imageToArchive_[ind] = (max > 0 ? max-1 : 0);
      }
    }
  
    // Write out the 1-pixel crosshair in y.
  
    for(unsigned iy=0; iy < GRABBER_YNPIX; iy++) { 
      unsigned ind = iy*GRABBER_XNPIX + ixmid;
      imageToArchive_[ind] = (max > 0 ? max-1 : 0);

      if(nx_%2==0) {
	ind = iy*GRABBER_XNPIX + (ixmid-1);
	imageToArchive_[ind] = (max > 0 ? max-1 : 0);
      }
    }
  
    haveImage_ = true;

  } catch(...) {
    guard_.unlock();
  }

  // Relinquish exclusive access to the image buffer.
  
  guard_.unlock();
}

/**.......................................................................
 * Get statistics on the last received image
 */
ImageHandler::ImStat ImageHandler::getStats()
{
  return getStats(0, 0, nx_-1, ny_-1);
}

/**.......................................................................
 * Get stats (possibly on a subset of the image)
 */
ImageHandler::ImStat 
ImageHandler::
getStats(unsigned ixmin, unsigned iymin, unsigned ixmax, unsigned iymax)
{
  if(!haveImage_) 
    ThrowSimpleError("No image has been installed for channel: " << channel_);

  ImStat stat;

  // Flip indices if they were specified backwards

  unsigned itmp=ixmax;
  if(ixmin > ixmax) {
    ixmax = ixmin;
    ixmin = itmp;
  }

  itmp=iymax;
  if(iymin > iymax) {
    iymax = iymin;
    iymin = itmp;
  }

  try {

    lock();

    bool first=true;
    double val;
    unsigned ind;

    for(unsigned iy=iymin; iy <= iymax; iy++) {
      for(unsigned ix=ixmin; ix <= ixmax; ix++) {
      
	ind = (GRABBER_YNPIX - iy - 1) * GRABBER_XNPIX + ix;
	val = (double)(imageToStore_[ind]);
      
	// Skip this pixel if excluded by the user

	if(!usePixel_[ind])
	  continue;

	if(first) {
	  stat.max_   = val;
	  stat.min_   = val;
	  stat.ixmax_ = ix;
	  stat.iymax_ = iy;
	  stat.ixmin_ = ix;
	  stat.iymin_ = iy;
	
	  stat.mean_ = 0.0;
	  stat.rms_  = 0.0;
	  stat.snr_  = 0.0;

	  stat.n_ = 0;

	  first = false;
	}

	if(val > stat.max_) {
	  stat.max_   = val;
	  stat.ixmax_ = ix;
	  stat.iymax_ = iy;
	}

	if(val < stat.min_) {
	  stat.min_   = val;
	  stat.ixmin_ = ix;
	  stat.iymin_ = iy;
	}
      
	// Accumulate first and second moments
      
	stat.mean_ += (val - stat.mean_) / (++stat.n_);
      }
    }

    // Loop a second time to calculate the rms

    stat.n_ = 0;
    for(unsigned iy=iymin; iy <= iymax; iy++) {
      for(unsigned ix=ixmin; ix <= ixmax; ix++) {
      
	ind = (ny_ - iy - 1) * nx_ + ix;
	val = (double)(imageToStore_[ind]);
      
	// Skip this pixel if excluded by the user

	if(!usePixel_[ind])
	  continue;

	// Convert to the quantity we need to accumulate the rms (val
	// - mean)^2

	val -= stat.mean_;
	val *= val;

	// Accumulate second moments. With the first loop in y,
	// indFrom runs from 0 to NPIX, so we can safely use it as the
	// point counter.
      
	stat.rms_ += (val - stat.rms_) / (++stat.n_);
      }
    }

    // Correct for n-1

    double corr = double(stat.n_)/(stat.n_-1);
    stat.rms_ *= corr;

    stat.snr_ = stat.max_/stat.rms_;

  } catch(...) {

    // Unlock this object if an error occurs

    unlock();

    // Now throw an error

    ThrowError("Error occurred during stat calculation");
  }

  unlock();

  return stat;
}

/**.......................................................................
 * Return the offset of the max in x and y, converted to horizontal
 * and vertical offsets, in degrees
 */
void ImageHandler::
getOffsetInfo(gcp::util::Angle& xSkyOffset, gcp::util::Angle& ySkyOffset, 
	      unsigned& ipeak, unsigned&  jpeak)
{
  // First re-compute statistics on the image

  ImStat stat = getStats();

  ipeak = stat.ixmax_;
  jpeak = stat.iymax_;

  pixelToSkyOffset(ipeak, jpeak, xSkyOffset, ySkyOffset);
}

/**.......................................................................
 * Return information about the max
 */
void ImageHandler::getPeakInfo(double& peak, double& snr)
{
  // First re-compute statistics on the image

  ImStat stat = getStats();

  peak = stat.max_;
  snr  = stat.snr_;
}

/**.......................................................................
 * Public method to set the deck angle rotation sense
 */
void ImageHandler::setDeckAngleRotationSense(RotationSense sense)
{
  deckAngleRotationSense_ = sense;
}

/**.......................................................................
 * Public method to set the optical camera image directions
 */
void ImageHandler::setXImDir(ImDir dir)
{
  xImDir_ = dir;
}

/**.......................................................................
 * Public method to set the optical camera image directions
 */
void ImageHandler::setYImDir(ImDir dir)
{
  yImDir_ = dir;
}

/**.......................................................................
 * Public method to reset the optical camera FOV
 */
void ImageHandler::setFov(const Angle& fov) 
{
  fov_ = fov;
}

/*
 * Public method to set the optical camera FOV
 */
void ImageHandler::setAspectRatio(double aspectRatio)
{
  aspectRatio_ = (aspectRatio > 0.0 ? aspectRatio : defaultAspectRatio_);
}

/**.......................................................................
 * Public method to set the optical camera collimation
 */
void ImageHandler::setRotationAngle(const gcp::util::Angle& rotationAngle)
{
  rotationAngle_ = rotationAngle;
}

/**.......................................................................
 * Public method to set the deck angle
 */
void ImageHandler::setDeckAngle(const gcp::util::Angle& deckAngle)
{
  deckAngle_ = deckAngle;
}

/**.......................................................................
 * Public method to set the number of images to combine
 */
void ImageHandler::setNCombine(unsigned nCombine)
{
  nCombine_ = nCombine;
}

/**.......................................................................
 * Public method to set the channel
 */
void ImageHandler::setChannel(gcp::grabber::Channel::FgChannel channel)
{
  channel_ = channel;
}

/**.......................................................................
 * Public method to set the channel
 */
void ImageHandler::setChannel(unsigned channel)
{
  channel_ = gcp::grabber::Channel::intToChannel(channel);
}
      
/**.......................................................................
 * Public method to set the flatfield
 */
void ImageHandler::setFlatfieldType(unsigned flatfieldType)
{
  flatfieldType_ = flatfieldType;
}

/**.......................................................................
 * Convert from grabber pixel offset to sky offset
 */
void ImageHandler::pixelToSkyOffset(unsigned ix, unsigned iy, Angle& xsky, Angle& ysky)
{
  // Convert from frame-grabber pixel number to an offset, in
  // fractional pixels
  
  double xpix =   xa_ + ix * dx_;
  
  // We flipped the image, so flip the sign of the y-offset too
    
  double ypix = -(ya_ + iy * dy_);
    
  // To correct to the star position, we may have to move the
  // telescope in the opposite direction of the offsets.
    
  xpix *=  (xImDir_ == UPRIGHT ? 1 : -1);  
  ypix *=  (yImDir_ == UPRIGHT ? 1 : -1);  

  // Rescale to degrees, using the current FOV and aspect ratio
  
  double xdeg = xpix * fov_.degrees()/GRABBER_XNPIX;
  double ydeg = ypix * fov_.degrees()/GRABBER_YNPIX * aspectRatio_;

  // Now calculate the transformation between pixel coordinates and
  // sky coordinates

  gcp::util::Angle angle;
  gcp::util::Angle deckAngle;
  
  if(deckAngleRotationSense_ == CW)
    deckAngle.setRadians(+deckAngle_.radians());
  else
    deckAngle.setRadians(-deckAngle_.radians());
  
  // The rotation angle is always specified as a positive CW angle, so
  // simply add the two
    
  angle = rotationAngle_ + deckAngle;
  double rad = angle.radians();

  xsky.setDegrees(xdeg * cos(rad) - ydeg * sin(rad));
  ysky.setDegrees(xdeg * sin(rad) + ydeg * cos(rad));
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::util::operator<<(ostream& os, ImageHandler::ImStat& stat)
{
  os << "ixmax= " << stat.ixmax_ << std::endl;
  os << "iymax= " << stat.iymax_ << std::endl;
  os << "min  = " << stat.min_   << std::endl;
  os << "max  = " << stat.max_   << std::endl;
  os << "mean = " << stat.mean_  << std::endl;
  os << "rms  = " << stat.rms_   << std::endl;
  os << "snr  = " << stat.snr_   << std::endl;
  os << "n    = " << stat.n_     << std::endl;

  return os;
}


void ImageHandler::addIncludeBox(unsigned ixmin, unsigned iymin, 
				 unsigned ixmax, unsigned iymax)
{
  Box box(ixmin, iymin, ixmax, iymax, true);

  for(unsigned i=0; i < boxes_.size(); i++) {

    if(boxes_[i].inc_) {
      boxes_[i] = box;
      updatePixelFlags();
      return;
    }

  }

  // If no inc box was already found, add this new one

  boxes_.push_back(box);
  updatePixelFlags();
}

void ImageHandler::addExcludeBox(unsigned ixmin, unsigned iymin, 
				 unsigned ixmax, unsigned iymax)
{
  Box box(ixmin, iymin, ixmax, iymax, false);
  boxes_.push_back(box);
  updatePixelFlags();
}

void ImageHandler::deleteNearestBox(unsigned ix, unsigned iy)
{
  // If no boxes are defined, return

  if(boxes_.size() == 0)
    return;

  // Else iterate over boxes, looking for the closest one

  std::vector<ImageHandler::Box>::iterator iBox;
  std::vector<ImageHandler::Box>::iterator boxMin;

  double distMin, dist;
  for(iBox=boxes_.begin(); iBox != boxes_.end(); iBox++) {

    // If this is the first box, store it as the current minimum

    if(iBox == boxes_.begin()) {

      dist    = iBox->shortestDistanceFrom(ix, iy);
      distMin = dist;
      boxMin  = iBox;

      // Else check if the current box is closer than the last one

    } else {

      dist = iBox->shortestDistanceFrom(ix, iy);

      if(dist < distMin) {
	distMin = dist;
	boxMin = iBox;
      }
    }
  }

  // Remove the closest box we found

  boxes_.erase(boxMin);
  updatePixelFlags();
}

void ImageHandler::deleteAllBoxes()
{
  boxes_.resize(0);
  updatePixelFlags();
}

/**.......................................................................
 * Update the pixel flags in response to a new box configuration
 */
void ImageHandler::updatePixelFlags()
{
  unsigned ind;

  for(unsigned iy=0; iy < ny_; iy++) {
    for(unsigned ix=0; ix < nx_; ix++) {
      ind = (GRABBER_YNPIX - iy - 1) * GRABBER_XNPIX + ix;
      usePixel_[ind] = !pixelIsExcluded(ix, iy);
    }
  }
}

/**.......................................................................
 * Return true if a pixel is excluded by any box
 */
bool ImageHandler::pixelIsExcluded(unsigned ix, unsigned iy)
{
  // Iterate over all known boxes

  for(unsigned i=0; i < boxes_.size(); i++) {

    // If this is an include box (there can be only one), return tru
    // if this point lies outside of it

    if(boxes_[i].inc_) {

      if(!((ix >= boxes_[i].ixmin_ && ix <= boxes_[i].ixmax_) && 
	   (iy >= boxes_[i].iymin_ && iy <= boxes_[i].iymax_))) {
	return true;
      }
      
      // Else if this is an exclude box, return true if this point
      // lies inside it
      
    } else {
      if((ix >= boxes_[i].ixmin_ && ix <= boxes_[i].ixmax_) && 
	 (iy >= boxes_[i].iymin_ && iy <= boxes_[i].iymax_)) {
	return true;
      }
    }
  }
  
  // Else no box excluded this point

  return false;
}

/**.......................................................................
 * Constructor for Box
 */
ImageHandler::Box::Box()
{
  ixmin_ = 0;
  ixmax_ = GRABBER_XNPIX-1;
  iymin_ = 0;
  iymax_ = GRABBER_YNPIX-1;

  inc_   = true;
};

/**.......................................................................w
 * Constructor with assignment
 */
ImageHandler::Box::Box(unsigned ixmin, unsigned iymin, 
		       unsigned ixmax, unsigned iymax,
		       bool inc)
{
  ImageHandler::rectifyCoords(ixmin, iymin, ixmax, iymax);

  ixmin_ = ixmin;
  ixmax_ = ixmax;
  iymin_ = iymin;
  iymax_ = iymax;

  inc_ = inc;
};

/**.......................................................................
 * Copy constructors
 */
ImageHandler::Box::Box(const Box& box) 
{
  *this = (Box&)box;
};

ImageHandler::Box::Box(Box& box) 
{
  *this = box;
};

/**.......................................................................
 * Assignment operators
 */
void ImageHandler::Box::operator=(const ImageHandler::Box& box) 
{
  *this = (ImageHandler::Box&)box;
};

void ImageHandler::Box::operator=(ImageHandler::Box& box) 
{
  ixmin_ = box.ixmin_;
  ixmax_ = box.ixmax_;
  iymin_ = box.iymin_;
  iymax_ = box.iymax_;

  inc_   = box.inc_;
};

/**.......................................................................
 * Check passed limits.  Rearrange if min > max, and truncate pixel
 * values that exceed the image size
 */
void ImageHandler::rectifyCoords(unsigned& ixmin, unsigned& iymin, 
				 unsigned& ixmax, unsigned& iymax)
{
  unsigned ixmn, ixmx, iymn, iymx;

  ixmn = (ixmin < ixmax) ? ixmin : ixmax;
  ixmx = (ixmin < ixmax) ? ixmax : ixmin;

  iymn = (iymin < iymax) ? iymin : iymax;
  iymx = (iymin < iymax) ? iymax : iymin;

  if(ixmn < 0)
    ixmn = 0;

  if(iymn < 0)
    iymn = 0;

  if(ixmx > GRABBER_XNPIX-1)
    ixmx = GRABBER_XNPIX-1;

  if(iymx > GRABBER_YNPIX-1)
    iymx = GRABBER_YNPIX-1;

  ixmin = ixmn;
  ixmax = ixmx;

  iymin = iymn;
  iymax = iymx;
}

/**.......................................................................
 * Return the shortest distance from this box
 */
double ImageHandler::Box::shortestDistanceFrom(unsigned ix, unsigned iy)
{
  double dx, dy;
  double dix = (double)ix;
  double diy = (double)iy;
  double dixmin = (double)ixmin_;
  double dixmax = (double)ixmax_;
  double diymin = (double)iymin_;
  double diymax = (double)iymax_;

  switch (locationOf(ix, iy)) {
  case ABOVE_LEFT:
    dx = (double)(dix - dixmin);
    dy = (double)(diy - diymax);
    break;
  case ABOVE:
    dx = 0.0;
    dy = (double)(diy - diymax);
    break;
  case ABOVE_RIGHT:
    dx = (double)(dix - dixmax);
    dy = (double)(diy - diymax);
    break;
  case RIGHT:
    dx = (double)(dix - dixmax);
    dy = 0.0;
    break;
  case BELOW_RIGHT:
    dx = (double)(dix - dixmax);
    dy = (double)(diy - diymin);
    break;
  case BELOW:
    dx = 0.0;
    dy = (double)(diy - diymin);
    break;
  case BELOW_LEFT:
    dx = (double)(dix - dixmin);
    dy = (double)(diy - diymin);
    break;
  case LEFT:
    dx = (double)(dix - dixmin);
    dy = 0.0;
    break;
  default:
    {
      double dl, dt, dr, db;
      dl = (double)(dix - dixmin);
      dt = (double)(diy - diymax);
      dr = (double)(dix - dixmax);
      db = (double)(diy - diymin);

      dx = fabs(db) < fabs(dt) ? db : dt;
      dx = fabs(dx) < fabs(dl) ? dx : dl;
      dx = fabs(dx) < fabs(dr) ? dx : dr;
      dy = 0.0;
    }
    break;
  }

  return sqrt(dx*dx + dy*dy);
}

/**.......................................................................
 * Return the orientation of a pixel wrt to this box
 */
ImageHandler::Box::Location 
ImageHandler::Box::locationOf(unsigned ix, unsigned iy)
{
  // Left of the box

  if(ix <= ixmin_) {

    if(iy >= iymax_) {
      return ABOVE_LEFT;
    } else if(iy <= iymin_) {
      return BELOW_LEFT;
    } else {
      return LEFT;
    }

    // Right of the box

  } else if(ix >= ixmax_) {

    if(iy >= iymax_) {
      return ABOVE_RIGHT;
    } else if(iy <= iymin_) {
      return BELOW_RIGHT;
    } else {
      return RIGHT;
    }

    // Center of the box

  } else {

    if(iy >= iymax_) {
      return ABOVE;
    } else if(iy <= iymin_) {
      return BELOW;
    } else {
      return INSIDE;
    }

  }

}
