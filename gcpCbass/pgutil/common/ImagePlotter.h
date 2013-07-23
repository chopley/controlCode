// $Id: ImagePlotter.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_GRABBER_IMAGEPLOTTER_H
#define GCP_GRABBER_IMAGEPLOTTER_H

/**
 * @file ImagePlotter.h
 * 
 * Tagged: Mon Jun 11 11:42:18 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author Erik Leitch
 */

#include <iostream>

#include "gcp/util/common/ImageHandler.h"

#include "gcp/util/common/Angle.h"

#include "gcp/control/code/unix/viewer_src/color_tab.h"

namespace gcp {
  namespace grabber {

    class ImagePlotter : public gcp::util::ImageHandler {
    public:

      /**
       * Constructor.
       */
      ImagePlotter();

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, ImagePlotter& obj);

      /**
       * Destructor.
       */
      virtual ~ImagePlotter();

      // Open a new pgplot device

      void open(char* device);

      void draw(bool cmapUpdate=false);
      void drawDouble();

      void setGridInterval(gcp::util::Angle& interval);

      // Install a new image in the image buffer

      void installNewImage(unsigned short* image);

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

      bool& doGrid() {
	return doGrid_;
      }

      bool& doCompass() {
	return doCompass_;
      }

      bool& doBullseye() {
	return doBullseye_;
      }

      bool& doCrosshair() {
	return doCrosshair_;
      }

      bool& doBoxes() {
	return doBoxes_;
      }

      void setContrast(double contrast = defaultContrast_);
      void setBrightness(double brightness = defaultBrightness_);

      void installColormap(std::string cmap);
      std::string colormapName();

      // Fiddle the contrast and brightness of a frame grabber image

      void fiddleContrast(double xa, double ya);

      void setDisplayedRange(double xa, double xb, double ya, double yb);
      void setDisplayedRange();

      void setPgplotId(int pgplotId);
      int pgplotId();

      void initialize();
      void getGreyscale();
      void drawImage(bool cmapUpdate=false);
      void drawGrid();
      void drawBullseye();
      void drawCompass();
      void drawCrosshair();
      void drawBoxes();

      static const double defaultContrast_;
      static const double defaultBrightness_;

    private:

      bool first_;
      int  pgplotId_;
      double wxa_, wxb_;
      double wya_, wyb_;
      double dwx_, dwy_;

      double wxaDisp_, wxbDisp_;
      double wyaDisp_, wybDisp_;

      // Greyscale limits

      double wza_, wzb_;

      gcp::util::Angle gridInterval_;

      bool isZoomed_;
      bool imageReceived_;
      bool doGrid_;
      bool doBullseye_;
      bool doCompass_;
      bool doCrosshair_;
      bool doBoxes_;
      bool havePeak_;

      unsigned ixpeak_, iypeak_;

      double contrast_;
      double brightness_;
      double backoff_;

      Cmap* cmap_;               // The color map for image display

      float imageToPlot_[GRABBER_IM_SIZE];

    }; // End class ImagePlotter

  } // End namespace grabber
} // End namespace gcp

#endif // End #ifndef GCP_GRABBER_IMAGEPLOTTER_H
