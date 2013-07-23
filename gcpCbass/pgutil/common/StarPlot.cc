#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* Header files */

#include "cpgplot.h"

//#include "gcp/control/code/unix/viewer_src/date.h"
#include "gcp/control/code/share/slalib/slalib.h"
#include "gcp/control/code/unix/libsrc_src/source.h"

#include "gcp/util/common/Coordinates.h"
#include "gcp/util/common/Matrix.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/HourAngle.h"
#include "gcp/util/common/PtSrcTiler.h"
#include "gcp/util/common/Vector.h"

#include "gcp/pgutil/common/StarPlot.h"

#include <iomanip>

using namespace gcp::util;
using namespace gcp::control;

/* Constants */

#define MAXSTAR 2000
#define NAMELEN 12
#define STEP    1       /* time between plots in hours */
#define N_OBSERV 2      /* number of known observatory sites */

#define CATFILE "/home/roto/eml/projects/gcp/gcp/array/ephem/stars.cat"

/**.......................................................................
 * Read star positions from specified file 
 */
StarPlot::StarPlot() : 
  nvss_("/data/gcpdaq/catalogs/nvss.fits"), 
  first_("/data/gcpdaq/catalogs/first.fits"),
  gcpCal_("/home/gcpdaq/gcp_analysis/catalog/gcpCalList.txt")
{
  magLim_      = 0.0;
  magLimIsSet_ = false;

  fluxLimIsSet_ = false;
  markCounter_ = 0;
  maxmag_      = 0.0;
  max_         = 2000;
  isZoomed_    = false;

  elLimit_.setDegrees(40.0);
  dispElLimit_ = elLimit_;

  pgplotId_ = 0;

  initCatalog();

  // Initialize sites we know about

  Site site;

  site.name_ = "OVRO";
  site.latitude_.setDegrees("37:13:57.5");
  site.longitude_.setDegrees("-118:17:45.7"); 

  knownSites_.push_back(site);

  site.name_ = "Pole";
  site.latitude_.setDegrees("-90:00:00");
  site.longitude_.setDegrees("00:00:00");

  knownSites_.push_back(site);

  site.name_ = "SA";
  site.latitude_.setDegrees("-25:53:23");
  site.longitude_.setDegrees("27:41:07");

  knownSites_.push_back(site);

  // Default the site to OVRO

  setSite("SA");

  lstIsSet_ = false;
  mjdIsSet_ = false;

  // Initialize the plot to display the full range

  isZoomed_ = false;
}

StarPlot::~StarPlot() {}

void StarPlot::setLatitude(Angle latitude)
{
  latitude_ = latitude;

  if(!isZoomed_)
    dispLatitude_ = latitude;
}

void StarPlot::setLongitude(Angle longitude)
{
  longitude_ = longitude;

  if(!isZoomed_)
    dispLongitude_ = longitude;
}

void StarPlot::setElevationLimit(Angle elLimit)
{
  elLimit_ = elLimit;

  if(!isZoomed_)
    dispElLimit_ = elLimit;
}

/**.......................................................................
 * Initialize catalog
 */
void StarPlot::initCatalog()
{
  static char* planet[] = {"Sun", "Mercury", "Venus", "Moon", "Mars",
			   "Jupiter", "Saturn", "Uranus", "Neptune"};

  // Clear the list

  starlist_.resize(0);

  // put planets in positions 0-8; ra and dec will be computed later 

  for(unsigned n=0; n < 9; n++) {

    Object star;

    star.ra_    =  0.0;
    star.dec_   =  0.0;
    star.mag_   = -1.0;
    star.name_  =  planet[n];
    star.vis_   =  1;
    star.mark_  =  0;
    star.type_  =  StarPlot::SOLAR;

    starlist_.push_back(star);
  }  
}

/**.......................................................................
 * Read star positions from specified file 
 */
void StarPlot::readCatalog()
{
  readCatalog(catalogFile_);
}

/**.......................................................................
 * Read star positions from specified calfile 
 */
void StarPlot::readCal(std::string catalogFile)
{
  Flux min, max;

  min.setJy(0.0);
  max.setJy(1e9);

  gcpCal_.setCatalogFile(catalogFile);

  readPtSrc("cal", min, max);
}

/**.......................................................................
 * Read star positions from specified file 
 */
void StarPlot::readCatalog(std::string catalogFile)
{
  FILE *catalog;

  SourceCatalog* sc = 0;

  sc = new_SourceCatalog();

  if(!sc)
    ThrowError("Couldn't allocate source catalog");

  (void) read_SourceCatalog(sc, "", (char*)catalogFile.c_str());

  // Extract stars from catalog 

  unsigned iSrc;

  for(iSrc=0; iSrc < nSource(sc); iSrc++) {

    Source* src = find_SourceByNumber(sc, iSrc);

    if(src->id.type != SRC_J2000)
      continue;

    Object star;

    star.name_ = src->id.name;
    star.ra_   = src->j2000.ra;
    star.dec_  = src->j2000.dec;
    star.mag_  = src->j2000.mag;
    star.vis_  = 1;
    star.mark_ = 0;

    if (star.mag_ > maxmag_ && star.mag_ < 30.0)
      maxmag_ = star.mag_;

    star.type_   = StarPlot::SRCLIST;
    star.survey_ = catalogFile;

    star.dist_.setDegrees(0);

    float angle = rand() * 360.0/RAND_MAX;
    star.angle_.setDegrees(angle);

    starlist_.push_back(star);
  }
  
  if (iSrc < nSource(sc)) {
    ReportError("More than " << max_ << " stars in catalog: truncated");
  } else {
    ReportError(starlist_.size() << " stars read from catalog " << catalogFile_);
  }

  // If no magnitude limit is set, set it from the catalog

  if(!magLimIsSet_)
    magLim_ = 0.5*ceil(2.0*maxmag_);

  if(sc)
    sc = del_SourceCatalog(sc);
}

