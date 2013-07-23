// $Id: StarPlot.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_UTIL_STARPLOT_H
#define GCP_UTIL_STARPLOT_H

/**
 * @file StarPlot.h
 * 
 * Tagged: Thu May 10 20:19:34 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author username: Command not found.
 */
#include <string>
#include <vector>
#include <iostream>

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Flux.h"
#include "gcp/util/common/FirstFitsReader.h"
#include "gcp/util/common/NvssReader.h"
#include "gcp/util/common/NedReader.h"
#include "gcp/util/common/SzaCalReader.h"

namespace gcp {
  namespace util {

    class StarPlot {
    public:

      enum {
	UNKNOWN =   0x0,
	SOLAR   =   0x2,
	OPTICAL =   0x4,
	RADIO   =   0x8,

	SRCLIST =  0x10,
	NVSS    =  0x20,
	FIRST   =  0x40,
	GCPCAL  =  0x80,
	NED     = 0x100,
      };

      struct Object {

	std::string name_;
	std::string survey_;
	double ra_;
	double dec_;
	double mag_;
	double az_;
	double el_;
	double x_;
	double y_;
	Flux flux_;
	double specInd_;
	Angle diam_;
	Angle maj_;
	Angle majerr_;
	Angle min_;
	Angle minerr_;
	int vis_;
	int mark_;
	Angle dist_;
	Angle angle_;

	unsigned type_;
	unsigned nedFlags_;
	
	float size();
	unsigned sym();
	unsigned color();

	Object();

	Object(const Object& obj) {
	  *this = (Object&)obj;
	};

	Object(Object& obj) {
	  *this = obj;
	};

	Object(PtSrcReader::Source& src);

	void operator=(const Object& obj) {
	  *this = (Object&)obj;
	};

	void operator=(Object& obj) {

	  name_      = obj.name_;
	  survey_    = obj.survey_;
	  ra_        = obj.ra_;
	  dec_       = obj.dec_;
	  mag_       = obj.mag_;
	  flux_      = obj.flux_;
	  az_        = obj.az_;
	  el_        = obj.el_;
	  x_         = obj.x_;
	  y_         = obj.y_;
	  diam_      = obj.diam_;
	  vis_       = obj.vis_;
	  mark_      = obj.mark_;
	  type_      = obj.type_;
	  nedFlags_  = obj.type_;
	  specInd_   = obj.specInd_;
	  dist_      = obj.dist_;
	  angle_     = obj.angle_;
	};

	static bool isLessThan(StarPlot::Object& obj1, StarPlot::Object& obj2) 
	{
	  return (obj1.survey_ < obj2.survey_) || (obj1.survey_ == obj2.survey_ && obj1.name_ <= obj2.name_);
	}

	static bool isEqualTo(StarPlot::Object& obj1, StarPlot::Object& obj2) 
	{
	  return (obj1.survey_ == obj2.survey_ && obj1.name_ == obj2.name_);
	}

	// Format where this source came from

	std::string origin();

	friend std::ostream& operator<<(std::ostream& os, Object& obj);
      };

      struct Rect {
	double x1;
	double x2;
	double y1;
	double y2;
      };

      struct Mark {
	int mark;
	int id;
      };

      struct Site {
	std::string name_;
	Angle latitude_;
	Angle longitude_;
      };

      /**
       * Constructor.
       */
      StarPlot();

      /**
       * Destructor.
       */
      virtual ~StarPlot();

      void initCatalog();

      void setCatalog(std::string catalog);
      void readCatalog(std::string catalog);
      void readCatalog();

      void readCal(std::string catalog);

      void setLatitude(Angle latitude);
      void setLongitude(Angle longitude);
      void setElevationLimit(Angle el);

      void plotPolar();
      void plotPolar(double mjd);

      void openDevice(char* device);
      void setSiteName(std::string name);

      void setMagLim(double maglim);
      void setFluxLim(Flux flux);

      void setSunDist(double dist);
      void setMoonDist(double dist);

      void setSite(std::string site);
      void setSite(std::string site, Angle& lat, Angle& lng);

      void redraw();

      void removeDuplicates();

      StarPlot::Object mark(double x, double y);
      void clearMarks();
      void showMarkedStars();
      PtSrcReader* ptSrcReader(std::string survey);

      void readPtSrc(std::string survey, Flux& fmin, Flux& fmax);
      void readPtSrc(std::string survey, Flux& fmin, Flux& fmax, Angle& radius, Angle& lng, Angle& lat);
      void readPtSrc(std::string survey, Angle& radius, Angle& lng, Angle& lat);

      unsigned addFlags(std::string survey);
      
      void setDisplayLst(HourAngle lst);
      void clearDisplayLst();

      void setDisplayMjd(TimeVal mjd);
      void clearDisplayMjd();

      void setRange(double xa, double xb, double ya, double yb);
      void setRange(Angle& lat, Angle& lng, Angle& el);
      void setRange();

      void searchNed(double xa, double xb, double ya, double yb);
      void setSunDist(Angle& angle);
      void setMoonDist(Angle& angle);

    private:

      Angle distance(Angle& ra1, Angle& dec1, Angle& ra2, Angle& dec2);

      double parseSexagesimal(char *string);
      double zeap(double theta);

      void projectZea(double az, double el, double *x, double *y);
      void deprojectZea(double x, double y, Angle& az, Angle& el);

      int identify(double x, double y);
      void greekify(char *input, char *output);
      void initBox(Rect& box);
      double getTime(void);
      void setRadioVis(int radio_vis);
      void drawMoon(double phase, int bg, int fg);
      static int markCmp(const Mark* a, const Mark* b);
      void markStarsFromFile(char* file);
      void mjdToDate(double mjd, char* date_string, int size);
      void listMarkedStars();
      double getMjd();

    public:
      void drawCircle(double az, double el, Angle& radius, unsigned npt);

      // Return the az, el of a circle of npts at an angular
      // separation of radius from the fiducial az, el

      void getAzElCircle(Angle& az, Angle& el, Angle& radius, unsigned npt, 
			 std::vector<double>& azs, std::vector<double>& els);

      std::string catalogFile_;
      std::vector<Object> starlist_;
      double magLim_;
      bool magLimIsSet_;

      Flux fluxLim_;
      bool fluxLimIsSet_;

      unsigned markCounter_;
      double maxmag_;
      unsigned max_;

      // The site and elevation limit

      gcp::util::Angle latitude_;
      gcp::util::Angle longitude_;
      gcp::util::Angle elLimit_;

      // The site and elevation limit to display

      bool isZoomed_;
      gcp::util::Angle dispLatitude_;
      gcp::util::Angle dispLongitude_;
      gcp::util::Angle dispElLimit_;

      gcp::util::HourAngle displayLst_;
      bool lstIsSet_;

      gcp::util::TimeVal displayMjd_;
      bool mjdIsSet_;

      int pgplotId_;

      std::string siteName_;
      std::vector<Site> knownSites_;
      
      NvssReader nvss_;
      FirstFitsReader first_;
      SzaCalReader gcpCal_;
      NedReader ned_;

    }; // End class StarPlot

  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_STARPLOT_H
