#include "gcp/pgutil/common/ImagePlotter.h"

#include "gcp/util/common/Exception.h"

#include "gcp/control/code/unix/libunix_src/common/control.h"

#include "cpgplot.h"

#include<iostream>

#include <string.h>

using namespace std;
using namespace gcp::grabber;
using namespace gcp::control;
using namespace gcp::util;

const double ImagePlotter::defaultContrast_   = -1.0;
const double ImagePlotter::defaultBrightness_ =  0.5;

/**.......................................................................
 * Constructor.
 */
ImagePlotter::ImagePlotter() : gcp::util::ImageHandler()
{
  initialize();
}

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::grabber::operator<<(std::ostream& os, ImagePlotter& obj)
{
  os << "Default output operator for class: ImagePlotter" << std::endl;
  return os;
};

/**.......................................................................
 * Destructor.
 */
ImagePlotter::~ImagePlotter() {}

void ImagePlotter::open(char* device)
{
  if(pgplotId_ <= 0)
    pgplotId_ = cpgopen(device);
}

/**.......................................................................
 * Initialize the image
 */
void ImagePlotter::initialize() 
{
  first_         = true;
  pgplotId_      = -1;

  isZoomed_      = false;
  imageReceived_ = false;
  doGrid_        = false;
  doBullseye_    = false;
  doCompass_     =  true;
  doCrosshair_   =  true;
  havePeak_      = false;

  gridInterval_.setArcSec(30.0);

  backoff_       =    10;

  setContrast();
  setBrightness();
  
  wza_ = 0.0;
  wzb_ = 1.0;

  cmap_ = NULL;
  for(unsigned i=0;i < n_std_cmap;i++) {
    if(strcmp(std_cmaps[i].name,"grey")==0)
      cmap_ = &std_cmaps[i];
  }
}

void ImagePlotter::installColormap(std::string cmap)
{
  for(unsigned i=0;i < n_std_cmap;i++) {
    if(strcmp(std_cmaps[i].name, (char*)cmap.c_str())==0)
      cmap_ = &std_cmaps[i];
  }
}

std::string ImagePlotter::colormapName()
{
  for(unsigned i=0;i < n_std_cmap;i++) {
    if(cmap_ == &std_cmaps[i]) {
      return std_cmaps[i].name;
    }
  }
  return "";
}

/**.......................................................................
 * Draw an image
 */
void ImagePlotter::draw(bool cmapUpdate)
{
  // Direct PGPLOT output to the parent plot.

  if(pgplotId_ <= 0)
    ThrowError("Invalid pgplot device");

  // Only draw the data array if we have a valid image

  if(imageReceived_) {

    // Direct output to the appropriate pgplot device

    cpgslct(pgplotId_);

    // Don't show the image until it has been redrawn.
    
    cpgbbuf();
    
    drawImage(cmapUpdate);

    if(doGrid_)
      drawGrid();

    if(doBullseye_)
      drawBullseye();

    if(doCompass_)
      drawCompass();

    if(doCrosshair_ && havePeak_)
      drawCrosshair();

    if(doBoxes_)
      drawBoxes();

    cpgebuf();
  }  
}

/**.......................................................................
 * Draw an image
 */
void ImagePlotter::drawDouble()
{
  // Direct PGPLOT output to the parent plot.

  if(pgplotId_ <= 0)
    ThrowError("Invalid pgplot device");

  // Only draw the data array if we have a valid image

  if(imageReceived_) {

    // Direct output to the appropriate pgplot device

    cpgslct(pgplotId_);

    // Don't show the image until it has been redrawn.
    
    cpgbbuf();
    
    drawImage();

    if(doGrid_)
      drawGrid();

    if(doBullseye_)
      drawBullseye();

    if(doCompass_)
      drawCompass();

    if(doBoxes_)
      drawBoxes();

    if(doCrosshair_ && havePeak_)
      drawCrosshair();

    drawImage();

    if(doGrid_)
      drawGrid();

    if(doBullseye_)
      drawBullseye();

    if(doCompass_)
      drawCompass();

    if(doBoxes_)
      drawBoxes();

    if(doCrosshair_ && havePeak_)
      drawCrosshair();

    cpgebuf();
  }  
}

/**.......................................................................
 * Method to re-draw the image
 */