double StarPlot::parseSexagesimal(char *s)
{
  double deg=0.0, min=0.0, sec=0.0;
  char *ss;
  char *input=s;
  /* Optional sign (+ or -) */
  int sign = 1;
  if (*s == '+') {
    s++;
  } else if (*s == '-') {
    s++;
    sign = -1;
  }
  /* Degrees part */
  deg = strtod(s, &ss);
  /* Minutes part */
  if (*ss == ':') {
    s = ss+1;
    min = strtod(s, &ss);
    /* Seconds part */
    if (*ss == ':') {
      s = ss+1;
      sec = strtod(s, &ss);
    }
  }

  /* Check for error */

  if (*ss != '\0') {
    fprintf(stderr, "Invalid sexagesimal number: %s\n", input);
    return 0.0;
  } else {
    return sign*(deg + min/60.0 + sec/3600.0);
  }
}

/**--------------------------------------------------------------------
 * Zenithal equal-area projection: convert ZA to radius in projection.
 * The radius in the projection is 2*sin(ZA/2).
 *
 * Input: ZA (rad)
 * Returns: radius
 */
double StarPlot::zeap(double theta)
{
  return 2.0*sin(theta/2.0);
}

/**-----------------------------------------------------------------------
 * Zenithal equal-area projection: the radius in the projection is
 * 2*sin(ZA/2).
 *
 * Input: AZ, EL
 * Output: X, Y
 */
void StarPlot::deprojectZea(double x, double y, Angle& az, Angle& el)
{
  double radius = sqrt(x*x + y*y);
  az.setRadians(atan2(-y, -x) + M_PI/2);
  el.setRadians(M_PI/2.0 - 2.0*asin(radius/2.0));
}

/**-----------------------------------------------------------------------
 * Zenithal equal-area projection: the radius in the projection is
 * 2*sin(ZA/2).
 *
 * Input: AZ, EL
 * Output: X, Y
 */
void StarPlot::projectZea(double az, double el, double *x, double *y)
{
  double radius = zeap(M_PI/2.0 - el);
  *x = -radius*sin(az);
  *y =  radius*cos(az);
}

void StarPlot::initBox(Rect& box)
{
  double radius = zeap(M_PI/2.0 - dispElLimit_.radians());
  box.x1 = -1.1*radius;
  box.x2 =  1.1*radius;
  box.y1 = -1.1*radius;
  box.y2 =  1.1*radius;
}

/**-----------------------------------------------------------------------
 * Plot star positions in zenithal equal-area projection.
 */
void StarPlot::plotPolar()
{
  TimeVal tVal;
  tVal.setToCurrentTime();

  plotPolar(tVal.getMjd());
}

/**.......................................................................
 * Make the plot
 */
