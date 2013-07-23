#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

#include <cpgplot.h>

#include "axes.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

/*
 * The tick_interval() function fills an object of the following type
 * with information about the major and minor tick intervals to use
 * with a given range of axis values. The axis values at which major
 * ticks should be placed are given by:
 *
 *  major_tick = offset + j * major_step   [j=ja..jb]
 *
 * Similarly, minor ticks should be placed at:
 *
 *  minor_tick = offset + i * minor_step   [i=ia..ib when i % nminor != 0]
 *
 * If the range of values on the axis is much smaller than the magnitude
 * of values on the axis then the major axis labels will become long and
 * may overlap. One way to avoid this it is to switch to composing major
 * labels that have had a large offset removed. That offset is then displayed
 * as a separate label. For this purpose tick.offset has been chosen to be
 * suitable to be used as such an offset. It is suggested that you switch
 * to this mode when tick.maj_prec > 5. In the following
 * notes this mode is refered to as incremental while the normal mode in
 * which the offset is included in the label, is called composite.
 *
 * When rendering composite labels or the offset for incremental labels
 * you should display tick.maj_prec significant figures. This can be
 * accomplished with the precision value of the printf %g directive.
 * When rendering incremental labels then the equivalent number of
 * significant figures to display is inc_prec. Using these values will
 * ensure that the change in value from one major tick to the next will
 * be visible, while preventing the false extra digits that sometimes
 * are generated because a fractional decimal value can't be represented
 * exactly in binary.
 *
 * When rendering composite labels or the offset label, it is suggested
 * that you switch to using scientific notation when
 * 4 < tick.maj_exp < -4. To display scientific notation the
 * mantissa is given by tick_value / tick.maj_scale and the exponent
 * is given by tick.maj_exp. The equivalent values for rendering
 * incremental tick labels are tick.inc_exp, tick.inc_scale and
 * tick.inc_prec.
 *
 * Note that render_label() composes pgplot label strings, using the
 * suggested rules given above.
 */
typedef struct {
  double offset;      /* The offset to add to major axis incremental */
                      /*  tick values. */
  double major_step;  /* The interval between major tick marks */
  int nmajor;         /* The number of visible major-axis ticks */
  int ja,jb;          /* The major-axis tick indexes of the first and last */
                      /*  visible ticks on the axis. */
  int maj_prec;       /* The number of significant figures to display when */
                      /*  rendering composite major-axis tick labels */
  int maj_exp;        /* The power-of-ten exponent to use when rendering */
                      /*  composite major-axis labels using scientific */
                      /*  notation. */
  double maj_scale;   /* The value to divide a composite tick value by */
                      /*  to form a scientific-notation mantissa. */
  int inc_prec;       /* The number of significant figures to display when */
                      /*  rendering incremental major-axis tick labels */
  int inc_exp;        /* The power-of-ten exponent to use when rendering */
                      /*  incremental major-axis labels using scientific */
                      /*  notation. */
  double inc_scale;   /* The value to divide a composite tick value by */
                      /*  to form a scientific-notation mantissa. */
  double minor_step;  /* The interval between minor tick marks */
  int nminor;         /* The number of visible minor-axis ticks */
  int ia,ib;          /* The minor-axis tick indexes of the first and last */
                      /*  visible minor axis ticks on the axis. */
} TickData;

static int tick_interval(int options, double va, double vb, TickData *tick);

static void render_label(double value, double scale, int prec, int exp,
			 char *label);
static char *trim_decimal_places(char *string);

/*
 * Plot a tick label and/or tick mark at a given normalized position
 * along a given axis line. The primary difference between this function
 * and pgtick() is that this function allows the justification of the
 * label to be specified. Given that the labels of utc axes are always
 * anotated with two levels of labels, both parallel to the axis,
 * allowance for arbitrary orientation angles between the labels and
 * the axis is not implemented in this function.
 */
static void plot_tick(int options, float x1, float y1, float x2, float y2,
		      float posn, float tikl, float tikr,
		      float disp, float just, char *text);

/*
 * Return the Modified Julian Day number of the first day of a given year.
 */
static long get_mjd_of_year(int year);

/*
 * Return the number of days in a given month.
 */
static int length_of_month(int year, int month);

/*
 * Parameterize the conversion factor used to scale an angle in radians
 * to the equivalent angle in degrees.
 */
#define R2D 57.29577951308232087679815481410517033240547246656458
#define PI_OVER_2 1.5707963267948966192313216916397514420985846996875

/*
 * Parameterize the offset between Julian Date and Modified Julian Date.
 */
#define MJD_OFFSET 2400000.5   /* mjd = jd - 2400000.5 */

/*
 * Split a Modified Julian Date into Julian Day Number and time of day
 * components.
 */
static int split_mjd(double mjd, long *jd, double *tod);

/*
 * Convert a Julian Day Number to Gregorian Calendar Date.
 */
static int jd_to_date(long jd, int *year, int *month, int *day);

/*
 * Convert a normalized time of day to clock time.
 */
static int tod_to_time(double tod, int *hour, int *minute, double *secs);

/*
 * Return the ordinal suffix of an integer.
 */
static char *ordinal_suffix(int ordinal);

static int plot_years(int options, float x1, float y1, float x2, float y2,
		      float majl, float majr, float minf, float disp,
		      double mjd1, double mjd2, int year1, int year2);
static int plot_months(int options, float x1, float y1, float x2, float y2,
		       float majl, float majr, float minf, float disp,
		       char *mnames[12], double mdmin, double mdmax,
		       int year, int m1lo, int m1hi, int m2lo, int m2hi,
		       int *yrtick);
static int plot_days(int options, float x1, float y1, float x2, float y2,
		     float majl, float majr, float minf, float disp,
		     int year, int month, double mdmin, double mdmax,
		     int *mntick);
static int plot_hours(int options, float x1, float y1, float x2, float y2,
		      float majl, float majr, float minf, float disp,
		      double hmin, double hmax, int *dytick);
static int plot_minutes(int options, float x1, float y1, float x2, float y2,
			float majl, float majr, float minf, float disp,
			double mmin, double mmax, int *hrtick);
static int plot_seconds(int options, float x1, float y1, float x2, float y2,
			float majl, float majr, float minf, float disp,
			double smin, double smax, int *mintick);
static int plot_subsec(int options, float x1, float y1, float x2, float y2,
		       float majl, float majr, float minf, float disp,
		       double smin, double smax);

/*.......................................................................
 * Plot and anotate a linear axis between given world coordinates.
 *
 * Input:
 *  options      int  A bit-wise union of AxisOptions enumerators.
 *  x1, y1     float  The world coordinates of one endpoint of the axis.
 *  x2, y2     float  The world coordinates of the other endpoint of the axis.
 *  mjd1,mjd2 double  The range of UTC's to plot, expressed as Modified Julian
 *                    Dates. Both mjd1 and mjd2 should be positive and mjd1
 *                    must be less than mjd2.
 *  majl,majr  float  The lengths of major ticks on the right and/or left
 *                    sides of the axis.
 *  minf       float  The lengths of minor axis ticks as a faction of the
 *                    the major axis ticks.
 *  disp       float  The displacement between the lines of text, measured
 *                    in character heights.
 * Output:
 *  return       int  0 - OK.
 *                    1 - Error.
 */
