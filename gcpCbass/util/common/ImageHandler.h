// $Id: ImageHandler.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_IMAGEHANDLER_H
#define GCP_UTIL_IMAGEHANDLER_H

/**
 * @file ImageHandler.h
 * 
 * Tagged: Fri Jun  8 12:35:42 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */

#include <iostream>
#include <vector>

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Mutex.h"

#include "gcp/grabber/common/Channel.h"

#include "gcp/control/code/unix/libunix_src/common/control.h"
#include "gcp/control/code/unix/libunix_src/common/optcam.h"

namespace gcp {
  namespace util {

    class ImageHandler {
    public:

      // Object to encapsulate image statistics

      struct ImStat {
	unsigned ixmax_;
	unsigned iymax_;

	unsigned ixmin_;
	unsigned iymin_;
	
	double min_;
	double max_;

	double mean_;
	double rms_;
	double snr_;

	unsigned n_;

	friend std::ostream& operator<<(std::ostream& os, ImStat& stat);
      };

      // Default optical camera field of view

      static const gcp::util::Angle defaultFov_;

      // Default Aspect ratio

      static const double defaultAspectRatio_;

      // Default rotation angle for the camera

      static const gcp::util::Angle defaultRotationAngle_;

      // For experiments with a "deck" axis, this will store the
      // default sense of the deck rotaion (ie, CW or CCW)

      static const gcp::control::RotationSense defaultDeckAngleRotationSense_;

      // Default channel

      static const gcp::grabber::Channel::FgChannel defaultChannel_;

      // The Default flatfielding type

      static const unsigned defaultFlatfieldType_;

      // The number of frames to combine 

      static const unsigned defaultNcombine_;

      // The sense of the x,y-axis increment

      static const gcp::control::ImDir defaultXImDir_;
      static const gcp::control::ImDir defaultYImDir_;

      /**
       * Constructor.
       */
      ImageHandler();

      /**
       * Copy Constructor.
       */
      ImageHandler(const ImageHandler& objToBeCopied);

      /**
       * Copy Constructor.
       */
      ImageHandler(ImageHandler& objToBeCopied);

      /**
       * Const Assignment Operator.
       */
      void operator=(const ImageHandler& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(ImageHandler& objToBeAssigned);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, ImageHandler& obj);

      /**
       * Destructor.
       */
      virtual ~ImageHandler();

      //------------------------------------------------------------
      // The following functions are virtual, so that inheritors can
      // define what else, if anything, should happen when these
      // parameters are reset
      //------------------------------------------------------------

      // Public method to set the deck angle rotation sense

      virtual void setDeckAngleRotationSense(gcp::control::RotationSense sense);

      // Public method to set the number of images to combine

      virtual void setNCombine(unsigned ncombine);

      // Public method to set the channel

      virtual void setChannel(gcp::grabber::Channel::FgChannel channel);

      virtual void setChannel(unsigned channel);
      
      // Public method to set the flatfield

      virtual  void setFlatfieldType(unsigned flatfieldType);

      // Public method to set the optical camera image directions

      virtual void setXImDir(gcp::control::ImDir dir);

      
      // Public method to set the optical camera image directions

      virtual void setYImDir(gcp::control::ImDir dir);

      // Public method to reset the optical camera FOV

      virtual void setFov(const gcp::util::Angle& fov);
      
      // Public method to set the optical camera FOV

      virtual void setAspectRatio(double aspectRatio);
      
      // Public method to set the optical camera collimation

      virtual void setRotationAngle(const gcp::util::Angle& rotationAngle);

      // Public method to set the deck angle

      virtual void setDeckAngle(const gcp::util::Angle& deckAngle);

      // Install a new image in the image buffer

      virtual void installNewImage(unsigned short* image);

      virtual void installNewImage(unsigned short* image, unsigned int utc[2], 
				   gcp::util::Angle* az, gcp::util::Angle* el, 
				   gcp::util::Angle* dk=0);

      // Get statistics on the last received image

      ImStat getStats();
      virtual ImStat getStats(unsigned ixmin, unsigned iymin, 
			      unsigned ixmax, unsigned iymax);