void StarPlot::plotPolar(double mjd)
{
  int i, j, lh, lm;
  char label[128];
  double x, y;
  double za_limit;
  double radius;
  double xpos, ypos, ra, dec, halim, az, el;
  double cos_zalim, cos_lat, sin_lat, cos_halim;
  double ha, aaz, ael;
  double lst;
  double diam;
  double dlsun, dbsun, dlmoon, dbmoon, phase;
  int vis;
  char *tag;
  Rect box;
  int names = 0;

  // Redefine color 6 to look pale blue

  // Find current LST (approximate; ignores equation of equinoxes)

  lst = slaDranrm(slaGmst(mjd) + dispLongitude_.radians());  

  // Update planet positions 

  for (i=0; i<9; i++) {
    slaRdplan(mjd, i, dispLongitude_.radians(), dispLatitude_.radians(), 
	      &starlist_[i].ra_, &starlist_[i].dec_, &diam);

    starlist_[i].diam_.setRadians(diam);
  }

  // Approximate phase of Moon (difference in ecliptic longitude of
  // Sun and Moon)

  slaEqecl(starlist_[0].ra_, starlist_[0].dec_, mjd, &dlsun, &dbsun);
  slaEqecl(starlist_[3].ra_, starlist_[3].dec_, mjd, &dlmoon, &dbmoon);

  phase = slaDrange(dlmoon - dlsun)/(2*M_PI);

  if (phase < 0.0)
    phase += 1.0;

  initBox(box);

  // The graph occupies a circle with radius specified by the
  // zenith-angle limit

  za_limit = M_PI/2.0 - dispElLimit_.radians();
  radius = zeap(za_limit);

  //  cpgbbuf();

  cpgeras();
  
  // Annotation 

  //  cpgsave();

  cpgsci(1);
  cpgsch(1.0);
  cpgslw(2);

  cpgsvp(0.0, 1.0, 0.0, 1.0);
  cpgmtxt("T", -1.5, 0.0, 0.0, (char*)siteName_.c_str());
  cpgsch(0.8);
  sprintf(label, "Latitude: %.1f°", dispLatitude_.degrees());
  cpgmtxt("T", -4.0, 0.0, 0.0, label);
  sprintf(label, "Longitude: %.1f°", dispLongitude_.degrees());
  cpgmtxt("T", -5.5, 0.0, 0.0, label);
  sprintf(label, "Min elevation: %.1f°", dispElLimit_.degrees());
  cpgmtxt("T", -7.0, 0.0, 0.0, label);

  if(magLimIsSet_) {
    sprintf(label, "Mag < %.1f", magLim_);
    cpgmtxt("T", -8.5, 0.0, 0.0, label);
  }

  if(fluxLimIsSet_) {
    sprintf(label, "Flux > %.1f Jy", fluxLim_.Jy());
    cpgmtxt("T", -10.0, 0.0, 0.0, label);
  }

  mjdToDate(mjd, label, sizeof(label));
  cpgmtxt("T", -1.5, 0.95, 1.0, label);
  lh = (int) (lst * HourAngle::hourPerRad_);
  lm = (int) ((lst * HourAngle::hourPerRad_-lh)*60.0);
  sprintf(label, "LST: %.2d\\uh\\d%.2d\\um\\d", lh, lm);
  cpgmtxt("T", -4.0, 0.95, 1.0, label);

  // Sun elevation 

  slaRdplan(mjd, i, dispLongitude_.radians(), dispLatitude_.radians(), 
	    &ra, &dec, &diam);
  slaDe2h(lst - ra, dec, dispLatitude_.radians(), &az, &el);
  sprintf(label, "Sun elevation: %.1f°", el * Angle::degPerRad_);
  cpgmtxt("B", -0.5, 0.0, 0.0, label);

  showMarkedStars();

  // Moon phase 

  if (phase < 0.05 || phase > 0.95)
    tag = "New";
  else if (phase > 0.2 && phase < 0.3)
    tag = "Q1";
  else if (phase > 0.45 && phase < 0.55)
    tag = "Full";
  else if (phase > 0.7 && phase < 0.8)
    tag = "Q3";
  else
    tag = "";

  sprintf(label, "Moon phase: %.1f %s", phase, tag);
  cpgmtxt("B", -0.5, 1.0, 1.0, label);
  cpgsvp(0.93, 0.99, 0.05, 0.11);
  drawMoon(phase, 16, 7);

  //  cpgunsa();

  // Define viewport and shade the circle of the map

  //  cpgsave();

  cpgsvp(0.0, 1.0, 0.0, 1.0);
  cpgwnad(box.x1, box.x2, box.y1, box.y2);

  cpgsci(16);
  cpgsfs(1);
  cpgcirc(0.0, 0.0, radius);

  //  cpgunsa();

  // Dec grid and labels
 
  //  cpgsave();

  cpgsci(15);
  cpgsch(0.7);
  cos_zalim = cos(za_limit);
  cos_lat = cos(dispLatitude_.radians());
  sin_lat = sin(dispLatitude_.radians());
  for (i=-80; i <= 80; i += 10) {
    dec = i* (1.0/Angle::degPerRad_);

    // Find the HA limit corresponding to the ZA limit 

    cos_halim = (cos_zalim - sin_lat*sin(dec))/(cos_lat*cos(dec));

    if (cos_halim > 1.0) 
      // this dec not visible 
      continue;
    if (cos_halim < -1.0)
      // circumpolar 
      halim = M_PI;
    else
      halim = acos(cos_halim);

    // Draw the line of constant declination 

    ypos = 0.0;
    for (j=0; j <= 100; j++) {
      ha = -halim + j*(2.0*halim/100);
      slaDe2h(ha, dec, dispLatitude_.radians(), &aaz, &ael);
      projectZea(aaz, ael, &x, &y);
      if (j == 0)
	cpgmove(x,y);
      else
	cpgdraw(x,y);
      if (j == 50) ypos = y;
    }
    sprintf(label, "\\gd=%d", i);
    cpgptxt(0.0, ypos + 0.02*radius, 0.0, 0.5, label);
  }

  //  cpgunsa();

  // HA grid 
  
  //  cpgsave();

  cpgsci(15);
  cpgsch(0.7);
  for (i=-12; i <= 11; i += 1) {

    ha = i*(1.0/HourAngle::hourPerRad_);

    // Is the north pole visible?

    slaDe2h(0.0, M_PI/2.0, dispLatitude_.radians(), &aaz, &ael);
    if (ael > dispElLimit_.radians()) {
      vis = 1;
      projectZea(aaz, ael, &x, &y);
      cpgmove(x, y);
    } else {
      vis = 0;
    }

    // Draw the line of constant HA 

    for (j=0; j <= 200; j++) {
      dec = (90.0 - j*(180.0/200.0))/Angle::degPerRad_;
      slaDe2h(ha, dec, dispLatitude_.radians(), &aaz, &ael);
      projectZea(aaz, ael, &x, &y);
      if (ael > dispElLimit_.radians()) {
	if (vis) 
	  cpgdraw(x,y);
	else
	  cpgmove(x, y);
	vis = 1;
      } else {
	vis = 0;
      }
    }
  }


  //  cpgunsa();

  // AZ/EL grid
 
  //  cpgsave();

  cpgsci(1);
  cpgsfs(2);

  for (i=10; i <= za_limit * Angle::degPerRad_; i += 10) {
    cpgcirc(0.0, 0.0, zeap(i*(1.0/Angle::degPerRad_)));
  }

  for (i=0; i < 180; i +=30) {
    projectZea(i*(1.0/Angle::degPerRad_), dispElLimit_.radians(), &x, &y);
    cpgmove(-x, -y);
    cpgdraw(x, y);
  }

  //  cpgunsa();

  // AZ/EL labels
 
  //  cpgsave();
  cpgsci(6);
  cpgsch(0.7);
  for (i=10; i < za_limit * Angle::degPerRad_; i += 10){
    x = zeap(i*(1.0/Angle::degPerRad_));
    y = 0.02*radius;
    sprintf(label, "%d", 90 - i);
    cpgptxt(x, y, 0.0, 0.5, label);
  }
  for (i=0; i < 360; i +=30) {
    x = -1.02*radius*sin(i*(1.0/Angle::degPerRad_));
    y = 1.02*radius*cos(i*(1.0/Angle::degPerRad_));
    sprintf(label, "%s%d", i==0 ? "AZ = " : "", i);

    cpgptxt(x, y, i, 0.5, label);
  }

  cpgsch(1.0);

  float xc,yc;

  xc = 0.0;
  yc = 1.08*radius;
  cpgpt(1, (const float*)&xc, (const float*)&yc, 'N');

  xc = -1.08*radius;
  yc =   0.0;
  cpgpt(1, (const float*)&xc, (const float*)&yc,  'E');

  xc =   0.0;
  yc = -1.08*radius;
  cpgpt(1, (const float*)&xc, (const float*)&yc, 'S');

  xc = 1.08*radius;
  yc =   0.0;
  cpgpt(1, (const float*)&xc, (const float*)&yc, 'W');
 
  //  cpgunsa();

  // Galactic plane

  //  cpgsave();
  cpgslw(1);
  cpgsci(14);
  vis = 0;
  for (i=0; i<=400; i++) {
    slaGaleq((2*M_PI)*i/400.0, 0.0, &ra, &dec);
    slaDe2h(lst - ra, dec, dispLatitude_.radians(), &az, &el);
    projectZea(az, el, &xpos, &ypos);
    if (el > dispElLimit_.radians()) {
      if (vis)
	cpgdraw(xpos, ypos);
      else
	cpgmove(xpos, ypos);
      vis = 1;
    } else {
      vis = 0;
    }
  }
  //  cpgunsa();

  /* Ecliptic */

  //  cpgsave();
  cpgslw(1);
  cpgsls(2);
  cpgsci(14);
  vis = 0;
  for (i=0; i<=400; i++) {
    slaEcleq((2*M_PI)*i/400.0, 0.0, mjd, &ra, &dec);
    slaDe2h(lst - ra, dec, dispLatitude_.radians(), &az, &el);
    projectZea(az, el, &xpos, &ypos);
    if (el > dispElLimit_.radians()) {
      if (vis)
	cpgdraw(xpos, ypos);
      else
	cpgmove(xpos, ypos);
      vis = 1;
    } else {
      vis = 0;
    }
  }
  //  cpgunsa();
 
  // Now plot sources

  cpgsls(1);

  //  cpgsave();
  cpgsch(1.0);
  cpgsci(2);

  for (i=0; i < starlist_.size(); i++) {
    double size;
    int color, sym;
    char greek[20];
    starlist_[i].x_ = 1e6;
    starlist_[i].y_ = 1e6;
    slaDe2h(lst - starlist_[i].ra_, starlist_[i].dec_, 
	    dispLatitude_.radians(), &az, &el);
    starlist_[i].az_ = az;
    starlist_[i].el_ = el;

    if (el > dispElLimit_.radians() && starlist_[i].vis_) {

      projectZea(az, el, &xpos, &ypos);

      if (xpos > box.x1 && xpos < box.x2 && ypos > box.y1 && ypos < box.y2) {

	starlist_[i].x_ = xpos;
	starlist_[i].y_ = ypos; 

	size  = starlist_[i].size();
	sym   = starlist_[i].sym();
	color = starlist_[i].color();

	cpgsch(size);
	cpgsci(i < 9 ? 7 : color);

	if(sym == 768) {
	  cpgptxt(xpos, ypos, starlist_[i].angle_.degrees(), 0.5, "\\(0768)");
	} else {
	  cpgpt1(xpos, ypos, sym);
	}

	if (i < 9 || names) {
	  cpgsch(0.7);
	  greekify((char*)starlist_[i].name_.c_str(), greek);
	  cpgtext(xpos, ypos, greek);
	}

	if (starlist_[i].mark_) {
	  cpgsch(0.7);
	  cpgsci(1);
	  cpgpt1(xpos, ypos, 25);
	}

      }
    }

    // If the distance around the source is non-zero, draw it too

    if(starlist_[i].dist_.degrees() > 0.0) {
      drawCircle(az, el, starlist_[i].dist_, 100);
    }
    
  }
  
  //  cpgunsa();
}

