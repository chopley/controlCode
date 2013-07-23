#ifndef monitor_viewer_h
#define monitor_viewer_h

#include <stddef.h>  /* size_t */

#include "color_tab.h"
#include "arraymap.h"
#include "monitor_stream.h"
#include "im_monitor_stream.h"
#include "input.h"
#include "optcam.h"

/*
 * The MonitorViewer object records the contents and state of the viewer.
 */
typedef struct MonitorViewer MonitorViewer;

MonitorViewer *new_MonitorViewer(size_t buff_size);
MonitorViewer *del_MonitorViewer(MonitorViewer *view);
int rem_MonitorViewer_pages(MonitorViewer *view);
int rem_MonitorViewer_plots(MonitorViewer *view);
int change_MonitorStream(MonitorViewer *view, gcp::control::MonitorStream *ms, int defer);
int change_ImMonitorStream(MonitorViewer *view, ImMonitorStream *ims);
int clr_MonitorBuff(MonitorViewer *view);
ArrayMap *MonitorViewer_ArrayMap(MonitorViewer *view);
int MonitorViewer_fd(MonitorViewer *view);
int MonitorViewer_im_fd(MonitorViewer *view);
gcp::control::MsSendState update_MonitorViewer(MonitorViewer *view);
gcp::control::MsReadState read_MonitorViewer_frame(MonitorViewer *view);
ImsReadState read_MonitorViewer_image(MonitorViewer *view);
gcp::control::MsSendState send_MonitorViewer_msg(MonitorViewer *view, int dowait);
ImsSendState send_MonitorViewer_im_msg(MonitorViewer *view, int dowait);
gcp::control::MsSendState set_MonitorViewer_interval(MonitorViewer *view, unsigned interval);
gcp::control::MsSendState queue_MonitorViewer_rewind(MonitorViewer *view);
int can_MonitorViewer_rewind(MonitorViewer *view);
bool is_MonitorViewer_archivedOnly(MonitorViewer* view);

/*
 * The viewer can contain many plots, each of which is rendered on a separate
 * pgplot output device.
 */
typedef struct MonitorPlot MonitorPlot;

unsigned tag_of_MonitorPlot(MonitorPlot *plot);
MonitorPlot *add_MonitorPlot(MonitorViewer *view, char *device, char* type);

/*
 * Enumerate the possible scrolling modes.
 */
typedef enum {
  SCROLL_DISABLED,     /* No scrolling required */
  SCROLL_MAXIMUM,      /* Scroll to keep the maximum visible */
  SCROLL_MINIMUM       /* Scroll to keep the minimum visible */
} MpScrollMode;

typedef enum {
  PLOT_NORM,
  PLOT_POWSPEC
} MpPlotType;

int config_MonitorPlot(MonitorPlot *plot, char *title, double xleft,
		       double xright, unsigned marker_size, int join,
		       MpScrollMode scroll_mode, double scroll_margin,
		       char *xreg, char *xlabel,
		       char* type, int npt, double dx, bool linAxis=true);

MonitorPlot *rem_MonitorPlot(MonitorPlot *plot);
int rem_MonitorPlot_graphs(MonitorPlot *plot);
int limit_MonitorPlot(MonitorPlot *plot);
MonitorPlot *find_MonitorPlot(MonitorViewer *view, unsigned tag);
int full_MonitorPlot_xrange(MonitorPlot *plot, double *xmin, double *xmax);
int update_MonitorPlot(MonitorPlot *plot, int force);
int resize_MonitorPlot(MonitorPlot *plot);

/*
 * Each plot can contain many graphs, with all graphs sharing the same x-axis.
 */
typedef struct MonitorGraph MonitorGraph;

unsigned tag_of_MonitorGraph(MonitorGraph *graph);
MonitorGraph *add_MonitorGraph(MonitorPlot *plot);
int config_MonitorGraph(MonitorGraph *graph, double ybot, double ytop,
			char *ylabel, char *yregs, unsigned int bits, 
			bool track, bool vecAv, bool linAxis, unsigned apodType);

