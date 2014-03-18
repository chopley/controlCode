// $Id: Image.h,v 1.1.1.1 2009/07/06 23:57:18 eml Exp $

#ifndef GCP_GRABBER_IMAGE_H
#define GCP_GRABBER_IMAGE_H

/**
 * @file Image.h
 * 
 * Tagged: Fri Jan 11 15:59:26 NZDT 2008
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:18 $
 * 
 * @author SPT
 */

#include <iostream>
#include <valarray>

#include "gcp/grabber/common/Channel.h"

namespace gcp {
  namespace grabber {

    class Image {
    public:

      /**
       * Constructor.
       */
      Image();

      /**
       * Destructor.
       */
      virtual ~Image();

      /**
       * Randomly generate a field of stars
       */
      void addFakeImage();
      
      /**
       * Add a ramp
       */
      void addRampImage();
      
      /**
       * Copy the float version of the image into the short buffer,
       * and perform any other finalizing steps before sending off the
       * image.
       */
      void finalize();
      
      /**
       * Flatfield an image by subrtacting off the mean of each row
       */
      void flatfield();

      void rowFlatfield();
      void imageFlatfield();
      
      float getHotPixels(std::valarray<float>& image, float nsigma);

      float getMaskedMean(std::valarray<float>& image);

      float getMaskedSigma(std::valarray<float>& image, float mean);

      unsigned outlierReject(std::valarray<float>& image, 
			     float& mean, float& sigma,
			     float nsigma);

      // Store the current image as the flatfield

      void storeCurrentImageAsFlatfield();

    public:

      // A buffer where we will store the image just received from the
      // frame grabber

      std::valarray<unsigned short> image_;

      // A flat-field image

      std::valarray<float> flatfieldImage_;

      // A buffer where images can be integrated

      std::valarray<float> intImage_;

      // Masks used for nulling bad pixels

      std::valarray<bool> pixelMask_;

      // Outlier rejection

      unsigned nSigma_;

      // The type of flatfielding to perform for this image

      unsigned flatfield_;

      // True if a flatfield image has been stored

      bool haveFlatfieldImage_;

      // The maximum value of the flatfield image

      float maxFlatfieldVal_;

      // The channel this image is associated with

      Channel::FgChannel channel_;

      // The number of frames to combine for this channel

      unsigned int nCombine_;

    }; // End class Image

  } // End namespace grabber
} // End namespace gcp



#endif // End #ifndef GCP_GRABBER_IMAGE_H