      // Return the offset of the peak in x and y, converted to
      // horizontal and vertical offsets, in degrees

      void getOffsetInfo(gcp::util::Angle& horiz, gcp::util::Angle& vert, 
			 unsigned& ipeak, unsigned&  jpeak);

      /**
       * Return information about the peak
       */
      void getPeakInfo(double& peak, double& snr);

      void addIncludeBox(unsigned ixmin, unsigned iymin, 
			 unsigned ixmax, unsigned iymax);

      void addExcludeBox(unsigned ixmin, unsigned iymin, 
			 unsigned ixmax, unsigned iymax);

      void deleteNearestBox(unsigned ix, unsigned iy);
      void deleteAllBoxes();

      class Box {
      public:

	// Enumerate possible positions of a point w.r.t. this box

	enum Location {
	  INSIDE,

	  ABOVE_LEFT,
	  ABOVE,
	  ABOVE_RIGHT,

	  RIGHT,

	  BELOW_RIGHT,
	  BELOW,
	  BELOW_LEFT,

	  LEFT
	};

	// The boundaries of this box

	unsigned ixmin_;
	unsigned ixmax_;
	unsigned iymin_;
	unsigned iymax_;

	// True if this box defines an area that should be included
	// False if this box defines an area that should be excluded

	bool inc_;

	// Return the shortest distance from this box
	
	double shortestDistanceFrom(unsigned ix, unsigned iy);

	// Return the orientation of a pixel wrt to this box

	Location locationOf(unsigned ix, unsigned iy);

	// Check passed limits.  Rearrange if min > max, and truncate
	// pixel values that exceed the image size

	void rectifyCoords(unsigned& ixmin, unsigned& iymin, 
			   unsigned& ixmax, unsigned& iymax);

	// Constructors and assignment operators

	Box();

	Box(unsigned ixmin, unsigned ixmax,
	    unsigned iymin, unsigned iymax,
	    bool inc);

	Box(const Box& box);

	Box(Box& box);

	void operator=(const Box& box);

	void operator=(Box& box);
      };

      std::vector<Box> boxes_;

      gcp::util::Angle fov_;
      double aspectRatio_;
      gcp::util::Angle deckAngle_;
      gcp::util::Angle rotationAngle_;
      gcp::control::RotationSense deckAngleRotationSense_;
      gcp::grabber::Channel::FgChannel channel_;
      unsigned flatfieldType_;
      unsigned nCombine_;
      gcp::control::ImDir xImDir_;
      gcp::control::ImDir yImDir_;

      std::vector<unsigned short> imageToArchive_;

    protected:

      bool haveImage_;               // True when an image has been installed
      double dx_;                    // The x-increment of a pixel 
      double dy_;                    // The y-increment of a pixel 
      double xa_;                    // The x-coordinate of the blc of the image 
      double ya_;                    // The y-coordinate of the blc of the image 
      double xb_;                    // The x-coordinate of the trc of the image 
      double yb_;                    // The y-coordinate of the trc of the image 

      // Size of the image, in pixels

      unsigned nx_;
      unsigned ny_;

      bool usePixel_[GRABBER_IM_SIZE];
      unsigned short imageToStore_[GRABBER_IM_SIZE];
      unsigned int utc_[2];
      
      //------------------------------------------------------------
      // Public access to locks on this object
      //------------------------------------------------------------

      void lock() {
	guard_.lock();
      }

      void unlock() {
	guard_.unlock();
      }

      // Convert from util pixel offset to sky offset

      void pixelToSkyOffset(unsigned ix, unsigned iy, gcp::util::Angle& xsky, gcp::util::Angle& ysky);

    private:

      gcp::util::Mutex guard_;

      // Initialize image parameters to defaults

      void initializeCameraDefaults();

      // Initialize image statistics to defaults

      void initializeImageStats();

      void initialize();

      void updatePixelFlags();

      bool pixelIsExcluded(unsigned ix, unsigned iy);

      static void rectifyCoords(unsigned& ixmin, unsigned& ixmax,
				unsigned& iymin, unsigned& iymax);

    }; // End class ImageHandler

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_IMAGEHANDLER_H