MonitorGraph *rem_MonitorGraph(MonitorGraph *graph);
int clr_MonitorGraph_regs(MonitorGraph *graph);
MonitorGraph *find_MonitorGraph(MonitorPlot *plot, unsigned tag);
int update_MonitorGraph(MonitorGraph *graph, int force);
int int_MonitorGraph(MonitorGraph *graph, bool integrate);
int auto_MonitorGraph_yrange(MonitorGraph *graph, double *ymin, double *ymax);
int powSpecPkIdent(MonitorGraph* graph, int npk, double xleft, double xright, bool full);

/*
 * The viewer can also contain multiple pages, consisting of a collection
 * of textual fields.
 */
typedef struct MonitorPage MonitorPage;

unsigned tag_of_MonitorPage(MonitorPage *page);
MonitorPage *add_MonitorPage(MonitorViewer *view);
MonitorPage *find_MonitorPage(MonitorViewer *view, unsigned tag);
MonitorPage *rem_MonitorPage(MonitorPage *page);
int freeze_MonitorPage(MonitorPage *page);
int unfreeze_MonitorPage(MonitorPage *page);

/*
 * Each field on a page shows the instantaneous value of one register,
 * textually formatted in one of a variety of ways.
 */
typedef struct MonitorField MonitorField;

/*
 * Describe the textual output formats supported by MonitorField's.
 */
typedef enum {
  MF_FIXED_POINT,  /* Fixed point decimal (%f) */
  MF_SCIENTIFIC,   /* Fixed point exponential (%e) */
  MF_FLOATING,     /* Floating point (%g) */
  MF_SEXAGESIMAL,  /* Degrees/hours,minutes,seconds format */
  MF_INTEGER,      /* Base 10 integer */
  MF_HEX,          /* Base 16 integer */
  MF_OCTAL,        /* Base 8 integer */
  MF_BINARY,       /* Base 2 integer */
  MF_STRING,       /* A printable string */
  MF_DATE,         /* The date and time from a mjd/ms register pair */
  MF_BIT,          /* Display a single bit of an unsigned integer field */
  MF_ENUM,         /* Enumerated names for integer values, taken from a */
                   /*  user specified list of names, starting with the */
                   /*  name to give the value 0. If an integer value */
                   /*  is out of the range of the name list, the value */
                   /*  is displayed as a base 10 integer. */
  MF_BOOL,
  MF_COMPLEX_FIXED
} MonitorFormat;

MonitorField *add_MonitorField(MonitorPage *page);
unsigned tag_of_MonitorField(MonitorField *field);
int config_MonitorField(MonitorField *field, char *regname,
			MonitorFormat format, char *flags,
			int width, int precision, int misc,
			char *names, int warn, double vmin, double vmax,
			int dopage, int nframe);
MonitorField *rem_MonitorField(MonitorField *field);
MonitorField *find_MonitorField(MonitorPage *page, unsigned tag);
int rem_MonitorPage_fields(MonitorPage *page);

typedef struct MonitorImage MonitorImage;

int open_MonitorImage(MonitorViewer *view, char *device);
void draw_MonitorImage_data(MonitorViewer *view);

void set_MonitorImage_centroid(MonitorViewer *view, unsigned xpeak, unsigned ypeak);
void find_MonitorImage_centroid(MonitorViewer *view, double& xcntr, 
				double& ycntr, int centroid);
void fid_MonitorImage_contrast(MonitorViewer *view, double xa, double ya);
void set_MonitorImage_range(MonitorViewer *view, double xa, double xb, double ya, double yb);
int install_MonitorImage_colormap(MonitorViewer *view, char *name);
int reset_MonitorImage_colormap(MonitorViewer *view);
int reset_MonitorImage_contrast(MonitorViewer *view);
void get_MonitorImage_greyscale(MonitorImage *image);
void get_MonitorImage_stats(MonitorViewer *view, double xmin, double xmax,
			    double ymin, double ymax, double *min, double *max,
			    double *mean, double *rms, int *npoint);
void set_MonitorImage_step(MonitorViewer *view, double interval);

void set_MonitorImage_fov(MonitorViewer *view, double fov);
void set_MonitorImage_aspect(MonitorViewer *view, double aspect);
void toggle_MonitorImage_grid(MonitorViewer *view);
void toggle_MonitorImage_bullseye(MonitorViewer *view);
void toggle_MonitorImage_crosshair(MonitorViewer *view);
void toggle_MonitorImage_compass(MonitorViewer *view);
void set_MonitorImage_compass(MonitorViewer *view, double angle);