int linear_axis(int options, float x1, float y1, float x2, float y2,
		double v1, double v2, float majl, float majr,
		float minf, float disp)
{
  TickData tick;      /* Tick-mark details */
  char label[80];     /* The buffer in which to compose tick-labels */
  int split;          /* True to split labels into an offset plus increments */
  int i;
/*
 * If the axis would have zero length then there is nothing to be plotted.
 */
  if(x1 == x2 && y1 == y2)
    return 0;
/*
 * Determine suitable major and minor tick intervals.
 */
  if(tick_interval(options, v1, v2, &tick))
    return 1;
/*
 * Don't display the axis until finished.
 */
  cpgbbuf();
/*
 * Draw the axis line.
 */
  cpgmove(x1, y1);
  cpgdraw(x2, y2);
/*
 * If the size of the interval between major ticks is much less than
 * the size of an absolute tick, split the tick labels between a
 * large offset and small increments.
 */
  split = tick.maj_prec > 5 || tick.maj_exp < -3 || tick.maj_exp > 3;
  if(split && options & AXIS_WITH_LABELS) {
    strcpy(label, " ");
    if(fabs(tick.offset) > DBL_EPSILON) {
      render_label(tick.offset, tick.maj_scale, tick.maj_prec, tick.maj_exp,
		   label+strlen(label));
      strcat(label, " + ");
    };
    sprintf(label + strlen(label), "label x 10\\u%d\\d", tick.inc_exp);
    if(options & AXIS_OUTWARD_TEXT) {
      plot_tick(options & ~AXIS_OUTWARD_TEXT, x1, y1, x2, y2, 0.01,
		majl, majr, disp > 0 ? (1.0+4*disp):(4*disp-1.0), 0.0, label);
    } else {
      plot_tick(options, x1, y1, x2, y2, 0.0,
		majl, majr, disp > 0 ? (1.0+2*disp):(2*disp-1.0), 0.0, label);
    };
  };
/*
 * Plot the major and minor tick marks and anotate the major tick
 * marks.
 */
  for(i=tick.ja; i<=tick.jb; i++) {
    double increment = i * tick.major_step;
    double vmajor = tick.offset + increment;
/*
 * Compose a major-tick label, using scientific notation where
 * necessary.
 */
    if(options & AXIS_WITH_LABELS) {
      if(split)
	sprintf(label, "%.*f", tick.inc_prec-1, increment / tick.inc_scale);
      else
	render_label(vmajor, tick.maj_scale, tick.maj_prec, tick.maj_exp,
		     label);
    } else {
      label[0] = '\0';
    };
/*
 * Plot the label.
 */
    plot_tick(options, x1, y1, x2, y2, (vmajor-v1) / (v2-v1),
	      majl, majr, disp, 0.5, label);
  };
/*
 * Plot the minor tick marks.
 */
  for(i=tick.ia; i<=tick.ib; i++) {
    double vminor = tick.offset + i * tick.minor_step;
    plot_tick(options, x1, y1, x2, y2, (vminor-v1) / (v2-v1),
		majl * minf, majr * minf, 0.0, 0.5, "");
  };
/*
 * Reveal the axis.
 */
  cpgebuf();
  return 0;
}

/*.......................................................................
 * Given a range of Modified Julian Dates plot a date-and-time axis
 * between given world coordinates.
 *
 * Input:
 *  options      int  A bit-wise union of AxisOptions enumerators.
 *  x1, y1     float  The world coordinates of one endpoint of the axis.
 *  x2, y2     float  The world coordinates of the other endpoint of the axis.
 *  mjd1,mjd2 double  The range of UTC's to plot, expressed as Modified Julian
 *                    Dates. Both mjd1 and mjd2 should be positive and mjd1
 *                    must be less than mjd2.
 *  majl,majr  float  The lengths of major ticks on the right and/or left
 *                    sides of the axis.
 *  minf       float  The lengths of minor axis ticks as a faction of the
 *                    the major axis ticks.
 *  disp       float  The displacement between the lines of text, measured
 *                    in character heights.
 * Output:
 *  return       int  0 - OK.
 *                    1 - Error.
 */