void StarPlot::drawCircle(double az, double el, Angle& radius, unsigned npt)
{
  Angle azAngle, elAngle;
  azAngle.setRadians(az);
  elAngle.setRadians(el);
  
  std::vector<double> azs, els;
  std::vector<float> xs, ys;
  double x, y;
  
  getAzElCircle(azAngle, elAngle, radius, npt, azs, els);

  xs.resize(npt);
  ys.resize(npt);

  bool first = true;
  for(unsigned i=0; i < npt; i++) {
    projectZea(azs[i], els[i], &x, &y);

    xs[i] = x;
    ys[i] = y;

    if(els[i] > dispElLimit_.radians()) {
      if(first) {
	cpgmove(xs[i], ys[i]);
	first = false;
      } else {
	cpgdraw(xs[i], ys[i]);
      }
    } else {
      if(!first) {
	first = true;
      }
    }

  }
}

/**.......................................................................
 * Convert to Greek letters
 */
void StarPlot::greekify(char *input, char *output)
{
  strcpy(output, " ");
  if (input[0]=='A' && input[1]=='l' && input[2]=='p') {
    strcat(output, "\\ga");
    strcat(output, input+3);
  } else if (input[0]=='B' && input[1]=='e' && input[2]=='t') {
    strcat(output, "\\gb");
    strcat(output, input+3);
  } else if (input[0]=='G' && input[1]=='a' && input[2]=='m') {
    strcat(output, "\\gg");
    strcat(output, input+3);
  } else if (input[0]=='D' && input[1]=='e' && input[2]=='l') {
    strcat(output, "\\gd");
    strcat(output, input+3);
  } else if (input[0]=='E' && input[1]=='p' && input[2]=='s') {
    strcat(output, "\\ge");
    strcat(output, input+3);
  } else if (input[0]=='Z' && input[1]=='e' && input[2]=='t') {
    strcat(output, "\\gz");
    strcat(output, input+3);
  } else if (input[0]=='E' && input[1]=='t' && input[2]=='a') {
    strcat(output, "\\gy");
    strcat(output, input+3);
  } else if (input[0]=='T' && input[1]=='h' && input[2]=='e') {
    strcat(output, "\\gh");
    strcat(output, input+3);
  } else if (input[0]=='I' && input[1]=='o' && input[2]=='t') {
    strcat(output, "\\gi");
    strcat(output, input+3);
  } else if (input[0]=='K' && input[1]=='a' && input[2]=='p') {
    strcat(output, "\\gk");
    strcat(output, input+3);
  } else if (input[0]=='L' && input[1]=='a' && input[2]=='m') {
    strcat(output, "\\gl");
    strcat(output, input+3);
  } else if (input[0]=='M' && input[1]=='u') {
    strcat(output, "\\gm");
    strcat(output, input+2);
  } else if (input[0]=='N' && input[1]=='u') {
    strcat(output, "\\gn");
    strcat(output, input+2); 
  } else if (input[0]=='X' && input[1]=='i') {
    strcat(output, "\\gc");
    strcat(output, input+2);
  } else if (input[0]=='O' && input[1]=='m' && input[2]=='i') {
    strcat(output, "\\go");
    strcat(output, input+3);
  } else if (input[0]=='P' && input[1]=='i') {
    strcat(output, "\\gp");
    strcat(output, input+2);
  } else if (input[0]=='R' && input[1]=='h' && input[2]=='o') {
    strcat(output, "\\gr");
    strcat(output, input+3);
  } else if (input[0]=='S' && input[1]=='i' && input[2]=='g') {
    strcat(output, "\\gs");
    strcat(output, input+3);
  } else if (input[0]=='T' && input[1]=='a' && input[2]=='u') {
    strcat(output, "\\gt");
    strcat(output, input+3);
  } else if (input[0]=='U' && input[1]=='p' && input[2]=='s') {
    strcat(output, "\\gu");
    strcat(output, input+3);
  } else if (input[0]=='P' && input[1]=='h' && input[2]=='i') {
    strcat(output, "\\gf");
    strcat(output, input+3);
  } else if (input[0]=='C' && input[1]=='h' && input[2]=='i') {
    strcat(output, "\\gx");
    strcat(output, input+3);
  } else if (input[0]=='P' && input[1]=='s' && input[2]=='i') {
    strcat(output, "\\gq");
    strcat(output, input+3);
  } else if (input[0]=='O' && input[1]=='m' && input[2]=='e') {
    strcat(output, "\\gw");
    strcat(output, input+3);
  } else {
    strcat(output, input);
  }
}


