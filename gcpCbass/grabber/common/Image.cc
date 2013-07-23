#include "gcp/grabber/common/Flatfield.h"
#include "gcp/grabber/common/Image.h"

#include "gcp/control/code/unix/libunix_src/common/optcam.h"
#include "gcp/control/code/unix/libunix_src/common/scanner.h"

#include <iostream>

using namespace std;

using namespace gcp::grabber;

/**.......................................................................
 * Constructor.
 */
Image::Image()
{
  nSigma_ = 10;

  image_.resize(GRABBER_IM_SIZE);
  intImage_.resize(GRABBER_IM_SIZE);
  flatfieldImage_.resize(GRABBER_IM_SIZE);
  
  pixelMask_.resize(GRABBER_IM_SIZE);
  pixelMask_ = 1;

  nCombine_ = 1;
 
  haveFlatfieldImage_ = false;
}

/**.......................................................................
 * Destructor.
 */
Image::~Image() {}

/**.......................................................................
 * Store the current image as the flatfield
 */
void Image::storeCurrentImageAsFlatfield()
{
  // Scan the image for hot pixels.  These will subsequently be set to
  // the mean in returned images

  float mean = getHotPixels(intImage_, nSigma_);

  // Now copy the image into the flatfield buffer

  float* srcPtr  = &intImage_[0];
  float* destPtr = &flatfieldImage_[0];

  haveFlatfieldImage_ = true;
  maxFlatfieldVal_    = intImage_[0];

  for(unsigned iPix = 0; iPix < flatfieldImage_.size(); iPix++) {

    destPtr[iPix] = pixelMask_[iPix] ? srcPtr[iPix] : mean;

    // Compute the max

    if(destPtr[iPix] > maxFlatfieldVal_)
      maxFlatfieldVal_ = destPtr[iPix];
  }
}

/**.......................................................................
 * Scan the passed image for hot pixels.
 */
float Image::getHotPixels(std::valarray<float>& image, float nsigma)
{
  unsigned nRejected = 0;
  bool first = true;

  COUT("Inside gethotpixels");

  // Initialize the pixel masks to accept all pixels

  pixelMask_  = true;

  // A first call to compute the mean and sigma

  float mean  = getMaskedMean(intImage_);
  float sigma = getMaskedSigma(intImage_, mean);

  COUT("Mean is: " << mean);
  COUT("Sigma is: " << sigma);

  // Now iterate, until no more pixels are rejected

  //  do {
    nRejected = outlierReject(image, mean, sigma, nsigma);
    COUT("Rejected: " << nRejected << " mean = " << mean);

    //  } while(nRejected > 0);
  
  // Return the mean of the final unrejected pixels

  return mean;
}

/**.......................................................................
 * Iterate through the image, rejecting pixel values greater than
 * nsigma from the mean
 */
float Image::getMaskedMean(std::valarray<float>& image)
{  
  float mean=0.0;
  unsigned nPix=0;

  // Iterate over pixels, rejecting points that are > nsigma from the
  // old mean.  At the same time, we compute the new mean from the
  // unrejected pixels

  for(unsigned iPix = 0; iPix < image.size(); iPix++) {

    // Only evaluate this pixel if it isn't already masked

    if(pixelMask_[iPix]) {

	// Calculate the running mean

	mean += (image[iPix] - mean)/(nPix+1);
	++nPix;
    }
  }

  return mean;
}

/**.......................................................................
 * Calculate the rms over unmasked pixels
 */
float Image::getMaskedSigma(std::valarray<float>& image, float mean)
{  
  // Now iterate a second time to compute the rms of the unrejected
  // points.

  unsigned nPix =   0;
  float sigma   = 0.0;
  float val;

  for(unsigned iPix = 0; iPix < image.size(); iPix++) {

    // Only evaluate this pixel if it isn't already masked

    if(pixelMask_[iPix]) {
      
      val = image[iPix] - mean;
      val *= val;

      // Compute the running mean of the second moment

      sigma += (val - sigma)/(nPix+1);
      ++nPix;
    }
  }

  // Now correct for n-1

  sigma = nPix*(sigma/(nPix-1));

  return sigma;
}

/**.......................................................................
 * Iterate through the image, rejecting pixel values greater than
 * nsigma from the mean
 */
unsigned Image::outlierReject(std::valarray<float>& image, 
				float& mean, float& sigma,
				float nsigma)
{  
  float oldMean  = mean;
  float oldSigma = sigma;

  unsigned nPix=0, nRejected=0;

  // Iterate over pixels, rejecting points that are > nsigma from the
  // old mean.  At the same time, we compute the new mean from the
  // unrejected pixels

  mean = 0.0;
  for(unsigned iPix = 0; iPix < image.size(); iPix++) {

    // Only evaluate this pixel if it isn't already masked

    if(pixelMask_[iPix]) {

      if(fabs(image[iPix]-oldMean) > nsigma*oldSigma) {

	pixelMask_[iPix]  = false;
	++nRejected;

      } else {

	// Calculate the running mean

	mean += (image[iPix] - mean)/(nPix+1);
	++nPix;
      }

    }
  }

  // Now iterate a second time to compute the rms of the unrejected
  // points.

  nPix=0;
  float val;
  sigma = 0.0;
  for(unsigned iPix = 0; iPix < image.size(); iPix++) {

    // Only evaluate this pixel if it isn't already masked

    if(pixelMask_[iPix]) {
      
      val = image[iPix] - mean;
      val *= val;

      // Compute the running mean of the second moment

      sigma += (val - sigma)/(nPix+1);
      ++nPix;
    }
  }

  // Now correct for n-1

  sigma = nPix*(sigma/(nPix-1));

  // Return the number of points we rejected on this iteration

  return nRejected;
}