int utc_axis(int options, float x1, float y1, float x2, float y2,
	     double mjd1, double mjd2, float majl, float majr,
	     float minf, float disp)
{
  int yr1,yr2;           /* The first and last visible year boundaries */
  char extra[80];        /* The text to place below the line of tick labels */
                         /*  This second label contains the most significant */
                         /*  part of the date */
  int len;               /* The number of characters in extra[] */
  long jd1;              /* The Julian Day Number of mjd1 */
  double tod1;           /* The normalized time-of-day in mjd1 */
  int year1,month1,day1; /* The Gregorian Calenday date of jd1 */
  int hour1, min1;       /* The components of the 24-hour clock-time that */
  double sec1;           /*  correspond to tod1 */
  long jd2;              /* The Julian Day Number of mjd2 */
  double tod2;           /* The normalized time-of-day in mjd2 */
  int year2,month2,day2; /* The Gregorian Calenday date of jd2 */
  int hour2, min2;       /* The components of the 24-hour clock-time that */
  double sec2;           /*  correspond to tod1 */
  int yrtick,mntick;     /* The year and month of the first labelled tick */
  int dytick,hrtick;     /* The day and hour of the first labelled tick */
  int mintick,sectick;   /* The month and second of the first tick */
/*
 * The following enumeration values record the type of the major axis
 * labels.
 */
  enum {LBL_YEARS, LBL_MONTHS, LBL_DAYS, LBL_HOURS, LBL_MINS, LBL_SECS} mode;
/*
 * List the abbreviated names for the 12 months of the year.
 */
  char *mnames[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
/*
 * If the axis would have zero length then there is nothing to be plotted.
 */
  if(x1 == x2 && y1 == y2)
    return 0;
/*
 * Make sure that the axis is plottable.
 */
  if(mjd1 >= mjd2) {
    fprintf(stderr, "utc_axis: MJD1 >= MJD2.\n");
    return 1;
  };
/*
 * Make sure that the numbers differ by an order of magnitude more
 * than the advertised precision of a double precision number. This
 * will ensure that we can split the range into up to 10 ticks without
 * hitting precision problems.
 */
  if(fabs(mjd2-mjd1) <=
     fabs(fabs(mjd1)>fabs(mjd2) ? mjd1:mjd2) * 10.0 * DBL_EPSILON) {
    fprintf(stderr, "utc_axis: There is insufficient precision to represent the given range.\n");
    return 1;
  };
/*
 * Break the Modified Julian Dates into Julian Day Number and normalized
 * time-of-day.
 */
  if(split_mjd(mjd1, &jd1, &tod1) ||
     split_mjd(mjd2, &jd2, &tod2))
    return 1;
/*
 * Convert from Julian Day Number to Gregorian Calendar Date.
 */
  if(jd_to_date(jd1, &year1, &month1, &day1) ||
     jd_to_date(jd2, &year2, &month2, &day2))
    return 1;
/*
 * Convert the normalized time-of-day to clock time.
 */
  if(tod_to_time(tod1, &hour1, &min1, &sec1) ||
     tod_to_time(tod2, &hour2, &min2, &sec2))
    return 1;
/*
 * Start by assuming that the first tick will be at the start of the
 * axis.
 */
  yrtick = year1;
  mntick = month1;
  dytick = day1;
  hrtick = hour1;
  mintick = min1;
  sectick = (int) sec1;
/*
 * Don't reveal the axis until it has been completely drawn.
 */
  cpgbbuf();
/*
 * Draw the axis line.
 */
  cpgmove(x1, y1);
  cpgdraw(x2, y2);
/*
 * Find the first and last year boundaries that lie on the axis.
 */
  yr1 = year1 + (month1==1 && day1==1 && tod1==0.0 ? 0 : 1);
  yr2 = year2;
/*
 * If the axis contains more than 2 year boundaries, plot a multi-year axis.
 */
  if(yr2 - yr1 + 1 > 2) {
    if(plot_years(options, x1, y1, x2, y2, majl, majr, minf, disp, mjd1, mjd2,
		  yr1, yr2))
      return 1;
    mode = LBL_YEARS;
  } else {
/*
 * Convert the axis limits into days in the first month.
 */
    double mdmin = day1 + tod1;
    double mdmax = day1 + (jd2 - jd1) + tod2;
/*
 * Get the month in which the axis starts, and the first month whose start
 * appears on the axis.
 */
    int m1lo = month1;
    int m1hi = m1lo + (day1 > 1 || tod1 > 0.0 ? 1:0);
/*
 * Get the month in which the axis ends, and the last month whose start
 * appears on the axis.
 */
    int m2lo = month2 + (year2 - year1) * 12;
    int m2hi = m2lo + (day2 > 1 || tod2 > 0.0 ? 1:0);
/*
 * If the axis contains more than 3 month boundaries, plot a month-of-year axis.
 */
    if(m2lo - m1hi + 1 > 3) {
      plot_months(options, x1, y1, x2, y2, majl, majr, minf, disp,
		  mnames, mdmin, mdmax, year1, m1lo, m1hi, m2lo, m2hi, &yrtick);
      mode = LBL_MONTHS;
/*
 * If the axis contains more than 3 day boundaries, plot a day-of-month axis.
 */
    } else if(floor((mdmax) - ceil(mdmin)) + 1 > 3) {
      plot_days(options, x1, y1, x2, y2, majl, majr, minf, disp,
		year1, month1, mdmin, mdmax, &mntick);
      mode = LBL_DAYS;
    } else {
/*
 * Get the axis limits in hours wrt the day at the start
 * of the axis.
 */
      double hmin = tod1 * 24;
      double hmax = (tod2 + (jd2-jd1)) * 24;
/*
 * If the axis contains more than 2 hour boundaries, plot an hour-of-day
 * axis.
 */
      if(floor((hmax) - ceil(hmin)) + 1 > 2) {
	plot_hours(options, x1, y1, x2, y2, majl, majr, minf, disp,
		   hmin, hmax, &dytick);
	mode = LBL_HOURS;
      } else {
	double mtmp;  /* The unused integral return value of modf() */
/*
 * Get the axis limits in minutes wrt the hour at the start
 * of the axis.
 */
	double mmin = modf(tod1 * 24.0, &mtmp) * 60;
	double mmax = mmin + ((tod2-tod1) + (jd2-jd1)) * 1440;
/*
 * If the axis contains more than 2 minute boundaries, plot a minute-of-hour
 * axis. 
 */
	if(floor((mmax) - ceil(mmin)) + 1 > 2) {
	  plot_minutes(options, x1, y1, x2, y2, majl, majr, minf, disp,
		       mmin, mmax, &hrtick);
	  mode = LBL_MINS;
	} else {
	  double stmp;  /* The unused integral return value of modf() */
/*
 * Get the axis limits in seconds wrt the minute at the start
 * of the axis.
 */
	  double smin = modf(tod1 * 1440, &stmp) * 60;
	  double smax = smin + ((tod2-tod1) + (jd2-jd1)) * 86400;
/*
 * If there are more than 2 second boundaries on the axis, plot a
 * second of minute axis.
 */
	  if(floor((smax) - ceil(smin)) + 1 > 2) {
	    plot_seconds(options, x1, y1, x2, y2, majl, majr, minf, disp,
			 smin, smax, &mintick);
	    mode = LBL_SECS;
	  } else {
/*
 * If there are less than 3 second boundaries on the axis, plot a
 * fraction-of-second axis.
 */
	    plot_subsec(options, x1, y1, x2, y2, majl, majr, minf, disp,
			smin, smax);
	    mode = LBL_SECS;
	  };
	};
      };
    };
  };
/*
 * We need to construct a secondary label that specifies at what
 * point in time the first fine-scale axis label refers to.
 * First ripple through any date increments reported by the
 * above routines to produce a valid date for this label.
 */
  switch(mode) {    /* Note the lack of break statements */
  case LBL_SECS:    /* The first labelled second may be in the next minute */
    if(mintick >= 60) {
      hrtick += mintick / 60;
      mintick = mintick % 60;
    };
  case LBL_MINS:    /* The first labelled minute may be in the next hour */
    if(hrtick >= 24) {
      dytick += hrtick / 24;
      hrtick = hrtick % 24;
    };
  case LBL_HOURS:   /* The first labelled hour may be in the next day */
    while(dytick > length_of_month(yrtick, mntick)) {
      dytick -= length_of_month(yrtick, mntick);
      if(++mntick == 13) {
	mntick = 1;
	yrtick++;
      };
    };
  case LBL_DAYS:    /* The first labelled day may be in the next month */
    if(mntick > 12) {
      int m = mntick - 1;
      yrtick += m / 12;
      mntick = 1 + m % 12;
    };
  case LBL_MONTHS:  /* The first labelled month may be in the next year */
  case LBL_YEARS:
    break;
  };
/*
 * Render the secondary label.
 */
  len = 0;
  extra[0] = '\0';
  if(mode > LBL_YEARS && options & AXIS_WITH_LABELS) {
    len += sprintf(extra, "%d", yrtick);
    if(mode > LBL_MONTHS) {
      len += sprintf(extra + len, " %s", mnames[mntick-1]);
      if(mode > LBL_DAYS) {
	len += sprintf(extra + len, " %d", dytick);
	if(mode > LBL_HOURS) {
	  len += sprintf(extra + len, " %dh", hrtick);
	  if(mode > LBL_MINS) {
	    len += sprintf(extra + len, ":%dm", mintick);
	  };
	};
      };
    };
  };
/*
 * If a second-level label was constructed render it.
 */
  if(extra[0]) {
    float extra_disp = disp > 0 ? (1.0+disp):(disp-1.0);
    plot_tick(options, x1, y1, x2, y2, 0.0, 0.0, 0.0, extra_disp, 0.0, extra);
  };
/*
 * Reveal the axis.
 */
  cpgebuf();
  return 0;
}

/*.......................................................................
 * This function is called to plot decimal years when the range of the
 * axis is such that month divisions would be too small to see.
 * It takes proper account of leap years. It should only be called
 * when there are at least 3 year boundaries visible on the axis.
 *
 * Input:
 *  options        int  A bit-wise OR of AxisOptions enumerators.
 *  x1,y1 etc..         Please see the utc_axis() argument documentation
 *                      for explanations of the initial 8 arguments.
 *  mjd1,mjd2   double  The range of Modified Julian Dates on the axis.
 *  year1,year2    int  The first and last visible year boundaries.
 * Output:
 *  return         int  0 - OK.
 *                      1 - Error.
 */
static int plot_years(int options, float x1, float y1, float x2, float y2,
		      float majl, float majr, float minf, float disp,
		      double mjd1, double mjd2, int year1, int year2)
{
  char buf[20];       /* The buffer to use when composing tick labels */
  TickData tick;      /* Tick-mark details */
  double prev_mjd;    /* The MJD of the start of the previous year */
  int year;           /* The year of a major tick mark */
  int dmajor;         /* The number of years between major tick marks */
  int i;
/*
 * Determine suitable major and minor tick intervals.
 */
  if(tick_interval(options, year1, year2, &tick))
    return 1;
/*
 * Given that plot_months() is called when there are less than 3 years
 * across the axis, the major tick interval will be an integral number
 * of years. Enforce this.
 */
  dmajor = (int)(floor(tick.major_step + 0.5));
/*
 * Adjust year1 and year2 to represent the first and last years
 * at which major-axis ticks are to be placed.
 */
  if(year1 >= 0)
    year1 = dmajor * ((year1+dmajor-1)/dmajor);
  else
    year1 = -dmajor * ((-year1)/dmajor);
  if(year2 >= 0)
    year2 = dmajor * (year2/dmajor);
  else
    year2 = -dmajor * (((-year2)+dmajor-1)/dmajor);
/*
 * Determine the MJD of the tick that precedes the first visible tick.
 */
  prev_mjd = get_mjd_of_year(year1 - dmajor);
/*
 * Plot the major tick marks.
 */
  for(year=year1; year <= year2+dmajor; year += dmajor) {
/*
 * Get the MJD of the start of the year.
 */
    double major_mjd = get_mjd_of_year(year);
/*
 * Plot the major tick mark and its label.
 */
    if(major_mjd <= mjd2) {
      if(options & AXIS_WITH_LABELS) {
	if(tick.maj_exp > 3) {
	  sprintf(buf, "%d\\x10\\u%d\\d", (int)(year/tick.maj_scale),
		  tick.maj_exp);
	} else {
	  sprintf(buf, "%d", year);
	};
      } else {
	buf[0] = '\0';
      };
      plot_tick(options, x1, y1, x2, y2, (major_mjd - mjd1) / (mjd2 - mjd1),
		majl, majr, disp, 0.5, buf);
    };
/*
 * Plot minor tick marks.
 */
    for(i=1; i<tick.nminor; i++) {
      double minor_mjd = prev_mjd + (major_mjd - prev_mjd) *
	(double)i/tick.nminor;
      if(minor_mjd >= mjd1) {
	plot_tick(options, x1, y1, x2, y2, (minor_mjd - mjd1) / (mjd2 - mjd1),
		majl * minf, majr * minf, 0.0, 0.5, "");
      };
    };
/*
 * The current major tick becomes the previous major tick
 * in the next iteration.
 */
    prev_mjd = major_mjd;
  };
  return 0;
}

/*.......................................................................
 * The following function is called to plot major ticks at month boundaries.
 * It should only be called when there are more than 3 month boundaries
 * and 2 or less year boundaries on the axis.
 *
 * Input:
 *  options       int  A bit-wise OR of AxisOptions enumerators.
 *  x1,y1 etc..        Please see the utc_axis() argument documentation
 *                     for explanations of the initial 8 arguments.
 *  mnames    char **  Abbreviated names for each of the 12 Gregorian
 *                     calendar months.
 *  mdmin   double     The value at the start of the axis as a floating
 *                     point day number within the month of the start
 *                     of the axis (1 <= mdmin < 32).
 *  mdmax   double     The value at the end of the axis as a floating
 *                     point day number within the month of the start
 *                     of the axis (mdmax > mdmin).
 *  year       int     The integer year at the start of the axis.
 *
 *                     In the following, note that m1lo and m2hi are the
 *                     month boundaries that bracket the axis, whereas
 *                     m1hi and m2lo denote the first and last month
 *                     boundaries that lie within the bounds of the axis.
 *
 *  m1lo       int     The month (1-12) at the start of the axis.
 *  m1hi       int     The first month whose boundary is visible on the
 *                     axis. Note that m1hi is either the same or 1 more
 *                     than m1lo (ie. either 12 or 13 when m1lo=12).
 *  m2lo       int     The last month that is visible on the axis,
 *                     expressed as a month in the 'year' (m2lo >= m1hi).
 *  m2hi       int     This equals m2lo if the month boundary for m2lo
 *                     occurs precisely at the end of the axis. Otherwise
 *                     it is m2lo+1.
 * Input/Output:
 *  yrtick     int *   The year associated with the first labelled tick.
 * Output:
 *  return     int  0 - OK.
 *                  1 - Error.
 */
static int plot_months(int options, float x1, float y1, float x2, float y2,
		       float majl, float majr, float minf, float disp,
		       char *mnames[12], double mdmin, double mdmax,
		       int year, int m1lo, int m1hi, int m2lo, int m2hi,
		       int *yrtick)
{
  int dmajor;     /* The number of months to leave between major tick marks */
  int month;      /* A month in the range m1lo..m2hi */
  int month_size; /* The number of days in month 'month' */
  int day;        /* A day number wrt the start of the first month. */
  int first=1;    /* True until the first tick has been labelled */
/*
 * Determine the number of month boundaries that appear on the axis.
 */
  int nmonths = m2lo - m1hi + 1;
/*
 * Determine how many months to label per year.
 */
  if(nmonths <= 7) {         /* Label every month */
    dmajor = 1;
  } else if(nmonths <= 12) { /* Label Jan Mar May Jul Sep Nov */
    dmajor = 2;
  } else if(nmonths <= 24) { /* Label Jan May Sep */
    dmajor = 4;
  } else {                   /* Label Jan Jul */
    dmajor = 6;
  };
/*
 * Draw the major tick marks.
 */
  for(month=m1lo, day=1; month<=m2hi; month++, day+=month_size) {
    int month_of_year = ((month - 1) % 12) + 1;
    int year_of_month = year + (month - 1)/12;
    month_size = length_of_month(year_of_month, month_of_year);
/*
 * Is the month boundary visible?
 */
    if(day >= mdmin && day <= mdmax) {
/*
 * Plot a major tick?
 */
      if((month_of_year-1) % dmajor == 0) {
	plot_tick(options, x1, y1, x2, y2, (day-mdmin) / (mdmax-mdmin),
		  majl, majr, disp, 0.5,
		  options ? mnames[month_of_year-1] : (char* )"");
	if(first) {
	  *yrtick = year_of_month;
	  first = 0;
	};
/*
 * Plot a minor tick.
 */
      } else {
	plot_tick(options, x1, y1, x2, y2, (day-mdmin) / (mdmax-mdmin),
		majl * minf, majr * minf, 0.0, 0.5, "");
      };
    };
/*
 * If the major ticks sample every month, then plot minor tick marks
 * at every 5 days (wrt day 1).
 */
    if(dmajor == 1) {
      int dy;
      for(dy=5; dy<=25; dy+=5) {
	double d = day+dy;
	if(d >= mdmin && d <= mdmax) {
	  plot_tick(options, x1, y1, x2, y2, (d-mdmin)/(mdmax-mdmin),
		    majl * minf, majr * minf, 0.0, 0.5, "");
	};
      };
    };
  };
  return 0;
}

/*.......................................................................
 * The following function is called to plot major ticks at day boundaries.
 * It should only be called when there are more than 3 day boundaries
 * and 3 or less month boundaries on the axis.
 *
 * Input:
 *  options       int  A bit-wise OR of AxisOptions enumerators.
 *  x1,y1 etc..        Please see the utc_axis() argument documentation
 *                     for explanations of the initial 8 arguments.
 *  year       int     The integer year at the start of the axis.
 *  month      int     The month (1-12) within 'year' at the start of
 *                     the axis.
 *  mdmin   double     The value at the start of the axis as a floating
 *                     point day number within the month of the start
 *                     of the axis (1 <= mdmin < 32).
 *  mdmax   double     The value at the end of the axis as a floating
 *                     point day number within the month of the start
 *                     of the axis (mdmax > mdmin).
 *  mntick     int *   The month associated with the first labelled tick
 *                     counted from the start of the year at the start
 *                     of the axis (ie. it can excede 12).
 * Output:
 *  return        int  0 - OK.
 *                     1 - Error.
 */
static int plot_days(int options, float x1, float y1, float x2, float y2,
		     float majl, float majr, float minf, float disp,
		     int year, int month, double mdmin, double mdmax,
		     int *mntick)
{
  char buf[8];       /* The buffer to use when composing tick labels */
  int day_of_month;  /* The day in month 'month' that is being processed */
  int day;           /* The continuous day number of the day being processed */
  int month_size;    /* The number of days in the current month */
  int dmajor;        /* The number of days between major tick marks */
  int dminor;        /* The number of days between minor tick marks */
  int first = 1;     /* True until the first tick has been drawn */
/*
 * Determine the day boundaries that bracket the axis, expressed as days
 * of the first month. 
 */
  int day1 = (int)floor(mdmin);
  int day2 = (int)ceil(mdmax);
/*
 * Determine the number of day boundaries that appear on the axis.
 */
  int ndays = (int)((floor(mdmax) - ceil(mdmin)) + 1);
/*
 * Determine how many days to label per month.
 */
  if(ndays <= 6) {         /* Label every day */
    dmajor = 1;
    dminor = 1;
  } else if(ndays <= 14) { /* Label every 2nd day */
    dmajor = 2;
    dminor = 1;
  } else if(ndays <= 35) { /* Label every 5th day */
    dmajor = 5;
    dminor = 1;
  } else {                 /* Label every 10th day */
    dmajor = 10;
    dminor = 2;
  };
/*
 * While drawing ticks, we have to be take account of what the date
 * is.
 */
  day_of_month = day1;
  month_size = length_of_month(year, month);
  *mntick = month;
/*
 * Draw the major and minor tick marks.
 */
  for(day=day1; day <= day2; day++,day_of_month++) {
/*
 * Are we starting a new month?
 */
    if(day_of_month > month_size) {
      if(first)	(*mntick)++;
      if(++month > 12) {
	month = 1;
	year++;
      };
      day_of_month = 1;
      month_size = length_of_month(year, month);
    };
/*
 * Is the day boundary visible?
 */
    if(day >= mdmin && day <= mdmax) {
/*
 * Plot a major tick mark?
 */
      if((day_of_month-1) % dmajor == 0 &&
	 ((month_size - day_of_month) > dmajor/2 || ndays < 10)) {
	if(options & AXIS_WITH_LABELS) {
	  sprintf(buf, "%d%s", day_of_month, ordinal_suffix(day_of_month));
	} else {
	  buf[0] = '\0';
	};
	plot_tick(options, x1, y1, x2, y2, (day-mdmin) / (mdmax-mdmin),
		  majl, majr, disp, 0.5, buf); 
	first = 0;
      } else if((day_of_month-1) % dminor == 0) {
	plot_tick(options, x1, y1, x2, y2, (day-mdmin) / (mdmax-mdmin),
		  majl * minf, majr * minf, 0.0, 0.5, "");
      };
    };
/*
 * If the major tick interval was a single day, plot minor tick marks
 * every 6 hours.
 */
    if(dmajor == 1) {
      int hour;
      for(hour=6; hour<24; hour+=6) {
	double d = day + hour / 24.0;
	if(d >= mdmin && d <= mdmax) {
	  plot_tick(options, x1, y1, x2, y2, (d-mdmin) / (mdmax-mdmin),
		    majl * minf, majr * minf, 0.0, 0.5, "");
	};
      };
    };
  };
  return 0;
}

/*.......................................................................
 * The following function is called to plot major ticks at hour boundaries.
 * It should only be called when there are 3 or more hour boundaries
 * and 3 or less day boundaries on the axis.
 *
 * Input:
 *  options    int  A bit-wise OR of AxisOptions enumerators.
 *  x1,y1 etc..     Please see the utc_axis() argument documentation
 *                  for explanations of the initial 8 arguments.
 *  hmin   double   The value at the start of the axis as floating
 *                  point hours into the day of the start of the axis
 *                  (0 <= hmin < 24).
 *  hmax   double   The value at the end of the axis as floating
 *                  point hours into the day of the start of the axis
 *                  (hmax > hmin).
 * Input/Output:
 *  dytick    int * The value of *dytick provided by the caller will
 *                  be incremented by one for each day boundary that
 *                  follows the start of the axis up to the first
 *                  labelled tick. It thus provides a measure of the
 *                  day number associated with the first labelled tick.
 * Output:
 *  return    int   0 - OK.
 *                  1 - Error.
 */
static int plot_hours(int options, float x1, float y1, float x2, float y2,
		      float majl, float majr, float minf, float disp,
		      double hmin, double hmax, int *dytick)
{
  char buf[8];    /* The buffer to use when composing tick labels */
  int dmajor;     /* The number of hours between major tick marks */
  int dminor;     /* The number of hours between minor tick marks */
  int hour;       /* An hour in the range hour1..hour2 */
  int first = 1;  /* True until the first labelled tick has been drawn */
/*
 * Determine the hour boundaries that bracket the axis limits.
 */
  int hour1 = (int)floor(hmin);
  int hour2 = (int)ceil(hmax);
/*
 * Determine the number of hour boundaries that appear on the axis.
 */
  int nhours = (int)((floor(hmax) - ceil(hmin)) + 1);
/*
 * Determine how many hours to leave between tick marks.
 */
  if(nhours <= 6) {          /* Label every hour */
    dmajor = 1;
    dminor = 1;
  } else if(nhours <= 12) {  /* Label every second hour */
    dmajor = 2;
    dminor = 1;
  } else if(nhours <= 18) {  /* Label every 4th hour */
    dmajor = 4;
    dminor = 1;
  } else if(nhours <= 36) {  /* Label every 6th hour */
    dmajor = 6;
    dminor = 2;
  } else {                   /* Label every 12th hour */
    dmajor = 12;
    dminor = 4;
  };
/*
 * Plot the major and minor tick marks.
 */
  for(hour=hour1; hour <= hour2; hour++) {
/*
 * Is the hour boundary visible?
 */
    if(hour >= hmin && hour <= hmax) {
/*
 * Plot a major tick mark?
 */
      if(hour % dmajor == 0) {
	if(options & AXIS_WITH_LABELS) {
	  sprintf(buf, "%dh", hour % 24);
	} else {
	  buf[0] = '\0';
	};
	plot_tick(options, x1, y1, x2, y2, (hour-hmin) / (hmax-hmin),
		  majl, majr, disp, 0.5, buf);
	if(first) {
	  *dytick += hour / 24;
	  first = 0;
	};
      } else if(hour % dminor == 0) {
	plot_tick(options, x1, y1, x2, y2, (hour-hmin) / (hmax-hmin),
		majl * minf, majr * minf, 0.0, 0.5, "");
      };
    };
/*
 * If the major tick interval was a single hour, plot minor tick marks
 * every 10 minutes.
 */
    if(dmajor == 1) {
      int minute;
      for(minute=10; minute<60; minute+=10) {
	double h = hour + minute / 60.0;
	if(h >= hmin && h <= hmax) {
	  plot_tick(options, x1, y1, x2, y2, (h-hmin) / (hmax-hmin),
		  majl * minf, majr * minf, 0.0, 0.5, "");
	};
      };
    };
  };
  return 0;
}

/*.......................................................................
 * The following function is called to plot major ticks at minute
 * boundaries. It should only be called when there are 3 or more minute
 * boundaries and 2 or less hour boundaries on the axis.
 *
 * Input:
 *  options   int  A bit-wise OR of AxisOptions enumerators.
 *  x1,y1 etc..    Please see the utc_axis() argument documentation
 *                 for explanations of the initial 8 arguments.
 *  mmin   double  The value at the start of the axis as floating
 *                 point minutes into the hour of the start of the axis
 *                 (0 <= mmin < 60).
 *  mmax   double  The value at the end of the axis as floating
 *                 point minutes into the hour of the start of the axis
 *                 (mmax > mmin).
 * Input/Output:
 *  hrtick    int * The value of *hrtick provided by the caller will
 *                  be incremented by one for each hour boundary that
 *                  follows the start of the axis up to the first
 *                  labelled tick. It thus provides a measure of the
 *                  hour associated with the first labelled tick.
 * Output:
 *  return    int  0 - OK.
 *                 1 - Error.
 */
static int plot_minutes(int options, float x1, float y1, float x2, float y2,
			float majl, float majr, float minf, float disp,
			double mmin, double mmax, int *hrtick)
{
  char buf[8];     /* The buffer to use when composing tick labels */
  int dmajor;      /* The number of minutes between major tick marks */
  int dminor;      /* The number of minutes between minor tick marks */
  int minute;      /* A minute in the range minute1..minute2 */
  int first = 1;   /* True until the first labelled tick has been drawn */
/*
 * Determine the minute boundaries that bracket the axis limits.
 */
  int minute1 = (int)floor(mmin);
  int minute2 = (int)ceil(mmax);
/*
 * Determine the number of minute boundaries that appear on the axis.
 */
  int nminutes = (int)((floor(mmax) - ceil(mmin)) + 1);
/*
 * Determine how many minutes to leave between tick marks.
 */
  if(nminutes <= 6) {          /* Label every minute */
    dmajor = 1;
    dminor = 1;
  } else if(nminutes <= 12) {  /* Label every second minute */
    dmajor = 2;
    dminor = 1;
  } else if(nminutes <= 30) {  /* Label every 5th minute */
    dmajor = 5;
    dminor = 1;
  } else if(nminutes <= 60) {  /* Label every 10th minute */
    dmajor = 10;
    dminor = 2;
  } else {                     /* Label every 30th minute */
    dmajor = 30;
    dminor = 5;
  };
/*
 * Plot the major and minor tick marks.
 */
  for(minute=minute1; minute <= minute2; minute++) {
/*
 * Is the hour boundary visible?
 */
    if(minute >= mmin && minute <= mmax) {
/*
 * Plot a major tick mark?
 */
      if(minute % dmajor == 0) {
	if(options & AXIS_WITH_LABELS) {
	  sprintf(buf, "%dm", minute % 60);
	} else {
	  buf[0] = '\0';
	};
	plot_tick(options, x1, y1, x2, y2, (minute-mmin) / (mmax-mmin),
		majl, majr, disp, 0.5, buf);
	if(first) {
	  *hrtick += minute / 60;
	  first = 0;
	};
      } else if(minute % dminor == 0) {
	plot_tick(options, x1, y1, x2, y2, (minute-mmin) / (mmax-mmin),
		majl * minf, majr * minf, 0.0, 0.5, "");
      };
    };
/*
 * If the major tick interval was a single minute, plot minor tick marks
 * every 10 seconds.
 */
    if(dmajor == 1) {
      int second;
      for(second=10; second<60; second+=10) {
	double m = minute + second / 60.0;
	if(m >= mmin && m <= mmax) {
	  plot_tick(options, x1, y1, x2, y2, (m-mmin) / (mmax-mmin),
		  majl * minf, majr * minf, 0.0, 0.5, "");
	};
      };
    };
  };
  return 0;
}

/*.......................................................................
 * The following function is called to plot major ticks at seconds
 * boundaries. It should only be called when there are 3 or more second
 * boundaries and 2 or less minute boundaries on the axis.
 *
 * Input:
 *  options   int  A bit-wise OR of AxisOptions enumerators.
 *  x1,y1 etc..    Please see the utc_axis() argument documentation
 *                 for explanations of the initial 8 arguments.
 *  smin   double  The value at the start of the axis as floating
 *                 point seconds into the minute of the start of the axis
 *                 (0 <= smin < 60).
 *  smax   double  The value at the end of the axis as floating
 *                 point seconds into the minute of the start of the axis
 *                 (smax > smin).
 * Input/Output:
 *  mintick   int * The value of *mintick provided by the caller will
 *                  be incremented by one for each minute boundary that
 *                  follows the start of the axis up to the first
 *                  labelled tick. It thus provides a measure of the
 *                  minute associated with the first labelled tick.
 * Output:
 *  return    int  0 - OK.
 *                 1 - Error.
 */
static int plot_seconds(int options, float x1, float y1, float x2, float y2,
			float majl, float majr, float minf, float disp,
			double smin, double smax, int *mintick)
{
  char buf[8];       /* The buffer to use when composing tick labels */
  int dmajor;        /* The number of seconds between major tick marks */
  int dminor;        /* The number of seconds between minor tick marks */
  int second;        /* A second in the range second1..second2 */
  int first = 1;     /* True until the first labelled tick has been drawn */
/*
 * Determine the second boundaries that bracket the axis limits.
 */
  int second1 = (int)floor(smin);
  int second2 = (int)ceil(smax);
/*
 * Determine the number of second boundaries that appear on the axis.
 */
  int nseconds = (int)((floor(smax) - ceil(smin)) + 1);
/*
 * Determine how many seconds to leave between tick marks.
 */
  if(nseconds <= 6) {          /* Label every second */
    dmajor = 1;
    dminor = 1;
  } else if(nseconds <= 12) {  /* Label every second second */
    dmajor = 2;
    dminor = 1;
  } else if(nseconds <= 30) {  /* Label every 5th second */
    dmajor = 5;
    dminor = 1;
  } else if(nseconds <= 60) {  /* Label every 10th second */
    dmajor = 10;
    dminor = 2;
  } else {                     /* Label every 30th second */
    dmajor = 30;
    dminor = 5;
  };
/*
 * Plot the major and minor tick marks.
 */
  for(second=second1; second <= second2; second++) {
/*
 * Is the second boundary visible?
 */
    if(second >= smin && second <= smax) {
/*
 * Plot a major tick mark?
 */
      if(second % dmajor == 0) {
	if(options & AXIS_WITH_LABELS) {
	  sprintf(buf, "%ds", second % 60);
	} else {
	  buf[0] = '\0';
	};
	plot_tick(options, x1, y1, x2, y2, (second-smin) / (smax-smin),
		majl, majr, disp, 0.5, buf);
	if(first) {
	  *mintick += second / 60;
	  first = 0;
	};
      } else if(second % dminor == 0) {
	plot_tick(options, x1, y1, x2, y2, (second-smin) / (smax-smin),
		majl * minf, majr * minf, 0.0, 0.5, "");
      };
    };
/*
 * If the major tick interval was a single second, plot minor tick marks
 * every 100 milliseconds.
 */
    if(dmajor == 1) {
      int ms;
      for(ms=100; ms<1000; ms+=100) {
	double s = second + ms / 1000.0;
	if(s >= smin && s <= smax) {
	  plot_tick(options, x1, y1, x2, y2, (s-smin) / (smax-smin),
		  majl * minf, majr * minf, 0.0, 0.5, "");
	};
      };
    };
  };
  return 0;
}

/*.......................................................................
 * The following function is called to plot major ticks at seconds
 * boundaries. It should only be called when there are less than 3
 * second boundaries visible on the axis.
 *
 * Input:
 *  options   int  A bit-wise OR of AxisOptions enumerators.
 *  x1,y1 etc..    Please see the utc_axis() argument documentation
 *                 for explanations of the initial 8 arguments.
 *  smin   double  The value at the start of the axis as floating
 *                 point seconds into the minute of the start of the axis
 *                 (0 <= smin < 60).
 *  smax   double  The value at the end of the axis as floating
 *                 point seconds into the minute of the start of the axis
 *                 (smax > smin).
 * Output:
 *  return    int  0 - OK.
 *                 1 - Error.
 */
static int plot_subsec(int options, float x1, float y1, float x2, float y2,
		       float majl, float majr, float minf, float disp,
		       double smin, double smax)
{
  char buf[DBL_DIG+20]; /* The buffer to use when composing tick labels */
  TickData tick;        /* Tick-mark details */
  int i;
/*
 * Determine suitable major and minor tick intervals.
 */
  if(tick_interval(options, smin, smax, &tick))
    return 1;
/*
 * Plot the major and minor tick marks and label the major tick
 * marks with seconds modulo 60.
 */
  for(i=tick.ja; i<=tick.jb; i++) {
    double second = tick.offset + i * tick.major_step;
/*
 * Compose a label for the major tick mark, using scientific notation
 * where necessary.
 */
    if(options & AXIS_WITH_LABELS) {
      render_label(second, tick.inc_scale, tick.inc_prec, tick.inc_exp, buf);
/*
 * Append the 's' second indicator suffix.
 */
      strcat(buf, "s");
    } else {
      buf[0] = '\0';
    };
/*
 * Plot the tick mark.
 */
    plot_tick(options, x1, y1, x2, y2, (second-smin) / (smax-smin),
	      majl, majr, disp, 0.5, buf);
  };
/*
 * Plot the minor tick marks.
 */
  for(i=tick.ia; i <= tick.ib; i++) {
    double second = tick.offset + i * tick.minor_step;
    plot_tick(options, x1, y1, x2, y2, (second-smin) / (smax-smin),
	      majl * minf, majr * minf, 0.0, 0.5, "");
  };
  return 0;
}

/*.......................................................................
 * Plot an axis tick and/or its label. The label is always drawn parallel
 * to the axis. Note that the tick marks are drawn with pgmove and pgdraw
 * so they get clipped at the edge of the viewport.
 *
 * Input:
 *  options      int    A bit-wise OR of AxisOptions enumerators.
 *  x1, y1     float    The world coordinates of one endpoint of the
 *                      vector.
 *  x2, y2     float    The world coordinates of the other endpoint of
 *                      the vector.
 *  posn       float    The position to locate the hot-spot of the text
 *                      as a normalized fraction of the distance from
 *                      x1,y1 to x2,y2.
 *  tikl       float    The length of the tick mark on the left of
 *                      the line x1,y1 -> x2,y2, in units of the
 *                      character height.
 *  tikr       float    The length of the tick mark on the right of
 *                      the line x1,y1 -> x2,y2, in units of the
 *                      character height.
 *  just       float    The position of the hot-spot in the text string
 *                      as a normalized fraction of the length of the
 *                      string.
 *  disp       float    The displacement of the text perpendicular to the
 *                      vector in units of the character height. Positive
 *                      displacements cause the text to be displaced to
 *                      the right of the vector when looking along the
 *                      axis from x1,y1 to x2,y2. Negative displacements
 *                      place the text on the other side of the vector.
 *  text        char *  The text to be rendered.
 */
static void plot_tick(int options, float x1, float y1, float x2, float y2,
		      float posn, float tikl, float tikr,
		      float disp, float just, char *text)
{
  float wl,wr;  /* The X-axis world coordinates at the left and right */
                /*  edges of the window. */
  float wb,wt;  /* The Y-axis world coordinates at the bottom and top */
                /*  edges of the window. */
  float vl,vr;  /* The X-axis physical coordinates at the left and right */
                /*  edges of the window (mm). */
  float vb,vt;  /* The Y-axis physical coordinates at the bottom and top */
                /*  edges of the window (mm). */
  float xlen;   /* The X-axis world-coordinate length of the vector */
  float ylen;   /* The Y-axis world-coordinate length of the vector */
  float xpermm; /* The conversion factor from world X coordinates to mm */
  float ypermm; /* The conversion factor from world Y coordinates to mm */
  float x,y;    /* The world-coordinate position of the string */
  float xch,ych;/* The world-coordinate heights of characters drawn */
                /*  vertically and horizontally. */
  float xoff;   /* The world X length of a one-character-height line */
                /*  drawn physically perpendicular to the vector. */
  float yoff;   /* The world Y length of a one-character-height line */
                /*  drawn physically perpendicular to the vector. */
  double angle; /* The angle of the vector ccw from the X axis (radians) */

/*
 * When the displacement is positive we need to add in the distance
 * from the top of the text to the baseline.
 */
  if(disp > 0.0)
    disp++;
/*
 * Get the world-coordinate and physical geometry of the current window.
 */
  cpgqwin(&wl, &wr, &wb, &wt);
  cpgqvp(2, &vl, &vr, &vb, &vt);
/*
 * Compute the multiplicative conversion factor to scale mm units to
 * world coordinates.
 */
  xpermm = (wr-wl) / (vr-vl);
  ypermm = (wt-wb) / (vt-vb);
/*
 * Get the X and Y world-coordinate extents of the parallel vector.
 */
  xlen = x2 - x1;
  ylen = y2 - y1;
/*
 * Compute the physical angle of the line x1,y1..x2,y2, counterclockwise
 * from the X-axis.
 */
  angle = (xlen != 0.0 || ylen != 0.0) ? atan2(ylen/ypermm, xlen/xpermm) : 0.0;
/*
 * Get the current character heights in mm. Note that xch and
 * ych are documented as being the same when asking for physical
 * units like mm.
 */
  cpgqcs(2, &xch, &ych);
/*
 * Work out the world-coordinate x,y components of a one character height
 * line drawn physically perpendicular (counterclockwise) to the line.
 */
  xoff = xpermm * xch * sin(angle);
  yoff = -ypermm * xch * cos(angle);
/*
 * Compute the point on the axis through which the tick mark should pass.
 */
  x = x1 + xlen * posn;
  y = y1 + ylen * posn;
/*
 * Render the string.
 */
  if(options & AXIS_WITH_LABELS && text && *text) {
    if(options & AXIS_OUTWARD_TEXT) {
      angle -= PI_OVER_2;
      just = 1.0;
    };

#if 0
    if(strstr(text, "inf") != 0) {
      std::cout << "Got an inf tick" << std::endl;
    }
#endif

    cpgptxt(x + disp * xoff, y + disp * yoff, angle * R2D, just, text);
  };
/*
 * Draw the tick.
 */
  if(tikl != 0.0 || tikr != 0.0) {
    cpgmove(x - tikl * xoff, y - tikl * yoff);
    cpgdraw(x + tikr * xoff, y + tikr * yoff);
  };
}

/*.......................................................................
 * Determine a interval to use for a given range of axis values.
 *
 * Input:
 *  options      int    A bit-wise OR of AxisOptions enumerators.
 *  va,vb     double    The two end points of the range to divide into
 *                      ticks.
 * Input/Output:
 *  tick    TickData *  The chosen tick intervals will be placed in
 *                      *tick.
 * Output:
 *  return       int    0 - OK.
 *                      1 - Error.
 */
static int tick_interval(int options, double va, double vb, TickData *tick)
{
  double biggest;       /* The value with the largest magnitude on the axis */
  double ra,rb;         /* va - tick->offset, vb - tick->offset */
  double dv;            /* A trial tick interval */
  double step_exp;      /* The power-of-ten exponent of a step interval */
  double step_scale;    /* pow(10, step_exp) */
  int i;
/*
 * List suitable tick intervals for values in the range 10 > v >= 1.
 * The steps must be listed in decreasing order of the interval between
 * major tick-marks.
 */
  struct {
    int major_step;  /* A trial step interval */
    double coarse;   /* The minor tick interval to use for cramped axes */
    double medium;   /* The optimal minor tick interval */
    double fine;     /* The minor tick interval to use for sparse axes */
  } steps[] = {
    {5, 1.0, 1.0, 1.0},
    {2, 1.0, 0.5, 0.5},
    {1, 0.5, 0.2, 0.1}
  };
  const int nstep = sizeof(steps)/sizeof(steps[0]);
/*
 * Set the target minimum number of major-axis ticks.
 */
  int min_ticks = (options & AXIS_IS_CRAMPED) ? 2 : 3;
/*
 * Check the arguments.
 */
  if(va == vb) {
    fprintf(stderr, "tick_interval: Can't divide zero range into ticks.\n");
    return 1;
  };
  if(!tick) {
    fprintf(stderr, "tick_interval: NULL return container.\n");
    return 1;
  };
/*
 * Arrange va and vb into ascending order.
 */
  if(va > vb) {
    double vtmp = va;
    va = vb;
    vb = vtmp;
  };
/*
 * Determine step_exp and step_scale such that the range can be
 * represented as [(vb-va) = (vb-va)/step_scale * 10^step_exp].
 */
  step_exp = floor(log10(vb - va));
  step_scale = pow(10, step_exp);
/*
 * Deduce a constant offset for the axis values such that the values
 * on the axis and their range have roughly similar sizes. In order to
 * cater for those who decide to label ticks incrementally with
 * respect to this offset, choose the offset such that it is a number
 * that is easy for a human to add the tick values to by eye. This
 * means that we need a number that is divisable exactly by 10^(step_exp-1)
 * and that is smaller than the smallest magnitude value on the axis.
 * Also take this opportunity to determine the value on the axis that
 * has the largest magnitude.
 */
  dv = 10 * step_scale;
  if(va > 0.0) {           /* va and vb are both positive */
    tick->offset = dv * floor(va / dv);
    biggest = vb;
  } else if(vb < 0.0) {    /* va and vb are both negative */
    tick->offset = dv * ceil(vb / dv);
    biggest = va;
  } else {                 /* va <= 0 and vb >= 0 */
    tick->offset = 0.0;
    biggest = -va > vb ? va : vb;
  };
/*
 * Subtract off the chosen offset from the two limits of the axis
 * range.
 */
  ra = va - tick->offset;
  rb = vb - tick->offset;
/*
 * Work out the interval between divisions.
 */
  while(1) {
    for(i=0; i<nstep; i++) {
/*
 * Get the next tick interval to be tried.
 */
      dv = step_scale * steps[i].major_step;
/*
 * Compute the sequential index wrt 0 of the first and last ticks
 * in the range and count the number of ticks that they represent.
 */
      tick->nmajor = (int)(floor(rb / dv) - ceil(ra / dv) + 1.0);
/*
 * Stop searching at the largest tick interval that results in at
 * least 3 visible major tick marks.
 */
      if(tick->nmajor >= min_ticks)
	break;
    };
    if(tick->nmajor >= min_ticks)
      break;
/*
 * Try the next power of ten.
 */
    step_exp -= 1;
    step_scale /= 10;
  };
/*
 * To have got here an interval with at least 3 ticks must have been
 * found.
 *
 * Compute and record the values of the integer tick indexes ja and jb
 * such that the visible ticks can be calculated using the following
 * equation:
 *
 *   major_tick = offset + j * major_step    [j=ja..jb]
 */
  tick->major_step = dv;
  tick->ja = (int)(ceil(ra/dv));
  tick->jb = (int)(floor(rb/dv));
/*
 * Choose the minor-axis step size.
 */
  if(options & AXIS_IS_CRAMPED)
    tick->minor_step = step_scale * steps[i].coarse;
  else if(tick->nmajor <= 3)
    tick->minor_step = step_scale * steps[i].fine;
  else if(tick->nmajor <= 5)
    tick->minor_step = step_scale * steps[i].medium;
  else
    tick->minor_step = step_scale * steps[i].coarse;
  tick->nminor = (int)(floor(tick->major_step / tick->minor_step + 0.5));
/*
 * Compute the minor tick indexes ia and ib such that visible ticks
 * can be placed at:
 *
 *  minor_tick = offset + i * minor_step    [i=ia..ib (i%nminor != 0)]
 */
  tick->ia = (int)ceil(ra/tick->minor_step);
  tick->ib = (int)floor(rb/tick->minor_step);
/*
 * When displaying a composite tick label or the offset using scientific
 * notation we need to know both the integral exponent to display and what
 * to divide the mantissa by to convert it into a number between >= 1.0
 * and < 10. In addition, regardless of whether scientific notation is
 * to be used, we also need to know the number of decimal significant
 * figures needed to display the effect of adding one step interval
 * between major ticks.
 */
  tick->maj_exp   = (int)floor(log10(fabs(biggest)));
  tick->maj_scale = (int)pow(10, tick->maj_exp);
  tick->maj_prec  = (int)(tick->maj_exp - step_exp + 1);
/*
 * Work out the equivalent information to the above for displaying ticks
 * from which tick->offset has been subtracted.
 */
  tick->inc_exp = (int)(floor(log10(dv * abs((abs(tick->jb) > abs(tick->ja) ?
				    tick->jb:tick->ja)))));
  tick->inc_scale = pow(10, tick->inc_exp);
  tick->inc_prec  = (int)(tick->inc_exp - step_exp + 1);

  return 0;
}

/*.......................................................................
 * Compose a major-axis tick label using pgplot escapes.
 * If the power-of-ten exponent is < -4 or > 3 then scientific notation
 * is used.
 *
 * Input:
 *  value  double    The value to be rendered.
 *  scale  double    The value to divide a composite tick value by
 *                   to form a scientific-notation mantissa.
 *  prec      int    The number of significant figures to display.
 *  exp       int    The power-of-ten exponent to use when rendering
 *                   using scientific notation.
 * Input/Output:
 *  label    char *  A character array of at least DBL_DIG + 20 elements.
 *                   The label will be rendered in this array and
 *                   terminated with a '\0' character.
 */
static void render_label(double value, double scale, int prec, int exp,
			 char *label)
{
  if(exp < -4 || exp > 4) {   /* Use scientific notation */
    sprintf(label, "%.*f", prec-1, value/scale);
    sprintf(trim_decimal_places(label), "\\x10\\u%d\\d", exp);
  } else if(exp+1 >= prec) {     /* Plot as an integer */
    sprintf(label, "%.0f", value);
  } else {                       /* Plot as a floating point number */
    sprintf(label, "%.*f", prec-exp-1, value);
    (void) trim_decimal_places(label);
  };

#if 0
  if(strstr(label, "inf")!=0) {
    std::cout << "Hello" << std::endl;
  }
#endif
}

/*.......................................................................
 * This is a private function of render_label, that is used to trim
 * trailing zeroes from the fractional part of a floating point written
 * with %.*f. If no non-zero fractional digits remain, then the decimal
 * point is also removed.
 *
 * Input:
 *  string     char *   The string that contains the rendered number.
 * Output:
 *  return     char *   The pointer to the newly written '\0' terminator
 *                      of the string.
 */
static char *trim_decimal_places(char *string)
{
  char *cptr;  /* A pointer into string[] */
  char *last;  /* A pointer to the last non-zero fractional digit in string[] */
/*
 * See if the mantissa contains a decimal point.
 */
  for(cptr=string; *cptr && *cptr!='.'; cptr++)
    ;
/*
 * No decimal point?
 */
  if(*cptr == '\0')
    return cptr;
/*
 * Find the last significant non-zero digit.
 */
  last = cptr - 1;
  for(++cptr; *cptr; cptr++) {
    if(*cptr != '0')
      last = cptr;
  };
/*
 * Terminate the string after the last significant non-zero digit.
 */
  if(*last)
    *(++last) = '\0';
  return last;
}

/*.......................................................................
 * Return the Modified Julian Date of the start of first day of a given
 * year. The algorithm is derived from equation 12.92-1 in the
 * "Explanatory Supplement to the Astronomical Almanac".
 *
 * Input:
 *  year     int   The Gregorian calendar year to return an MJD for.
 *                 The result is valid for year >= -4712.
 */
static long get_mjd_of_year(int year)
{
  return (1461L * (year + 4799L))/4 - (3*((year+4899L)/100))/4 - 2431739L;
}

/*.......................................................................
 * Split a Modified Julian Date into Julian Day Number and time-of-day
 * components.
 *
 * Input:
 *  mjd     double    The Modified Julian Date (Julian Date - 2400000.5).
 * Input/Ouput:
 *  jd        long *  The Julian Day Number will be assigned to *jd.
 *  tod     double *  The time-of-day, expressed as a normalized fraction
 *                    of a day, will be assigned to *tod.
 * Output:
 *  return     int    0 - OK.
 *                    1 - Error.
 */
static int split_mjd(double mjd, long *jd, double *tod)
{
  double num,frc;   /* The integral and fractional parts of the mjd */
/*
 * Check the inputs.
 */
  if(!jd || !tod) {
    fprintf(stderr, "split_mjd: NULL argument(s).\n");
    return 1;
  };
  if(mjd < -MJD_OFFSET || mjd > (LONG_MAX - MJD_OFFSET)) {
    fprintf(stderr, "Illegal Modified Julian Date.\n");
    return 1;
  };
/*
 * Separate the integral and fractional parts of the day.
 */
  frc = modf(mjd, &num);
  if(frc < 0.0) {
    frc += 1.0;
    num -= 1.0;
  };
/*
 * Convert the Modified Julian Day number to a Julian Day Number,
 * and record the time-of-day for return.
 */
  *jd = (long int)(num + MJD_OFFSET + 0.5);
  *tod = frc;
  return 0;
}

/*.......................................................................
 * Convert a Julian Day Number to a Gregorian calendar date.
 *
 * Input:
 *  jd     long   The Julian Day Number to be converted. This must be
 *                >= 0.
 * Input/Ouput:
 *  year     int *  The Gregorian year (including century) will be assigned
 *                  to *year.
 *  month    int *  The calendar month (1-12) will be assigned to *month.
 *  day      int *  The day of the month (1-31) will be assigned to *day.
 * Output:
 *  return   int    0 - OK.
 *                  1 - Error.
 */
static int jd_to_date(long jd, int *year, int *month, int *day)
{
  long p,q,r,s;   /* Intermediate variables */
/*
 * Check the inputs.
 */
  if(!year || !month || !day) {
    fprintf(stderr, "jd_to_date: NULL argument(s).\n");
    return 1;
  };
  if(jd < 0) {
    fprintf(stderr, "Illegal Julian Day Number.\n");
    return 1;
  };
/*
 * Convert the day number into Gregorian calendar components using the
 * algorithm from the "Explanatory Supplement to the Astronomical
 * Almanac" (equation 12.92-2) but with variables L, N, I, J replaced
 * by p,q,r,s.
 */
  p = jd + 68569L;
  q = (4*p)/146097L;
  p = p - (146097L * q + 3) / 4;
  r = (4000L * (p+1L)) / 1461001L;
  p = p - (1461L * r)/4L + 31L;
  s = (80L * p) / 2447L;
  *day = p - (2447L * s) / 80L;
  *month = s + 2L - 12L * (s/11L);
  *year = 100L * (q-49L) + r + (s/11L);
  return 0;
}

/*.......................................................................
 * Split a fraction of a day into 24-hour clock components.
 *
 * Input:
 *  tod   double    The time-of-day expressed as a normalized fraction of
 *                  a day.
 * Input/Ouput:
 *  hour     int *  The hour of day (0-23) will be assigned to *hour.
 *  minute   int *  The minute of the hour (0-59) will be assigned to *minute.
 *  secs  double *  The number of seconds into the minute (including
 *                  fractional seconds) will be assigned to *secs.
 * Output:
 *  return   int    0 - OK.
 *                  1 - Error.
 */
static int tod_to_time(double tod, int *hour, int *minute, double *secs)
{
  double num,frc;   /* The integral and fractional parts of a number */
/*
 * Check the inputs.
 */
  if(!hour || !minute || !secs) {
    fprintf(stderr, "tod_to_time: NULL argument(s).\n");
    return 1;
  };
  if(tod < 0.0 || tod > 1.0) {
    fprintf(stderr, "Illegal normalized time of day (%g).\n", tod);
    return 1;
  };
/*
 * Split the time-of-day into clock components.
 */
  frc = modf(tod * 24, &num);
  *hour = (int)num;
  frc = modf(frc * 60, &num);
  *minute = (int)num;
  *secs = frc * 60;
  return 0;
}

/*.......................................................................
 * Return the number of days in a given month of a given year.
 *
 * Input:
 *  year     int    The year that contains the month.
 *  month    int    The month (1-12) to query.
 * Output:
 *  return   int    The number of days in the specified month, or
 *                  0 if the arguments are invalid.
 */
static int length_of_month(int year, int month)
{
  int is_leap;  /* True if the year is a leap year */
/*
 * Record the number of days per month for a normal year and a leap year.
 */
  static char daytab[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
  };
/*
 * Check the inputs.
 */
  if(month < 1 || month > 12) {
    fprintf(stderr, "length_of_month: Month %d out of range 1-12.\n", month);
    return 0;
  };
/*
 * Is the specified year a leap year?
 */
  is_leap =  (year%4 == 0 && year%100 != 0) || year%400 == 0;
/*
 * Return the month size.
 */
  return daytab[is_leap?1:0][month-1];
}

/*.......................................................................
 * Return the suffix ordinal suffix suitable for a given ordinal integer.
 * (eg. for 3 "rd").
 *
 * Input:
 *  ordinal    int    The number for which the suffix is needed.
 * Output:
 *  return    char *  A static string containing the suffix.
 */
static char *ordinal_suffix(int ordinal)
{
  if(ordinal < 0)
    ordinal = -ordinal;
  if(ordinal > 10 && ordinal < 20) {  /* The teens all use "th" */
    return "th";
  } else {
    switch(ordinal % 10) {
    case 1:
      return "st";
    case 2:
      return "nd";
    case 3:
      return "rd";
    default:
      return "th";
    };
  };
}