/**.......................................................................
 * Find the star nearest the cursor
 */
int StarPlot::identify(double x, double y)
{
  int i, id;
  double dx, dy, rsq, rsqmin;

  id = -1;
  rsqmin = 0.25;

  for (i=0; i < starlist_.size(); i++) {
    dx = starlist_[i].x_ - x;
    dy = starlist_[i].y_ - y;
    rsq = dx*dx + dy*dy;

    if (rsq < rsqmin) {
      rsqmin = rsq;
      id = i;
    }

  }

  return id;
}


/**-----------------------------------------------------------------------
 *  Find current UTC and return as Modified Julian date 
 */
double StarPlot::getTime(void)
{
  double date;
  int j;
  time_t now = time(NULL);
  struct tm *t = gmtime(&now);
  slaCldj(1900+t->tm_year, 1+t->tm_mon, t->tm_mday, &date, &j);
  if (j != 0)
    fputs("Bad date received from gmtime\n", stderr);
  return date + (t->tm_hour + (t->tm_min + (t->tm_sec)/60.0)/60.0)/24.0;
}

/**--------------------------------------------------------------------
 * Select/deselect radio sources (i.e., stars with magnitude > 30) 
 */
void StarPlot::setRadioVis(int vis)
{
  int i;

  for (i=0; i < starlist_.size(); i++) {
    if (starlist_[i].mag_ > 30.0) {
      starlist_[i].vis_ = vis;
    }
  }
}

/**--------------------------------------------------------------------
 * Set the distance from the sun to display
 */
void StarPlot::setSunDist(Angle& angle)
{
  for(unsigned i=0; i < starlist_.size(); i++) {
    if(strcasecmp((char*)starlist_[i].name_.c_str(), "Sun")==0) {
      starlist_[i].dist_ = angle;
    } 
  }
}

/**--------------------------------------------------------------------
 * Set the distance from the moon to display
 */
void StarPlot::setMoonDist(Angle& angle)
{
  for(unsigned i=0; i < starlist_.size(); i++) {
    if(strcasecmp((char*)starlist_[i].name_.c_str(), "Moon")==0) {
      starlist_[i].dist_ = angle;
    } 
  }
}

/**--------------------------------------------------------------------
 * Select/deselect by magnitude (ignoring "radio" sources)
 */
void StarPlot::setMagLim(double maglim)
{
  int i;

  for (i=0; i < starlist_.size(); i++) {
    if (starlist_[i].mag_ < 30.0) {
      starlist_[i].vis_ = (starlist_[i].mag_ < maglim);
    }
  }

  magLimIsSet_ = true;
  magLim_      = maglim; 
}

/**--------------------------------------------------------------------
 * Select/deselect by flux (ignoring "optical" sources)
 */
void StarPlot::setFluxLim(Flux fluxLim)
{
  int i;

  for (i=0; i < starlist_.size(); i++) {
    if (starlist_[i].type_ & StarPlot::RADIO)
      starlist_[i].vis_ = (starlist_[i].flux_.Jy() > fluxLim.Jy());
  }

  fluxLimIsSet_ = true;
  fluxLim_      = fluxLim; 
}

/*--------------------------------------------------------------------*/

void StarPlot::drawMoon(double phase, int bg, int fg)
{
  int i;
  float x[40], y[40];
  float cosphase, sign;

  //  cpgsave();
  cpgwnad(-1.1, 1.1, -1.1, 1.1);
  cpgsci(bg);
  cpgrect(-1.1, 1.1, -1.1, 1.1);
  cpgsci(0);
  cpgsfs(2);

  /* Draw outline of moon's disk */

  cpgcirc(0.0, 0.0, 1.0);

  /* Compute visible part */

  cosphase = cos(2.0*M_PI*phase);

  if (phase < 0.5) {
    sign = +1;
  } else {
    sign = -1;
  }
  for (i=0; i<20; i++) {
    x[i] = sign*sin(M_PI*i/20.0);
    y[i] = cos(M_PI*i/20.0);
  }
  for (i=20; i<40; i++) {
    x[i] = x[i-20]*cosphase;
    y[i] = -y[i-20];
  }

  /* Draw  visible part */

  cpgsfs(1);
  cpgsci(fg);
  cpgpoly(40, x, y);
  //  cpgunsa();
}

/**--------------------------------------------------------------------
 * List names of all stars with non-zero mark, in numerical order of
 * mark
 */
void StarPlot::listMarkedStars()
{
  Mark marklist[MAXSTAR];
  int i;
  int nmarked = 0;

  /* Make a list of the marked stars */

  for (i=0; i < starlist_.size(); i++) {
    if (starlist_[i].mark_) {
      marklist[nmarked].mark = starlist_[i].mark_;
      marklist[nmarked].id = i;
      nmarked++;
    }	
  }

  /* Sort the list into mark order */

  if (nmarked > 0)
    qsort(marklist, nmarked, sizeof(Mark),
	  (int(*)(const void*, const void*)) markCmp);

  /* Print the list */

  if (nmarked > 0) {
    printf("The following %d star%s marked:\n", nmarked,
	   nmarked > 1 ? "s are" : " is");
    for (i=0; i<nmarked; i++) {
      printf("  %s", (char*)starlist_[marklist[i].id].name_.c_str());
      if (i < nmarked-1)
	printf(",");
      if (i%8 == 7 || i == nmarked-1)
	printf("\n");
    }
  } else {
    printf("No stars are marked\n");
  }
}

/**-----------------------------------------------------------------------
 * Comparison function for application of qsort() to the marklist 
 */
int StarPlot::markCmp(const Mark* a, const Mark* b)
{
  return a->mark - b->mark;
}

/**.......................................................................
 * Scan the named file for possible star names (any alphanumeric
 * string is a candidate). Each name is checked to see if it is the
 * name of a star in the current star catalog (case-insensitive
 * comparison), and if it, that star is marked. 
 */