void ImagePlotter::drawImage(bool cmapUpdate) 
{
  // Install the viewport and world coordinates of the graph.
    
  cpgsci(1);
  cpgask(0);
  cpgpage();  
  cpgsch(1.0f);
    
  setPlotBoundaries();

  // Compute the greyscale range of the currently displayed data.

  getGreyscale();

  cpgctab(cmap_->l, cmap_->r, cmap_->g, cmap_->b, cmap_->n, contrast_, brightness_);

  // Set the transformation matrix for the data array.

  float tr[6];

  // cpgimag() will map the image to a 1-offset position, relative to
  // C-indexed arrays.  That is, if tr[0] = tr[3] = 0.0, and tr[1] =
  // tr[5] = 1.0, then the center of the blc pixel will be mapped to
  // (1.5, 1.5).  To force cpgimag() to map the blc pixel to (0.5,
  // 0.5), we must offset tr[0] and tr[3] by -1 * dx and -1 * dy,
  // respectively.  Since my wxa_ and wya_ are already offset by
  // -dwx_/2 and -dwy_/2 from the (0,0) pixel center, we only need to
  // tell cpgimag() to offset by an additional -dwx_/2, -dwy_/2

  tr[0]= wxa_-dwx_/2;
  tr[1]= dwx_;
  tr[2]= 0.0;

  tr[3]= wya_-dwy_/2;
  tr[4]= 0.0;
  tr[5]= dwy_;

  if(fabs(wzb_ - wza_) < 0.001) {
    wza_ =  0;
    wzb_ =  1;
  }

  cpgbbuf();

  if(first_) {
    cpgctab(cmap_->l, cmap_->r, cmap_->g, cmap_->b, cmap_->n, contrast_, brightness_);
    first_ = false;
    cmapUpdate = true;
  }

  // Now plot the image

  cpgimag(imageToPlot_, nx_, ny_, 1, nx_, 1, ny_,
	  wzb_, wza_, tr); 

  // If we are changing colormaps, redraw

  if(cmapUpdate) {
    cpgctab(cmap_->l, cmap_->r, cmap_->g, cmap_->b, cmap_->n, contrast_, brightness_);

    cpgimag(imageToPlot_, nx_, ny_, 1, nx_, 1, ny_,
	    wzb_, wza_, tr); 
  }

  cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);

  cpgebuf();
}

/**.......................................................................
 * Calculate plot limits
 */
void ImagePlotter::setPlotBoundaries()
{
  calculatePlotLimits();

  cpgsvp(0.1, 0.9, 0.1, 0.9);
  cpgswin(wxaDisp_, wxbDisp_, wyaDisp_, wybDisp_);
  cpgwnad(wxaDisp_, wxbDisp_, wyaDisp_, wybDisp_);
}

/**.......................................................................
 * Calculate plot limits
 */
void ImagePlotter::calculatePlotLimits()
{
  // I will display pixels centered at the corresponding location on
  // the sky.  That is, pixel (0,0) in C-indexing will be _centered_
  // on (0.0, 0.0), not have its blc at (0,0).  This means that the
  // plot boundary should extend from -nx/2*scale to +nx/2*scale
  
  double arcsec = fov_.arcsec();
  dwx_ =  arcsec/nx_;
  dwy_ = (arcsec*aspectRatio_)/ny_;

  wxa_ = -(nx_ * dwx_)/2;
  wxb_ =  (nx_ * dwx_)/2;

  wya_ = -(ny_ * dwy_)/2;
  wyb_ =  (ny_ * dwy_)/2;

  // If we are not currently zoomed, set the displayed limits to the
  // full limits

  if(!isZoomed_) {
    wxaDisp_ = wxa_;
    wxbDisp_ = wxb_;
    wyaDisp_ = wya_;
    wybDisp_ = wyb_;
  }

}

/**.......................................................................
 * Compute the range of greyscale levels to be displayed for the new
 * plot boundaries.
 */ 
void ImagePlotter::getGreyscale()
{
  // Compute the new displayed greylevel range.

  float zmin=0.0,zmax=0.0,range;
  float x,y;
  bool first=true;
  int index;
  unsigned ind, n=0;
  
  float val;

  for(unsigned i=0; i < nx_; i++) 
    for(unsigned j=0; j < ny_; j++) {

      // If this pixel is excluded, skip it

      ind = (GRABBER_YNPIX - j - 1) * GRABBER_XNPIX + i;
      if(!usePixel_[ind])
	continue;

      x = wxa_ + i * dwx_;
      y = wya_ + j * dwy_;

      if(x <= wxbDisp_ && x >= wxaDisp_ && y <= wybDisp_ && y >= wyaDisp_) {


#if 0
	index = i + nx_*j;
	val = imageToPlot_[index];
#else
	val = imageToPlot_[ind];
#endif
	if(first) {
	  zmin = zmax = val;
	  first = false;
	}

	zmin = zmin < val ? zmin : val;
	zmax = zmax > val ? zmax : val;
      }
    }

  range = zmax-zmin;
  
  // Protect against zero range

  wza_ = zmin==zmax ? zmin - 0.2*range : zmin;
  wzb_ = zmin==zmax ? zmax + 0.2*range : zmax;
}

