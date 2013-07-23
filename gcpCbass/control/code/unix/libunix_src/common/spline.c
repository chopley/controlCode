#include <stdlib.h>
#include <math.h>
#include <string.h> /* memcpy */

#ifdef VXW
#include "vxWorks.h"
#endif

#include "lprintf.h"
#include "spline.h"

/*
 * Compile the float version of the spline module.
 */
#undef SP_OBJ
#undef SP_TYPE
#define SP_OBJ FltSpline
#define SP_TYPE float

#include "spline_template.c"

/*
 * Compile the double version of the spline module.
 */
#undef SP_OBJ
#undef SP_TYPE
#define SP_OBJ DblSpline
#define SP_TYPE double

#include "spline_template.c"