void StarPlot::markStarsFromFile(char *file)
{
  FILE *fp;
  int ch, i, inword = 0;
  char word[NAMELEN];

  fp = fopen(file, "r");
  if (!fp) {
    fprintf(stderr, "Can't open file \"%s\"\n", file);
    return;
  }

  while ((ch = fgetc(fp)) != EOF) {
    if (isalpha(ch) || isdigit(ch) || ch == '-' || ch == '+') {
      if (inword < NAMELEN-1) {
	word[inword] = ch;
	inword += 1;
      }
    } else if (inword) {
      word[inword] = '\0';
      for (i=0; i <starlist_.size(); i++) {
	if (strcasecmp(word, (char*)starlist_[i].name_.c_str()) == 0) {
	  starlist_[i].mark_ = ++(markCounter_);
	  break;
	}
      }
      inword = 0;
    }
  }

  if (fclose(fp) == EOF) {
    fprintf(stderr, "Can't close file \"%s\"\n", file);
  }

  return;
}

/* Convert MJD to printable date string, using MCS routines */

void StarPlot::mjdToDate(double mjd, char* date_string, int size)
{
  OutputStream *output;
  output = new_OutputStream();
  if(!output)
    goto error;
  if(open_StringOutputStream(output, 1, date_string, size)) {
    del_OutputStream(output);
    goto error;
  };
  if(output_utc(output, "", 0, 0, mjd)) {
    del_OutputStream(output);
    goto error;
  };
  del_OutputStream(output);
  return;

error:
  strncpy(date_string, "[error]", size);
  return;
}


float StarPlot::Object::size()
{
  static float symMin  = 0.5;
  static float symMax  = 2;
  static float fluxMin = 1;
  static float fluxMax = 15;
  static float alpha = (fluxMax * symMin - fluxMin * symMax)/(symMax - symMin);
  static float n = (fluxMax + alpha)/symMax;

  if(type_ & StarPlot::RADIO) {
    if(flux_.Jy() < fluxMin) {
      return symMin;
    } else {
      return (flux_.Jy() + alpha)/n;
    }
  }

  if(strcasecmp((char*)name_.c_str(), "Sun")==0) {
    return 1.0;
  } else if(strcasecmp((char*)name_.c_str(), "Moon")==0) {
    return 1.0;
  } else if(type_ == StarPlot::SOLAR) {
    
    return 2*diam_.arcsec()/45;
    
  } else {

    if (mag_ > 30.0) {
      return 1.0;
    } else if (mag_ < 0.5) {
      return 2.0;
    } else {
      return 2.0/(mag_ + 0.5);
    }

  }
}

unsigned StarPlot::Object::sym()
{
  if(type_ & StarPlot::RADIO) {
    return 768;
  }

  if(strcasecmp((char*)name_.c_str(), "Sun")==0)
    return 9;
  else if(strcasecmp((char*)name_.c_str(), "Moon")==0)
    return 2291;
  else if(type_ == SOLAR) {
    return 17;
  } else {
    if(mag_ > 30.0) {
      return 3;
    } else if (mag_ < 0.5) {
      return 18;
    } else {
      return 18;
    }
  }
}

unsigned StarPlot::Object::color()
{
  if(type_ & StarPlot::RADIO) 
    return 12;

  if(mag_ > 30.0) {
    return 3;
  } else if (mag_ < 0.5) {
    return 2;
  } else {
    return 2;
  }
}

void StarPlot::openDevice(char* device)
{
  if(pgplotId_ <= 0)
    pgplotId_ = cpgopen(device);

  if(pgplotId_ <= 0)
    ThrowError("Unable to open pgplot device: " << device);
}

void StarPlot::redraw()
{
  if(pgplotId_ <= 0)
    ThrowError("No plot device");

  cpgslct(pgplotId_);
  cpgask(0);

  cpgscr(16, 0.65, 0.75, 0.85);
  cpgpage();

  plotPolar(getMjd());

  //  cpgebuf();
}

void StarPlot::setDisplayMjd(TimeVal mjd)
{
  displayMjd_ = mjd;
  mjdIsSet_ = true;
  lstIsSet_ = false;
}

void StarPlot::clearDisplayMjd()
{
  mjdIsSet_ = false;
}

void StarPlot::setDisplayLst(HourAngle lst)
{
  displayLst_ = lst;
  lstIsSet_ = true;
  mjdIsSet_ = false;
}

void StarPlot::clearDisplayLst()
{
  lstIsSet_ = false;
}

double StarPlot::getMjd()
{
  // Get the current LST

  TimeVal tVal;
  tVal.setToCurrentTime();

  if(lstIsSet_) {

    // Calculate the time difference between now and the requested lst:
    
    HourAngle lstNow, dLst;
    lstNow.setRadians(slaDranrm(slaGmst(tVal.getMjd()) + dispLongitude_.radians()));
    
    dLst = displayLst_ - lstNow;

    tVal.incrementSeconds(dLst.seconds());

  } else if(mjdIsSet_) {
    tVal = displayMjd_;
  }

  return tVal.getMjd();
}

void StarPlot::setSiteName(std::string name)
{
  siteName_ = name;
}

void StarPlot::setSite(std::string site)
{
  for(unsigned iSite=0; iSite < knownSites_.size(); iSite++) {

    if(strcasecmp(site.c_str(), knownSites_[iSite].name_.c_str())==0) {

      setSiteName(knownSites_[iSite].name_);
      setLatitude(knownSites_[iSite].latitude_);
      setLongitude(knownSites_[iSite].longitude_);
      return;
    }

  }
  
  ThrowSimpleError("Unrecognized site: " << site);
}

void StarPlot::setSite(std::string site, Angle& lat, Angle& lng)
{
  for(unsigned iSite=0; iSite < knownSites_.size(); iSite++) {

    if(strcasecmp(site.c_str(), knownSites_[iSite].name_.c_str())==0) {

      setSiteName(knownSites_[iSite].name_);
      setLatitude(knownSites_[iSite].latitude_);
      setLongitude(knownSites_[iSite].longitude_);

      lat = knownSites_[iSite].latitude_;
      lng = knownSites_[iSite].longitude_;

      return;
    }

  }
  
  ThrowSimpleError("Unrecognized site: " << site);
}

StarPlot::Object StarPlot::mark(double x, double y)
{
  int i, id;
  double dx, dy, rsq, rsqmin;
  bool first=true;

  for (i=0; i < starlist_.size(); i++) {
    dx = starlist_[i].x_ - x;
    dy = starlist_[i].y_ - y;
    rsq = dx*dx + dy*dy;
  
    if (first || rsq < rsqmin) {
      rsqmin = rsq;
      id = i;
      first = false;
    }
  }

  starlist_[id].mark_ = true;

  return starlist_[id];
}