/**.......................................................................
 * Method to draw a grid of specified interval over the image
 */
void ImagePlotter::drawGrid() 
{ 
  // Draw the grid of requested width
  
  int i,j,ngridx,ngridy;
  cpgsci(10);
  double interval = gridInterval_.arcsec();

  if(interval > 0) {

    ngridx = (int)((wxb_)/interval+1);
    ngridy = (int)((wyb_)/interval+1);

    for(i=0; i < ngridx; i++) {
      cpgmove( i*interval, wya_);
      cpgdraw( i*interval, wyb_);
      cpgmove(-i*interval, wya_);
      cpgdraw(-i*interval, wyb_);
    }

    for(j=0;j < ngridy;j++) {
      cpgmove(wxa_,  j*interval);
      cpgdraw(wxb_,  j*interval);
      cpgmove(wxa_, -j*interval);
      cpgdraw(wxb_, -j*interval);
    }
  }
}

/**.......................................................................
 * Method to draw a series of concentric rings of the specified grid
 * interval
 */
void ImagePlotter::drawBullseye()
{
  // Draw the largest number of concentric rings which will fit in the
  // window.
  
  int i,nring;
  double interval = gridInterval_.arcsec();
  
  cpgsci(7);
  if(interval > 0) {
    nring = (int)((wyb_)/interval);
    cpgsfs(2);
    for(i=0;i < nring;i++) 
      cpgcirc(0.0, 0.0, (i+1)*interval);
  }
}

/**.......................................................................
 * Draw the image compass
 */
void ImagePlotter::drawCompass()
{
  // Finally, draw the compass lines, if requested.  Elevation axis
  // is a vector at an angle clockwise from vertical.
  
  // Set the length to 1/3 of the smallest dimension displayed

  float xlen, ylen, len;

  ylen = (wybDisp_ - wyaDisp_)/3;
  xlen = (wxbDisp_ - wxaDisp_)/3;
  len = xlen < ylen ? xlen : ylen;

  // Steve finds it more convenient to always have the vectors
  // centered on the origin

  float xcen, ycen;

  //  xcen = (wxaDisp_ + wxbDisp_)/2;
  //  ycen = (wyaDisp_ + wybDisp_)/2;

  xcen = 0.0;
  ycen = 0.0;

  float xel,yel,xaz,yaz;

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

  xel = xcen + len*sin(rad);
  yel = ycen + len*cos(rad);
  xaz = xcen + len*sin(rad+M_PI/2);
  yaz = ycen + len*cos(rad+M_PI/2);
  
  cpgsci(1);
  
  xel = (xImDir_ == UPRIGHT ? xel : -xel);
  xaz = (xImDir_ == UPRIGHT ? xaz : -xaz);
  
  yel = (yImDir_ == UPRIGHT ? yel : -yel);
  yaz = (yImDir_ == UPRIGHT ? yaz : -yaz);
  
  cpgsci(5);
  cpgarro(xcen, ycen, xel, yel);
  cpgsci(1);
  
  cpgarro(xcen, ycen, xaz, yaz);
  
  // And write text 2 character heights away from the tips of the
  // vectors.

  len *= 1.2;
  xel  = xcen + len*sin(rad);
  yel  = ycen + len*cos(rad);
  xaz  = xcen + len*sin(rad+M_PI/2);
  yaz  = ycen + len*cos(rad+M_PI/2);
  
  xel = (xImDir_ == UPRIGHT ? xel : -xel);
  xaz = (xImDir_ == UPRIGHT ? xaz : -xaz);
  
  yel = (yImDir_ == UPRIGHT ? yel : -yel);
  yaz = (yImDir_ == UPRIGHT ? yaz : -yaz);
  
  /*
   * Write only if the coordinates are inside the current display window.
   */
  if(xel >= wxaDisp_ && xel <= wxbDisp_ && 
     yel >= wyaDisp_ && yel <= wybDisp_)
    cpgsci(5);

  cpgtext(xel, yel,"El");
  cpgsci(1);
  
  if(xaz >= wxaDisp_ && xaz <= wxbDisp_ && 
     yaz >= wyaDisp_ && yaz <= wybDisp_)
    cpgtext(xaz, yaz,"Az");
}

