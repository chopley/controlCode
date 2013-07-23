// $Id: PgUtil.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_UTIL_PGUTIL_H
#define GCP_UTIL_PGUTIL_H

/**
 * @file PgUtil.h
 * 
 * Tagged: Fri Aug 22 11:03:53 PDT 2008
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author username: Command not found.
 */
#include "gcp/pgutil/common/color_tab.h"
#include <string>

namespace gcp {
  namespace util {

    class PgUtil {
    public:

      /**
       * Constructor.
       */
      PgUtil();

      /**
       * Destructor.
       */
      virtual ~PgUtil();

      enum {
	BLACK        = 0,
	WHITE        = 1,
	RED          = 2,
	GREEN        = 3,
	DEEP_BLUE    = 4,
	PALE_BLUE    = 5,
	MAGENTA      = 6,
	YELLOW       = 7,
	ORANGE       = 8,
	YELLOW_GREEN = 9,
	SAGE_GREEN   = 10,
	SLATE_BLUE   = 11,
	PURPLE       = 12,
	PINK         = 13,
	DARK_GRAY    = 14,
	DARK_GREY    = 14,
	LIGHT_GRAY   = 15,
	LIGHT_GREY   = 15,
      };

      static int open(std::string device);

      static void close();

      static int v_grey2(int ndata, float *zdata, int nx,int ny, 
			 float xmina=0, float xmaxa=1, float ymina=0, float ymaxa=1, 
			 float *flag=0,float z1=0, float z2=0, 
			 char *xlab="", char *ylab="", char *title="", char *unit="");

      static int v_radplot(float data[],int nbin, float rmin, float rmax, 
			   float xmin, float xmax, float ymin, float ymax, int nx, int ny);

      // Plot a simple line plot.
      
      static int v_lplot(int narr, float xarr[], float yarr[], char xlab[], char ylab[],
			 char title[]);

      static void setOverplot(bool overplot) {
	overplot_ = overplot;
      }

      static void setVp(bool vp) {
	vp_ = vp;
      }

      static void setTick(bool tick) {
	tick_ = tick;
      }

      static void setLabel(bool label) {
	label_ = label;
      }

      static void setWedge(bool wedge) {
	wedge_ = wedge;
      }

      static void setUsedefs(bool usedefs) {
	usedefs_ = usedefs;
      }

      static void setXmin(float xmin) {
	xmin_ = xmin;
      }

      static void setXmax(float xmax) {
	xmax_ = xmax;
      }

      static void setYmin(float ymin) {
	ymin_ = ymin;
      }

      static void setYmax(float ymax) {
	ymax_ = ymax;
      }

      static void setColormap(std::string cmap);

    private:

      static Cmap* cmap_;
      static bool overplot_;
      static bool vp_;
      static bool tick_;
      static bool label_;
      static bool wedge_;
      static bool usedefs_;
      static float xmin_, xmax_;
      static float ymin_, ymax_;

      // Module for v_lplot.

      static int v_lnewx(float xmins, float xmaxs, float ymins, float ymaxs, 
			 float *xmin, float *xmax, float *ymin, float *ymax, 
			 int narr,float xarr[], float yarr[]);
	
      // Module for v_lplot.

      static int v_lwrite(int narr, float xarr[], float yarr[], float xmin,
			  float xmax, float ymin, float ymax);

      // Module for v_lplot.

      static int v_lnum(int narr, float xarr[], float yarr[], float xmin,
			float xmax, float ymin, float ymax);

      // Module for v_lplot.
      
      static int v_lten(int narr, float xarr[], float yarr[], float xmin,
			float xmax, float ymin, float ymax);

      // Module for v_hist.

      static int v_lzoom(float xmins, float xmaxs, float ymins, float ymaxs, 
			 float *xmin, float *xmax, float *ymin, float *ymax);

      // Module for v_lplot -- Draw the line plot.
      
      static int v_ldraw(int narr, float xarr[], float yarr[], char xlab[], char ylab[], char title[]);

    }; // End class PgUtil

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_PGUTIL_H