void StarPlot::clearMarks()
{
  for (unsigned i=0; i < starlist_.size(); i++) {
    starlist_[i].mark_ = false;
  }
}

/**--------------------------------------------------------------------
 * List names of all stars with non-zero mark, in numerical order of
 * mark
 */
void StarPlot::showMarkedStars()
{
  std::ostringstream os;
  float ch;

  cpgqch(&ch);
  cpgsch(0.8);

  for (unsigned i=0; i < starlist_.size(); i++) {
    if (starlist_[i].mark_ && starlist_[i].vis_) {
      
      Object& obj = starlist_[i];

      os << "Marked source is: ";
      cpgmtxt("B", -6.0, 0.0, 0.0, os.str().c_str());

      cpgsci(7);
      os.str("");
      os << " " << starlist_[i].name_ << " (" << starlist_[i].survey_ << ")";

      if(starlist_[i].type_ == StarPlot::SOLAR) {

	if(starlist_[i].diam_.arcmin() > 1.0)
	  os << " (" << std::setprecision(2) << starlist_[i].diam_.degrees() << "°)";
	else
	  os << " (" << std::setprecision(2) << starlist_[i].diam_.arcsec() << "\")";

      } else if(starlist_[i].type_ & StarPlot::OPTICAL) {
	os << " (mag = " << std::setprecision(2) << starlist_[i].mag_ << ")";
      } else if(starlist_[i].type_ & StarPlot::GCPCAL) {
	os << " (" << std::setprecision(2) << starlist_[i].flux_.Jy() << " Jy, " << std::setprecision(3) << starlist_[i].specInd_ << ")";
      } else if(starlist_[i].type_ & StarPlot::RADIO) {
	os << " (flux = " << std::setprecision(2) << starlist_[i].flux_.Jy() << " Jy)";
      }

      cpgmtxt("B", -4.0, 0.0, 0.0, os.str().c_str());

      // Now print the location

      os.str("");

      HourAngle ra;
      ra.setRadians(obj.ra_);

      Declination dec;
      dec.setRadians(obj.dec_);

      os << ra << " " << dec;

      cpgmtxt("B", -2.0, 0.0, 0.0, os.str().c_str());

      cpgsci(1);
    }	
  }

  cpgsch(ch);

}

PtSrcReader* StarPlot::ptSrcReader(std::string survey)
{
  if(strcasecmp((char*)survey.c_str(), "nvss")==0)
    return &nvss_;
  else if(strcasecmp((char*)survey.c_str(), "first")==0)
    return &first_;
  else if(strcasecmp((char*)survey.c_str(), "cal")==0)
    return &gcpCal_;
  else if(strcasecmp((char*)survey.c_str(), "ned")==0)
    return &ned_;
  else {
    ThrowError("Unrecognized survey: " << survey);
    return 0;
  }
}

unsigned StarPlot::addFlags(std::string survey)
{
  if(strcasecmp((char*)survey.c_str(), "nvss")==0)
    return NVSS;
  else if(strcasecmp((char*)survey.c_str(), "first")==0)
    return FIRST;
  else if(strcasecmp((char*)survey.c_str(), "cal")==0)
    return GCPCAL;
  else if(strcasecmp((char*)survey.c_str(), "ned")==0)
    return NED;
  else {
    ThrowError("Unrecognized survey: " << survey);
    return UNKNOWN;
  }
}

void StarPlot::readPtSrc(std::string survey, Flux& fmin, Flux& fmax)
{
  Angle radius;
  radius.setDegrees(90.0);
  readPtSrc(survey, fmin, fmax, radius, dispLongitude_, dispLatitude_);
}

void StarPlot::readPtSrc(std::string survey, Angle& radius, Angle& lng, 
			 Angle& lat)
{
  Flux fmin, fmax;
  fmin.setJy(0.0);
  fmax.setJy(10000);
  readPtSrc(survey, fmin, fmax, radius, lng, lat);
}

void StarPlot::readPtSrc(std::string survey, Flux& fmin, Flux& fmax, 
			 Angle& radius, Angle& lng, Angle& lat)
{
  PtSrcReader* reader = ptSrcReader(survey);
  unsigned flags = addFlags(survey) | RADIO;
  TimeVal tVal;
  tVal.setToCurrentTime();

  double mjd = tVal.getMjd();

  /* Find current LST (approximate; ignores equation of equinoxes) */

  double lst = slaDranrm(slaGmst(mjd) + lng.radians());  

  HourAngle ra;
  ra.setRadians(lst);

  Declination dec;
  
  dec.setRadians(lat.radians());
  
  std::vector<PtSrcReader::Source> sources = 
    reader->findSources(ra, dec, radius, fmin, fmax, false);

  for(unsigned i=0; i < sources.size(); i++) {
    Object obj(sources[i]);
    obj.type_ = flags;
    float angle = rand() * 360.0/RAND_MAX;
    obj.angle_.setDegrees(angle);

    starlist_.push_back(obj);
  }

  removeDuplicates();
}

StarPlot::Object::Object()
{
  type_  = StarPlot::UNKNOWN;
  vis_   = true;
  mark_  = false;
  dist_.setDegrees(0.0);
}

StarPlot::Object::Object(PtSrcReader::Source& src) 
{
  name_    = src.name_.str();
  survey_  = src.survey_;
  ra_      = src.ra_.radians();
  dec_     = src.dec_.radians();
  flux_    = src.peak_;
  type_    = StarPlot::RADIO;
  vis_     = true;
  mark_    = false;
  specInd_ = src.specInd_;
  dist_.setDegrees(0.0);
}

void StarPlot::setRange(double xa, double xb, double ya, double yb)
{
  Angle az1, az2;
  Angle el1, el2;

  deprojectZea(xa, ya, az1, el1);
  Declination lat1;
  lat1.setRadians(dispLatitude_.radians() + el1.radians() - M_PI/2);

  deprojectZea(xb, yb, az2, el2);
  Declination lat2;
  lat2.setRadians(dispLatitude_.radians() + el2.radians() - M_PI/2);

  Angle dist = distance(az1, el1, az2, el2);

  // Calculate the longitude and latitude of the anchor point,
  // relative to the current latitude and longitude

  Vector<double> ll = Coordinates::lngLatAndAzElToLngLat(dispLongitude_, dispLatitude_, az1, el1);

  // Set the center of the displayed range to the anchor point

  dispLongitude_.setRadians(ll[0]);
  dispLatitude_.setRadians(ll[1]);
  dispElLimit_.setRadians(M_PI/2 - dist.radians());
}