/**.......................................................................
 * Draw a cross-hair
 */
void ImagePlotter::drawCrosshair()
{
  int ci, lw;
  cpgqci(&ci);
  cpgqlw(&lw);
  cpgsci(5);
  cpgslw(1);

  double xpeak, ypeak;

  pixelToWorld(ixpeak_, iypeak_, xpeak, ypeak);

  // Draw a cross-hair cursor with a center region excised

  cpgmove(xpeak, wya_);
  cpgdraw(xpeak, ypeak - backoff_);

  cpgmove(xpeak, wyb_);
  cpgdraw(xpeak, ypeak + backoff_);

  cpgmove(wxa_, ypeak);
  cpgdraw(xpeak - backoff_, ypeak);

  cpgmove(wxb_, ypeak);
  cpgdraw(xpeak + backoff_, ypeak);

  cpgsci(ci);
  cpgslw(lw);
}

/**.......................................................................
 * Drawboxes
 */
void ImagePlotter::drawBoxes()
{
  int ci, lw;

  // Query the current color index and line width for restoral on exit

  cpgqci(&ci);
  cpgqlw(&lw);

  for(unsigned i=0; i < boxes_.size(); i++) {
    double xmin, xmax, ymin, ymax;

    pixelToWorld(boxes_[i].ixmin_, boxes_[i].iymin_, xmin, ymin);
    pixelToWorld(boxes_[i].ixmax_, boxes_[i].iymax_, xmax, ymax);

    // Draw a box

    cpgsci(boxes_[i].inc_ ? 10 : 2);

    cpgmove(xmin, ymin);
    cpgdraw(xmin, ymax);
    cpgdraw(xmax, ymax);
    cpgdraw(xmax, ymin);
    cpgdraw(xmin, ymin);
  }

  cpgsci(ci);
  cpgslw(lw);
}

/**.......................................................................
 * Overloaded method to install the new image.  This also installs it
 * in the pgplot buffer
 */
void ImagePlotter::installNewImage(unsigned short* image)
{
  gcp::util::ImageHandler::installNewImage(image);

  unsigned indFrom, indTo;

  for(unsigned iy=0; iy < GRABBER_YNPIX; iy++) {
    for(unsigned ix=0; ix < GRABBER_XNPIX; ix++) {
      
      // Flip the image in y
      
      indFrom = iy * GRABBER_XNPIX + ix;
      indTo   = (GRABBER_YNPIX - iy - 1) * GRABBER_XNPIX + ix;
      
      imageToPlot_[indTo] = (float)image[indFrom];
    }
  }

  imageReceived_ = true;
  havePeak_      = false;
}

void ImagePlotter::testDraw()
{
  unsigned nx=10, ny=10;
  float test[100];

  for(unsigned i=0;i < 100; i++)
    test[i] = (float)i;

  cpgopen("/xs");

  cpgsci(1);
  cpgask(0);
  cpgpage();  
  cpgsch(1.0f);

  double dx = 1.0;
  double dy = 1.0;
  double wxa, wxb, wya, wyb;

  wxa = -(nx * dx)/2;
  wxb =  (nx * dx)/2;
  wya = -(ny * dy)/2;
  wyb =  (ny * dy)/2;

  cpgsvp(0.1,0.9,0.1,0.9);
  cpgswin(wxa-0.5*dx, wxb+0.5*dx, wya-0.5*dy, wyb+0.5*dy);
  cpgwnad(wxa-0.5*dx, wxb+0.5*dx, wya-0.5*dy, wyb+0.5*dy);

  cpgctab(cmap_->l, cmap_->r, cmap_->g, cmap_->b, cmap_->n, contrast_, brightness_);

  // Set the transformation matrix for the data array.

  float tr[6];

  // cpgimag() will map the image to a 1-offset position, relative to
  // C-indexed arrays.  That is, if tr[0] = tr[3] = 0.0, and tr[1] =
  // tr[5] = 1.0, then the center of the blc pixel will be mapped to
  // (1.5, 1.5).  To force cpgimag() to map the blc pixel to (0.5,
  // 0.5), we must offset tr[0] and tr[3] by -1 * dx and -1 * dy,
  // respectively

  tr[0]= wxa-dx/2;
  tr[1]= dx;
  tr[2]= 0.0;

  tr[3]= wya-dy/2;
  tr[4]= 0.0;
  tr[5]= dy;

  cpgimag(test, 10, 10, 1, 10, 1, 10,
	  99, 0, tr); 

  cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
  cpglab("\\gD(\")","","");
}