/**.......................................................................
 * Randomly generate a field of stars
 */
void Image::addFakeImage()
{
  static unsigned ncount = 0;

  // Randomly generate some sources

  int i,irow,icol,ind;
  float xval,yval,val;

  // Randomly generate an integer between 1 and 10

  int nran = 1+(int)(10.0*rand()/(RAND_MAX+1.0));
  int iran[10],jran[10];
  float fluxran[10];

 // randomly generate positions for the sources

  COUT("Randomly generating fluxes for : " << nran);
  for(i=0;i < nran;i++) {
    iran[i] = (int)((GRABBER_XNPIX-1.0)*rand()/(RAND_MAX+1.0));
    jran[i] = (int)((GRABBER_YNPIX-1.0)*rand()/(RAND_MAX+1.0));

    // And randomly generate a flux for this source, between 10 and 100

    fluxran[i] = 1.0+(10.0*rand()/RAND_MAX);
  }

  for(irow=0;irow < GRABBER_YNPIX;irow++)

    for(icol=0;icol < GRABBER_XNPIX;icol++) {
      ind = irow*GRABBER_XNPIX + icol;
      val = 0.0;

      for(i=0;i < nran;i++) {
        xval = iran[i] - irow;
        yval = jran[i] - icol;
        val += fluxran[i]*exp(-(xval*xval + yval*yval)/(2*10*10));
      }

      // Now add in the star, but with a ramp
      
      intImage_[ind] = val + irow*0.001;
    }

#if 0
  static unsigned x[5] = {1,   200, 34, 56, 90};
  static unsigned y[5] = {113, 400, 32, 10, 76};

  for(i=0;i < 5;i++) {
    ind = x[i]*GRABBER_XNPIX + y[i];
    intImage_[ind] += 255;
  }
#endif

  ++ncount;
}

/**.......................................................................
 * Add a ramp
 */
void Image::addRampImage()
{
  unsigned ind;

  for(unsigned irow=0;irow < GRABBER_YNPIX;irow++)
    for(unsigned icol=0;icol < GRABBER_XNPIX;icol++) {
      ind = irow*GRABBER_XNPIX + icol;
      intImage_[ind] = irow;
    }
}

/**.......................................................................
 * Copy the float version of the image into the short buffer, and
 * perform any other finalizing steps before sending off the image.
 */
void Image::finalize() 
{
  if(flatfield_ != Flatfield::FLATFIELD_NONE) 
    flatfield();

  // Convert the image to the appropriate output format
  
  for(unsigned iImage=0; iImage < GRABBER_IM_SIZE; iImage++) 
    image_[iImage] = (intImage_[iImage] < 0.0) ? 0 : 
      (unsigned short) intImage_[iImage];
}

/**.......................................................................
 * Flatfield an image by subtracting off the mean of each row
 */
void Image::flatfield()
{
  COUT("Inside flatfieldImage");

  switch(flatfield_) {
  case Flatfield::FLATFIELD_ROW:
    rowFlatfield();
    break;
  case Flatfield::FLATFIELD_IMAGE:
    imageFlatfield();
    break;
  default:
    break;
  }
}

void Image::rowFlatfield()
{
  float val, mean;
  
  // Iterate over each row of the image

  for(unsigned iRow=0; iRow < GRABBER_YNPIX; iRow++) {

    // For each row, calculate the mean over the row

    mean = 0.0;
    for(unsigned iCol=0; iCol < GRABBER_XNPIX; iCol++) {
      val = intImage_[iRow * GRABBER_XNPIX + iCol];
      mean += (val-mean)/(iCol+1);
    }

    // Now subtract it off of this row

    for(unsigned iCol=0; iCol < GRABBER_XNPIX; iCol++) 
      intImage_[iRow * GRABBER_XNPIX + iCol] -= mean;
  }
}

/**.......................................................................
 * Divide the image by the flatfield image
 */
void Image::imageFlatfield()
{
  if(!haveFlatfieldImage_)
    return;

  // Get the mean of the integrated image.  This will replace the
  // pixel values for masked pixels

  float mean  = getMaskedMean(intImage_);

  COUT("Masked mean is: " << mean);
  COUT("Max flatfield is: " << maxFlatfieldVal_);

  // Iterate over each row of the image

  float* imPtr = &intImage_[0];
  float* ffPtr = &flatfieldImage_[0];

  for(unsigned iPix=0; iPix < intImage_.size(); iPix++) {

    imPtr[iPix] = 
      (pixelMask_[iPix] ? imPtr[iPix] : mean)/ffPtr[iPix] * maxFlatfieldVal_;

  }
}