void StarPlot::searchNed(double xa, double xb, double ya, double yb)
{
  Angle az1, az2;
  Angle el1, el2;

  deprojectZea(xa, ya, az1, el1);
  deprojectZea(xb, yb, az2, el2);

  Angle dist = distance(az1, el1, az2, el2);

  // Calculate the longitude and latitude of the anchor point,
  // relative to the current latitude and longitude

  Vector<double> ll = Coordinates::lngLatAndAzElToLngLat(dispLongitude_, 
							 dispLatitude_, az1, el1);

  // Set the center of the displayed range to the anchor point

  Angle lng, lat;
  lng.setRadians(ll[0]);
  lat.setRadians(ll[1]);

  readPtSrc("ned", dist, lng, lat);
}

void StarPlot::setRange(Angle& lat, Angle& lng, Angle& el)
{
  dispLatitude_  = lat;
  dispLongitude_ = lng;
  dispElLimit_   = el;
}

void StarPlot::setRange()
{
  dispLatitude_  = latitude_;
  dispLongitude_ = longitude_;
  dispElLimit_   = elLimit_;
}

Angle StarPlot::distance(Angle& ra1, Angle& dec1, Angle& ra2, Angle& dec2)
{
  Vector<double> v1(3);
  Vector<double> v2(3);

  v1[0] = cos(dec1.radians()) * cos(ra1.radians());
  v1[1] = cos(dec1.radians()) * sin(ra1.radians());
  v1[2] = sin(dec1.radians());

  v2[0] = cos(dec2.radians()) * cos(ra2.radians());
  v2[1] = cos(dec2.radians()) * sin(ra2.radians());
  v2[2] = sin(dec2.radians());

  Angle dist;
  dist.setRadians(acos(v1*v2));

  return dist;
}

void StarPlot::removeDuplicates()
{
  //  starlist_.sort(StarPlot::Object::isLessThan);
  //  starlist_.unique(StarPlot::Object::isLessThan);
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
std::ostream& 
gcp::util::operator<<(std::ostream& os, StarPlot::Object& obj)
{
  os << "  Name:   " << " " << obj.name_     << std::endl;
  os << "  Origin: " << " " << obj.origin() << std::endl;

  // If the source was from a survey or list, print that now

  if(obj.type_ & StarPlot::SRCLIST) {
    os << "  File:   " << " " << obj.survey_   << std::endl;
  } else if(obj.type_ & StarPlot::SOLAR) {

  } else {
    os << "  Survey: " << " " << obj.survey_   << std::endl;
  }

  // Now print any physical information we know about the source

  if(obj.type_ == StarPlot::SOLAR) {

    if(obj.diam_.arcmin() > 1.0)
      os << "  Size:   " << " " << std::setprecision(2) << obj.diam_.degrees() << " deg";
    else
      os << "  Size:   " << " " << std::setprecision(2) << obj.diam_.arcsec() << " asec";
    
  } else if(obj.type_ == StarPlot::OPTICAL) {
    os <<   "  Mag:    " << " " << std::setprecision(2) << obj.mag_;
  } else if(obj.type_ & StarPlot::GCPCAL) {
    os <<   "  Flux:   " << " " << std::setprecision(2) << obj.flux_.Jy() << " Jy, spectral index: " << std::setprecision(3) << obj.specInd_;
  } else if(obj.type_ & StarPlot::RADIO) {
    os <<   "  Flux:   " << " " << std::setprecision(2) << obj.flux_.Jy() << " Jy";
  }

  return os;
}

std::string StarPlot::Object::origin()
{
  if(type_ & SOLAR)
    return "Solar system source";

  if(type_ & SRCLIST)
    return "Local source list";

  if(type_ & NVSS)
    return "Local NVSS Catalog";

  if(type_ & FIRST)
    return "Local FIRST Catalog";

  if(type_ & GCPCAL)
    return "GCP calibrator list";

  if(type_ & NED)
    return "NED query";
}

/**.......................................................................
 * Return the az, el of a circle of npts at an angular separation of
 * radius from the fiducial az, el
 */
void StarPlot::getAzElCircle(Angle& az, Angle& el, Angle& radius, unsigned npt,
			     std::vector<double>& azs, std::vector<double>& els) 
{
  azs.resize(npt);
  els.resize(npt);

  // Form the rotation matrix for converting from the coordinate
  // system around the nominal pointing to the UEN coordinate system

  gcp::util::Matrix<double> azRot(3,3);
  gcp::util::Matrix<double> elRot(3,3);

  double caz = cos(az.radians());
  double saz = sin(az.radians());
  double cel = cos(el.radians());
  double sel = sin(el.radians());

  azRot[0][0] =  caz; azRot[0][1] =  saz; azRot[0][2] =   0;
  azRot[1][0] = -saz; azRot[1][1] =  caz; azRot[1][2] =   0;
  azRot[2][0] =    0; azRot[2][1] =    0; azRot[2][2] =   1;

  elRot[0][0] =    1; elRot[0][1] =    0; elRot[0][2] =   0;
  elRot[1][0] =    0; elRot[1][1] =  sel; elRot[1][2] = cel;
  elRot[2][0] =    0; elRot[2][1] = -cel; elRot[2][2] = sel;

  Matrix<double> rot = (azRot * elRot);

  double phi, dphi = 2*M_PI/npt;
  double crad = cos(radius.radians());
  double srad = sin(radius.radians());

  Vector<double> vec(3);
  Vector<double> enu(3);

  for(unsigned i=0; i < npt; i++) {
    phi = i * dphi;

    // Get the coordinates of this point in a coordinate system
    // aligned with the source pointing direction

    vec[0] = srad * cos(phi);
    vec[1] = srad * sin(phi);
    vec[2] = crad;

    // Now rotate into the ENU coordinate system

    enu = rot * vec;

    // And compute the new az, el position

    azs[i] = atan2(enu[0], enu[1]);
    els[i] = asin(enu[2]/enu.magnitude());
  }
}