/**.......................................................................
 * Convert from pixel number to world coordinate
 */
void ImagePlotter::pixelToWorld(unsigned ix, unsigned iy, double& x, double& y)
{
  calculatePlotLimits();

  double xa, ya, dx, dy;

  xa = (-(double)(nx_)/2 + 0.5) * dwx_;
  ya = (-(double)(ny_)/2 + 0.5) * dwy_;

  x =   xa + ix*dwx_;

  // Sign of y is flipped wrt to x, as the frame grabber natively
  // inverts the y axis, and we are assuming that the inputs are ix,
  // iy from the frame grabber
 
  y = -(ya + iy*dwy_);
}

/**.......................................................................
 * Convert from world coordinates to pixel number
 */
void ImagePlotter::
worldToPixel(double x, double y, unsigned& ix, unsigned& iy, bool truncate)
{
  calculatePlotLimits();

  double xa, ya, dx, dy;

  xa = (-(double)(nx_)/2 + 0.5) * dwx_;
  ya = (-(double)(ny_)/2 + 0.5) * dwy_;

  double ypix = -(y + ya)/dwy_;
  double xpix =  (x - xa)/dwx_;
  
  int ixpix = ((xpix - (unsigned)xpix) < 0.5) ? (unsigned)xpix : (unsigned)xpix + 1;
  int iypix = ((ypix - (unsigned)ypix) < 0.5) ? (unsigned)ypix : (unsigned)ypix + 1;

  if(ixpix < 0) {
    if(truncate)
      ixpix = 0;
    else
      ThrowError("X coordinate does not lie within the image");
  }

  if(iypix < 0) {
    if(truncate)
      iypix = 0;
    else
      ThrowError("Y coordinate does not lie within the image");
  }

  if(ixpix > nx_-1) {
    if(truncate)
      ixpix = nx_-1;
    else
      ThrowError("X coordinate does not lie within the image");
  }

  if(iypix > ny_-1) {
    if(truncate)
      iypix = ny_-1;
    else
      ThrowError("Y coordinate does not lie within the image");
  }

  ix = ixpix;
  iy = iypix;
}

void ImagePlotter::worldToSkyOffset(double x, double y, 
				    Angle& xsky, Angle& ysky)
{
  unsigned ix, iy;
  worldToPixel(x, y, ix, iy);
  pixelToSkyOffset(ix, iy, xsky, ysky);
}

void ImagePlotter::setPeak(unsigned ix, unsigned iy)
{
  ixpeak_ = ix;
  iypeak_ = iy;

  havePeak_ = true;
}

void ImagePlotter::setContrast(double contrast)
{
  contrast_ = contrast;
}

void ImagePlotter::setBrightness(double brightness)
{
  brightness_ = brightness;
}

void ImagePlotter::setGridInterval(gcp::util::Angle& interval)
{
  gridInterval_ = interval;
}

/**.......................................................................
 * Fiddle the contrast and brightness of a frame grabber image.
 */
void ImagePlotter::fiddleContrast(double xa, double ya)
{
  double contra, bright;
  double ymid = (wyb_ + wya_)/2;
  
  // Compute the contrast and brightness from the cursor position.

  contra = 5.0 * (ya-ymid)/(ya < ymid ? (wya_-ymid) : -(wyb_-ymid));
  bright = 0.5 + 1.0 * (fabs(contra)+1.0) *
    ((xa - wxb_)/(wxa_ - wxb_) - 0.5);
  
  setContrast(contra);
  setBrightness(bright);
}

void ImagePlotter::setDisplayedRange()
{
  isZoomed_ = false;
}

void ImagePlotter::setDisplayedRange(double xa, double xb, double ya, double yb)
{
  // Corners may not be selected in the order we think!  Handle the
  // case where the user selects different orders than blc -> trc

  wxaDisp_ = xa < xb ? xa : xb;
  wxbDisp_ = xb > xa ? xb : xa;
  wyaDisp_ = ya < yb ? ya : yb;
  wybDisp_ = yb > ya ? yb : ya;

  isZoomed_ = true;
}

void ImagePlotter::setPgplotId(int pgplotId)
{
  pgplotId_ = pgplotId;
}

int ImagePlotter::pgplotId()
{
  return pgplotId_;
}