/*
 * Where provided, the following optional function is used to
 * delete the user_data argument of scroll and field callbacks.
 *
 * Input:
 *  user_data    void *  The argument of the same name that was
 *                       passed to mp_scroll_callback() or
 *                       add_MonitorField().
 * Output:
 *  return       void *  The deleted user_data (always return NULL).
 */
#define MP_DEL_FN(fn) void *(fn)(void *user_data)

/*
 * A function of the following type is registered to each text field of
 * a page. It is called whenever new data arrives to dispatch the
 * the formatted value of the associated register element.
 *
 * Input:
 *  user_data     void *   Anonymous data registered with the function.
 *  page   MonitorPage *   The page to which the field belongs.
 *  value         char *   The formatted value of the register.
 *  warn           int     This is non-zero if the field-value is out
 *                         outside a previously specified range.
 *  page           int     This is non-zero if we should activate the pager
 */
#define MP_FIELD_FN(fn) void (fn)(void *user_data, MonitorPage *page, \
				  char *value, int warn, int dopage, int reset)

int mf_callback_fn(MonitorField *field, MP_FIELD_FN(*field_fn),
		   void *user_data, MP_DEL_FN(*del_fn));
/*
 * A function of the following type can be registered to be called
 * whenever a given plot is scrolled.
 *
 * Input:
 *  user_data     void *   Anonymous data registered with the function.
 *  plot   MonitorPlot *   The plot that was scrolled.
 *  wxa,wxb     double     The new scrolled X-axis limits (world
 *                         coordinates).
 */
#define MP_SCROLL_FN(fn) void (fn)(void *user_data, MonitorPlot *plot, \
				   double wxa, double wxb)

int mp_scroll_callback(MonitorPlot *plot, MP_SCROLL_FN(*user_fn),
		       void *user_data, MP_DEL_FN(*del_fn));
int mp_cursor_to_graph(MonitorPlot *plot, MonitorGraph **graph,
		       double *x, double *y);
int mp_graph_to_cursor(MonitorGraph *graph, double *x, double *y);


// Deprecated

int find_Monitor_point(MonitorGraph *graph, double x, double y,
		       gcp::util::RegDescription& desc, double *xo, double *yo);

// New version

int findMonitorPoint(MonitorGraph *graph, 
		     double x, double y,
		     gcp::util::RegDescription& closestReg, 
		     double& xo, double& yo);

typedef enum {
  MRS_UNINIT,
  MRS_MEAN,
  MRS_RMS,
} MonitorRegStatState;

typedef struct {
  double min;        /* The minimum buffered value */
  double max;        /* The maximum buffered value */
  double mean;       /* The mean of the buffered values */
  double rms;        /* The rms difference from the mean */
  int npoint;       /* The number of points sampled */
  double nsig;       /* nsig from zero = mean/(sigma/sqrt(n)) */
  MonitorRegStatState state;

  void insert(double xval, double yval);

} MonitorRegStats;



int mp_global_RegStats(MonitorViewer *view, gcp::util::RegDescription* ydesc,
		       MonitorRegStats *stats);
int mpRegStats(MonitorPlot *plot, double xa, double xb, 
	       gcp::util::RegDescription& ydesc, MonitorRegStats *stats);
int mp_hardcopy(MonitorPlot *plot, char *device);

int mp_resize_buffer(MonitorViewer *view, size_t size);

int mp_xaxis_limits(MonitorPlot *plot, double *xmin, double *xmax);

int mp_set_calfile(MonitorViewer *view, char *calfile);
int reset_MonitorField_counters(MonitorViewer *view);

void outputRegName(OutputStream* output, 
		   MonitorGraph* graph, 
		   gcp::util::RegDescription& desc);

// Output the name of a register for statistics calculations

void outputStatName(OutputStream* output, 
		    MonitorGraph* graph, 
		    gcp::util::RegDescription& desc);

RegInputMode inputStatMode(MonitorPlot* plot);
RegOutputMode outputStatMode(MonitorPlot* plot);

void change_MonitorImage_ximdir(MonitorViewer* view, int dir);
void change_MonitorImage_yimdir(MonitorViewer* view, int dir);

#endif
