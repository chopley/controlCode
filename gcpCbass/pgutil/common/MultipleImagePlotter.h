// $Id: MultipleImagePlotter.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_GRABBER_MULTIPLEIMAGEPLOTTER_H
#define GCP_GRABBER_MULTIPLEIMAGEPLOTTER_H

/**
 * @file MultipleImagePlotter.h
 * 
 * Tagged: Tue Jan 15 15:56:46 NZDT 2008
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author SPT
 */

#include <iostream>

#include "gcp/pgutil/common/ImagePlotter.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/ImageHandler.h"

#include "gcp/control/code/unix/viewer_src/color_tab.h"

namespace gcp {
  namespace grabber {

    class MultipleImagePlotter {
    public:

      // Constructor.

      MultipleImagePlotter();

      // Destructor.

      virtual ~MultipleImagePlotter();

      // Set the current image to the specified index

      void setCurrentImage(unsigned short imageIndex);
      unsigned short getCurrentChannel();

      // Return the current Image

      ImagePlotter& currentImage();

      // Return the requested image

      ImagePlotter& getImage(unsigned iChan);

      // Open a new pgplot device

      void open(char* device);
      void draw(bool cmapUpdate=false);
      void drawDouble();
      void setGridInterval(gcp::util::Angle& interval);

      // Install a new image in the image buffer

      void installNewImage(unsigned short channel, unsigned short* image);

      void testDraw();

      // Given a pixel, in grabber pixel orientation, return the world
      // coordinate

      void pixelToWorld(unsigned ix, unsigned iy, double& x, double& y);

      // Convert from world to pixel coordinate.  If truncate = true,
      // and the world coordinate lies outside the image boundary, the
      // returned pixel values will be truncated to the nearest valid
      // pixel.  If truncate = false, this function will throw an
      // error if the world coordinate lies outside the image

      void worldToPixel(double x, double y, unsigned& ix, unsigned& iy, 
			bool truncate=false);

      void worldToSkyOffset(double x, double y, 
			    gcp::util::Angle& xsky, gcp::util::Angle& ysky);

      // Reset plot limits
      
      void setPlotBoundaries();
      
      // Calculate plot limits

      void calculatePlotLimits();

      // Set a peak location (in pixel units)

      void setPeak(unsigned ix, unsigned iy);

      bool& doGrid();

      bool& doCompass();

      bool& doBullseye();

      bool& doCrosshair();

      void setContrast(double contrast = ImagePlotter::defaultContrast_);
      void setBrightness(double brightness = ImagePlotter::defaultBrightness_);

      void installColormap(std::string cmap);

      // Fiddle the contrast and brightness of a frame grabber image

      void fiddleContrast(double xa, double ya);

      void setDisplayedRange(double xa, double xb, double ya, double yb);
      void setDisplayedRange();

      void initialize();
      void getGreyscale();
      void drawImage(bool cmapUpdate=false);
      void drawGrid();
      void drawBullseye();
      void drawCompass();
      void drawCrosshair();

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

      // Get statistics on the last received image

      gcp::util::ImageHandler::ImStat getStats();

      gcp::util::ImageHandler::ImStat 
	getStats(unsigned ixmin, unsigned iymin, 
		 unsigned ixmax, unsigned iymax);

      // Return the offset of the peak in x and y, converted to
      // horizontal and vertical offsets, in degrees

      void getOffsetInfo(gcp::util::Angle& horiz, gcp::util::Angle& vert, 
			 unsigned& ipeak, unsigned&  jpeak);

      // Return information about the peak

      void getPeakInfo(double& peak, double& snr);


    private:

      std::vector<ImagePlotter> images_;

      unsigned imageIndex_;

    }; // End class MultipleImagePlotter
    
  } // End namespace grabber
} // End namespace gcp



#endif // End #ifndef GCP_GRABBER_MULTIPLEIMAGEPLOTTER_H
