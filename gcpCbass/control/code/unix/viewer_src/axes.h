#ifndef axes_h
#define axes_h

/*
 * The following enumerators represent axis-labeling options that are
 * designed to be OR'd together and presented as the 'options' argument
 * of linear_axis() and utc_axis().
 */
typedef enum {
  AXIS_WITH_LABELS=1,  /* Draw numeric labels next to the tick marks */
  AXIS_OUTWARD_TEXT=2, /* When plotting tick labels, plot them projecting out */
                       /*  from the axis instead of parallel to it. */
  AXIS_IS_CRAMPED=4    /* Normally it is guaranteed that there will be */
                       /*  at least 3 major axis ticks. With this option */
                       /*  the guarantee becomes "at least 2 ticks". */
} AxisOptions;

/*
 * Plot a linear axis between specified world-coordinates.
 */
int linear_axis(int options, float x1, float y1, float x2, float y2,
		double v1, double v2, float majl, float majr,
		float minf, float disp);

/*
 * Plot an axis for a range of Modified Julian Dates, labelled with
 * date and time.
 */
int utc_axis(int options, float x1, float y1, float x2, float y2,
	     double mjd1, double mjd2, float majl, float majr,
	     float minf, float disp);

#endif
