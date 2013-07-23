#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <time.h>

#include "cpgplot.h"
#include "color_tab.h"
#include "monitor_viewer.h"
#include "freelist.h"
#include "regset.h"
#include "regcal.h"
#include "output.h"
#include "const.h"
#include "astrom.h"
#include "lprintf.h"
#include "axes.h"

#include "gcp/util/common/CircBuf.h"
#include "gcp/util/common/Complex.h"
#include "gcp/util/common/Coord.h"
#include "gcp/util/common/Dft.h"
#include "gcp/util/common/DoubleBufferVec.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/ImageHandler.h"
#include "gcp/util/common/RegAxisRange.h"
#include "gcp/util/common/RegCal.h"
#include "gcp/util/common/RegDate.h"
#include "gcp/util/common/RegDescription.h"
#include "gcp/util/common/RegParser.h"
#include "gcp/util/common/TimeVal.h"

#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;
using namespace gcp::control;
using namespace gcp::util;

#define PGCFS 1
#define EPS 1e-12
#define MULTI_TEST

#define AXLAB_LEN 80          /* The maximum number of characters per
				 axis label */
#define MP_INDENT_STEP 2      /* The indentation step of
				 output_MonitorViewer */
#define MP_TOP_MARGIN 0.05f   /* The margin above the plot (NDC) */
#define MP_BOT_MARGIN 0.075f  /* The margin below the plot (NDC) */
#define MP_LEFT_MARGIN 0.14f  /* The margin to the left of the plot (NDC) */
#define MP_RIGHT_MARGIN 0.01f /* The margin to the right of the plot (NDC) */
#define MP_XNUM_SIZE  0.9f /* The character size used for X-axis numbers */
#define MP_YNUM_SIZE  0.9f /* The character size used for Y-axis numbers */
#define MP_AXLAB_SIZE 1.0f /* The character size used for axis labels */
#define MP_TITLE_SIZE 1.0f /* The character size used for graph titles */
#define MP_POINT_SYMBOL -1 /* The PGPLOT symbol of plotted data points */
#define MP_MIN_CI 1        /* The lowest color index of plotted data points */
#define MP_MAX_CI 15       /* The highest color index of plotted data points */
#define MP_MIN_SCROLL 1.0  /* The minimum scroll as a percentage x-axis */
#define MP_TICK_BORDER 0.02 /* The x-axis border needed to avoid tick marks */
#define MP_MAX_FIELD 100   /* The maximum width of a text output field */
#define MP_Y_MARGIN 0.05   /* The fraction of the plotted Y-axis range to */
                           /*  add when autoscaling the Y-axis. */
#define MP_AXIS_PREC 20000 /* When there is insufficient precision to divide */
                           /*  a given axis range into at least MP_AXIS_PREC */
                           /*  distinct parts, the range will be widened */
                           /*  accordingly. Currently this number is huge */
                           /*  because of a loss of precision in pgplot */
                           /*  that is equal to the pixel width of the */
                           /*  current device (10000 for /ps). */
#define MP_REG_LEN 100     /* The maximum length of a register specification */
                           /*  string. */
/*
 * When drawing multiple points a buffer will be used to accumulate up
 * to MP_BUF_NPT at a time to be passed to pgpt() as an array. This
 * is faster than sending one point at a time.
 */
#define MP_BUF_NPT 100

/*
 * For efficiency reasons read_MonitorViewer_frame() attempts to read
 * up to MP_READ_SIZE frames at a time before displaying the
 * results. To do this it needs free buffer space, so MP_READ_SIZE
 * frames in the buffer are kept empty by erasing old frames when
 * necessary.
 *
 * If the input stream is non-blocking and the monitor viewer can read
 * and display a frame faster than the input stream can supply data,
 * then the value of MP_READ_SIZE is irrelevant and a value as small
 * as 1 can be used. If however the input stream is connected to a
 * fast data source (eg. a file) then a larger value of MP_READ_SIZE
 * makes reading the file faster, albeit at the expense of slower
 * responsivity in the user interface and at the expense of a reduction
 * in the available buffer space.
 */
#define MP_READ_SIZE 100

/*
 * While loading data with on-the-fly display disabled, text pages
 * are updated every MP_PAGE_SYNC seconds (or whenever a stream
 * boundary is reached).
 */
#define MP_PAGE_SYNC 1

/*
 * Set the freelist blocking factors for various monitor-viewer object types.
 */
#define TRACE_BLK_FACT 50 /* The MonitorTrace free-list blocking factor */
#define GRAPH_BLK_FACT 20 /* The MonitorGraph free-list blocking factor */
#define PLOT_BLK_FACT  10 /* The MonitorPlot free-list blocking factor */
#define PAGE_BLK_FACT  10 /* The MonitorPage free-list blocking factor */
#define FIELD_BLK_FACT 50 /* The MonitorField free-list blocking factor */
/*
 * Forward declare object types that are needed before they are defined.
 */
typedef struct MonitorTrace MonitorTrace;
typedef struct MonitorBuff MonitorBuff;
typedef struct MonitorBuffMap MonitorBuffMap;

struct MonitorImage {
  
  // The data to be displayed.
  
  float data[GRABBER_IM_SIZE];
  
  // Configuration items.
  
  char title[AXLAB_LEN+1];/* The title of the plot */
  char xlab[AXLAB_LEN+1]; /* The text to use to label the X axis of the plots */
  
  // Implementation details.
  
  int pgplot_id;           /* The PGPLOT plot ID to use with cpgslct() */
  int zoomed;              /* True when the plot has been zoomed */
  int display;             /* True when image data has been received */
  int dogrid;              /* If true, display the grid */
  int dobull;              /* If true, display the bullseye */
  double xa,xb;            /* The full left..right X-axis limits */
  double ya,yb;            /* The full bottom..top Y-axis limits */
  double za,zb;            /* The full greyscale range */
  double fov;              /* Field of view of the camera, in arcminutes */
  double wxa,wxb;          /* The current left..right X-axis limits */
  double wya,wyb;          /* The current bottom..top Y-axis limits */
  double wza,wzb;          /* The currently displayed greyscale range */
  int update_needed;       /* True if the plot axes are out of date */
  float vxa,vxb;           /* The horizontal limits of the viewport in NDC */
  float vya,vyb;           /* The vertical limits of the viewport in NDC */
  int nx,ny;               /* Number of pixels in each dimension of the image */
  float aspect;            /* The aspect ratio of the monitor. */
  double contra,bright;    /* Display controls. */
  double interval;         /* The interval (arcseconds) between bullseye rings*/
  int docompass;           /* If true, display the compass */
  int docrosshair;         /* If true, display the crosshair */
  bool havePeak;
  double xpeak, ypeak;
  double backoff;      
  double angle;            /* The current angle of elevation clockwise of the 
			      upwards direction of the TV */
  int ximdir;
  int yimdir;
  
  Cmap *cmap;              /* The color map for image display */
  MonitorViewer *view;     /* The resource object of the parent viewer */
};

/*
 * Monitor fields are used to display, in text form, the value of a
 * single element of a given register.
 */
struct MonitorField {
  unsigned tag;          /* A unique field identifier tag */
  MP_FIELD_FN(*field_fn);/* The function that dispatches field-value strings */
  MP_DEL_FN(*del_fn);    /* The destructor of 'user_data' */
  void *user_data;       /* Arbitrary data to be passed to field_fn() */
  char regname[MP_REG_LEN+1]; /* The name of the register to be displayed */
  MonitorFormat format;  /* The display format */
  char flags[6];         /* Room for the 5 printf flags [-+ 0#] plus '\0' */
  int width;             /* The minimum width of the field */
  int precision;         /* The number of significant figures in float formats*/
  int misc;              /* Format-specific attribute */
  char **names;          /* A dynamically allocated array of enumeration */
                         /*  names to be indexed by integer register */
                         /*  values in bit and enumeration formats. */
  int nname;             /* The number of elements in names[] */
  int warn;              /* True to enable out-of-range warnings */
  int dopage;            /* True to enable out-of-range paging */
  int nframe;            /* The number of frames an out of range
                            condition must persist before the pager is
                            activated */
  int counter;           /* The number of frames during which this
			    field has been out of range.  We will
			    activate the pager only when the counter
			    equals the trigger number.  Since we don't
			    want to be sending page commands
			    continuously, only page when the counter
			    equals the trigger, not after it has
			    surpassed it.  If the register drops back
			    into range, we will reset the counter. */
  double vmin, vmax;     /* The expected range of values */
  gcp::util::RegDescription* desc;
  int regname_invalid;   /* Prevent redundant error messages by keeping a */
                         /*  record of whether regname[] is known to be */
                         /*  invalid. This record is reset whenever regname[] */
                         /*  or the register map are changed. */
  MonitorPage *page;     /* The parent page of the field */
  MonitorField *next;    /* The next of a list of fields in a page */
};

static MonitorField *new_MonitorField(MonitorPage *page);
static MonitorField *del_MonitorField(MonitorField *field);

/*
 * A page consists of a set of textual output fields, each of which
 * displays the latest value of a single element of a register.
 */
struct MonitorPage {
  unsigned tag;            /* A unique page identifier tag */
  MonitorViewer *view;     /* The parent viewer */
  MonitorField *fields;    /* The list of display fields */
  MonitorField *tail_field;/* The last field in 'fields' */
  MonitorPage *next;       /* The next of the list of pages in a viewer */
  int frozen;              /* True to stop monitoring this page */
  unsigned next_field_tag; /* The id tag to assign to the next new field */
                           /* This starts at 1 and is incremented by one */
                           /* every time a new field is created */
};

static MonitorPage *new_MonitorPage(MonitorViewer *view);
static MonitorPage *del_MonitorPage(MonitorPage *page);
static int add_MonitorPage_regs(MonitorPage *page, ArrayMap *arraymap);
static void draw_MonitorPage(MonitorPage *page, 
			     gcp::util::RegCal::RegCalData* calData, 
			     int reset);
static int clr_MonitorPage_regs(MonitorPage *page);

/**.......................................................................
 * Define an object to manage a single FFT
 */
struct Spectrum {
  
  enum PlotState {
    PS_FIRST,
    PS_FIRST_READY,
    PS_DATA_READY,
    PS_DATA_PLOTTED
  };
  
  Spectrum()
  {
    dft   = 0;
    ydata = 0;
    initialize();
  }
  
  void initialize()
  {
    ymin  = 0.0;
    ymax  = 0.0;
    state = PS_FIRST;
    updateNeeded = false;
  }
  
  void resize(unsigned npt, unsigned ntrans, 
	      Dft::Apodization apod=Dft::APOD_RECTANGLE) 
  {
    if(dft==0) {
      dft   = new Dft(npt, true, apod);
    } else {
      dft->resize(npt);
    }
    
    if(ydata==0) {
      ydata = new DoubleBufferVec<float>(ntrans);
    } else {
      ydata->resize(ntrans);
    }
    
    // And reinitialize pertinent variables
    
    initialize();
  }
  
  virtual ~Spectrum() {}
  
  // Method of Spectrum class to draw power spectrum data and step the
  // state machine of a power spectrum plot
  
  void stepPowSpecState(MonitorGraph* graph, int ci, bool erase);
  
  // Method of Spectrum to push a single sample onto the dft ring
  // buffer
  
  void pushSample(double sample, bool& first, double& ymin, double& ymax, 
		  bool doLog);
  
  // Draw the power spectrum data
  
  void redrawPowSpecData(MonitorGraph* graph, int ci, bool erase);
  
  // Set the apodization type

  void setApodizationType(Dft::Apodization type);

  // members
  
  Dft* dft;
  DoubleBufferVec<float>* ydata;
  double ymin;
  double ymax;
  PlotState state;
  
  bool updateNeeded;
};

/**.......................................................................
 * And an object to manage a group of spectra
 */
struct PowSpecManager {
  
  PowSpecManager()
  {
    initialize();
  }
  
  PowSpecManager(unsigned nspectra, unsigned npt, unsigned ntrans, 
		 Dft::Apodization apod=Dft::APOD_RECTANGLE) 
  {
    spectra.resize(nspectra);
    resize(npt, ntrans, apod);
    initialize();
  }
  
  void initialize() 
  {
    ymin  = 0.0;
    ymax  = 0.0;
    updateNeeded = false;
  }
  
  void resize(unsigned nspectra)
  {
    spectra.resize(nspectra);
    initialize();
  }
  
  void resize(unsigned npt, unsigned ntrans, Dft::Apodization apod=Dft::APOD_RECTANGLE) 
  {
    for(unsigned i=0; i < spectra.size(); i++)
      spectra[i].resize(npt, ntrans, apod);
    
    initialize();
  }
  
  void resize(unsigned nspectra, unsigned npt, unsigned ntrans, Dft::Apodization apod=Dft::APOD_RECTANGLE)  
  {
    resize(nspectra);
    resize(npt, ntrans, apod);
  }

  virtual ~PowSpecManager() {}
  
  void setApodizationType(gcp::util::Dft::Apodization type) {
    for(unsigned iSpec=0; iSpec < spectra.size(); iSpec++)
      spectra[iSpec].setApodizationType(type);
  }

  // Keep a vector of spectra
  
  vector<Spectrum> spectra; 
  
  bool updateNeeded;
  double ymin;
  double ymax;
};

/*
 * A trace designates the set of lines on a graph that display the
 * values of one or more elements of a single register, from a single
 * register specification.
 *
 * Note that register specifications can now include regexp
 * expressions, so that a trace object may manage more than one
 * effective trace on a graph
 */
struct MonitorTrace {
  
  MonitorGraph *graph;        /* The graph that contains the traces */
  char regname[MP_REG_LEN+1]; /* The textual register specification */
  int regname_invalid;        /* Prevent redundant error messages by */
                              /*  keeping a record of whether regname[] */
                              /*  is known to be invalid. This record is */
                              /*  reset whenever regname[] or the register */
                              /*  map are changed. */
  vector<RegDescription>* descs;
  MonitorTrace *next;         /* The next trace in the list */
  
  PowSpecManager* powSpec;
  
  // Method to add the registers in this trace to the map of registers
  // to be monitored
  
  bool addRegs();
  
  bool checkRegConsistency();
  
  // Push power spectrum data into the Dft container for this trace
  
  void pushPowSpecData(bool& first, double& ymin, double& ymax);
  
  // Draw, stepping the state of a power spectrum plot accordingly
  
  void stepPowSpecState(int& ci, int erase);
  
  // Find the closest point
  
  void findClosestPoint(double x, double y, 
			double x_to_mm, double y_to_mm, 
			bool& first, 
			double& min_x, double& min_y, double& min_rsq,
			RegDescription& closestReg);
  
  // Find the closest point for normal plots
  
  void findClosestPointNorm(double x, double y, 
			    double x_to_mm, double y_to_mm, 
			    bool& first, 
			    double& min_x, double& min_y, double& min_rsq, 
			    RegDescription& closestReg);
  
  // Find the closest point for power spectrum plots
  
  void findClosestPointPowSpec(double x, double y, 
			       double x_to_mm, double y_to_mm, 
			       bool& first, 
			       double& min_x, double& min_y, double& min_rsq, 
			       RegDescription& closestReg);
  
  // Get statistics
  
  void getStats(double xa, double xb, MonitorRegStats& s, RegDescription* desc);
  
  void getStatsNorm(double xa, double xb, MonitorRegStats& s, 
		    RegDescription* desc);
  
  void getStatsPowSpec(double xa, double xb, MonitorRegStats& s, 
		       RegDescription* desc);
  
  void getStatsSpectrum(double xa, double xb, MonitorRegStats& s, 
			RegDescription* desc);
};

static MonitorTrace *new_MonitorTrace(MonitorGraph *graph);
static MonitorTrace *del_MonitorTrace(MonitorTrace *trace);

/*
 * A given graph displays one or more registers within a single set
 * of x and y axes.
 */
struct MonitorGraph {
  /*
   * Configuration items.
   */
  MonitorTrace *traces;    /* The list of registers to plot on the graph */
  char ylab[AXLAB_LEN+1];  /* The text Y-axis label */
  double wya,wyb;          /* The y-axis limits */
  double range;
  bool track;
  unsigned int bits;       /* An optional bitmask of bits to be displayed */
                           /*  individually (0 if not wanted). */
  /*
   * Implementation details.
   */
  unsigned tag;           /* A graph identifier tag that is unique within */
                          /* the scope of the parent plot */
  int update_needed;      /* True if the graph axes are out of date */
  int started;            /* True after any of graph has been plotted */
  float vxa,vxb;          /* The horizontal limits of the viewport in NDC */
  float vya,vyb;          /* The vertical limits of the viewport in NDC */
  MonitorPlot *plot;      /* The plot that contains the graph */
  MonitorGraph *next;     /* The next graph in the list */
  
  struct {
    bool updateNeeded;
    double ymin;
    double ymax;
    bool linAxis;

    unsigned npk;
    bool pkIdent;
    bool fullXrange;
    double pkXmin;
    double pkXmax;
    gcp::util::Dft::Apodization apodType;
  } powSpec;
  
  struct {
    double xa;
    double xb;
    double ya;
    double yb;
    bool xIsLinear;
    bool yIsLinear;
  } lastPlot;
  
  bool addTraces(char* yregs);
  
  // Get the aspect of the x-axis register
  
  RegAspect getXAspect();
  
  // Methods for drawing data
  
  void drawNormal(int oldest, int newest, int erase);
  void drawPowSpec(int erase);
  
  void integrate(bool doInt);
  void integrateNorm(bool doInt);
  void integratePowSpec(bool doInt);
  void setVectorAverage(bool vecAv);
  void setLinearAxis(bool linAxis);
  void setApodizationType(gcp::util::Dft::Apodization type);
};

static MonitorGraph *new_MonitorGraph(MonitorPlot *plot);
static MonitorGraph *del_MonitorGraph(MonitorGraph *graph);
static void draw_MonitorGraph_axes(MonitorGraph *graph, int erase);
static void draw_MonitorGraph_data(MonitorGraph *graph,
				   int oldest, int newest, int erase);
static void drawMonitorGraphData(MonitorGraph *graph,
				 int oldest, int newest, int erase);
static bool checkYAxis(MonitorGraph *graph, int oldest, int newest, 
		       float* ya, float* yb);

static void drawNext(MonitorGraph* graph,
		     gcp::util::RegDescription& xdesc,
		     gcp::util::RegDescription& ydesc,
		     int isnap,
		     bool isPoint,	    
		     int& nbuf,
		     bool& first,
		     int bit=0);

static void drawPoint(MonitorGraph* graph, 
		      gcp::util::RegDescription& xdesc,
		      gcp::util::RegDescription& ydesc,
		      int isnap,
		      int& nbuf,
		      int bit);

static void drawNormalLine(MonitorGraph* graph, 
			   gcp::util::RegDescription& xdesc,
			   gcp::util::RegDescription& ydesc,
			   int isnap,
			   int& nbuf,
			   int bit);

static void drawPhaseLine(MonitorGraph* graph, 
			  gcp::util::RegDescription& xdesc,
			  gcp::util::RegDescription& ydesc,
			  int isnap,
			  bool& first);

static void getStat(MonitorGraph* graph, 
		    gcp::util::RegDescription& xdesc,
		    gcp::util::RegDescription& ydesc,
		    int isnap,
		    double xa, 
		    double xb,
		    MonitorRegStats& stats);

static void draw_value_points(MonitorGraph *graph, int oldest, int newest,
			      int x_offset, int y_offset);

static void drawValueSa(MonitorGraph* graph, 
			MonitorTrace* trace,
			int oldest,
			int newest,
			bool erase,
			int& ci);


static void drawValueMaSe(MonitorGraph* graph, 
			  MonitorTrace* trace,
			  int isnap,
			  bool erase,
			  int& ci);

static void drawValueMaMe(MonitorGraph* graph, 
			  MonitorTrace* trace,
			  int isnap,
			  bool erase,
			  int& ci);

static void getStatsSa(MonitorTrace* trace,
		       double xa, double xb,
		       MonitorRegStats& stats,
		       gcp::util::RegDescription* statDesc=0);

static void getStatsMaSe(MonitorTrace* trace,
			 double xa, double xb,
			 MonitorRegStats& stats,
			 gcp::util::RegDescription* statDesc=0);

static void getStatsMaMe(MonitorTrace* trace,
			 double xa, double xb,
			 MonitorRegStats& stats,
			 gcp::util::RegDescription* statDesc=0);

static void findClosestPointSa(MonitorTrace* trace,
			       double x, double y,
			       double x_to_mm, double y_to_mm,
			       gcp::util::RegDescription& closestReg,
			       double& min_x, double& min_y, double& min_rsq,
			       bool& first);

static void findClosestPointMaSe(MonitorTrace* trace,
				 double x, double y,
				 double x_to_mm, double y_to_mm,
				 gcp::util::RegDescription& closestReg,
				 double& min_x, double& min_y, double& min_rsq,
				 bool& first);

static void findClosestPointMaMe(MonitorTrace* trace,
				 double x, double y,
				 double x_to_mm, double y_to_mm,
				 gcp::util::RegDescription& closestReg,
				 double& min_x, double& min_y, double& min_rsq,
				 bool& first);

static void checkPoint(MonitorGraph* graph, 
		       gcp::util::RegDescription& xdesc,
		       gcp::util::RegDescription& ydesc,
		       int isnap,
		       double x, double y,
		       double x_to_mm, double y_to_mm,
		       gcp::util::RegDescription& closestReg,
		       double& min_x, double& min_y, double& min_rsq, 
		       bool& first);

static void draw_logic_points(MonitorGraph *graph, int oldest, int newest,
			      int x_offset, int y_offset);

static void drawLogicSa(MonitorGraph* graph, 
			MonitorTrace* trace,
			int oldest,
			int newest,
			bool erase,
			int& ci);

static void drawLogicMaSe(MonitorGraph* graph, 
			  MonitorTrace* trace,
			  int isnap,
			  bool erase,
			  int& ci);

static void drawLogicMaMe(MonitorGraph* graph, 
			  MonitorTrace* trace,
			  int isnap,
			  bool erase,
			  int& ci);

static void draw_value_lines(MonitorGraph *graph, int oldest, int newest,
			     int x_offset, int y_offset);
static void draw_logic_lines(MonitorGraph *graph, int oldest, int newest,
			     int x_offset, int y_offset);
static void draw_phase_lines(MonitorGraph *graph, int oldest, int newest,
			     int x_offset, int y_offset);
static void find_normal_point(MonitorGraph *graph, MonitorBuff *mb,
			      double x, double y,
			      double x_to_mm, double y_to_mm,
			      int x_offset, int y_offset,
			      int oldest, int newest, double *min_rsq,
			      int *min_snap, int *first, int *found);
static void find_logic_point(MonitorGraph *graph, MonitorBuff *mb,
			     double x, double y,
			     double x_to_mm, double y_to_mm,
			     int x_offset, int y_offset,
			     int oldest, int newest, double *min_rsq,
			     int *min_snap, int *first, int *found);

static void flag_MonitorGraph(MonitorGraph *graph);

static double x_ndc_to_graph(MonitorGraph *graph, double ndc_x);
static double y_ndc_to_graph(MonitorGraph *graph, double ndc_y);
static double x_graph_to_ndc(MonitorGraph *graph, double world_x);
static double y_graph_to_ndc(MonitorGraph *graph, double world_y);

static void checkXaxisValidity(char* xregname, 
			       char* yregname, 
			       MonitorBuffMap* map);

static void checkXaxisValidity(char* xregname, 
			       std::vector<gcp::util::RegDescription>& xdescs,
			       std::vector<gcp::util::RegDescription>& ydescs);

static int mpTraceRegStats(MonitorPlot *plot, MonitorTrace* trace,
			   double xa, double xb, 
			   MonitorRegStats *stats,
			   gcp::util::RegDescription* desc=0);

static MonitorTrace* 
findTrace(MonitorPlot* plot, gcp::util::RegDescription& desc);

/*
 * Provide a container for collecting scrolling information.
 */
typedef struct {
  MpScrollMode mode;      /* The scrolling mode */
  double margin;          /* The fractional x-axis length to scroll */
  MP_SCROLL_FN(*user_fn); /* The current scroll callback (or NULL) */
  void *user_data;        /* Anonymous data to be passed to user_fn() */
  MP_DEL_FN(*del_fn);     /* The destructor function for 'user_data' */
} MpScroll;

/* Plot X-axis statistics */

typedef struct {
  int empty;              /* True until data has been received */
  double xmax, xmin;      /* The min and max values of the X-axis register */
} MpPlotStats;

/*
 * Each plot contains one or more graphs, stacked vertically one above
 * the other such that they share the same x-axis.
 */
struct MonitorPlot {
  /*
   * Configuration items.
   */
  double xleft, xright;      /* The range of the X axis */
  char regname[MP_REG_LEN+1];/* The name of the X-axis register */
  unsigned marker_size;      /* The line width of each data marker or line */
  int join;                  /* Join neighboring data-points with lines? */
  char title[AXLAB_LEN+1];   /* The title of the plot */
  char xlabel[AXLAB_LEN+1];  /* The x-axis label */
  /*
   * Implementation details.
   */
  unsigned tag;            /* A plot identifier tag that is unique within */
                           /* the scope of the viewer */
  int pgplot_id;           /* The PGPLOT plot ID to use with cpgslct() */
  int oldest;              /* The snapshot number of the last clear operation */
  MonitorGraph *graphs;    /* The list of active graphs */
  MonitorGraph *tail_graph;/* The last graph in 'graphs' */
  int ngraph;              /* The number of graphs in graphs[] */
  unsigned next_graph_tag; /* The id tag to assign to the next new graph */
                           /* This starts at 1 and is incremented by one */
                           /* every time a new graph is created */
  double wxa,wxb;          /* The current left..right X-axis limits */
  double xlength;          /* xright - xleft */
  MpPlotStats stats;       /* X-axis register statistics */
  MpScroll scroll;         /* Horizontal scrolling information */
  std::vector<gcp::util::RegDescription>* xdescs;
  int regname_invalid;     /* Prevent redundant error messages by */
                           /*  keeping a record of whether regname[] */
                           /*  is known to be invalid. This record is */
                           /*  reset whenever regname[] or the register */
                           /*  map are changed. */
  int update_needed;       /* True if the plot axes are out of date */
  float vxa,vxb;           /* The horizontal limits of the viewport in NDC */
  float vya,vyb;           /* The vertical limits of the viewport in NDC */
  MonitorViewer *view;     /* The resource object of the parent viewer */
  MonitorPlot *next;       /* The next plot in the list */
  
  MpPlotType type;         // Specifies the type of plot (normal,
			   // power spectrum)
  
  struct {
    unsigned npt;            // If type == PLOT_POWSPEC, this is the
			     // length of the input array to be
			     // transformed
    unsigned nel;            // The number of elements all register
			     // specifications of this plot type must
			     // have
    unsigned ntrans;         // The length of the transform
    bool first;              // True before the first register of this
			     // plot has been successfully parsed
    gcp::util::TimeVal* dt;  // If type == PLOT_POWSPEC, this is the
			     // time resolution of the axis
    
    gcp::util::Frequency* dv;// The frequency resolution of the transform
    
    vector<float>* xdataLinear;// An array in which the xvalues of the
			       // transform will be stored
    vector<float>* xdataLog;   // An array in which the xvalues of the
			       // transform will be stored
    bool updateNeeded;
    
    unsigned nSec;           // The dt argument converted to nanoSeconds
    
    bool linAxis;            // True if power spectra should be
			     // plotted with a linear x-axis
  } powSpec;
  
  //------------------------------------------------------------
  // Methods of MonitorPlot
  //------------------------------------------------------------
  
  bool updateNeeded();     // Return whether or not this plot needs to
			   // be updated
  void updateXaxisStats(unsigned nread); // Recalculate xaxis stats
					 // after new data have been
					 // read
  bool xAxisIsValid();
  
  // Return true if this plot has a single register as its x-axis
  
  bool isSingleXAxis();
  
  // Return true if this plot has a single-element x-axis register
  
  bool xAxisIsSingleElement();
  
  void update(unsigned nread);
  
  void updateNormal(unsigned nread);
  
  void updatePowSpec(unsigned nread);
  
  // Read trace specifications from a char array and add traces to
  // correspond
  
  bool checkXregisters(std::vector<RegDescription>& xdesc);
  
  // Method to return the xregister specification of a plot
  
  bool checkYregisters(char* yregs, 
		       std::vector<RegDescription>& xdesc,
		       std::vector<RegDescription>& ydesc);
  
  // Count the number of active traces in this plot
  
  unsigned countTraces();
  
  // Iterate over all traces, pushing new data into the power spectra
  // containers.
  
  void pushPowSpecData();
  
  // Iterate over all traces in this plot, replotting as necessary
  
  void replotPowSpec();
  
  // Called to end a buffered redraw of a power spectrum plot
  
  void endBufferedPowSpecAxisRedraw();
  
  // Start a buffered redraw of the axes for this plot
  
  void beginBufferedPowSpecAxisRedraw();
  
  // Methods of powSpec
  
  void initPowSpec(unsigned n, double dtInMilliSeconds, 
		   double& xleft, double& right, bool linAxis);
  
};

static void identifyPeaks(MonitorGraph* graph, unsigned npt, float* xptr, float* yptr);

//-----------------------------------------------------------------------
// Methods of MonitorPlot
//-----------------------------------------------------------------------

static MonitorPlot *new_MonitorPlot(MonitorViewer *view, char *device);
static MonitorPlot *del_MonitorPlot(MonitorPlot *plot);

static int scroll_MonitorPlot(MonitorPlot *plot);
static void flag_MonitorPlot(MonitorPlot *plot);
static int get_MonitorPlot_stats(MonitorPlot *plot);
static void draw_MonitorPlot_title(MonitorPlot *plot, int erase);
static int add_MonitorPlot_regs(MonitorPlot *plot, ArrayMap *arraymap,
				MonitorBuffMap *map);

static int add_MonitorPlot_regs(MonitorViewer *view, MonitorBuffMap *map,
				char *regname, vector<RegDescription>* descs,
				bool split=false);

static int add_MonitorPlot_reg(MonitorViewer *view, MonitorBuffMap *map,
			       char *regname, gcp::util::RegDescription* desc);

static int clr_MonitorPlot_regs(MonitorPlot *plot);

/*
 * Each register that has been selected for monitoring is associated
 * with an element of the following type.
 */
typedef struct {
  int oldest;         /* The first snapshot that sampled the register */
  int slot;           /* The archive-frame slot that contains the register */
  /*  For amplitudes and phases derived from complex */
  /*  pairs of register elements, 'slot' is the index */
  /*  of the first element (ie the real part) of the */
  /*  register pair. */
  RegAspect aspect;    /* The buffered form of the register */
  
  unsigned nInt;      // The number of snapshots which have been
		      // coadded to produce the current value of this
		      // aspect
  unsigned intType;
  gcp::util::Complex<float>* lastComplexVal;
  gcp::util::RegDate* lastDate;
} MonitorBuffReg;

/*
 * An array of the following type records the buffered locations of
 * each register element in the current register map. Unbuffered
 * elements are denoted with buffer locations of -1.
 */
typedef struct {
  int a[REG_NASPECT]; /* The buffer location of each aspect of the */
  /*  register element. The a[] array is designed */
  /*  to be indexed by RegAspect enumerators. */
} MonitorBuffOff;

/*
 * The following two objects encapsulate buffered monitor data.
 * The buffer is a 1D array organized as a 2D array in which
 * each block of arraymap_size consecutive elements contains a snapshot of
 * register values at a single time. The buffer is a fixed size, so
 * whenever arraymap_size is changed, the maximum number of snapshots that
 * will fit within the buffer is recomputed and recorded in 'max_snap'.
 * When this number is exceeded the oldest snapshot is erased from the
 * display, then overwritten by the newest. Snapshot numbers (oldest
 * and newest) increment past max_snap, so they should be taken modulo
 * max_snap when used to index into the buffer.
 */
struct MonitorBuffMap {
  ArrayMap *arraymap;    /* The array map that the map is valid for */
  bool archivedOnly_;
  int max_snap;         /* The number of snapshots that can be buffered */
  int nsnap;            /* The current number of buffered snapshots */
  int oldest;           /* The oldest snapshot is at: */
  /*  MonitorBuff::data[nregs * (oldest % max_snap)] */
  int newest;           /* The newest snapshot is at: */
  /*  MonitorBuff::data[nregs * (newest % max_snap)] */
  int arraymap_size;    /* The number of elements in the array map */
  int nbuf[REG_NASPECT];/* The number of buffered elements per aspect */
  MonitorBuffReg *regs;  /* Details of each buffered register within a */
                         /*  snapshot, in the order in which it appears in */
                         /*  the buffer. The array has room for regmap_size */
                         /*  REG_PLAIN registers, and for regmap_size/2 */
                         /*  of each of the aspects derived from pairs of */
                         /*  registers. */
  int regs_dim;         /* The dimension of regs[] */
  int nregs;            /* The total number of registers per snapshot */
  MonitorBuffOff *offset;/* For each aspect of each register in the */
                         /*  register map, the location at which that */
                         /*  aspect is found within any given snapshot */
                         /*  in the buffer is given as an offset relative */
                         /*  to the first buffered element of that snapshot. */
                         /*  Register aspects that are not being buffered, */
                         /*  signified with an offset of -1 */
  unsigned* integrationFlags;    /* The integration flag for each register in the map */
};

struct MonitorBuff {
  double *data;         /* Up to 'data_dim' buffered register element values */
  int data_dim;        /* The dimension of data[] */
  MonitorBuffMap *old;  /* The previous buffer map */
  MonitorBuffMap *map;  /* The layout of the buffered data within data[] */
};

/*
 * The following object encapsulates the X and Y axis arrays that are
 * used to buffer up multiple points to be drawn by a single call to
 * pgpt().
 */
typedef struct {
  float x[MP_BUF_NPT];    /* The X-axis coordinates of each point */
  float y[MP_BUF_NPT];    /* The Y-axis coordinates of each point */
} MpPointBuf;

/*
 * An output stream is used to format textual values of output fields.
 * An member of the following type contains the output stream and the
 * string that it writes to.
 */
typedef struct {
  char string[MP_MAX_FIELD];  /* The output string */
  OutputStream *stream;       /* A stream wrapper around string[] */
} MpFormatBuf;

/*
 * This following object is the top level object of the viewer
 * hierarchy.
 */
struct MonitorViewer {
  FreeList* trace_mem;    /* A free-list for allocating MonitorTrace objects */
  FreeList* graph_mem;    /* A free-list for allocating MonitorGraph objects */
  FreeList* plot_mem;     /* A free-list for allocating MonitorPlot objects */
  FreeList* page_mem;     /* A free-list for allocating MonitorPage objects */
  FreeList* field_mem;    /* A free-list for allocating MonitorField objects */
  
  InputStream* input;     /* An input stream for parsing register names. */
  
  MonitorPage* pages;     /* The list of active pages */
  MonitorPage* tail_page; /* The last page in 'pages' */
  
  MonitorPlot* plots;     /* The list of active plots */
  MonitorPlot* tail_plot; /* The last plot in 'plots' */
  
  MonitorBuff*      mb;   /* Buffered monitor data */
  MonitorStream*    ms;   /* A monitor-data stream or NULL if not assigned */
  MonitorImage   image;   /* The frame grabber plot descriptor */
  
  gcp::util::ImageHandler* imageHandler_;
  
  ImMonitorStream* ims;   /* An image  monitor-data stream or NULL if not 
			     assigned */
  MpPointBuf ptbuf;       /* Container of arrays for buffering point plotting */
  MpFormatBuf fmt;        /* A formatting buffer for text fields */
  
  unsigned next_page_tag; /* The id tag to assign to the next new page */
                          /* This starts at 1 and is incremented by one */
                          /* every time a new page is created */
  unsigned next_plot_tag; /* The id tag to assign to the next new plot */
                          /* This starts at 1 and is incremented by one */
                          /* every time a new plot is created */
  int defer_display;      /* True to disable on-the-fly display while loading */
  time_t page_preview;    /* While loading data with on-the-fly display */
                          /*  disabled, text pages are updated every */
                          /*  MP_PAGE_SYNC seconds. page_preview records */
                          /*  the time of the last update. */
  int update_needed;      /* True after viewer configuration changes */
  int send_in_progress;   /* This is true during the time that it takes to */
                          /* send an outgoing message over the monitor stream */
  int im_send_in_progress;/* This is true during the time that it takes to */
                          /* send an outgoing message over the image monitor 
			     stream */
  unsigned interval;      /* The last requested sampling interval */
  char *calfile;          /* The name of the current calibration file */
  
  // Method called whenever new data are available to be read, to
  // erase any data from current plots whose slots in the ring buffer
  // will be overwritten by the new read
  
  void eraseOldDataIfNecessary(unsigned read_size);
  
  // Called after new data have been read to copy calibrated registers
  // into the buffer
  
  void updateCalibratedRegisters();
  
  // Called after new data have been read to update plots
  
  void updatePlots(unsigned nread);
  
  // Called after new data have been read to update pages
  
  void updatePages();
  
};

static void flag_MonitorViewer(MonitorViewer *view, int propagate);
static int clr_MonitorViewer_regs(MonitorViewer *view);

static MonitorBuffMap *new_MonitorBuffMap(MonitorViewer *view);
static int fill_MonitorBuffMap(MonitorViewer *view, MonitorBuffMap *map);
static MonitorBuffMap *del_MonitorBuffMap(MonitorBuffMap *map);
static MonitorBuff *new_MonitorBuff(size_t size);
static MonitorBuff *del_MonitorBuff(MonitorBuff *mb);
static int config_MonitorBuff(MonitorViewer *view);
static int refine_MonitorPlot_oldest(MonitorPlot *plot);
static int remap_MonitorBuff(MonitorViewer *view);
static int cmp_MonitorBuffReg(const void *v1, const void *v2);
static int page_preview_pending(MonitorViewer *view);

static int shrink_MonitorBuff(MonitorViewer *view, int size);
static int expand_MonitorBuff(MonitorViewer *view, int size);
static void vet_axis_limits(double *a1, double *a2);
static void init_MonitorImage(MonitorImage *image);

/*.......................................................................
 * Create an object for recording formatting details for displaying the
 * latest value of a given register element on a given page. The object
 * is initialized with default values and is not valid for displaying
 * a register value until it has been configured with
 * config_MonitorField().
 *
 * Input:
 *  page      MonitorPage *   The page that the field belongs to.
 */
static MonitorField *new_MonitorField(MonitorPage *page)
{
  MonitorViewer *view;   /* The resource object of the monitor viewer */
  MonitorField *field;   /* The newly created field */
  /*
   * Check arguments.
   */
  if(!page) {
    lprintf(stderr, "new_MonitorField: NULL argument.\n");
    return NULL;
  };
  /*
   * Get the monitor viewer resource object.
   */
  view = page->view;
  /*
   * Allocate the container.
   */
  field = (MonitorField* )new_FreeListNode("new_MonitorField", view->field_mem);
  if(!field)
    return NULL;
  /*
   * Before attempting any operation that might fail, initialize the
   * container at least up to the point at which it is safe to pass
   * it to del_MonitorField().
   */
  field->tag        = page->next_field_tag++;
  field->field_fn   = 0;
  field->del_fn     = 0;
  field->user_data  = NULL;
  field->regname[0] = '\0';
  field->format     = MF_FLOATING;
  field->flags[0]   = '\0';
  field->width      = 0;
  field->precision  = 0;
  field->misc       = 0;
  field->names      = NULL;
  field->nname      = 0;
  field->warn       = 0;
  field->vmin       = 0.0;
  field->vmax       = 0.0;
  field->dopage     = 0;
  field->counter    = 0;
  field->nframe     = 1;
  field->regname_invalid = 0;
  field->page       = page;
  field->next       = NULL;
  
  field->desc = 0;
  field->desc = new gcp::util::RegDescription();
  
  /*
   * Return the new object.
   */
  return field;
}
/*.......................................................................
 * Configure or reconfigure a given register field.
 *
 * Note that a subsequent call to update_MonitorViewer() will be needed
 * before the field will be displayable.
 *
 * Input:
 *  field    MonitorField *   The field to be configured.
 *  regname          char *   The name of the register element to be
 *                            rendered.
 *  format  MonitorFormat     The form in which to display the value
 *                            as a text string (see monitor_viewer.h).
 *  flags            char *   A string concatenation of zero or more
 *                            printf formatting flags, taken from the
 *                            set '-', '+', ' ', '0', '#'.
 *  width             int     The printf field width format specifier.
 *  precision         int     The printf precision format specifier.
 *                            In the case of MF_SEXAGESIMAL this is
 *                            interpretted as the number of decimal
 *                            places in the seconds field.
 *  misc              int     Format specific width attribute. Currently
 *                            this is only used to set the minimum width
 *                            of the sexagesimal hours field, and the bit
 *                            number when displaying single bits.
 *  names            char *   The list of enumeration names to use with
 *                            bit and enum fields. For other formats this
 *                            argument is ignored, so it can be NULL.
 *  warn              int     If true, enable the warn argument of field_fn().
 *  vmin, vmax     double     When field_fn() (see mf_callback_fn()) is
 *                            called to report a new field value, its
 *                            'warn' argument is set to 0 if the field
 *                            value is in the range vmin..vmax, and 1
 *                            otherwise. These values are ignored for
 *                            string fields.
 *  dopage            int     If true, enable paging if the register goes out 
 *                            of range.
 *  nframe            int     The number of frames this condition must persist
 *                            before the pager is activated.
 * Output:
 *  return            int     0 - OK.
 *                            1 - Error (the field will be left as it was).
 */
int config_MonitorField(MonitorField *field, char *regname,
			MonitorFormat format, char *flags,
			int width, int precision, int misc,
			char *names, int warn, double vmin, double vmax,
			int dopage, int nframe)
{
  MonitorViewer *view;    /* The resource object of the monitor viewer */
  gcp::util::RegDescription desc;
  char **name_array=NULL; /* The array of names in names[] */
  int nname=0;            /* The number of elements in name_array[] */
  int i;
  gcp::util::RegParser parser;
  
  // Check arguments.
  
  if(!field || !regname || !flags) {
    lprintf(stderr, "config_MonitorField: NULL argument.\n");
    return 1;
  };
  
  // Get the monitor viewer resource object.
  
  view = field->page->view;
  
  // Is the field register being changed?
  
  if(field->desc->iSlot() < 0 || strcmp(field->regname, regname) != 0) {
    InputStream *stream = view->input;
    
    // Check the syntax of the register specification.
    
    if(open_StringInputStream(stream, 0, regname)) {
      lprintf(stderr, "config_MonitorField: InputStream failure.");
      return 1;
    };
    
    gcp::util::RegDescription desc;
    
    try {
      desc = parser.inputReg(stream, true, REG_INPUT_ELEMENT, true);
    } catch(gcp::util::Exception& err) {
      std::ostringstream os;
      os << err.what();
      lprintf(stderr, os.str().c_str());
      close_InputStream(stream);
      return 1;
    }
    
    if(parser.validity() == REG_INVALID) {
      close_InputStream(stream);
      return 1;
    };
    
    close_InputStream(stream);
    
    // Record the new register name.
    
    if(strlen(regname) > MP_REG_LEN) {
      lprintf(stderr, "Register name too long: %s\n", stream->work);
      return 1;
    };
    strcpy(field->regname, regname);
    
    // Invalidate the previously recorded register-map specification.
    
    field->desc->reset();
    field->regname_invalid = 0;
    
    // Mark the viewer as needing to update the selection of monitored
    // registers.
    
    flag_MonitorViewer(view, 0);
  } else {
    desc = *(field->desc);
  };
  
  // Check for consistency between the register specification and the
  // requested display format.
  
  switch(format) {
  case MF_FIXED_POINT:  // Fixed point decimal (%f) 
  case MF_SCIENTIFIC:   // Fixed point exponential (%e) 
  case MF_FLOATING:     // Floating point (%g) 
  case MF_SEXAGESIMAL:  // Degrees/hours,minutes,seconds format 
  case MF_INTEGER:      // Base 10 integer 
  case MF_HEX:          // Base 16 integer 
  case MF_OCTAL:        // Base 8 integer 
  case MF_BINARY:       // Base 2 integer 
  case MF_DATE:         // Date and time from mjd/time-of-day register pair 
  case MF_BOOL:
  case MF_COMPLEX_FIXED:
    if(desc.nEl() != 1) {
      lprintf(stderr, "Only one register element is permitted per field.\n");
      return 1;
    };
    break;
  case MF_BIT:
    if(desc.nEl() != 1) {
      lprintf(stderr, "Only one register element is permitted per field.\n");
      return 1;
    };
    if(misc < 0 || misc > 31) {
      lprintf(stderr, "Bit number outside the available range of 0..31.\n");
      return 1;
    };
    if(precision <= 0 || precision >= MP_MAX_FIELD)
      precision = MP_MAX_FIELD-1;
    break;
  case MF_STRING:       /* Printable string */
    if(precision <= 0 || precision >= MP_MAX_FIELD)
      precision = MP_MAX_FIELD-1;
    break;
  case MF_ENUM:
    if(desc.nEl() != 1) {
      lprintf(stderr, "Only one register element is permitted per field.\n");
      return 1;
    };
    if(!names) {
      lprintf(stderr, "The enumeration format needs a list of names.\n");
      return 1;
    };
    if(precision <= 0 || precision >= MP_MAX_FIELD)
      precision = MP_MAX_FIELD-1;
    break;
  default:
    lprintf(stderr, "config_MonitorField: Unknown format.\n");
    return 1;
  };
  
  // Make a dynamic copy of the list of names.
  
  if(names && names[0]!='\0') {
    char *string;     // The dynamically allocated copy of the names[] string 
    char *src,*dst;   // Pointers into the source and destination strings 
    char *last;       // The last printable character copied into dst 
    
    // Allocate memory for the string copy.
    
    string = (char* )malloc(strlen(names)+1);
    if(!string) {
      lprintf(stderr, "config_MonitorField: Insufficient memory to copy names.\n");
      return 1;
    };
    
    // While copying the input string, remove unprintable characters
    // and collapse word separators into single commas.
    
    src = names;
    dst = string;
    last = NULL;
    while(1) {
      
      // Whenever a comma separator or the end of the string is
      // encountered, terminate the preceding enumerator name just
      // after the last printable character that was copied into the
      // destination string.
      
      if(*src==',' || *src=='\0') {
	do {
	  if(last)
	    dst = last + 1;
	  *dst++ = '\0';
	  nname++;
	  last = NULL;
	  if(*src) {
	    src++;		// Skip the comma 
	    while(isspace((int) *src)) // Skip spaces 
	      src++;
	  };
	} while(*src == ',');
      };
      
      // Copy printable characters into the output string.
      
      if(isprint((int) *src)) { // Copy printable characters 
	last = dst;
	*dst++ = *src++;
      } else if(isspace((int) *src)) { // Replace tabs etc with spaces 
	*dst++ = ' ';
	src++;
      } else if(*src) {		// Skip unprintable characters 
	src++;
      } else {			// End of string 
	break;
      };
    };
    
    // Allocate an array to point to each of the enumeration strings
    // in string[].
    
    name_array = (char** )malloc(sizeof(char *) * nname);
    if(!name_array) {
      free(string);
      lprintf(stderr,
	      "config_MonitorField: Insufficient memory to index enumerators.\n");
      return 1;
    };
    
    // Initialize each element of the array to point at the
    // corresponding enumerator name in string[]. Note that
    // del_MonitorField() free()s the whole of the above copy of the
    // original string by applying free to name_array[0]. If you
    // change the order of this array, be sure to modify
    // del_MonitorField() accordingly.
    
    name_array[0] = dst = string;
    for(i=1; i<nname; i++) {
      while(*dst++)
	;
      name_array[i] = dst;
    };
  };
  
  // Discard any legacy enumeration names.
  
  if(field->names) {
    if(field->names[0])
      free(field->names[0]);
    free(field->names);
  };
  
  // Record the new configuration.
  
  field->format    = format;
  field->flags[0]  = '\0';
  field->width     = width;
  field->precision = precision;
  field->misc      = misc;
  field->names     = name_array;
  field->nname     = nname;
  field->warn      = warn;
  field->dopage    = dopage;
  field->nframe    = nframe;
  field->counter   = 0;
  field->vmin      = vmin;
  field->vmax      = vmax;
  
  // Copy the string of printf flags. While doing this quietly reject
  // invalid and duplicate flags, so as to guarantee that the
  // statically sized field->flags[] array is long enough.
  
  {
    char *src, *dst;  // Pointers into the source and destination flag arrays 
    int seen_plus = 0; // True after a '+' has been encountered in flags[] 
    int seen_left = 0; // True after a '-' has been encountered in flags[] 
    int seen_space = 0;// True after a ' ' has been encountered in flags[] 
    int seen_hash = 0; // True after a '#' has been encountered in flags[] 
    int seen_zero = 0; // True after a '0' has been encountered in flags[] 
    for(src=flags, dst=field->flags; *src; src++) {
      switch(*src) {
      case '-':
	if(!seen_left++)
	  *dst++ = '-';
	break;
      case '+':
	if(!seen_plus++)
	  *dst++ = '+';
	break;
      case ' ':
	if(!seen_space++)
	  *dst++ = ' ';
	break;
      case '#':
	if(!seen_hash++)
	  *dst++ = '#';
	break;
      case '0':
	if(!seen_zero++)
	  *dst++ = '0';
	break;
      };
    };
    
    // Terminate the field->flags[] string.
    
    *dst = '\0';
  };
  
  return 0;
}

/*.......................................................................
 * Register a new callback function to use to output the value of a
 * register field.
 *
 * Input:
 *  field    MonitorField *   The field to assign the callback to.
 *  field_fn  MP_FIELD_FN(*)  The function to call to dispatch the new
 *                            stringized value (see monitor_viewer.h).
 *  user_data        void *   Arbitrary data to be passed to field_fn().
 *  del_fn      MP_DEL_FN(*)  The destructor function of user_data.
 *                            Send 0 if not required.
 * Output:
 *  return            int     0 - OK.
 *                            1 - Error.
 */
int mf_callback_fn(MonitorField *field, MP_FIELD_FN(*field_fn),
		   void *user_data, MP_DEL_FN(*del_fn))
{
  /*
   * Check the arguments.
   */
  if(!field) {
    lprintf(stderr, "mf_callback_fn: NULL argument.\n");
    return 1;
  };
  /*
   * Delete any legacy callback data.
   */
  if(field->user_data && field->del_fn)
    field->user_data = field->del_fn(field->user_data);
  /*
   * Install the new callback.
   */
  field->field_fn = field_fn;
  field->del_fn = del_fn;
  field->user_data = user_data;
  return 0;
}

/*.......................................................................
 * Delete a redundant MonitorField object.
 *
 * Input:
 *  field    MonitorField *  The object to be deleted.
 * Output:
 *  return   MonitorField *  The deleted object (Always NULL).
 */
static MonitorField *del_MonitorField(MonitorField *field)
{
  if(field) {
    MonitorViewer *view = field->page->view;
    /*
     * Delete the list of enumeration names.
     */
    if(field->names) {
      if(field->names[0])
	free(field->names[0]);
      free(field->names);
    };
    /*
     * Delete any callback data.
     */
    if(field->user_data && field->del_fn)
      field->user_data = field->del_fn(field->user_data);
    
    // Delete the register description
    
    if(field->desc != 0) {
      delete field->desc;
      field->desc = 0;
    }
    
    /*
     * Return the field to the freelist.
     */
    field = (MonitorField* )del_FreeListNode("del_MonitorField", 
					     view->field_mem, field);
  };
  return NULL;
}

/*.......................................................................
 * Add a register field to a page. Note that a subsequent call to
 * config_MonitorField() will be needed before the field will be
 * displayable.
 *
 * Input:
 *  page      MonitorPage *   The page that is to contain the field.
 * Output:
 *  return   MonitorField *   The field that was added, or NULL on error.
 */
MonitorField *add_MonitorField(MonitorPage *page)
{
  MonitorField *field;  /* The object to be added to the list */
  /*
   * Allocate the new field object.
   */
  field = new_MonitorField(page);
  if(!field)
    return NULL;
  /*
   * Append to the end of the list of fields in the parent page.
   */
  if(page->tail_field)
    page->tail_field->next = field;
  else
    page->fields = field;
  page->tail_field = field;
  return field;
}

/*.......................................................................
 * Remove a given field from its parent page and delete it.
 *
 * Input:
 *  field  MonitorField *  The field to be removed.
 * Output:
 *  return MonitorField *  The removed and deleted field (ie. NULL).
 */
MonitorField *rem_MonitorField(MonitorField *field)
{
  if(field) {
    MonitorPage *page = field->page;
    MonitorField *node;         /* The node being compared against 'field' */
    MonitorField *prev = NULL;  /* The node that precedes 'node' */
    for(node=page->fields; node && node != field; prev=node,node=node->next)
      ;
    if(!node) {
      lprintf(stderr, "rem_MonitorField: Field not found.\n");
      return NULL;
    };
    /*
     * Remove the field from the list by linking around it.
     */
    if(prev)
      prev->next = field->next;
    else
      page->fields = field->next;
    /*
     * If the field was at the end of the list, make the tail point
     * at the previous node in the list.
     */
    if(field == page->tail_field)
      page->tail_field = prev;
    /*
     * Delete the field.
     */
    field = del_MonitorField(field);
    /*
     * Mark the viewer as needing to have its register selection updated.
     */
    flag_MonitorViewer(page->view, 0);
  };
  return NULL;
}

/*.......................................................................
 * Find a monitor field of a given page via its unique field
 * identification tag.
 *
 * Input:
 *  page    MonitorPage *  The page that contains the field.
 *  tag        unsigned    The tag to identify the field by.
 * Output:
 *  return MonitorField *  The located field, or NULL if not found.
 */
MonitorField *find_MonitorField(MonitorPage *page, unsigned tag)
{
  MonitorField *field;  /* The field being checked */
  /*
   * Check arguments.
   */
  if(!page) {
    lprintf(stderr, "find_MonitorField: NULL page argument.\n");
    return NULL;
  };
  for(field=page->fields; field; field=field->next) {
    if(field->tag == tag)
      return field;
  };
  lprintf(stderr, "find_MonitorField: Field not found.\n");
  return NULL;
}

/*.......................................................................
 * Create a new object that records the configuration of a page.
 *
 * Input:
 *  view   MonitorViewer *  The host viewer.
 * Output:
 *  return   MonitorPage *  The new object, or NULL on error.
 */
static MonitorPage *new_MonitorPage(MonitorViewer *view)
{
  MonitorPage *page;   /* The object to be returned */
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "new_MonitorPage: NULL view argument.\n");
    return NULL;
  };
  /*
   * Allocate the container.
   */
  page = (MonitorPage* )new_FreeListNode("new_MonitorPage", view->page_mem);
  if(!page)
    return NULL;
  /*
   * Before attempting any operation that might fail, initialize the
   * container at least up to the point at which it is safe to pass
   * it to del_MonitorTrace().
   */
  page->tag            = view->next_page_tag++;
  page->view           = view;
  page->fields         = NULL;
  page->tail_field     = NULL;
  page->next           = NULL;
  page->frozen         = 0;
  page->next_field_tag = 1;

  return page;
}

/*.......................................................................
 * Delete a redundant MonitorPage object.
 *
 * Input:
 *  page     MonitorPage *  The object to be deleted.
 * Output:
 *  return   MonitorPage *  The deleted object (Always NULL).
 */
static MonitorPage *del_MonitorPage(MonitorPage *page)
{
  if(page) {
    MonitorViewer *view = page->view;
    /*
     * Delete the list of fields.
     */
    while(page->fields)
      rem_MonitorField(page->fields);
    /*
     * Return the container to the free-list.
     */
    page = (MonitorPage* )del_FreeListNode("del_MonitorPage", 
					   view->page_mem, page);
  };
  return NULL;
}

/*.......................................................................
 * Add a page to the viewer.
 *
 * Input:
 *  view    MonitorViewer *  The viewer to add the page to.
 * Output:
 *  return    MonitorPage *  The page that was added, or NULL on error.
 */
MonitorPage *add_MonitorPage(MonitorViewer *view)
{
  MonitorPage *page;  /* The object to be added to the list */
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "add_MonitorPage: NULL argument.\n");
    return NULL;
  };
  /*
   * Allocate the new page object.
   */
  page = new_MonitorPage(view);
  if(!page)
    return NULL;
  /*
   * Append to the end of the list of pages in the viewer.
   */
  if(view->tail_page)
    view->tail_page->next = page;
  else
    view->pages = page;
  view->tail_page = page;
  /*
   * Mark the viewer as needing to have its register selection updated.
   */
  flag_MonitorViewer(view, 0);
  return page;
}

/*.......................................................................
 * Remove a given page from a viewer and delete it.
 *
 * Input:
 *  page   MonitorPage *  The page to be removed.
 * Output:
 *  return MonitorPage *  The removed and deleted page (ie. NULL).
 */
MonitorPage *rem_MonitorPage(MonitorPage *page)
{
  if(page) {
    MonitorViewer *view = page->view; /* The host viewer */
    MonitorPage *node;         /* The node being compared against 'page' */
    MonitorPage *prev = NULL;  /* The node that precedes 'node' */
    for(node=view->pages; node && node != page; prev=node,node=node->next)
      ;
    if(!node) {
      lprintf(stderr, "rem_MonitorPage: Page not found.\n");
      return NULL;
    };
    /*
     * Remove the page from the list by linking around it.
     */
    if(prev)
      prev->next = page->next;
    else
      view->pages = page->next;
    /*
     * If the page was at the end of the list, make the tail point
     * at the previous node in the list.
     */
    if(page == view->tail_page)
      view->tail_page = prev;
    /*
     * Delete the page.
     */
    page = del_MonitorPage(page);
    /*
     * Mark the viewer as needing to have its register selection updated.
     */
    flag_MonitorViewer(view, 0);
  };
  return NULL;
}

/*.......................................................................
 * Delete the fields of a given page along with their configurations.
 *
 * Input:
 *  page  MonitorPage *  The page to be modified.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
int rem_MonitorPage_fields(MonitorPage *page)
{
  /*
   * Check arguments.
   */
  if(!page) {
    lprintf(stderr, "rem_MonitorPage_fields: NULL argument.\n");
    return 1;
  };
  /*
   * Delete the fields that the page contained.
   */
  while(page->fields)
    rem_MonitorField(page->fields);
  /*
   * Mark the viewer as needing to have its register selection updated.
   */
  flag_MonitorViewer(page->view, 0);
  return 0;
}

/*.......................................................................
 * Delete the current pages and their configurations.
 *
 * Input:
 *  view  MonitorViewer *  The monitor viewer resource object.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
int rem_MonitorViewer_pages(MonitorViewer *view)
{
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "rem_MonitorViewer_pages: NULL argument.\n");
    return 1;
  };
  /*
   * Delete the current list of pages.
   */
  while(view->pages)
    rem_MonitorPage(view->pages);
  /*
   * Mark the monitor viewer as being inconsistent with the
   * current register selection.
   */
  flag_MonitorViewer(view, 1);
  return 0;
}

/*.......................................................................
 * Clear the register specifications of a given page.
 *
 * Input:
 *  page   MonitorPage *  The page to be removed.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
static int clr_MonitorPage_regs(MonitorPage *page)
{
  MonitorField *field;   // The field being cleared 
  
  // Clear the register-map specifications of each field.
  
  for(field=page->fields; field; field=field->next) {
    if(field->desc != 0)
      field->desc->reset();
    field->regname_invalid = 0;
  };
  
  return 0;
}

/*.......................................................................
 * Find a monitor page of a given page via its unique page
 * identification tag.
 *
 * Input:
 *  view  MonitorViewer *  The parent viewer.
 *  tag        unsigned    The tag to identify the page by.
 * Output:
 *  return  MonitorPage *  The located page, or NULL if not found.
 */
MonitorPage *find_MonitorPage(MonitorViewer *view, unsigned tag)
{
  MonitorPage *page;  // The page being checked 
  
  // Check arguments.
  
  if(!view) {
    lprintf(stderr, "find_MonitorPage: NULL view argument.\n");
    return NULL;
  };
  
  for(page=view->pages; page; page=page->next) {
    if(page->tag == tag)
      return page;
  };
  
  lprintf(stderr, "find_MonitorPage: Page not found.\n");
  return NULL;
}

/*.......................................................................
 * Redraw a given page of register fields from the latest register frame
 * to have been received.
 *
 * Input:
 *  page   MonitorPage *  The page to update.
 *  cal     RegCalData *  The newly read frame of calibrated registers.
 */
static void draw_MonitorPage(MonitorPage *page, 
			     gcp::util::RegCal::RegCalData* calData, 
			     int reset)
{
  MonitorViewer *view = page->view; // The parent viewer of the page 
  MonitorField *field;              // The field being drawn 
  MpFormatBuf *fmt;                 // The formatting buffer 
  int warn;                         // True if the value is outside of 
                                    //  the expected field range. 
  int dopage;                       // True if we should activate the pager 
  
  // Ignore this call if the page is not ready to be drawn to.
  
  if(page->frozen)
    return;
  
  // Get the formatting buffer.
  
  fmt = &view->fmt;
  
  // Extract each field from the latest register frame.
  
  for(field=page->fields; field; field = field->next) {
    if(field->field_fn) {
      
      try {
	if(field->desc->iSlot() >= 0) {
	  
	  // Prepare the formatting stream.
	  
	  clr_StringOutputStream(fmt->stream);
	  
	  // Unpack a string from an array of register elements?
	  
	  if(field->format == MF_STRING) {

	    try {
	      calData->getCalString(field->desc, fmt->string, 
				    field->precision+1);
	    } catch(...) {
	      fmt->string[0] = '\0';
	    };
	    
	    if(output_string(fmt->stream, ET_ALL, field->flags, field->width,
			     field->precision, -1, fmt->string)) {
	      fmt->string[0] = '\0';
	    }
	    warn = 0;
	  } else {
	    
	    double value;
	    gcp::util::Complex<float> cVal;
	    
	    // Get the value to be formatted.
	    
	    switch(field->desc->aspect()) {
	    default:
	    case REG_PLAIN:
	      if(field->desc->block()->isUtc()){
		gcp::util::RegDate date;
		calData->getCalDate(field->desc, date.data());
		value = date.mjd();
	      } else if(field->desc->block()->isComplex()) {
		calData->getCalComplexFloat(field->desc, cVal.data());
		value = cVal.amp();
	      } else
		calData->getCalDouble(field->desc, &value);
	      break;
	    case REG_REAL:
	      {
		gcp::util::Complex<float> cVal(0.0,0.0);
		calData->getCalComplexFloat(field->desc, cVal.data());
		value = cVal.real();
	      } 
	      break;
	    case REG_IMAG:
	      {
		gcp::util::Complex<float> cVal(0.0,0.0);
		calData->getCalComplexFloat(field->desc, cVal.data());
		value = cVal.imag();
	      } 
	      break;
	    case REG_AMP:		// The modulus of a complex register 
	      {
		gcp::util::Complex<float> cVal(0.0,0.0);
		calData->getCalComplexFloat(field->desc, cVal.data());
		value = cVal.amp();
	      } 
	      break;
	    case REG_PHASE:	// The modulus of a complex register 
	      {
		gcp::util::Complex<float> cVal(0.0,0.0);
		calData->getCalComplexFloat(field->desc, cVal.data());
		value = cVal.phaseInDegrees();
	      } 
	      break;
	    case REG_DATE:	// The Modified Julian Date of a utc pair 
	      {
		gcp::util::RegDate date;
		calData->getCalDate(field->desc, date.data());
		value = date.mjd();
	      }
	      break;
	    case REG_TIME:	// The time of day of a utc pair 
	      {
		gcp::util::RegDate date;
		calData->getCalDate(field->desc, date.data());
		value = date.timeInHours();
	      }
	      break;
	    };
	    
	    // Format the value in the requested manner into
	    // fmt->string, by way of the wrapper stream fmt->stream.
	    
	    switch(field->format) {
	    case MF_FIXED_POINT:	// Fixed point decimal (%f) 
	      output_double(fmt->stream, field->flags, field->width,
			    field->precision, 'f', value);
	      warn = (value < field->vmin || value > field->vmax);
	      break;
	    case MF_SCIENTIFIC:	// Fixed point exponential (%e) 
	      output_double(fmt->stream, field->flags, field->width,
			    field->precision, 'e', value);
	      warn = (value < field->vmin || value > field->vmax);
	      break;
	    case MF_FLOATING:	// Floating point (%g) 
	      output_double(fmt->stream, field->flags, field->width,
			    field->precision, 'g', value);
	      warn = (value < field->vmin || value > field->vmax);
	      break;
	    case MF_SEXAGESIMAL:	// Degrees/hours,minutes,seconds format 
	      output_sexagesimal(fmt->stream, field->flags, field->width,
				 field->misc, field->precision, value);
	      warn = (value < field->vmin || value > field->vmax);
	      break;
	    case MF_INTEGER:	// Base 10 integer 
	    case MF_HEX:		// Base 16 integer 
	    case MF_OCTAL:	// Base 8 integer 
	    case MF_BINARY:	// Base 2 integer 
	      
	      // Get the display base.
	      
	      {
		OutputBase base;
		switch(field->format) {
		default:
		case MF_INTEGER:
		  base = OUT_DECIMAL;
		  break;
		case MF_HEX:
		  base = OUT_HEX;
		  break;
		case MF_OCTAL:
		  base = OUT_OCTAL;
		  break;
		case MF_BINARY:
		  base = OUT_BINARY;
		  break;
		};
		
		// Display the value as a signed long if it is negative.
		// Otherwise display it as an unsigned long.
		
		if(value < 0) {
		  int lval = (int) value;
		  output_int(fmt->stream, base, field->flags, field->width,
			      field->precision, lval);
		  warn = (lval < field->vmin ||
			  lval > field->vmax);
		} else {
		  int ulval = (unsigned int) value;
		  output_uint(fmt->stream, base, field->flags, field->width,
			       field->precision, ulval);
		  warn = (ulval < field->vmin ||
			  ulval > field->vmax);
		};
	      };
	      break;
	    case MF_COMPLEX_FIXED:
	      {
		// Output the real part
		
		output_double(fmt->stream, field->flags, field->width,
			      field->precision, 'f', 
			      field->desc->block()->isComplex() ? 
			      cVal.real() : value);
		
		// Output a separator
		
		std::ostringstream os;
		os << " " << (cVal.imag() < 0.0 ? "-" : "+") << " i ";
		unsigned length = strlen(os.str().c_str());
		
		if(output_string(fmt->stream, ET_ALL, field->flags, length,
				 length, -1, (char*)os.str().c_str())) 
		  fmt->string[0] = '\0';
		
		// And output the imaginary part
		
		output_double(fmt->stream, field->flags, field->width,
			      field->precision, 'f', 
			      field->desc->block()->isComplex() ? 
			      fabs(cVal.imag()) : 0.0);
		warn = 0;
	      }
	      break;
	    case MF_DATE:
	      output_utc(fmt->stream, field->flags, field->width, -3, value);
	      warn = (value < field->vmin || value > field->vmax);
	      break;
	    case MF_STRING:
	      warn = 0;
	      break;                // Handled above 
	    case MF_BIT:
	      {
		unsigned int bit = ((unsigned int) value) >> field->misc & 1U;
		char *string = (char* )((bit < (unsigned int)field->nname) ? 
					field->names[bit] : (bit ? "1":"0"));
		if(output_string(fmt->stream, ET_ALL, field->flags, field->width,
				 field->precision, -1, string)) {
		  fmt->string[0] = '\0';
		};
		warn = (bit < field->vmin || bit > field->vmax);
	      };
	      break;
	    case MF_ENUM:
	      {
		int ival = (int) value;
		if(ival >= 0 && ival < field->nname) {
		  if(output_string(fmt->stream, ET_ALL, field->flags,
				   field->width, field->precision, -1,
				   field->names[ival])) {
		    fmt->string[0] = '\0';
		  }
		} else {
		  output_int(fmt->stream, OUT_DECIMAL, field->flags,
			      field->width, 0, (int) ival);
		};
		warn = (ival < field->vmin || ival > field->vmax);
	      };
	      break;
	    case MF_BOOL:
	      {
		
		bool bval = (bool) value;
		
		if(output_string(fmt->stream, ET_NONE, field->flags,
				 bval ? 4 : 5, bval ? 4 : 5, -1,
				 bval ? (char*)"true" : (char*)"false")) 
		  fmt->string[0] = '\0';
		
		warn = (value < field->vmin || value > field->vmax);
	      }
	      break;
	    default:
	      fmt->string[0] = '\0';
	      warn = 0;
	      break;
	    };
	  };
	} else {
	  strcpy(fmt->string, "-");
	  warn = 1;
	};
	
	// If the register was out of range, increment the out-of-range
	// counter.  If not, reset the counter
	
	if(warn && field->dopage)
	  field->counter++;
	else
	  field->counter = 0;
	
	// Set the pager flag if we are paging, and the register has
	// been out of range for nframe frames
	
	dopage = field->dopage && field->counter==field->nframe;
	
	// Only set the warn flag if we are checking for it.
	
	warn = warn && field->warn;
	
	// Dispatch the string to the application.
	
	field->field_fn(field->user_data, page, fmt->string, warn, dopage, reset);
      } catch(...) {
	// Just skip this field.  The field will attempt to redraw
	// even while the user is inputting a field, so we must trap
	// if the user has made an error inputting a register
      }
    };
  };
  return;
}

/*.......................................................................
 * Temporarily stop updating the fields of a given monitor page.
 *
 * Input:
 *  page   MonitorPage *  The page to be frozen.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
int freeze_MonitorPage(MonitorPage *page)
{
  if(!page) {
    lprintf(stderr, "freeze_MonitorPage: NULL argument.\n");
    return 1;
  };
  page->frozen = 1;
  return 0;
}

/*.......................................................................
 * Unfreeze a frozen monitor page.
 *
 * Input:
 *  page   MonitorPage *  The page that is to be unfrozen.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
int unfreeze_MonitorPage(MonitorPage *page)
{
  if(!page) {
    lprintf(stderr, "unfreeze_MonitorPage: NULL argument.\n");
    return 1;
  };
  if(page->frozen) {
    page->frozen = 0;
  };
  return 0;
}

/*.......................................................................
 * Add the registers of a monitoring page to the set of registers that
 * are to be monitored.
 *
 * Input:
 *  page   MonitorPage *  The page to accomodate in the register set.
 *  regmap      RegMap *  The register map to use when looking up
 *                        registers.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
static int add_MonitorPage_regs(MonitorPage *page, ArrayMap *arraymap)
{
  MonitorField *field;  // An output field managed by the page 
  
  // Get the resource object of the viewer.
  
  MonitorViewer *view = page->view;
  
  // Get the viewer input stream for use in parsing register
  // specifications.
  
  InputStream *stream = view->input; // The stream to use to parse regname[] 
  
  // Include the register element of each field.
  
  for(field=page->fields; field; field=field->next) {
    RegInputMode mode;    // The register parsing mode 
    int extend;           // True to allow extended register notation 
    
    // If we have already reported that the register doesn't exist in
    // the current register map, don't try to look it up again.
    
    if(field->regname_invalid) {
      continue;
    }
    
    // Delete any previous register specification.
    
    field->desc->reset();
    
    // Determine how the register specification should be parsed.
    
    switch(field->format) {
    case MF_FIXED_POINT:
    case MF_SCIENTIFIC:
    case MF_FLOATING:
    case MF_SEXAGESIMAL:
    case MF_INTEGER:
    case MF_HEX:
    case MF_OCTAL:
    case MF_BINARY:
    case MF_DATE:
    case MF_BIT:
    case MF_ENUM:
    case MF_BOOL:
    case MF_COMPLEX_FIXED:
      mode = REG_INPUT_ELEMENT;
      extend = 1;
      break;
    case MF_STRING:
      //      mode = REG_INPUT_BLOCK;
      mode = REG_INPUT_RANGE;
      extend = 0;
      break;
    default:
      lprintf(stderr, "add_MonitorPage_regs: Missing switch case.\n");
      return 1;
    };
    
    // Parse the register name.
    
    if(open_StringInputStream(stream, 0, field->regname)) {
      return 1;
    }
    
    gcp::util::RegParser parser(view->ms->archivedOnly_);
    
    try {
      *field->desc = parser.inputReg(stream, true, mode, extend, arraymap);
    } catch(gcp::util::Exception& err) {
      lprintf(stderr, err.what());
    };
    
    if(parser.validity() == REG_VALID) {
      
      if(input_skip_white(stream, 1, 0)) {
	field->desc->reset();
      }	else if(stream->nextc != EOF) {
	lprintf(stderr, "Unexpected trailing characters in register name: %s\n",
		field->regname);
	field->desc->reset();
      } else if(field->desc->iSlot() < 0) {
	lprintf(stderr, "%s is not an archived register.\n", field->regname);
      };
    };
    
    close_InputStream(stream);
    
    // If the register doesn't exist, skip to the next field.
    
    if(field->desc->iSlot() < 0) {
      field->regname_invalid = 1;
      continue;
    };
    
    // Add the selected registers to the set that is to be provided by
    // the monitor stream.
    
    try {
      ms_prep_RegSet(view->ms)->addRegister(field->desc);
    } catch(...) {
      return 1;
    }
  };

  return 0;
}

static MonitorTrace* new_MonitorTrace(MonitorGraph* graph)
{
  MonitorViewer* view=0;   /* The resource object of the monitor viewer */
  MonitorTrace*  trace=0;  /* The object to be returned */
  
  // Check arguments.

  if(!graph) {
    lprintf(stderr, "new_MonitorTrace: NULL argument.\n");
    return NULL;
  };
  
  // Get the monitor viewer resource object.

  view = graph->plot->view;
  
  // Allocate the container.

  trace = (MonitorTrace* )new_FreeListNode("new_MonitorTrace", view->trace_mem);

  if(!trace)
    return NULL;

  // Before attempting any operation that might fail, initialize the
  // container at least up to the point at which it is safe to pass it
  // to del_MonitorTrace().

  trace->graph           = graph;
  trace->regname[0]      = '\0';
  trace->regname_invalid = 0;
  trace->next            = NULL;

  // Intialize the power spectrum container now

  trace->powSpec = new PowSpecManager();

  trace->descs = 0;
  trace->descs = new std::vector<gcp::util::RegDescription>;
  
  if(trace->descs == 0)
    return del_MonitorTrace(trace);
  
  return trace;
}

/*.......................................................................
 * Delete a redundant MonitorTrace object.
 *
 * Input:
 *  trace    MonitorTrace *  The object to be deleted.
 * Output:
 *  return   MonitorTrace *  The deleted object (Always NULL).
 */
static MonitorTrace *del_MonitorTrace(MonitorTrace *trace)
{
  if(trace) {
    
    if(trace->descs != 0) {
      delete trace->descs;
      trace->descs = 0;
    }

    if(trace->powSpec != 0) {
      delete trace->powSpec;
      trace->powSpec = 0;
    }

    MonitorViewer* view = trace->graph->plot->view;
    trace = (MonitorTrace* )del_FreeListNode("del_MonitorTrace", 
					     view->trace_mem, trace);
  };
  
  return NULL;
}

/*.......................................................................
 * Create a new object that records the configuration of a graph.
 *
 * Input:
 *  plot     MonitorPlot *  The plot that the trace belongs to.
 * Output:
 *  return  MonitorGraph *  The new object, or NULL on error.
 */
static MonitorGraph *new_MonitorGraph(MonitorPlot *plot)
{
  MonitorViewer *view;     /* The resource object of the monitor viewer */
  MonitorGraph *graph;   /* The object to be returned */
  
  // Check arguments.

  if(!plot) {
    lprintf(stderr, "new_MonitorGraph: NULL plot argument.\n");
    return NULL;
  };
  
  // Get the monitor viewer resource object.

  view = plot->view;
  
  // Allocate the container.

  graph = (MonitorGraph* )new_FreeListNode("new_MonitorGraph", view->graph_mem);

  if(!graph)
    return NULL;
  
  // Increment the tally of the number of active graphs.

  plot->ngraph++;
  
  // Before attempting any operation that might fail, initialize the
  // container at least up to the point at which it is safe to pass it
  // to del_MonitorTrace().

  memset(graph->ylab, '\0', AXLAB_LEN);
  graph->traces   = NULL;
  graph->wya      = 0.0;
  graph->wyb      = 1.0;
  graph->bits     = 0;
  graph->track    = false;
  graph->range    = 1.0;
  graph->tag      = plot->next_graph_tag++;
  graph->update_needed = 1;
  graph->started  = 0;
  graph->vxa      = graph->vxb = 0.0f;
  graph->vya      = graph->vyb = 0.0f;
  graph->plot     = plot;
  graph->next     = NULL;

  graph->powSpec.updateNeeded = false;
  graph->powSpec.ymin = 0.1;
  graph->powSpec.ymax = 1.0;
  graph->powSpec.linAxis = 1;

  graph->powSpec.pkIdent    = false;
  graph->powSpec.fullXrange = true;
  graph->powSpec.npk        =  0;
  graph->powSpec.pkXmin     =  0;
  graph->powSpec.pkXmax     =  0;

  return graph;
}

/*.......................................................................
 * Delete a redundant MonitorGraph object.
 *
 * Input:
 *  graph    MonitorGraph *  The object to be deleted.
 * Output:
 *  return   MonitorGraph *  The deleted object (Always NULL).
 */
static MonitorGraph *del_MonitorGraph(MonitorGraph *graph)
{
  if(graph) {
    MonitorPlot *plot = graph->plot;  /* The parent plot */
    MonitorViewer *view = plot->view; /* The parent viewer */
    MonitorTrace *next;               /* The next trace to be deleted */
    /*
     * Delete the list of traces.
     */
    next = graph->traces;
    while(next) {
      MonitorTrace *trace = next;
      next = trace->next;
      trace = del_MonitorTrace(trace);
    };
    /*
     * Return the container to the free-list.
     */
    graph = (MonitorGraph* )del_FreeListNode("del_MonitorGraph", 
					     view->graph_mem, graph);
    /*
     * Decrement the tally of the number of active graphs in the plot.
     */
    plot->ngraph--;
  };
  return NULL;
}

/*.......................................................................
 * Add a graph to a plot.
 *
 * Input:
 *  plot    MonitorPlot *  The plot to add the graph to.
 * Output:
 *  return MonitorGraph *  The graph that was added, or NULL on error.
 */
MonitorGraph *add_MonitorGraph(MonitorPlot *plot)
{
  MonitorGraph *graph;  /* The object to be added to the list */
  
  // Check arguments.

  if(!plot) {
    lprintf(stderr, "add_MonitorGraph: NULL argument.\n");
    return NULL;
  };
  
  // Allocate the new graph object.

  graph = new_MonitorGraph(plot);
  if(!graph)
    return NULL;
  
  // Append to the end of the list of graphs in the parent plot.

  if(plot->tail_graph)
    plot->tail_graph->next = graph;
  else
    plot->graphs = graph;

  plot->tail_graph = graph;
  
  // Mark the parent plot as needing to be redrawn, and mark the
  // viewer as needing to have its register selection updated.

  flag_MonitorPlot(plot);
  flag_MonitorViewer(plot->view, 0);

  return graph;
}

/*.......................................................................
 * Remove a given graph from its parent plot and delete it.
 *
 * Input:
 *  graph  MonitorGraph *  The graph to be removed.
 * Output:
 *  return MonitorGraph *  The removed and deleted graph (ie. NULL).
 */
MonitorGraph *rem_MonitorGraph(MonitorGraph *graph)
{
  if(graph) {
    MonitorPlot *plot = graph->plot; /* The plot to which the graph belongs */
    MonitorGraph *node;         /* The node being compared against 'graph' */
    MonitorGraph *prev = NULL;  /* The node that precedes 'node' */
    for(node=plot->graphs; node && node != graph; prev=node,node=node->next)
      ;
    if(!node) {
      lprintf(stderr, "rem_MonitorGraph: Graph not found.\n");
      return NULL;
    };
    /*
     * Remove the graph from the list by linking around it.
     */
    if(prev)
      prev->next = graph->next;
    else
      plot->graphs = graph->next;
    /*
     * If the graph was at the end of the list, make the tail point
     * at the previous node in the list.
     */
    if(graph == plot->tail_graph)
      plot->tail_graph = prev;
    /*
     * Delete the graph.
     */
    graph = del_MonitorGraph(graph);
    /*
     * Mark the parent plot as needing to be redrawn, and mark the viewer
     * as needing to have its register selection updated.
     */
    flag_MonitorPlot(plot);
    flag_MonitorViewer(plot->view, 0);
  };
  return NULL;
}

/*.......................................................................
 * Reconfigure an existing graph.
 *
 * Input:
 *  graph       MonitorGraph *  The graph to be configured.
 *  ybot              double    The value of the bottom edge of the Y-axis.
 *  ytop              double    The value of the top edge of the Y-axis.
 *  ylabel              char *  The label to plot along the y-axis.
 *  xreg                char *  The space-separated list of registers to
 *                              be plotted on the graph.
 *  bits       unsigned int    A mask of bits to be displayed individually,
 *                              or zero to display register values.
 * Output:
 *  return               int     0 - OK.
 *                               1 - Error.
 */
int config_MonitorGraph(MonitorGraph *graph, 
			double ybot,  double ytop,
			char *ylabel, char *yregs, 
			unsigned int bits, 
			bool track,
			bool vectorAverage,
			bool linAxis,
			unsigned apodType)
{
  std::vector<gcp::util::RegDescription> xdescs; // A vector of x-axis
						 // register
						 // specifications
  std::vector<gcp::util::RegDescription> ydescs; // One of the y-axis
						 // register
						 // specifications
  
  // Check arguments.
  
  if(!graph) {
    lprintf(stderr, "config_MonitorGraph: NULL argument.\n");
    return 1;
  };
  
  // If this is not a power spectrum plot, read the x-axis
  // specification.  If it is, there is no x-axies (x-axis will be
  // frequency derived from the FFT of the y-axes)

  // Check the syntax of the register specifications.
  // 
  // First Read the parent plot's x-register specification

  if(graph->plot->checkXregisters(xdescs))
    return 1;

  // Now read the y-register specifications and check for consistency with
  // the plot's x-axis

  if(graph->plot->checkYregisters(yregs, xdescs, ydescs))
    return 1;

  // If all checks out, re-iterate through the specified registers and
  // add a trace for each distinct one

  if(graph->addTraces(yregs))
    return 1;

  if(graph->plot->type == PLOT_POWSPEC) {

    // If we are tracking the spectrum, ignore the set limits

    if(track) {

      ybot = graph->powSpec.ymin;
      ytop = graph->powSpec.ymax;

      // Else check that the limits are > 0.0 is this is a log plot

    } else if(!linAxis) {
      ybot = (ybot > 0.0) ? ybot : graph->powSpec.ymin;
      ytop = (ytop > 0.0) ? ytop : graph->powSpec.ymax;
    }
  }

  // Sort the limits so that ytop > ybot, and widen the range if
  // necessary to accommodate precision restrictions.
  
  vet_axis_limits(&ybot, &ytop);
  
  if(graph->plot->type == PLOT_POWSPEC) {
    double yrange;
    
    if(graph->powSpec.linAxis) {
      yrange = ytop - ybot;
      
      if(yrange < EPS)
	  yrange = EPS;
      
      ybot -= 0.1*yrange;
      ytop += 0.1*yrange;
      
    } else {
      double lya = ybot > 0.0 ? log10(ybot) : 0.0;
      double lyb = ytop > 0.0 ? log10(ytop) : 0.0;
      
      yrange = lyb - lya;
      
      if(yrange < EPS)
	yrange = EPS;
      
      ybot = pow(10.0, lya - 0.1*yrange);
      ytop = pow(10.0, lyb + 0.1*yrange);
    }
  }

  // Install the new range.
  
  graph->wya   = ybot;
  graph->wyb   = ytop;

  graph->bits  = bits;
  graph->track = track;
  graph->range = (ytop-ybot)/2;

  // Record the x-axis label.
  
  strncpy(graph->ylab, ylabel ? ylabel : "", AXLAB_LEN);
  graph->ylab[AXLAB_LEN] = '\0';

  // Set the type of averaging

  graph->setVectorAverage(vectorAverage);

  // Set the type of apodization

  graph->setApodizationType((gcp::util::Dft::Apodization) apodType);

  // If this call changed the axis type for a power spectrum graph,
  // just change the axis type -- don't update the graph until new
  // data have arrived

  if(graph->plot->type == PLOT_POWSPEC && (linAxis != graph->powSpec.linAxis)) {
    graph->setLinearAxis(linAxis);
  } else {
    flag_MonitorGraph(graph);
  }

  return 0;
}

/*.......................................................................
 * Reconfigure an existing graph.
 *
 * Input:
 *  graph       MonitorGraph *  The graph to be configured.
 *  ybot              double    The value of the bottom edge of the Y-axis.
 *  ytop              double    The value of the top edge of the Y-axis.
 *  ylabel              char *  The label to plot along the y-axis.
 *  xreg                char *  The space-separated list of registers to
 *                              be plotted on the graph.
 *  bits       unsigned long    A mask of bits to be displayed individually,
 *                              or zero to display register values.
 * Output:
 *  return               int     0 - OK.
 *                               1 - Error.
 */
int int_MonitorGraph(MonitorGraph *graph, bool integrate)
{
  // Check arguments.
  
  if(!graph) {
    lprintf(stderr, "int_MonitorGraph: NULL argument.\n");
    return 1;
  };
  
  graph->integrate(integrate);

  return 0;
}

void MonitorGraph::integrate(bool doInt)
{
  switch (plot->type) {
  case PLOT_NORM:
    integrateNorm(doInt);
    break;
  case PLOT_POWSPEC:
    integratePowSpec(doInt);
    break;
  default:
    ThrowError("Unhandled plot type");
    break;
  }
}

void MonitorGraph::integrateNorm(bool doInt)
{
  // Get the resource object of the viewer.
  
  MonitorViewer *view = plot->view; 
  MonitorBuffMap* map = view->mb->map;

  // Iterate over all traces of this graph
	
  for(MonitorTrace* trace=traces; trace; trace=trace->next) {
    
    // Iterate over all registers in this trace
    
    for(unsigned iYreg=0; iYreg < trace->descs->size(); iYreg++) {
      
      gcp::util::RegDescription& desc = trace->descs->at(iYreg);
      
      // Iterate over all requested elements of this register,
      // toggling the integration status
      
      for(desc.begin(); !desc.isEnd(); ++desc) {
	int offset = map->offset[desc.currentSlot()].a[desc.aspect()];

	map->regs[offset].nInt = 0;
	map->regs[offset].lastComplexVal->initialize();
	map->regs[offset].lastDate->initialize();

	// Toggle the integration status

	if(doInt)
	  map->regs[offset].intType = desc.block()->flags_;
	else
	  map->regs[offset].intType = REG_NONE;
      }
    }
  }
}

void MonitorGraph::setApodizationType(gcp::util::Dft::Apodization apodType)
{
  powSpec.apodType = apodType;

  // Get the resource object of the viewer.
  
  MonitorViewer *view = plot->view; 

  // Iterate over all traces of this graph
	
  for(MonitorTrace* trace=traces; trace; trace=trace->next) {
    
    // Iterate over all spectra managed by this trace
    
    trace->powSpec->setApodizationType(apodType);
  }
}

void MonitorGraph::integratePowSpec(bool doInt)
{
  // Get the resource object of the viewer.
  
  MonitorViewer *view = plot->view; 

  // Iterate over all traces of this graph
	
  for(MonitorTrace* trace=traces; trace; trace=trace->next) {
    
    // Iterate over all spectra managed by this trace
    
    for(unsigned iSpec=0; iSpec < trace->powSpec->spectra.size(); iSpec++) {
      trace->powSpec->spectra[iSpec].dft->setAverage(doInt);
    }
  }
}

void MonitorGraph::setVectorAverage(bool vectorAverage)
{
  // This is a no-op for other types of plots than power spectra

  if(plot->type != PLOT_POWSPEC)
    return;

  // Get the resource object of the viewer.
  
  MonitorViewer *view = plot->view; 

  // Iterate over all traces of this graph
	
  for(MonitorTrace* trace=traces; trace; trace=trace->next) {
    
    // Iterate over all spectra managed by this trace
    
    for(unsigned iSpec=0; iSpec < trace->powSpec->spectra.size(); iSpec++) {
      trace->powSpec->spectra[iSpec].dft->setVectorAverage(vectorAverage);
    }

  }

}

void MonitorGraph::setLinearAxis(bool linAxis)
{
  // This is a no-op for other types of plots than power spectra

  if(plot->type != PLOT_POWSPEC)
    return;

  // Store the axis type

  powSpec.linAxis = linAxis;
}

/**.......................................................................
 * Check that a y-axis specification makes sense, given the x-axes
 */
static void checkXaxisValidity(char* xRegName, 
			       char* yRegName,
			       MonitorBuffMap* map)
{
  gcp::util::RegParser parser;

  std::vector<gcp::util::RegDescription> xdescs;
  std::vector<gcp::util::RegDescription> ydescs;

  xdescs = parser.inputRegs(xRegName, map->arraymap, REG_INPUT_RANGE, 
			    true, true);

  ydescs = parser.inputRegs(yRegName, map->arraymap, REG_INPUT_RANGE, 
			    true, true);

  // If the x-axis is a multi-axis register
  
  if(xdescs.size() > 1) {
    
    // For multiple x-axes, there must be a one-to-one
    // correspondence between the number of x and y registers
    
    if(ydescs.size() != xdescs.size())
      ThrowSimpleError(yRegName << " does not specify the same number "
		       "of registers as the x-axis specification (" 
		       << xRegName << ")");
    
    // If the x-axes are additionally multi-element, then each y-axis
    // register must also have the same number of elements
    
    unsigned nEl = xdescs.at(0).nEl();
    
    if(nEl > 1)
      for(unsigned iReg=0; iReg < ydescs.size(); iReg++)
	if(ydescs[iReg].nEl() != nEl)
	  ThrowSimpleError(yRegName << " does not specify the same number"
			   "of elements as the x-axis specification ("
			   << xRegName << ")");
  }
}

/**.......................................................................
 * Check that a y-axis specification makes sense, given the x-axis
 */
static void checkXaxisValidity(char* xRegName, 
			       std::vector<gcp::util::RegDescription>& xdescs,
			       std::vector<gcp::util::RegDescription>& ydescs)
{
  // If the x-axis is a multi-axis register
  
  if(xdescs.size() > 1) {
    
    // For multiple x-axes, there must be a one-to-one
    // correspondence between the number of x and y registers
    
    if(ydescs.size() != xdescs.size())
      ThrowSimpleError("Each y-axis must specify the same number "
		       "of registers as the x-axis specification (" 
		       << xRegName << ")");
    
    // If the x-axes are additionally multi-element, then each y-axis
    // register must also have the same number of elements
    
    unsigned nEl = xdescs.at(0).nEl();
    
    if(nEl > 1)
      for(unsigned iReg=0; iReg < ydescs.size(); iReg++)
	if(ydescs[iReg].nEl() != nEl)
	  ThrowSimpleError("Your y-axis registers must specify the same number "
			   "of elements as the x-axis specification ("
			   << xRegName << ")");
  }
}

/*.......................................................................
 * Find a monitor graph of a given plot via its unique graph
 * identification tag.
 *
 * Input:
 *  plot    MonitorPlot *  The plot that contains the graph.
 *  tag        unsigned    The tag to identify the graph by.
 * Output:
 *  return MonitorGraph *  The located graph, or NULL if not found.
 */
MonitorGraph *find_MonitorGraph(MonitorPlot *plot, unsigned tag)
{
  // Check arguments.

  if(!plot) {
    lprintf(stderr, "find_MonitorGraph: NULL plot argument.\n");
    return NULL;
  };

  for(MonitorGraph* graph=plot->graphs; graph; graph=graph->next) {
    if(graph->tag == tag)
      return graph;
  };

  lprintf(stderr, "find_MonitorGraph: Graph not found.\n");

  return NULL;
}

/*.......................................................................
 * Plot the axes and labels of a given graph.
 *
 * Input:
 *  graph MonitorGraph *  The graph to be adjusted.
 *  erase          int    If true, draw the axes in the background color.
 */
static void draw_MonitorGraph_axes(MonitorGraph *graph, int erase)
{
  MonitorPlot *plot = graph->plot;  // The parent plot of the graph 
  
  // If the plot contains more than two graphs, tell the axis
  // rendering functions to choose coarser tick intervals for the
  // y-axis.
  
  int cramp_option = plot->ngraph > 2 ? AXIS_IS_CRAMPED : 0;
  
  // Ignore this call if the graph is not ready to be plotted to.
  
  if(graph->update_needed && 
     !(graph->plot->type == PLOT_POWSPEC && graph->powSpec.updateNeeded))
    return;
  
  cpgslct(plot->pgplot_id);
  cpgslw(1);
  cpgsci(erase ? 0:1);
  cpgsvp(graph->vxa, graph->vxb, graph->vya, graph->vyb);
  cpgswin(0.0, 1.0, 0.0, 1.0);

  // Draw the X-axes according to their type.
  
  cpgsch(MP_XNUM_SIZE);
  
  double xa, xb, ya, yb;
  bool xIsLinear, yIsLinear;

  if(erase) {

    xa = graph->lastPlot.xa;
    xb = graph->lastPlot.xb;
    ya = graph->lastPlot.ya;
    yb = graph->lastPlot.yb;
    xIsLinear = graph->lastPlot.xIsLinear;
    yIsLinear = graph->lastPlot.yIsLinear;

  } else {

    xa = plot->wxa;
    xb = plot->wxb;
    ya = graph->wya;
    yb = graph->wyb;
    xIsLinear =  plot->powSpec.linAxis;
    yIsLinear = graph->powSpec.linAxis;

    // Store the last plot variables

    graph->lastPlot.xa = xa;
    graph->lastPlot.xb = xb;
    graph->lastPlot.ya = ya;
    graph->lastPlot.yb = yb;
    graph->lastPlot.xIsLinear = xIsLinear;
    graph->lastPlot.yIsLinear = yIsLinear;
  }

  // Iterate over all xaxes, only set to date if all are of the same
  // type

  RegAspect aspect = graph->getXAspect();

  switch(aspect) {
  case REG_DATE:
    
    // Draw the upper x-axis of the graph.
    
    utc_axis(0,                        // Don't draw numeric labels 
	     0.0, 1.0, 1.0, 1.0,       // x1,y1, x2,y2 
	     xa, xb,                   // mjd1, mjd2 
	     0.0, 0.7, 0.4, 0.7);      // majl, majr, minf, disp 
    
    // Draw the lower x-axis of the graph.
    
    utc_axis(graph->next==NULL ? AXIS_WITH_LABELS : 0, // Plot labels? 
	     0.0, 0.0, 1.0, 0.0,       // x1,y1, x2,y2 
	     xa, xb,                   // mjd1, mjd2 
	     0.7, 0.0, 0.4, 0.7);      // majl, majr, minf, disp 
    break;
  default:
    
    if(plot->type != PLOT_POWSPEC || xIsLinear) {

      cpgswin(0.0, 1.0, 0.0, 1.0);

      // Draw the upper x-axis of the graph.
    
      linear_axis(0,                     // Don't plot labels 
		  0.0, 1.0, 1.0, 1.0,    // x1,y1, x2,y2 
		  xa, xb,                // ymin, ymax 
		  0.0, 0.7, 0.4, 0.7);   // majl, majr, minf, disp 
      
      // Draw the lower x-axis of the graph.
      
      linear_axis(graph->next==NULL ? AXIS_WITH_LABELS : 0, // Plot labels? 
		  0.0, 0.0, 1.0, 0.0,    // x1,y1, x2,y2 
		  xa, xb,                // ymin, ymax 
		  0.7, 0.0, 0.4, 0.7);   // majl, majr, minf, disp 
    } else {

      cpgswin(log10(xa), log10(xb), 0, 1);

      if(graph->next==NULL) {
	cpgbox("BCNLST", 0.0, 0, "", 0.0, 0);
      } else {
	cpgbox("CNLST",  0.0, 0, "", 0.0, 0);
      }

    }

    if(graph->next==NULL && plot->type == PLOT_POWSPEC)
      cpgmtxt("B", 3, 1.0, 1.0, "(Hz)");

    break;
  };
  
  // Draw the user's axis label.
  
  if(!graph->next) {
    cpgsch(MP_AXLAB_SIZE);
    cpgmtxt("B", 2.5f, 0.5f, 0.5f, plot->xlabel);
  };
    
  // If this is not a power spectrum plot, or if it is, and we are
  // requested to draw linear axes, proceed here

  if(!plot->type == PLOT_POWSPEC || yIsLinear) {

    cpgswin(0.0, 1.0, 0.0, 1.0);

    // Draw the leftmost y-axis of the graph.
    
    cpgsch(MP_YNUM_SIZE);
    linear_axis(AXIS_WITH_LABELS | AXIS_OUTWARD_TEXT | cramp_option,
		0.0, 0.0, 0.0, 1.0,      // x1,y1, x2,y2 
		ya, yb,                  // ymin, ymax 
		0.0, 0.7, 0.4, -0.7);    // majl, majr, minf, disp 
    
    // Draw the rightmost y-axis of the graph.
    
    linear_axis(cramp_option,            // Don't draw numeric labels 
		1.0, 0.0, 1.0, 1.0,      // x1,y1, x2,y2 
		ya, yb,                  // ymin, ymax 
		0.7, 0.0, 0.4, 0.7);     // majl, majr, minf, disp 
    
    // Else draw log axes

  } else {
    cpgswin(0, 1, log10(ya), log10(yb));
    cpgbox("", 0.0, 0, "BCNLVST", 0.0, 0);
  }

  // Write the Y-axis label of the graph.
  
  cpgsch(MP_AXLAB_SIZE);
  cpgmtxt("L", 4.5f, 0.5f, 0.5f, graph->ylab);

  return;
}

/*.......................................................................
 * Draw one or more snapshots of a graph from data archived in the
 * monitor buffer.
 *
 * Input:
 *  graph   MonitorGraph *  The graph to update.
 *  oldest          int    The first snapshot to draw.
 *  newest          int    The last snapshot to draw.
 *  erase            int    If true, draw the snapshot data in the
 *                          background color to erase it.
 */
static void draw_MonitorGraph_data(MonitorGraph *graph,
				   int oldest, int newest, int erase)
{
  MonitorPlot*    plot = graph->plot;
  MonitorViewer*  view = plot->view;// The parent viewer of the plot 
  MonitorBuff*    mb = view->mb;    // The snapshot buffer 
  MonitorBuffMap* map = mb->map;    // The buffer configuration object 
  MonitorTrace*   trace;            // The trace being drawn 
  int ci = MP_MIN_CI;               // The start color index 
  int x_offset;                     // Snapshot offset of X-axis register 
  
  // Ignore this call if the graph is not ready to be plotted to.
  
  if(graph->update_needed || !map || newest < oldest)
    return;
  
  // Direct PGPLOT output to the parent plot.
  
  cpgslct(plot->pgplot_id);
  
  // Don't show the graph until it has been redrawn.
  
  cpgbbuf();
  
  // Install the viewport and world coordinates of the graph.  Note
  // that both world coordinate axes are shifted to start at zero so
  // that wxa and wya can be subtracted from the double precision
  // register values before converting to single precision. This
  // results in higher precision results as one zooms in on the data.
  
  cpgsvp(graph->vxa, graph->vxb, graph->vya, graph->vyb);
  cpgswin(0.0f, plot->wxb - plot->wxa, 0.0f, graph->wyb - graph->wya);
  cpgslw(plot->marker_size);
  
  // If we don't yet have a valid x-axis register, stop now.
  
  if(plot->xdescs->size() == 0) {
    cpgebuf();
    return;
  };
  
  // Find the offset of the X-axis register within buffered snapshots.
  
  x_offset = 
    map->offset[plot->xdescs->at(0).currentSlot()].a[plot->xdescs->at(0).aspect()];
  
  // Limit 'oldest' to the constraints of the plot and buffer.
  
  if(oldest < map->oldest)
    oldest = map->oldest;
  if(oldest < plot->oldest)
    oldest = plot->oldest;
  
  // Also constrain 'oldest' to the extent of buffered X-axis data.
  
  {
    int oldest_xreg = map->regs[x_offset].oldest;
    if(oldest_xreg > oldest)
      oldest = oldest_xreg;
  };
  
  // Constrain the newest snapshot to be plotted.
  
  if(newest > map->newest)
    newest = map->newest;
  
  // Draw all snapshots of a given trace register element at a time so
  // as to reduce calls to pgsci().
  
  for(trace=graph->traces; trace; trace=trace->next) {
    for(unsigned iReg=0; iReg < trace->descs->size(); iReg++) {
      
      // Get the descriptor for this register
      
      gcp::util::RegDescription& desc = trace->descs->at(iReg);
      
      // If this register is archived, continue
      
      if(desc.iSlot() >= 0) {
	
	// Iterate over all selected indices for this register
	
	for(desc.begin(); !desc.isEnd(); ++desc) {
	  
	  int y_offset = map->offset[desc.currentSlot()].a[desc.aspect()];
	  
	  // Get the index of the oldest recorded value of the current
	  // Y-axis register.
	  
	  int y_oldest = map->regs[y_offset].oldest;
	  if(y_oldest < oldest)
	    y_oldest = oldest;
	  
	  // Set the next highest color index for each register element,
	  // modulo the range of available colors.
	  
	  cpgsci(erase ? 0:ci);
	  if(++ci > MP_MAX_CI)
	    ci = MP_MIN_CI;
	  
	  // Extract and plot the current register element for each snapshot.
	  //
	  // Plot with points?
	  
	  if(!plot->join || newest==y_oldest) {
	    if(graph->bits)
	      draw_logic_points(graph, y_oldest, newest, x_offset, y_offset);
	    else
	      draw_value_points(graph, y_oldest, newest, x_offset, y_offset);
	    
	    // Draw with lines?
	    
	  } else {
	    if(desc.aspect() == REG_PHASE)
	      draw_phase_lines(graph, y_oldest, newest, x_offset, y_offset);
	    else if(graph->bits)
	      draw_logic_lines(graph, y_oldest, newest, x_offset, y_offset);
	    else
	      draw_value_lines(graph, y_oldest, newest, x_offset, y_offset);
	  };
	};
      };
    };
  };
  
  // Reveal the redrawn graph.
  
  cpgebuf();
  return;
}

/*.......................................................................
 * Draw one or more snapshots of a graph from data archived in the
 * monitor buffer.
 *
 * Input:
 *  graph   MonitorGraph *  The graph to update.
 *  oldest          int    The first snapshot to draw.
 *  newest          int    The last snapshot to draw.
 *  erase            int    If true, draw the snapshot data in the
 *                          background color to erase it.
 */
void MonitorGraph::drawPowSpec(int erase) 
{
  int ci = MP_MIN_CI;               // The start color index 
  
  // Ignore this call if the graph is not ready to be plotted to.
  
  if(update_needed)
    return;

  // Direct PGPLOT output to the parent plot.
  
  cpgslct(plot->pgplot_id);
  
  // Don't show the graph until it has been redrawn.
  
  cpgbbuf();
  
  // Install the viewport and world coordinates of the graph.  Note
  // that both world coordinate axes are shifted to start at zero so
  // that wxa and wya can be subtracted from the double precision
  // register values before converting to single precision. This
  // results in higher precision results as one zooms in on the data.
  
  cpgsvp(vxa, vxb, vya, vyb);

  float xa = plot->powSpec.linAxis ? plot->wxa : log10(plot->wxa);
  float xb = plot->powSpec.linAxis ? plot->wxb : log10(plot->wxb);
  float ya = powSpec.linAxis ? wya : log10(wya);
  float yb = powSpec.linAxis ? wyb : log10(wyb);

  cpgswin(xa, xb, ya, yb);

  cpgslw(plot->marker_size);
  
  // Draw all snapshots of a given trace register element at a time so
  // as to reduce calls to pgsci().
  
  for(MonitorTrace* trace=traces; trace; trace=trace->next) {
    trace->stepPowSpecState(ci, erase);
  }
  
  // Reveal the redrawn graph.
  
  cpgebuf();

  return;
}

/**.......................................................................
 * Check if the new data will be out of range for the current axes.
 * If true, we recalculate and return the range in this function so
 * that the axis can be redrawn
 *
 * Input:
 *  graph   MonitorGraph *  The graph to update.
 *  oldest          int    The first snapshot to draw.
 *  newest          int    The last snapshot to draw.
 *  erase            int    If true, draw the snapshot data in the
 *                          background color to erase it.
 */
static bool checkYAxis(MonitorGraph *graph, int oldest, int newest, 
		       float* ya, float* yb)
{
  MonitorPlot*    plot = graph->plot; // The parent plot of the graph 
  MonitorViewer*  view = plot->view;  // The parent viewer of the plot 
  MonitorBuff*    mb   = view->mb;    // The snapshot buffer 
  MonitorBuffMap* map  = mb->map;     // The buffer configuration object 

  int x_offset;                       // Snapshot offset of X-axis register 
  
  // Ignore this call if we are not tracking the y-range

  if(!graph->track)
    return false;

  // Ignore this call if the graph is not ready to be plotted to.
  
  if(graph->update_needed || !map || newest < oldest)
    return false;
  
  // If we don't yet have a valid x-axis register, stop now.
  
  if(plot->xdescs->size() == 0) {
    cpgebuf();
    return false;
  };
  
  // Constrain the newest snapshot to be plotted.
  
  if(newest > map->newest)
    newest = map->newest;
  
  // Iterate over all distinct traces in the graph.  Note that a
  // "trace" can now consist of multiple registers, which may be
  // plotted as a single trace, or multiple traces.
 
  int y_offset;
  float yval, ymin, ymax, ymean=0.0;
  unsigned n=0;
  double* frame = view->mb->data + (newest % map->max_snap) * map->nregs;

  bool first=true;

  for(MonitorTrace* trace=graph->traces; trace; trace=trace->next) {

    for(vector<RegDescription>::iterator iDesc=trace->descs->begin();
	iDesc != trace->descs->end(); iDesc++) {

      y_offset = map->offset[iDesc->currentSlot()].a[iDesc->aspect()];

      // If this is a normal value-trace, just compare the value

      if(!graph->bits) {

	yval   = frame[y_offset];
	ymean += (yval - ymean)/(++n);

	if(first) {
	  ymin = ymax = yval;
	  first = false;
	} else {
	  ymin = (ymin < yval) ? ymin : yval;
	  ymax = (ymax > yval) ? ymax : yval;
	}

	// Else check bits

      } else {

	for(unsigned bit=0; bit<32; bit++) {
	  if((graph->bits >> bit) & 1U) {
	    int on = ((unsigned) frame[y_offset] >> bit) & 1U;

	    yval   = (on ? (bit+0.25):(bit-0.25));
	    ymean += (yval - ymean)/(++n);

	    if(first) {
	      ymin = ymax = yval;
	      first = false;
	    } else {
	      ymin = (ymin < yval) ? ymin : yval;
	      ymax = (ymax > yval) ? ymax : yval;
	    }
	  }
	}
      }

    }
  }

  // If the min or max are currently out of range, and at least one
  // would be brought into range by re-setting the range, do so.  Else
  // do nothing.

  if((ymax > graph->wyb && ymax < ymean+graph->range) || 
     (ymin < graph->wya && ymin > ymean-graph->range)) {

    *yb = ymean + graph->range;
    *ya = ymean - graph->range;
    return true;

  }

  return false;
}

/*.......................................................................
 * Draw one or more snapshots of a graph from data archived in the
 * monitor buffer.
 *
 * Input:
 *  graph   MonitorGraph *  The graph to update.
 *  oldest          int    The first snapshot to draw.
 *  newest          int    The last snapshot to draw.
 *  erase            int    If true, draw the snapshot data in the
 *                          background color to erase it.
 */
static void drawMonitorGraphData(MonitorGraph *graph,
				 int oldest, int newest, int erase)
{
  switch(graph->plot->type) {
  case PLOT_NORM:
    graph->drawNormal(oldest, newest, erase);
    break;
  case PLOT_POWSPEC:
    graph->drawPowSpec(erase);
    break;
  default:
    ThrowError("Unhandled plot type");
    break;
  }
}


/**.......................................................................
 * Output the name of a register for statistics calculations
 */
void outputStatName(OutputStream* output, 
		    MonitorGraph* graph, 
		    gcp::util::RegDescription& desc)
{
  switch (graph->plot->type) {
  case PLOT_NORM:
    desc.output(output, REG_OUTPUT_ELEMENT);  
    break;
  case PLOT_POWSPEC:
    desc.output(output, REG_OUTPUT_RANGE);  
    break;
  default:
    ThrowError("Unhandled plot type");
    break;
  }
}


void MonitorGraph::drawNormal(int oldest, int newest, int erase)
{
  MonitorViewer*  view = plot->view; // The parent viewer of the plot 
  MonitorBuff*    mb   = view->mb;   // The snapshot buffer 
  MonitorBuffMap* map  = mb->map;    // The buffer configuration object 
  MonitorTrace*   trace;             // The trace being drawn 

  int x_offset;                      // Snapshot offset of X-axis register 
  
  // Ignore this call if the graph is not ready to be plotted to.
  
  if(update_needed || !map || newest < oldest)
    return;
  
  // Direct PGPLOT output to the parent plot.
  
  cpgslct(plot->pgplot_id);
  
  // Don't show the graph until it has been redrawn.
  
  cpgbbuf();
  
  // Install the viewport and world coordinates of the graph.  Note
  // that both world coordinate axes are shifted to start at zero so
  // that wxa and wya can be subtracted from the double precision
  // register values before converting to single precision. This
  // results in higher precision results as one zooms in on the data.
  
  cpgsvp(vxa, vxb, vya, vyb);
  cpgswin(0.0f, plot->wxb - plot->wxa, 0.0f, wyb - wya);
  cpgslw(plot->marker_size);
  
  // If we don't yet have a valid x-axis register, stop now.
  
  if(!plot->xAxisIsValid()) {
    cpgebuf();
    return;
  };
  
  // Limit 'oldest' to the constraints of the plot and buffer.
  
  if(oldest < map->oldest)
    oldest = map->oldest;
  if(oldest < plot->oldest)
    oldest = plot->oldest;
  
  // Also constrain 'oldest' to the extent of buffered X-axis data.
  
  int oldest_xreg;
  for(unsigned iXreg=0; iXreg < plot->xdescs->size(); iXreg++) {
    gcp::util::RegDescription& xdesc = plot->xdescs->at(iXreg);

    // First constrain to the extent of the visible data

    //    oldest_xreg = xdesc.oldestVisible();

    //    std::cout << "Oldest visible X: " << oldest_xreg << std::endl;

    //    if(oldest_xreg >= 0 && oldest_xreg > oldest)
    //      oldest = oldest_xreg;

    for(xdesc.begin(); !xdesc.isEnd(); ++xdesc) {
      x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
      
      oldest_xreg = map->regs[x_offset].oldest;
      if(oldest_xreg > oldest)
	oldest = oldest_xreg;
    }
  }
  
  // Constrain the newest snapshot to be plotted.
  
  if(newest > map->newest)
    newest = map->newest;
  
  // Iterate over all distinct traces in the graph.  Note that a
  // "trace" can now consist of multiple registers, which may be
  // plotted as a single trace, or multiple traces.
  
  int ci = MP_MIN_CI;

  for(trace=traces; trace; trace=trace->next) {

    // Now determine how the trace should be plotted

    // If this is a single x-axis

    if(plot->xdescs->size() == 1) {

      if(bits) {
	if(plot->xdescs->at(0).nEl() > 1 && !erase && newest > 0)
	  drawLogicSa(this, trace, oldest, newest-1, true, ci);
	drawLogicSa(this, trace, oldest, newest, erase, ci);
      } else {
#ifndef MULTI_TEST
	if(plot->xdescs->at(0).nEl() > 1 && !erase && newest > 0) {
	  drawValueSa(this, trace, oldest, newest-1, true, ci);
	}
#endif
	drawValueSa(this, trace, oldest, newest, erase, ci);
      }

      // Else we have multiple x-axes

    } else {

      // If these are single-element axes

      if(plot->xdescs->at(0).nEl() == 1) {

	if(bits) {
	  if(!erase && newest > 0)
	    drawLogicMaSe(this, trace, newest-1, true, ci);
	  drawLogicMaSe(this, trace, newest, erase, ci);
	} else {
	  if(!erase && newest > 0)
	    drawValueMaSe(this, trace, newest-1, true, ci);
	  drawValueMaSe(this, trace, newest, erase, ci);
	}

      } else {

	if(bits) {
	  if(!erase && newest > 0)
	    drawLogicMaMe(this, trace, newest-1, true, ci);
	  drawLogicMaMe(this, trace, newest, erase, ci);
	} else {
	  if(!erase && newest > 0)
	    drawValueMaMe(this, trace, newest-1, true, ci);
	  drawValueMaMe(this, trace, newest, erase, ci);
	}
      }
    }
  }

  // Reveal the redrawn graph.
  
  cpgebuf();

  return;
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as disconnected points.
 *
 * Input:
 *  graph      MonitorGraph *  The graph in which to draw.
 *  oldest             int    The oldest snapshot to draw.
 *  newest             int    The newest snapshot to draw.
 *  x_offset           int    The offset of the x-axis register within
 *                             a buffer frame.
 *  y_offset           int    The offset of the y-axis register within
 *                             a buffer frame.
 */
static void draw_value_points(MonitorGraph *graph, int oldest, int newest,
			      int x_offset, int y_offset)
{
  MonitorPlot*    plot  = graph->plot;  // The parent plot of the graph 
  MonitorViewer*  view  = plot->view;   // The parent viewer of the plot 
  MonitorBuff*    mb    = view->mb;     // The snapshot buffer 
  MonitorBuffMap* map   = mb->map;      // The buffer configuration object 
  MpPointBuf*     ptbuf = &view->ptbuf; // The point plotting buffer 

  float xlen = plot->wxb  - plot->wxa;  // The extent of the X-axis 
  float ylen = graph->wyb - graph->wya; // The extent of the Y-axis 

  int isnap;                            // The snapshot being drawn 
  int nbuf = 0;                         // The number points in ptbuf[] 
  
  // Draw dots at each y-axis value of the trace, over the specified
  // range of snapshots.
  
  for(isnap=oldest; isnap <= newest; isnap++) {
    double *frame = mb->data + (isnap % map->max_snap) * map->nregs;
    float xval = frame[x_offset] - plot->wxa;
    float yval = frame[y_offset] - graph->wya;
    
    // If the point is visible, add it to the point buffer.
    
    if(xval > 0.0f && xval < xlen && yval > 0.0f && yval < ylen) {

      ptbuf->x[nbuf] = xval;
      ptbuf->y[nbuf] = yval;
      
      // Flush the buffer to the display when the buffer is full.
      
      if(++nbuf >= MP_BUF_NPT) {
	cpgpt(nbuf, ptbuf->x, ptbuf->y, MP_POINT_SYMBOL);
	nbuf = 0;
      };
    };
  };
  
  // Flush any remaining buffered points to the display.
  
  if(nbuf)
    cpgpt(nbuf, ptbuf->x, ptbuf->y, MP_POINT_SYMBOL);
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to plot
 * one or more bits of the integer value of a register.
 *
 * Input:
 *  graph      MonitorGraph *  The graph in which to draw.
 *  oldest             int    The oldest snapshot to draw.
 *  newest             int    The newest snapshot to draw.
 *  x_offset           int    The offset of the x-axis register within
 *                             a buffer frame.
 *  y_offset           int    The offset of the y-axis register within
 *                             a buffer frame.
 */
static void draw_logic_points(MonitorGraph *graph, int oldest, int newest,
			      int x_offset, int y_offset)
{
  MonitorPlot *plot = graph->plot;      /* The parent plot of the graph */
  MonitorViewer *view = plot->view;     /* The parent viewer of the plot */
  MonitorBuff *mb = view->mb;           /* The snapshot buffer */
  MonitorBuffMap *map = mb->map;        /* The buffer configuration object */
  MpPointBuf *ptbuf = &view->ptbuf;     /* The point plotting buffer */
  float xlen = plot->wxb - plot->wxa;   /* The extent of the X-axis */
  float ylen = graph->wyb - graph->wya; /* The extent of the Y-axis */
  int bit;                              /* A bit to be plotted 0..31 */
  int nbuf = 0;                         /* The number points in ptbuf[] */
  /*
   * Draw dots at each y-axis value of the trace, over the specified range
   * of snapshots.
   */
  for(bit=0; bit<32; bit++) {
    if((graph->bits >> bit) & 1U) {
      int isnap;                           /* The snapshot being drawn */
      for(isnap=oldest; isnap <= newest; isnap++) {
	double *frame = mb->data + (isnap % map->max_snap) * map->nregs;
	float xval = frame[x_offset] - plot->wxa;
	int on = ((unsigned) frame[y_offset] >> bit) & 1U;
	float yval = (on ? (bit+0.25):(bit-0.25)) - graph->wya;
	
	// If the point is visible, add it to the point buffer.

	if(xval > 0.0f && xval < xlen && yval > 0.0f && yval < ylen) {
	  ptbuf->x[nbuf] = xval;
	  ptbuf->y[nbuf] = yval;
	  
	  // Flush the buffer to the display when the buffer is full.

	  if(++nbuf >= MP_BUF_NPT) {
	    cpgpt(nbuf, ptbuf->x, ptbuf->y, MP_POINT_SYMBOL);
	    nbuf = 0;
	  };
	};
      };
    };
  };
  
  // Flush any remaining buffered points to the display.

  if(nbuf)
    cpgpt(nbuf, ptbuf->x, ptbuf->y, MP_POINT_SYMBOL);
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * Input:
 *  graph      MonitorGraph *  The graph in which to draw.
 *  oldest             int    The oldest snapshot to draw.
 *  newest             int    The newest snapshot to draw.
 *  x_offset           int    The offset of the x-axis register within
 *                             a buffer frame.
 *  y_offset           int    The offset of the y-axis register within
 *                             a buffer frame.
 */
static void draw_value_lines(MonitorGraph *graph, int oldest, int newest,
			     int x_offset, int y_offset)
{
  MonitorPlot *plot = graph->plot;      /* The parent plot of the graph */
  MonitorViewer *view = plot->view;     /* The parent viewer of the plot */
  MonitorBuff *mb = view->mb;           /* The snapshot buffer */
  MonitorBuffMap *map = mb->map;        /* The buffer configuration object */
  MpPointBuf *ptbuf = &view->ptbuf;     /* The point plotting buffer */
  int isnap;                           /* The snapshot being drawn */
  int nbuf = 0;                         /* The number points in ptbuf[] */
  
  // Draw lines between the values of the register for all snapshots
  // between oldest and newest.

  for(isnap=oldest; isnap <= newest; isnap++) {
    double *frame = mb->data + (isnap % map->max_snap) * map->nregs;
    float xval = frame[x_offset] - plot->wxa;
    float yval = frame[y_offset] - graph->wya;
    
    // Add the latest point to the line vertex buffer.

    ptbuf->x[nbuf] = xval;
    ptbuf->y[nbuf] = yval;
    
    // Flush the buffer to the display when the buffer is full.

    if(++nbuf >= MP_BUF_NPT) {
      cpgline(nbuf, ptbuf->x, ptbuf->y);
      
      // Re-initialize the buffer with the end point of the last line,
      // so that the next line segment connects to it.

      ptbuf->x[0] = xval;
      ptbuf->y[0] = yval;
      nbuf = 1;
    };
  };
  
  // Flush any remaining buffered line segments to the display.

  if(nbuf > 1)
    cpgline(nbuf, ptbuf->x, ptbuf->y);
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a generic version to draw a single trace if the x-axis is a
 * single register, either single- or multi-element.  Used to
 * plot for example:
 *
 * 1) corr1.usbAvg      vs. corr0.usbAvg -- in this case yIter = true
 * 2) tracker.actual[0] vs. corr0.usbAvg -- in this case yIter = false
 * 3) tracker.actual[0] vs. frame.utc.   -- in this case yIter is irrelevant
 *
 * Note that normal plots (case 3) are a special case of this variety,
 * where the x-axis is a single-element.  In this case, the value of
 * yIter is irrelevant, since there is only one element to iterate
 * over.
 *
 * Input:
 *
 *  graph      MonitorGraph *  The graph in which to draw.
 *  xdesc      RegDescription& The x-register
 *  ydesc      RegDescription& The y-register
 *  oldest     int            The oldest snapshot being plotted
 *  newest     int            The newest snapshot being plotted
 *  yIter      bool            If true, iterate over the y-axis elements
 */
static void drawValueSa(MonitorGraph* graph, 
			MonitorTrace* trace,
			int globalOldest,
			int newest,
			bool erase,
			int& ci)
{
  static int nbuf = 0;      // The number points in ptbuf[] 
  bool first=true;
  int cisave = ci;

  // The buffer configuration object 

  MonitorPlot* plot   = graph->plot;
  MonitorBuffMap* map = plot->view->mb->map;  
  MpPointBuf& ptbuf   = plot->view->ptbuf;   // The point plotting buffer 
  
  int y_offset;
  int y_oldest;
  int oldest;

  //  std::cout << std::endl;

  gcp::util::RegDescription& xdesc = plot->xdescs->at(0);

#if 0
  xdesc.setFirst(true); // Mark the first visible snapshot for this
			// register as uninitialized
#endif

  // Iterate over all registers in this trace

  for(unsigned iReg=0; iReg < trace->descs->size(); iReg++) {

    gcp::util::RegDescription& ydesc = trace->descs->at(iReg);

#if 0
    ydesc.setFirst(true); // Mark the first visible snapshot for this
			  // register as uninitialized
#endif
    y_offset = map->offset[ydesc.currentSlot()].a[ydesc.aspect()];

    // Set the oldest index to the more recent of the global oldest
    // and the oldest recorded value for this register
    
    oldest = globalOldest;
    y_oldest = map->regs[y_offset].oldest;
    if(y_oldest > oldest)
      oldest = y_oldest;

#if 0
    // Set the oldest index to the more recent of the current oldest
    // and the oldest visible value for this register
    
    y_oldest = ydesc.oldestVisible();
    if(y_oldest >= 0 && y_oldest > oldest)
      oldest = y_oldest;
#endif

    bool isPoint = !plot->join;

    // If the x-axis is single-element, or the number of elements in
    // the fastest-changing axis don't match, iterate over all indices
    // of this register.

    if(xdesc.nEl() == 1 || xdesc.fastestNel() != ydesc.fastestNel()) {

      for(ydesc.begin(); !ydesc.isEnd(); ++ydesc) {
	
	cpgsci(erase ? 0 : ci);
	if(!erase)
	  if(++ci > MP_MAX_CI)
	    ci = MP_MIN_CI;
	
	first = true;
	nbuf = 0;

	// Draw dots at each y-axis value of the trace, over the specified
	// range of snapshots.
	
	for(int isnap=oldest; isnap <= newest; isnap++) {

	  // Iterate over all elements in this possibly multi-dimensional
	  // x-axis
	
	  for(xdesc.begin(); !xdesc.isEnd() && !ydesc.isEnd(); ++xdesc) 
	    drawNext(graph, xdesc, ydesc, isnap, isPoint, nbuf, first);
	}
      
	// Flush any remaining buffered line segments to the display.
	
	if(!isPoint && nbuf > 1) {
	  cpgline(nbuf, ptbuf.x, ptbuf.y);
	} else if(isPoint && nbuf > 0) {
	  cpgpt(nbuf, ptbuf.x, ptbuf.y, MP_POINT_SYMBOL);
	}

      }
    } else {

#ifdef MULTI_TEST

      nbuf = 0;
      first = true;
      for(int isnap=oldest; isnap <= newest; isnap++) {
	ci = cisave;
      
#endif
      // Start the descriptor iterator here.  The Y-descriptor might
      // have the same number of elements as the x-axis, or it may
      // have multiples of the x-axis length.  In the latter case, we
      // will draw each multiple as a separate trace.
      //
      // For example:
      //
      //     corr.band0.frequency vs. corr.band0.usb[0-2]
      //
      // where each index [0-2] of the y-axis is 16-elements long

      ydesc.begin();

      while(!ydesc.isEnd()) {

	cpgsci(erase ? 0 : ci);

#ifndef MULTI_TEST
	if(!erase)
	  if(++ci > MP_MAX_CI)
	    ci = MP_MIN_CI;

	nbuf = 0;
	first = true;
#else
	if(!erase)
	  if(++ci > MP_MAX_CI)
	    ci = MP_MIN_CI;
#endif
	
	// Iterate over all elements in this possibly multi-dimensional
	// x-axis.
	
	for(xdesc.begin(); !xdesc.isEnd() && !ydesc.isEnd(); 
	    ++xdesc, ++ydesc) 
#ifdef MULTI_TEST
	  drawNext(graph, xdesc, ydesc, isnap, isPoint, nbuf, first);
#else
	  drawNext(graph, xdesc, ydesc, newest, isPoint, nbuf, first);
#endif

	
	// Flush any remaining buffered line segments to the display.
	
	if(nbuf > 1) {
	  if(isPoint)
	    cpgpt(nbuf, ptbuf.x, ptbuf.y, MP_POINT_SYMBOL);
	  else {
	    cpgline(nbuf, ptbuf.x, ptbuf.y);
	  }
	}

      }
#ifdef MULTI_TEST
      }
#endif
    }
  }
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a generic version to draw a single trace if the x-axis is a
 * single register, either single- or multi-element.  Used to
 * plot for example:
 *
 * 1) corr1.usbAvg      vs. corr0.usbAvg -- in this case yIter = true
 * 2) tracker.actual[0] vs. corr0.usbAvg -- in this case yIter = false
 * 3) tracker.actual[0] vs. frame.utc.   -- in this case yIter is irrelevant
 *
 * Note that normal plots (case 3) are a special case of this variety,
 * where the x-axis is a single-element.  In this case, the value of
 * yIter is irrelevant, since there is only one element to iterate
 * over.
 *
 * Input:
 *
 *  graph      MonitorGraph *  The graph in which to draw.
 *  xdesc      RegDescription& The x-register
 *  ydesc      RegDescription& The y-register
 *  oldest     int            The oldest snapshot being plotted
 *  newest     int            The newest snapshot being plotted
 *  yIter      bool            If true, iterate over the y-axis elements
 */
static void findClosestPointSa(MonitorTrace* trace,
			       double x, double y,
			       double x_to_mm, double y_to_mm,
			       gcp::util::RegDescription& closestReg,
			       double& min_x, double& min_y, double& min_rsq,
			       bool& first)
{
  MonitorPlot* plot   = trace->graph->plot;
  MonitorBuffMap* map = plot->view->mb->map;  
  int oldest         = map->oldest;  
  int newest         = map->newest;  
  int y_offset;

  gcp::util::RegDescription& xdesc = plot->xdescs->at(0);

  // Iterate over all registers in this trace

  for(unsigned iReg=0; iReg < trace->descs->size(); iReg++) {

    gcp::util::RegDescription& ydesc = trace->descs->at(iReg);
    y_offset = map->offset[ydesc.currentSlot()].a[ydesc.aspect()];

    // Get the index of the oldest recorded value of the current
    // Y-axis register.
    
    int y_oldest = map->regs[y_offset].oldest;
    if(y_oldest < oldest)
      oldest = y_oldest;
  
    // If the x-axis is single-element, or the elements don't match,
    // iterate over all indices of this register

    if(xdesc.nEl() == 1 || xdesc.fastestNel() != ydesc.fastestNel()) {

      for(ydesc.begin(); !ydesc.isEnd(); ++ydesc) {
	
	// Draw dots at each y-axis value of the trace, over the specified
	// range of snapshots.
	
	for(int isnap=oldest; isnap <= newest; isnap++) {

	  // Iterate over all elements in this possibly multi-dimensional
	  // x-axis
	
	  for(xdesc.begin(); !xdesc.isEnd() && !ydesc.isEnd(); ++xdesc) 
	    checkPoint(trace->graph, xdesc, ydesc, isnap, 
		       x, y, x_to_mm, y_to_mm, 
		       closestReg,
		       min_x, min_y, min_rsq, first);
	}
      }
    } else {

      // Draw each y-axis value of the trace, over the specified
      // range of snapshots.
	
      // Iterate over all elements in this possibly multi-dimensional
      // x-axis

      ydesc.begin();

      while(!ydesc.isEnd()) {

	for(xdesc.begin(); !xdesc.isEnd() && !ydesc.isEnd(); ++xdesc, ++ydesc) 
	  checkPoint(trace->graph, xdesc, ydesc, newest, 
		     x, y, x_to_mm, y_to_mm, 
		     closestReg,
		     min_x, min_y, min_rsq, first);
      }
    }
  }
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a generic version to draw a single trace if the x-axis is a
 * single register, either single- or multi-element.  Used to
 * plot for example:
 *
 * 1) corr1.usbAvg      vs. corr0.usbAvg -- in this case yIter = true
 * 2) tracker.actual[0] vs. corr0.usbAvg -- in this case yIter = false
 * 3) tracker.actual[0] vs. frame.utc.   -- in this case yIter is irrelevant
 *
 * Note that normal plots (case 3) are a special case of this variety,
 * where the x-axis is a single-element.  In this case, the value of
 * yIter is irrelevant, since there is only one element to iterate
 * over.
 *
 * Input:
 *
 *  graph      MonitorGraph *  The graph in which to draw.
 *  xdesc      RegDescription& The x-register
 *  ydesc      RegDescription& The y-register
 *  oldest     int            The oldest snapshot being plotted
 *  newest     int            The newest snapshot being plotted
 *  yIter      bool            If true, iterate over the y-axis elements
 */
static void getStatsSa(MonitorTrace* trace,
		       double xa, double xb,
		       MonitorRegStats& stats,
		       gcp::util::RegDescription* statDesc)
{
  MonitorPlot* plot   = trace->graph->plot;
  MonitorBuffMap* map = plot->view->mb->map;  
  int oldest         = map->oldest;  
  int newest         = map->newest;  
  int y_offset;
  int statSlot        = statDesc==0 ? 0 : statDesc->currentSlot();

  gcp::util::RegDescription& xdesc = plot->xdescs->at(0);

  // Iterate over all registers in this trace

  for(unsigned iReg=0; iReg < trace->descs->size(); iReg++) {

    gcp::util::RegDescription& ydesc = trace->descs->at(iReg);
    y_offset = map->offset[ydesc.currentSlot()].a[ydesc.aspect()];

    // If we are checking stats for one trace only, skip if this one
    // doesn't match

    if(statDesc != 0 && !ydesc.contains(*statDesc))
      continue;

    // Get the index of the oldest recorded value of the current
    // Y-axis register.
    
    int y_oldest = map->regs[y_offset].oldest;

#if 0
    if(y_oldest < oldest)
      oldest = y_oldest;
#endif

    oldest = y_oldest;
  
    // If the x-axis is single-element, or the elements don't match,
    // iterate over all indices of this register

    if(xdesc.nEl() == 1 || xdesc.fastestNel() != ydesc.fastestNel()) {

      for(ydesc.begin(); !ydesc.isEnd(); ++ydesc) {
	
	if(statDesc != 0 && ydesc.currentSlot() != statSlot)
	  continue;

	// Draw dots at each y-axis value of the trace, over the specified
	// range of snapshots.
	
	for(int isnap=oldest; isnap <= newest; isnap++) {

	  // Iterate over all elements in this possibly multi-dimensional
	  // x-axis
	  
	  for(xdesc.begin(); !xdesc.isEnd() && !ydesc.isEnd(); ++xdesc) 
	    getStat(trace->graph, xdesc, ydesc, isnap, xa, xb, stats);
	}
      }
      
      // Else they match, and we are drawing a single "trace" over all
      // indices of this y register

    } else {
      
#if 0
      // Draw each y-axis value of the trace, over the specified
      // range of snapshots.
      
      ydesc.begin();

      while(!ydesc.isEnd()) {
	
	// Iterate over all elements in this possibly multi-dimensional
	// x-axis
	
	for(xdesc.begin(); !xdesc.isEnd() && !ydesc.isEnd(); 
	    ++xdesc, ++ydesc) 
	  getStat(trace->graph, xdesc, ydesc, newest, xa, xb, stats);
      }
#endif

#ifdef MULTI_TEST
	for(int isnap=oldest; isnap <= newest; isnap++) {
#else
	for(int isnap=newest; isnap <= newest; isnap++) {
#endif

	  // Draw each y-axis value of the trace, over the specified
	  // range of snapshots.
	  
	  for(ydesc.begin(); !ydesc.isEnd();) {
	    
	    if(statDesc != 0 && ydesc.currentSlot() != statSlot) {
	      ++ydesc;
	      continue;
	    }
	    
	    // Iterate over all elements in this possibly multi-dimensional
	    // x-axis
	    
	    for(xdesc.begin(); !xdesc.isEnd() && !ydesc.isEnd(); 
		++xdesc, ++ydesc) 
	      getStat(trace->graph, xdesc, ydesc, isnap, xa, xb, stats);
	    
	  }
	}
     }
  }
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a version to draw traces for plots with multiple (-axis)
 * single-element (hence the MaSe) registers specified in the
 * x-axis. register, either single- or multi-element.  Used to plot
 * for example:
 *
 * 1) corr*.usbAvg vs. corr*.freq  
 *
 * In this case, for each index in the y-axes, we want a single trace
 * over all elements of the x-axis.  This requires iterating over all
 * registers in the y-axis specification at fixed element index.
 *
 * Input:
 *
 *  graph      MonitorGraph*           The graph in which to draw.
 *  xdescs     vector<RegDescription>& The vector of x-axes
 *  ydescs     vector<RegDescription>& The vector of y-axes
 *  isnap      int                    The snapshot being plotted
 *  erase      bool                    True if we are erasing the plot
 */
static void drawValueMaSe(MonitorGraph* graph, 
			  MonitorTrace* trace,
			  int isnap,
			  bool erase,
			  int& ci)
{
  MpPointBuf& ptbuf   = graph->plot->view->ptbuf; // The point plotting buffer 
  bool isPoint = !graph->plot->join;

  // We are guaranteed to have the same number of elements in all
  // y-axes at this point

  unsigned nYel = trace->descs->at(0).nEl();

  // Iterate over all Y-indices
  
  for(unsigned iYel=0; iYel < nYel; iYel++) {

    // Set the next highest color index for each register element,
    // modulo the range of available colors.
    
    cpgsci(erase ? 0 : ci);
    if(!erase)
      if(++ci > MP_MAX_CI)
	ci = MP_MIN_CI;

    bool first = true;
    int nbuf  = 0;

    // Iterate over all registers at fixed index
    
    for(unsigned iReg=0; iReg < graph->plot->xdescs->size(); iReg++) {
      
      // We are guaranteed to have the same number of registers in x
      // and y axes
      
      gcp::util::RegDescription& xdesc = graph->plot->xdescs->at(iReg);
      gcp::util::RegDescription& ydesc = trace->descs->at(iReg);
      
      // If this is the first element, reset to the beginning  

      if(iYel==0)
	ydesc.begin();

      // Else increment to the next element

      else
	++ydesc;
      
      // Draw or continue to draw a line

      drawNext(graph, xdesc, ydesc, isnap, isPoint, nbuf, first);
    }
    
    // Flush any remaining buffered line segments to the display.
    
    if(nbuf > 1) {
      if(isPoint)
	cpgpt(nbuf, ptbuf.x, ptbuf.y, MP_POINT_SYMBOL);
      else
	cpgline(nbuf, ptbuf.x, ptbuf.y);
    }
  }
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a version to draw traces for plots with multiple (-axis)
 * single-element (hence the MaSe) registers specified in the
 * x-axis. register, either single- or multi-element. Used to plot
 * for example:
 *
 * 1) corr*.usbAvg vs. corr*.freq  
 *
 * In this case, for each index in the y-axes, we want a single trace
 * over all elements of the x-axis.  This requires iterating over all
 * registers in the y-axis specification at fixed element index.
 *
 * Input:
 *
 *  graph      MonitorGraph*           The graph in which to draw.
 *  xdescs     vector<RegDescription>& The vector of x-axes
 *  ydescs     vector<RegDescription>& The vector of y-axes
 *  isnap      int                    The snapshot being plotted
 *  erase      bool                    True if we are erasing the plot
 */
static void getStatsMaSe(MonitorTrace* trace,
			 double xa, 
			 double xb,
			 MonitorRegStats& stats,
			 gcp::util::RegDescription* statDesc)
{
  MonitorPlot* plot   = trace->graph->plot;
  MonitorBuffMap* map = plot->view->mb->map;  
  int isnap          = map->newest;  
  int statElement     = statDesc==0 ? 0 : statDesc->currentElement();

  // We are guaranteed to have the same number of elements in all
  // y-axes at this point

  unsigned nYel = trace->descs->at(0).nEl();

  // Iterate over all Y-indices
  
  for(unsigned iYel=0; iYel < nYel; iYel++) {

    // Iterate over all registers at fixed index
    
    for(unsigned iReg=0; iReg < trace->graph->plot->xdescs->size(); iReg++) {
      
      // We are guaranteed to have the same number of registers in x
      // and y axes
      
      gcp::util::RegDescription& xdesc = trace->graph->plot->xdescs->at(iReg);
      gcp::util::RegDescription& ydesc = trace->descs->at(iReg);
      
      // If this is the first element, reset to the beginning  

      if(iYel==0)
	ydesc.begin();

      // Else increment to the next element

      else
	++ydesc;
      
      // If we are calculating statistics for a single logical trace,
      // and this isn't it, skip

      if(statDesc != 0 && ydesc.currentElement() != statElement)
	continue;

      // Draw or continue to draw a line

      getStat(trace->graph, xdesc, ydesc, isnap, xa, xb, stats);
    }
  }
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a version to draw traces for plots with multiple (-axis)
 * single-element (hence the MaSe) registers specified in the
 * x-axis. register, either single- or multi-element. Used to plot
 * for example:
 *
 * 1) corr*.usbAvg vs. corr*.freq  
 *
 * In this case, for each index in the y-axes, we want a single trace
 * over all elements of the x-axis.  This requires iterating over all
 * registers in the y-axis specification at fixed element index.
 *
 * Input:
 *
 *  graph      MonitorGraph*           The graph in which to draw.
 *  xdescs     vector<RegDescription>& The vector of x-axes
 *  ydescs     vector<RegDescription>& The vector of y-axes
 *  isnap      int                    The snapshot being plotted
 *  erase      bool                    True if we are erasing the plot
 */
static void findClosestPointMaSe(MonitorTrace* trace,
				 double x, double y,
				 double x_to_mm, double y_to_mm,
				 gcp::util::RegDescription& closestReg,
				 double& min_x, double& min_y, double& min_rsq,
				 bool& first)
{
  MonitorPlot* plot   = trace->graph->plot;
  MonitorBuffMap* map = plot->view->mb->map;  
  int isnap          = map->newest;  

  // We are guaranteed to have the same number of elements in all
  // y-axes at this point

  unsigned nYel = trace->descs->at(0).nEl();

  // Iterate over all Y-indices
  
  for(unsigned iYel=0; iYel < nYel; iYel++) {

    // Iterate over all registers at fixed index
    
    for(unsigned iReg=0; iReg < trace->graph->plot->xdescs->size(); iReg++) {
      
      // We are guaranteed to have the same number of registers in x
      // and y axes
      
      gcp::util::RegDescription& xdesc = trace->graph->plot->xdescs->at(iReg);
      gcp::util::RegDescription& ydesc = trace->descs->at(iReg);
      
      // If this is the first element, reset to the beginning  

      if(iYel==0)
	ydesc.begin();

      // Else increment to the next element

      else
	++ydesc;
      
      // Draw or continue to draw a line

      checkPoint(trace->graph, xdesc, ydesc, isnap, 
		 x, y, x_to_mm, y_to_mm, 
		 closestReg,
		 min_x, min_y, min_rsq, first);
    }
  }
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a version to draw traces for plots with multiple (-axis)
 * multi-element (hence the MaMe) registers specified in the
 * x-axis. Used to plot things like:
 *
 * corr[1-2].usbAvg vs. corr[0-1].usbAvg
 *
 * In this case, for each index in the y-axes, we want a single trace
 * over all elements of each x-y pair of registers. 
 *
 * Input:
 *
 *  graph      MonitorGraph*           The graph in which to draw.
 *  xdescs     vector<RegDescription>& The vector of x-axes
 *  ydescs     vector<RegDescription>& The vector of y-axes
 *  isnap      int                    The snapshot being plotted
 *  erase      bool                    True if we are erasing the plot
 */
static void drawValueMaMe(MonitorGraph* graph, 
			  MonitorTrace* trace,
			  int isnap,
			  bool erase,
			  int& ci)
{
  MpPointBuf& ptbuf = graph->plot->view->ptbuf; // The point plotting buffer 
  bool isPoint = !graph->plot->join;

  //  std::cout << std::endl;

  // Iterate over all distinct registers in the x-axis.  Note that we
  // are guaranteed to have the same number of registers in x and y.
  // Furthermore, we are guaranteed to have the same number of
  // elements in each register
    
  for(unsigned iReg=0; iReg < graph->plot->xdescs->size(); iReg++) {
      
    // Set the next highest color index for each register element,
    // modulo the range of available colors.
    
    cpgsci(erase ? 0 : ci);
    if(!erase)
      if(++ci > MP_MAX_CI)
	ci = MP_MIN_CI;

    bool first = true;
    int nbuf = 0;

    // We are guaranteed to have the same number of registers in x
    // and y axes
    
    gcp::util::RegDescription& xdesc = graph->plot->xdescs->at(iReg);
    gcp::util::RegDescription& ydesc = trace->descs->at(iReg);
    
    for(xdesc.begin(), ydesc.begin(); !xdesc.isEnd(); ++xdesc, ++ydesc) 
      drawNext(graph, xdesc, ydesc, isnap, isPoint, nbuf, first);
  
    // Flush any remaining buffered line segments to the display.
  
    if(!isPoint && nbuf > 1) 
	cpgline(nbuf, ptbuf.x, ptbuf.y);
    else if(isPoint && nbuf > 0)
      cpgpt(nbuf, ptbuf.x, ptbuf.y, MP_POINT_SYMBOL);
  }
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a version to draw traces for plots with multiple (-axis)
 * multi-element (hence the MaMe) registers specified in the
 * x-axis. Used to plot things like:
 *
 * corr[1-2].usbAvg vs. corr[0-1].usbAvg
 *
 * In this case, for each index in the y-axes, we want a single trace
 * over all elements of each x-y pair of registers. 
 *
 * Input:
 *
 *  graph      MonitorGraph*           The graph in which to draw.
 *  xdescs     vector<RegDescription>& The vector of x-axes
 *  ydescs     vector<RegDescription>& The vector of y-axes
 *  isnap      int                    The snapshot being plotted
 *  erase      bool                    True if we are erasing the plot
 */
static void findClosestPointMaMe(MonitorTrace* trace,
				 double x, double y,
				 double x_to_mm, double y_to_mm,
				 gcp::util::RegDescription& closestReg,
				 double& min_x, double& min_y, double& min_rsq,
				 bool& first)
{
  MonitorPlot* plot   = trace->graph->plot;
  MonitorBuffMap* map = plot->view->mb->map;  
  int isnap          = map->newest;  

  // Iterate over all distinct registers in the x-axis.  Note that we
  // are guaranteed to have the same number of registers in x and y.
  // Furthermore, we are guaranteed to have the same number of
  // elements in each register
    
  for(unsigned iReg=0; iReg < trace->graph->plot->xdescs->size(); iReg++) {
      
    // We are guaranteed to have the same number of registers in x
    // and y axes
    
    gcp::util::RegDescription& xdesc = trace->graph->plot->xdescs->at(iReg);
    gcp::util::RegDescription& ydesc = trace->descs->at(iReg);
    
    for(xdesc.begin(), ydesc.begin(); !xdesc.isEnd(); ++xdesc, ++ydesc) 
      checkPoint(trace->graph, xdesc, ydesc, isnap, 
		 x, y, x_to_mm, y_to_mm, 
		 closestReg,
		 min_x, min_y, min_rsq, first);
  }
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a version to draw traces for plots with multiple (-axis)
 * multi-element (hence the MaMe) registers specified in the
 * x-axis. Used to plot things like:
 *
 * corr[1-2].usbAvg vs. corr[0-1].usbAvg
 *
 * In this case, for each index in the y-axes, we want a single trace
 * over all elements of each x-y pair of registers. 
 *
 * Input:
 *
 *  graph      MonitorGraph*           The graph in which to draw.
 *  xdescs     vector<RegDescription>& The vector of x-axes
 *  ydescs     vector<RegDescription>& The vector of y-axes
 *  isnap      int                    The snapshot being plotted
 *  erase      bool                    True if we are erasing the plot
 */
static void getStatsMaMe(MonitorTrace* trace,
			 double xa, 
			 double xb,
			 MonitorRegStats& stats,
			 gcp::util::RegDescription* statDesc)
{
  MonitorPlot* plot   = trace->graph->plot;
  MonitorBuffMap* map = plot->view->mb->map;  
  int isnap          = map->newest;  

  // Iterate over all distinct registers in the x-axis.  Note that we
  // are guaranteed to have the same number of registers in x and y.
  // Furthermore, we are guaranteed to have the same number of
  // elements in each register
    
  for(unsigned iReg=0; iReg < trace->graph->plot->xdescs->size(); iReg++) {
      
    // We are guaranteed to have the same number of registers in x
    // and y axes
    
    gcp::util::RegDescription& xdesc = trace->graph->plot->xdescs->at(iReg);
    gcp::util::RegDescription& ydesc = trace->descs->at(iReg);

    // If we are calculating statistics for a single logical trace,
    // and this isn't it, skip

    if(statDesc != 0 && &ydesc != statDesc)
      continue;

    for(xdesc.begin(), ydesc.begin(); !xdesc.isEnd(); ++xdesc, ++ydesc) 
      getStat(trace->graph, xdesc, ydesc, isnap, xa, xb, stats);
  }
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a generic version to draw a single trace if the x-axis is a
 * single register, either single- or multi-element.  Used to
 * plot for example:
 *
 * 1) corr1.usbAvg      vs. corr0.usbAvg -- in this case yIter = true
 * 2) tracker.actual[0] vs. corr0.usbAvg -- in this case yIter = false
 * 3) tracker.actual[0] vs. frame.utc.   -- in this case yIter is irrelevant
 *
 * Note that normal plots (case 3) are a special case of this variety,
 * where the x-axis is a single-element.  In this case, the value of
 * yIter is irrelevant, since there is only one element to iterate
 * over.
 *
 * Input:
 *
 *  graph      MonitorGraph *  The graph in which to draw.
 *  xdesc      RegDescription& The x-register
 *  ydesc      RegDescription& The y-register
 *  oldest     int            The oldest snapshot being plotted
 *  newest     int            The newest snapshot being plotted
 *  yIter      bool            If true, iterate over the y-axis elements
 */
static void drawLogicSa(MonitorGraph* graph, 
			MonitorTrace* trace,
			int globalOldest,
			int newest,
			bool erase,
			int& ci)
{
  int nbuf = 0;      // The number points in ptbuf[] 
  bool first=true;
  MpPointBuf& ptbuf = graph->plot->view->ptbuf; // The point plotting buffer 

  // The buffer configuration object 

  MonitorPlot* plot   = graph->plot;
  MonitorBuffMap* map = plot->view->mb->map;  
  int y_offset, y_oldest, oldest;

  gcp::util::RegDescription& xdesc = plot->xdescs->at(0);

#if 0
  xdesc.setFirst(true);
#endif

  // Iterate over all registers in this trace

  for(unsigned iReg=0; iReg < trace->descs->size(); iReg++) {

    gcp::util::RegDescription& ydesc = trace->descs->at(iReg);
    ydesc.setFirst(true);

    y_offset = map->offset[ydesc.currentSlot()].a[ydesc.aspect()];

    // Get the index of the oldest recorded value of the current
    // Y-axis register.
    
    // Set the oldest index to the more recent of the global oldest
    // and the oldest recorded value for this register
    
    oldest = globalOldest;
    y_oldest = map->regs[y_offset].oldest;
    if(y_oldest > oldest)
      oldest = y_oldest;

#if 0
    // Set the oldest index to the more recent of the current oldest
    // and the oldest visible value for this register

    y_oldest = ydesc.oldestVisible();
    if(y_oldest >= 0 && y_oldest > oldest)
      oldest = y_oldest;
#endif

    bool isPoint = !plot->join || newest==oldest;

    // If the x-axis is single-element, or the elements don't match,
    // iterate over all indices of this register

    if(xdesc.nEl() == 1 || xdesc.nEl() != ydesc.nEl()) {

      for(ydesc.begin(); !ydesc.isEnd(); ++ydesc) {
	
	cpgsci(erase ? 0 : ci);
	if(!erase)
	  if(++ci > MP_MAX_CI)
	    ci = MP_MIN_CI;
	
	nbuf=0;
	first = true;
	
	for(int bit=0; bit<32; bit++) {
	  if((graph->bits >> bit) & 1U) {
	    
	    // Draw dots at each y-axis value of the trace, over the specified
	    // range of snapshots.
	    
	    for(int isnap=oldest; isnap <= newest; isnap++) {
	      
	      // Iterate over all elements in this possibly multi-dimensional
	      // x-axis
	      
	      for(xdesc.begin(); !xdesc.isEnd() && !ydesc.isEnd(); ++xdesc) 
		drawNext(graph, xdesc, ydesc, isnap, isPoint, nbuf, first);
	    }
	    
	    // Flush any remaining buffered line segments to the display.
	    
	    if(nbuf > 1) {
	      if(isPoint)
		cpgpt(nbuf, ptbuf.x, ptbuf.y, MP_POINT_SYMBOL);
	      else
		cpgline(nbuf, ptbuf.x, ptbuf.y);
	    }
	  }
	}
      }
    } else {

      cpgsci(erase ? 0 : ci);
      if(!erase)
	if(++ci > MP_MAX_CI)
	  ci = MP_MIN_CI;
      
      nbuf = 0;
      first = true;

      for(int bit=0; bit<32; bit++) {
	if((graph->bits >> bit) & 1U) {
	  
	  // Draw each y-axis value of the trace, over the specified
	  // range of snapshots.
	  
	  for(int isnap=oldest; isnap <= newest; isnap++) {
	    
	    // Iterate over all elements in this possibly multi-dimensional
	    // x-axis
	    
	    for(xdesc.begin(), ydesc.begin(); !xdesc.isEnd() && !ydesc.isEnd(); 
		++xdesc, ++ydesc) 
	      drawNext(graph, xdesc, ydesc, isnap, isPoint, nbuf, first);
	  }
	  
	  // Flush any remaining buffered line segments to the display.
	  
	  if(nbuf > 1) {
	    if(isPoint)
	      cpgpt(nbuf, ptbuf.x, ptbuf.y, MP_POINT_SYMBOL);
	    else
	      cpgline(nbuf, ptbuf.x, ptbuf.y);
	  }
	}
      }
    }
  }
}


/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a version to draw traces for plots with multiple (-axis)
 * single-element (hence the MaSe) registers specified in the
 * x-axis. register, either single- or multi-element.  Used to plot
 * for example:
 *
 * 1) corr*.usbAvg vs. corr*.freq  
 *
 * In this case, for each index in the y-axes, we want a single trace
 * over all elements of the x-axis.  This requires iterating over all
 * registers in the y-axis specification at fixed element index.
 *
 * Input:
 *
 *  graph      MonitorGraph*           The graph in which to draw.
 *  xdescs     vector<RegDescription>& The vector of x-axes
 *  ydescs     vector<RegDescription>& The vector of y-axes
 *  isnap      int                    The snapshot being plotted
 *  erase      bool                    True if we are erasing the plot
 */
static void drawLogicMaSe(MonitorGraph *graph, 
			  MonitorTrace* trace,
			  int isnap,
			  bool erase,
			  int& ci)
{
  MpPointBuf& ptbuf = graph->plot->view->ptbuf; // The point plotting buffer 
  int nbuf=0;
  bool isPoint = !graph->plot->join;
  bool first=true;

  // We are guaranteed to have the same number of elements in all
  // y-axes at this point

  unsigned nYel = trace->descs->at(0).nEl();

  // Iterate over all Y-indices
  
  for(unsigned iYel=0; iYel < nYel; iYel++) {

    // Set the next highest color index for each register element,
    // modulo the range of available colors.
    
    cpgsci(erase ? 0 : ci);
    if(!erase)
      if(++ci > MP_MAX_CI)
	ci = MP_MIN_CI;

    nbuf = 0;
    first = true;

    for(int bit=0; bit<32; bit++) {
      if((graph->bits >> bit) & 1U) {

	bool first = true;
	int nbuf  = 0;

	// Iterate over all registers at fixed index
	
	for(unsigned iReg=0; iReg < graph->plot->xdescs->size(); iReg++) {
      
	  // We are guaranteed to have the same number of registers in x
	  // and y axes
	  
	  gcp::util::RegDescription& xdesc = graph->plot->xdescs->at(iReg);
	  gcp::util::RegDescription& ydesc = trace->descs->at(iReg);
	  
	  // If this is the first element, reset to the beginning  
	  
	  if(iYel==0)
	    ydesc.begin();
	  
	  // Else increment to the next element
	  
	  else
	    ++ydesc;
	  
	  // Draw or continue to draw a line
	  
	  drawNext(graph, xdesc, ydesc, isnap, isPoint, nbuf, first, bit);
	}
      }
      
      // Flush any remaining buffered line segments to the display.
      
      if(nbuf > 1) {
	if(isPoint)
	  cpgpt(nbuf, ptbuf.x, ptbuf.y, MP_POINT_SYMBOL);
	else
	  cpgline(nbuf, ptbuf.x, ptbuf.y);
      }
    }
  }
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of registers as joined points.
 *
 * This is a version to draw traces for plots with multiple (-axis)
 * multi-element (hence the MaMe) registers specified in the
 * x-axis. Used to plot things like:
 *
 * corr[1-2].usbAvg vs. corr[0-1].usbAvg
 *
 * In this case, for each index in the y-axes, we want a single trace
 * over all elements of each x-y pair of registers. 
 *
 * Input:
 *
 *  graph      MonitorGraph*           The graph in which to draw.
 *  xdescs     vector<RegDescription>& The vector of x-axes
 *  ydescs     vector<RegDescription>& The vector of y-axes
 *  isnap      int                    The snapshot being plotted
 *  erase      bool                    True if we are erasing the plot
 */
static void drawLogicMaMe(MonitorGraph* graph, 
			  MonitorTrace* trace,
			  int isnap,
			  bool erase,
			  int& ci)
{
  MpPointBuf& ptbuf = graph->plot->view->ptbuf; // The point plotting buffer 
  int nbuf=0;
  bool isPoint = !graph->plot->join;
  bool first = true;

  // Iterate over all distinct registers in the x-axis.  Note that we
  // are guaranteed to have the same number of registers in x and y.
  // Furthermore, we are guaranteed to have the same number of
  // elements in each register
    
  for(unsigned iReg=0; iReg < graph->plot->xdescs->size(); iReg++) {
      
    // Set the next highest color index for each register element,
    // modulo the range of available colors.
    
    cpgsci(erase ? 0 : ci);
    if(!erase)
      if(++ci > MP_MAX_CI)
	ci = MP_MIN_CI;

    nbuf = 0;
    first = true;

    for(int bit=0; bit<32; bit++) {
      if((graph->bits >> bit) & 1U) {

	bool first = true;
	int nbuf = 0;
	
	// We are guaranteed to have the same number of registers in x
	// and y axes
	
	gcp::util::RegDescription& xdesc = graph->plot->xdescs->at(iReg);
	gcp::util::RegDescription& ydesc = trace->descs->at(iReg);
	
	for(xdesc.begin(), ydesc.begin(); !xdesc.isEnd(); ++xdesc, ++ydesc) 
	  drawNext(graph, xdesc, ydesc, isnap, isPoint, nbuf, first, bit);
      }
  
      // Flush any remaining buffered line segments to the display.
      
      if(nbuf > 1) {
	if(isPoint)
	  cpgpt(nbuf, ptbuf.x, ptbuf.y, MP_POINT_SYMBOL);
	else
	  cpgline(nbuf, ptbuf.x, ptbuf.y);
      }
    }
  }
}

/**.......................................................................
 * Draw or continue to buffer the next item (point or line)
 */
static void drawNext(MonitorGraph* graph,
		     gcp::util::RegDescription& xdesc,
		     gcp::util::RegDescription& ydesc,
		     int isnap,
		     bool isPoint,
		     int& nbuf,
		     bool& first,
		     int bit)
{
  // If drawing a point

  if(isPoint) {
    drawPoint(graph, xdesc, ydesc, isnap, nbuf, bit);
  } else {

    // Else if drawing a line

    if(ydesc.aspect() == REG_PHASE)
      drawPhaseLine(graph, xdesc, ydesc, isnap, first);
    else
      drawNormalLine(graph, xdesc, ydesc, isnap, nbuf, bit);
  }
}

/**.......................................................................
 * Points are buffered and drawn whenever the pt buffer is full
 */
static void drawPoint(MonitorGraph* graph, 
		      gcp::util::RegDescription& xdesc,
		      gcp::util::RegDescription& ydesc,
		      int isnap,
		      int& nbuf,
		      int bit)
{
  MonitorPlot* plot   = graph->plot;    // The parent plot of the graph
  MonitorViewer* view = plot->view;     // The parent viewer of the plot 
  MonitorBuffMap* map = view->mb->map;  // The buffer configuration object 
  MpPointBuf* ptbuf   = &view->ptbuf;   // The point plotting buffer 

  int x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
  int y_offset = map->offset[ydesc.currentSlot()].a[ydesc.aspect()];
  double *frame = view->mb->data + (isnap % map->max_snap) * map->nregs;
  float xval    = frame[x_offset] - graph->plot->wxa;
  float yval;

  //  std::cout << "Drawing offset: " << y_offset << " for location: " << ydesc.currentSlot()
  //	    << ", aspect: " << ydesc.aspect() << std::endl;


  if(!graph->bits)
    yval = frame[y_offset] - graph->wya;
  else {
    int on = ((unsigned) frame[y_offset] >> bit) & 1U;
    yval   = (on ? (bit+0.25):(bit-0.25)) - graph->wya;
  }
  
#if 0
  float xlen = plot->wxb - plot->wxa;   // The extent of the X-axis 
  float ylen = graph->wyb - graph->wya; // The extent of the Y-axis 

  // If the point is visible, add it to the point buffer.
  
  if(xval > 0.0f && xval < xlen && yval > 0.0f && yval < ylen) {

    // Update the oldest visible point for this register, if none has
    // yet been set

    if(xdesc.isFirst()) {
      xdesc.setOldestVisible(isnap);
      xdesc.setFirst(false);
    }

    if(ydesc.isFirst()) {
      ydesc.setOldestVisible(isnap);
      ydesc.setFirst(false);
    }
  }
#endif 

  // And install it in the point buffer
  
  ptbuf->x[nbuf] = xval;
  ptbuf->y[nbuf] = yval;
  
  // Flush the buffer to the display when the buffer is full.
  
  if(++nbuf >= MP_BUF_NPT) {
    cpgpt(nbuf, ptbuf->x, ptbuf->y, MP_POINT_SYMBOL);
    nbuf = 0;
  };
}

/**.......................................................................
 * Add the next point to the stats buffer
 */
static void getStat(MonitorGraph* graph, 
		    gcp::util::RegDescription& xdesc,
		    gcp::util::RegDescription& ydesc,
		    int isnap,
		    double xa, 
		    double xb,
		    MonitorRegStats& s)
{
  MonitorPlot* plot   = graph->plot;    // The parent plot of the graph
  MonitorViewer* view = plot->view;     // The parent viewer of the plot 
  MonitorBuffMap* map = view->mb->map;  // The buffer configuration object 

  int x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
  int y_offset = map->offset[ydesc.currentSlot()].a[ydesc.aspect()];
  double *frame = view->mb->data + (isnap % map->max_snap) * map->nregs;
  double xval   = frame[x_offset];
  double yval;

  yval = frame[y_offset];
  
  // If the point is visible, add it to the point buffer.
  
  if(xval >= xa && xval <= xb) 
    s.insert(xval, yval);
}

void MonitorRegStats::insert(double xval, double yval)
{
  // If the container has not been initialized, do it now
  
  switch (state) {
    
    // The statistics container has not yet been initialized
    
  case MRS_UNINIT:
    min  = yval;
    max  = yval;
    mean = yval;
    npoint++;
    state = MRS_MEAN;
    break;
    
    // We are accumulating a mean
    
  case MRS_MEAN:
    if(yval < min)
      min = yval;
    else if(yval > max)
      max = yval;
    
    mean += (yval - mean) / (double) ++npoint;
    break;
    
    // We are accumulating an rms
    
  case MRS_RMS:
    {
      double dv = (yval - mean);
      rms += (dv*dv - rms) / (double) ++npoint;
    }
    break;
  };
}

/**.......................................................................
 * Get the x-y value of Add the next point to the stats buffer
 */
static void checkPoint(MonitorGraph* graph, 
		       gcp::util::RegDescription& xdesc,
		       gcp::util::RegDescription& ydesc,
		       int isnap,
		       double x, double y,
		       double x_to_mm, double y_to_mm,
		       gcp::util::RegDescription& closestReg,
		       double& min_x, double& min_y, double& min_rsq, 
		       bool& first)
{
  MonitorPlot* plot   = graph->plot;    // The parent plot of the graph
  MonitorViewer* view = plot->view;     // The parent viewer of the plot 
  MonitorBuffMap* map = view->mb->map;  // The buffer configuration object 

  int x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
  int y_offset = map->offset[ydesc.currentSlot()].a[ydesc.aspect()];
  double *frame = view->mb->data + (isnap % map->max_snap) * map->nregs;
  bool found = false;

  // If this is a logic point, iterate over bits.

  double dx = (frame[x_offset] - x) * x_to_mm;

  if(graph->bits) {
    int bit;
    for(bit=0; bit < 32; bit++) {
      int on = ((unsigned) frame[y_offset] >> bit) & 1U;

      float yval = (on ? (bit+0.25):(bit-0.25)) - graph->wya;
      double dy = (yval - y) * y_to_mm;
      double rsq = dx*dx + dy*dy;

      if(first || rsq < min_rsq) {
	first = false;
	found = true;
	min_rsq = rsq;
      };

    };

    // Else this is a normal point -- just check the y-value

  } else {

    double dy = (frame[y_offset] - y) * y_to_mm;
    double rsq = dx*dx + dy*dy;

    if(first || rsq < min_rsq) {
      first = false;
      found = true;
      min_rsq = rsq;
    };
  };
  
  // If a new closest register was found record its details. This
  // is done here to reduce the amount of computation done in the
  // the above functions.
  
  if(found) {
    min_x = frame[x_offset];
    min_y = frame[y_offset];
    
    closestReg = ydesc;
    
    // Set the element for this descriptor to the current element
    
    gcp::util::Coord coord = ydesc.currentCoord();
    closestReg.setCoord(coord);
  };
}

/**.......................................................................
 * Normal lines are buffered and drawn whenever the line buffer is full
 */
static void drawNormalLine(MonitorGraph* graph, 
			   gcp::util::RegDescription& xdesc,
			   gcp::util::RegDescription& ydesc,
			   int isnap,
			   int& nbuf,
			   int bit)
{
  MonitorPlot* plot   = graph->plot;    // The parent plot of the graph
  MonitorViewer* view = plot->view;     // The parent viewer of the plot 
  MonitorBuffMap* map = view->mb->map;  // The buffer configuration object 
  MpPointBuf* ptbuf   = &view->ptbuf;   // The point plotting buffer 

  int x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
  int y_offset = map->offset[ydesc.currentSlot()].a[ydesc.aspect()];
  double *frame = view->mb->data + (isnap % map->max_snap) * map->nregs;
  float xval    = frame[x_offset] - graph->plot->wxa;
  float yval;
  static int lastIsnap=-1;

  if(!graph->bits)
    yval = frame[y_offset] - graph->wya;
  else {
    int on = ((unsigned) frame[y_offset] >> bit) & 1U;
    yval   = (on ? (bit+0.25):(bit-0.25)) - graph->wya;
  }

#if 0
  float xlen = plot->wxb - plot->wxa;   // The extent of the X-axis 
  float ylen = graph->wyb - graph->wya; // The extent of the Y-axis 

  // If the point is visible, add it to the point buffer.
  
  if(xval > 0.0f && xval < xlen && yval > 0.0f && yval < ylen) {

    // If this is the first visible point in this line, set the first
    // visible point to the previous one, so that the line will emerge
    // continuously from outside the plot boundary
    
    if(xdesc.isFirst()) {
      xdesc.setOldestVisible(lastIsnap < 0 ? isnap : lastIsnap);
      xdesc.setFirst(false);
    }

    if(ydesc.isFirst()) {
      ydesc.setOldestVisible(lastIsnap < 0 ? isnap : lastIsnap);
      ydesc.setFirst(false);
    }
  }
#endif

  // Add the latest point to the line vertex buffer.
  
  ptbuf->x[nbuf] = xval;
  ptbuf->y[nbuf] = yval;
  
  // Flush the buffer to the display when the buffer is full.
  
  if(++nbuf >= MP_BUF_NPT) {
    cpgline(nbuf, ptbuf->x, ptbuf->y);
    
    // Re-initialize the buffer with the end point of the last line,
    // so that the next line segment connects to it.
    
    ptbuf->x[0] = xval;
    ptbuf->y[0] = yval;
    nbuf = 1;
  };

  lastIsnap = isnap;
}

/**.......................................................................
 * Phase lines are wrapped and each segment is drawn separately
 */
static void drawPhaseLine(MonitorGraph* graph, 
			  gcp::util::RegDescription& xdesc,
			  gcp::util::RegDescription& ydesc,
			  int isnap,
			  bool& first)
{
  MonitorPlot* plot   = graph->plot;    // The parent plot of the graph
  MonitorViewer* view = plot->view;     // The parent viewer of the plot 
  MonitorBuffMap* map = view->mb->map;  // The buffer configuration object 

  int x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
  int y_offset = map->offset[ydesc.currentSlot()].a[ydesc.aspect()];
  double *frame = view->mb->data + (isnap % map->max_snap) * map->nregs;
  float xval    = frame[x_offset] - graph->plot->wxa;
  float yval    = frame[y_offset] - graph->wya;

  static float old_xval = 0.0; // The previously plotted x-axis vertex 
  static float old_yval = 0.0; // The previously plotted y-axis vertex 
  static int lastIsnap = -1;

#if 0
  // If the point is visible, add it to the point buffer.

  float xlen    = plot->wxb - plot->wxa;   // The extent of the X-axis 
  if(xval > 0.0f && xval < xlen) {

    // If this is the first visible point in this line, set the first
    // visible point to the previous one, so that the line will emerge
    // continuously from outside the plot boundary
    
    if(xdesc.isFirst()) {
      xdesc.setOldestVisible(lastIsnap < 0 ? isnap : lastIsnap);
      xdesc.setFirst(false);
    }

    if(ydesc.isFirst()) {
      ydesc.setOldestVisible(lastIsnap < 0 ? isnap : lastIsnap);
      ydesc.setFirst(false);
    }
  }
#endif

  // Start the line?

  if(first) {
    old_xval = 0.0;
    old_yval = 0.0;
    cpgmove(xval, yval);
    first = false;
  } else {
    
    // Use the difference between the current phase and that of the
    // last plotted phase to determine the shortest distance between
    // the two points modulo 360 degrees. This may require the line to
    // be broken into two segments that cross the -180 and 180 degree
    // boundaries.

    double phsdif = yval - old_yval;

    if(phsdif > 180.0) {
      cpgdraw(xval, yval-360.0);
      cpgmove(old_xval, old_yval+360.0);
      cpgdraw(xval, yval);
    } else if(phsdif < -180.0) {
      cpgdraw(xval, yval+360.0);
      cpgmove(old_xval, old_yval-360.0);
      cpgdraw(xval, yval);
    } else {
      cpgdraw(xval, yval);
    };
  };
  
  old_yval = yval;
  old_xval = xval;

  lastIsnap = isnap;
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw a selection of the bits of a register as step-functions.
 *
 * Input:
 *  graph      MonitorGraph *  The graph in which to draw.
 *  oldest             int    The oldest snapshot to draw.
 *  newest             int    The newest snapshot to draw.
 *  x_offset           int    The offset of the x-axis register within
 *                             a buffer frame.
 *  y_offset           int    The offset of the y-axis register within
 *                             a buffer frame.
 */
static void draw_logic_lines(MonitorGraph *graph, int oldest, int newest,
			     int x_offset, int y_offset)
{
  MonitorPlot *plot = graph->plot;      /* The parent plot of the graph */
  MonitorViewer *view = plot->view;     /* The parent viewer of the plot */
  MonitorBuff *mb = view->mb;           /* The snapshot buffer */
  MonitorBuffMap *map = mb->map;        /* The buffer configuration object */
  MpPointBuf *ptbuf = &view->ptbuf;     /* The point plotting buffer */
  int bit;                              /* A bit to be plotted 0..31 */
  int nbuf = 0;                         /* The number points in ptbuf[] */
  /*
   * Draw lines between the values of the register for all snapshots between
   * oldest and newest.
   */
  for(bit=0; bit<32; bit++) {
    if((graph->bits >> bit) & 1U) {
      int isnap;                           /* The snapshot being drawn */
      for(isnap=oldest; isnap <= newest; isnap++) {
	double *frame = mb->data + (isnap % map->max_snap) * map->nregs;
	float xval = frame[x_offset] - plot->wxa;
	int on = ((unsigned) frame[y_offset] >> bit) & 1U;
	float yval = (on ? (bit+0.25):(bit-0.25)) - graph->wya;
	/*
	 * If the value of the bit changed, arrange to draw a vertical line
	 * between the old and new values, at the x-axis value of the current
	 * point.
	 */
	if(nbuf && ptbuf->y[nbuf-1] != yval) {
	  ptbuf->x[nbuf] = xval;
	  ptbuf->y[nbuf] = ptbuf->y[nbuf-1];
	  nbuf++;
	};
	/*
	 * Add the latest point to the line vertex buffer.
	 */
	ptbuf->x[nbuf] = xval;
	ptbuf->y[nbuf] = yval;
	nbuf++;
	/*
	 * Flush the buffer to the display when the buffer has less than one
	 * entry left unfilled, then re-initialize the buffer with the end point
	 * of the last line, so that the next line segment connects to it.
	 */
	if(nbuf >= MP_BUF_NPT - 2) {
	  cpgline(nbuf, ptbuf->x, ptbuf->y);
	  ptbuf->x[0] = xval;
	  ptbuf->y[0] = yval;
	  nbuf = 1;
	};
      };
      /*
       * Flush any remaining buffered line segments to the display.
       */
      if(nbuf > 1) {
	cpgline(nbuf, ptbuf->x, ptbuf->y);
	nbuf = 0;
      };
    };
  };
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw a selection of the bits of a register as step-functions.
 *
 * Input:
 *  graph      MonitorGraph *  The graph in which to draw.
 *  xdesc      RegDescription& The x-register
 *  ydesc      RegDescription& The y-register
 *  isnap      int            The snapshot being plotted
 */
static void drawLogicLine(MonitorGraph *graph, 
			  gcp::util::RegDescription& xdesc, 
			  gcp::util::RegDescription& ydesc,
			  int oldest,
			  int newest,
			  bool yIter)
{
  MonitorPlot *plot = graph->plot;      // The parent plot of the graph 
  MonitorViewer *view = plot->view;     // The parent viewer of the plot 
  MonitorBuff *mb = view->mb;           // The snapshot buffer 
  MonitorBuffMap *map = mb->map;        // The buffer configuration object 
  MpPointBuf *ptbuf = &view->ptbuf;     // The point plotting buffer 
  int bit;                              // A bit to be plotted 0..31 
  int nbuf = 0;                         // The number points in ptbuf[] 
  int x_offset;
  int y_offset;
  
  // Draw lines between the values of the register for all snapshots
  // between oldest and newest.

  for(bit=0; bit<32; bit++) {
    if((graph->bits >> bit) & 1U) {
      
      // Iterate over all snapshots
      
      for(int isnap=oldest; isnap <= newest; isnap++) {
	
	// If iterating over y, reset to the first element now
	
	if(yIter)
	  ydesc.begin();
	
	// Iterate over all elements in this possibly multi-dimensional
	// x-axis
	
	for(xdesc.begin(); !xdesc.isEnd() && !ydesc.isEnd(); ++xdesc) {
	  
	  x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
	  y_offset = map->offset[ydesc.currentSlot()].a[ydesc.aspect()];
	  
	  double *frame = mb->data + (isnap % map->max_snap) * map->nregs;
	  float xval = frame[x_offset] - plot->wxa;
	  int on = ((unsigned) frame[y_offset] >> bit) & 1U;
	  float yval = (on ? (bit+0.25):(bit-0.25)) - graph->wya;

	  // If the value of the bit changed, arrange to draw a
	  // vertical line between the old and new values, at the
	  // x-axis value of the current point.
	  
	  if(nbuf && ptbuf->y[nbuf-1] != yval) {
	    ptbuf->x[nbuf] = xval;
	    ptbuf->y[nbuf] = ptbuf->y[nbuf-1];
	    nbuf++;
	  };
	  
	  // Add the latest point to the line vertex buffer.

	  ptbuf->x[nbuf] = xval;
	  ptbuf->y[nbuf] = yval;
	  nbuf++;
	  
	  // Flush the buffer to the display when the buffer has less
	  // than one entry left unfilled, then re-initialize the
	  // buffer with the end point of the last line, so that the
	  // next line segment connects to it.

	  if(nbuf >= MP_BUF_NPT - 2) {
	    cpgline(nbuf, ptbuf->x, ptbuf->y);
	    ptbuf->x[0] = xval;
	    ptbuf->y[0] = yval;
	    nbuf = 1;
	  };

	  // If iterating over y, increment to the next index now.

	  if(yIter)
	    ++ydesc;
	};
      }

      // Flush any remaining buffered line segments to the display.
      
      if(nbuf > 1) {
	cpgline(nbuf, ptbuf->x, ptbuf->y);
	nbuf = 0;
      };
    };
  };
}

/*.......................................................................
 * This is a private function of draw_MonitorGraph_data(), used to
 * draw the values of a phase register using lines which wrap at -180 and
 * 180 degrees.
 *
 * Input:
 *  graph      MonitorGraph *  The graph in which to draw.
 *  oldest             int    The oldest snapshot to draw.
 *  newest             int    The newest snapshot to draw.
 *  x_offset           int    The offset of the x-axis register within
 *                             a buffer frame.
 *  y_offset           int    The offset of the y-axis register within
 *                             a buffer frame.
 */
static void draw_phase_lines(MonitorGraph *graph, int oldest, int newest,
			     int x_offset, int y_offset)
{
  MonitorPlot *plot = graph->plot;      /* The parent plot of the graph */
  MonitorViewer *view = plot->view;     /* The parent viewer of the plot */
  MonitorBuff *mb = view->mb;           /* The snapshot buffer */
  MonitorBuffMap *map = mb->map;        /* The buffer configuration object */
  int isnap;                           /* The snapshot being drawn */
  int first = 1;        /* True after the line has been started */
  float old_xval = 0.0; /* The previously plotted x-axis vertex */
  float old_yval = 0.0; /* The previously plotted y-axis vertex */
  /*
   * Draw lines between the values of the register for all snapshots between
   * oldest and newest.
   */
  for(isnap=oldest; isnap <= newest; isnap++) {
    double *frame = mb->data + (isnap % map->max_snap) * map->nregs;
    float xval = frame[x_offset] - plot->wxa;
    float yval = frame[y_offset] - graph->wya;
    /*
     * Start the line?
     */
    if(first) {
      cpgmove(xval, yval);
      first = 0;
    } else {
      /*
       * Use the difference between the current phase and that of the last
       * plotted phase to determine the shortest distance between the two
       * points modulo 360 degrees. This may require the line to be broken
       * into two segments that cross the -180 and 180 degree boundaries.
       */
      double phsdif = yval - old_yval;
      if(phsdif > 180.0) {
	cpgdraw(xval, yval-360.0);
	cpgmove(old_xval, old_yval+360.0);
	cpgdraw(xval, yval);
      } else if(phsdif < -180.0) {
	cpgdraw(xval, yval+360.0);
	cpgmove(old_xval, old_yval-360.0);
	cpgdraw(xval, yval);
      } else {
	cpgdraw(xval, yval);
      };
    };
    old_yval = yval;
    old_xval = xval;
  };
}

/*.......................................................................
 * Flag a monitor graph and its contents as needing to be updated. No
 * further plotting will be performed to the graph until
 * update_MonitorGraph(graph) is called.
 *
 * Input:
 *  graph   MonitorGraph *  The graph to be marked for update.
 */
static void flag_MonitorGraph(MonitorGraph *graph)
{
  graph->update_needed = 1;
}

//-----------------------------------------------------------------------
// Methods of MonitorPlot
//-----------------------------------------------------------------------

/*.......................................................................
 * Create a new plot object. This contains a list of graphs to be plotted
 * against a given X-axis.
 *
 * Input:
 *  view   MonitorViewer *  The resource object of the monitor viewer.
 *  device          char *  The PGPLOT specification of the graphics
 *                          device to assign to the plot.
 * Output:
 *  return   MonitorPlot *  The new object, or NULL on error.
 */
static MonitorPlot *new_MonitorPlot(MonitorViewer *view, char *device)
{
  MonitorPlot *plot;  /* The object to be returned */
  /*
   * Check arguments.
   */
  if(!view || !device) {
    lprintf(stderr, "new_MonitorPlot: NULL argument.\n");
    return NULL;
  };
  /*
   * Allocate the container.
   */
  plot = (MonitorPlot* )new_FreeListNode("new_MonitorPlot", view->plot_mem);
  if(!plot)
    return NULL;
  /*
   * Before attempting any operation that might fail, initialize the
   * container at leat up to the point at which it can safely be passed
   * to del_MonitorPlot().,
   */
  plot->xleft = 0.0;
  plot->xright = 0.0;
  plot->marker_size = 1U;
  plot->join = 1;
  plot->regname[0] = '\0';
  memset(plot->title, '\0', AXLAB_LEN);
  memset(plot->xlabel, '\0', AXLAB_LEN);
  plot->tag = view->next_plot_tag++;
  plot->pgplot_id = 0;
  plot->oldest = 0;
  plot->graphs = NULL;
  plot->tail_graph = NULL;
  plot->ngraph = 0;
  plot->next_graph_tag = 1;
  plot->wxa = 0.0;
  plot->wxb = 0.0;
  plot->xlength = 0.0;
  plot->stats.empty = 1;
  plot->stats.xmin = 0.0;
  plot->stats.xmax = 0.0;
  plot->scroll.mode = SCROLL_DISABLED;
  plot->scroll.margin = 50.0;
  plot->scroll.user_fn = 0;
  plot->scroll.user_data = NULL;
  plot->scroll.del_fn = 0;
  plot->xdescs = 0;
  plot->regname_invalid = 0;
  plot->update_needed = 1;

  plot->vxa  = MP_LEFT_MARGIN;
  plot->vxb  = 1.0f - MP_RIGHT_MARGIN;
  plot->vya  = MP_BOT_MARGIN;
  plot->vyb  = 1.0f - MP_TOP_MARGIN;
  plot->view = view;
  plot->next = NULL;

  plot->type = PLOT_NORM;

  plot->powSpec.npt   = 0;
  plot->powSpec.dt    = 0;
  plot->powSpec.dv    = 0;
  plot->powSpec.nel   = 0;
  plot->powSpec.first = true;
  plot->powSpec.updateNeeded = false;
  plot->powSpec.xdataLinear = 0;
  plot->powSpec.xdataLog    = 0;

  plot->powSpec.xdataLinear = new std::vector<float>(0);
  plot->powSpec.xdataLog    = new std::vector<float>(0);

  plot->xdescs = new std::vector<gcp::util::RegDescription>(0);
  
  if(plot->xdescs==0)
    return del_MonitorPlot(plot);

  // Allocate an object to store the time resolution for FFT plots

  plot->powSpec.dt = new gcp::util::TimeVal();
  
  if(plot->powSpec.dt == 0)
    return del_MonitorPlot(plot);

  // Allocate an object to store the frequency resolution for FFT plots

  plot->powSpec.dv = new gcp::util::Frequency();
  
  if(plot->powSpec.dv == 0)
    return del_MonitorPlot(plot);

  // Attempt to open the specified PGPLOT device.

  plot->pgplot_id = cpgopen(device);
  if(plot->pgplot_id <= 0)
    return del_MonitorPlot(plot);
  
  // Turn off new page prompting.

  cpgslct(plot->pgplot_id);
  cpgask(0);
  cpgpage();

  return plot;
}

/*.......................................................................
 * Delete a redundant MonitorPlot object.
 *
 * Input:
 *  plot     MonitorPlot *  The object to be deleted.
 * Output:
 *  return   MonitorPlot *  The deleted object (Always NULL).
 */
static MonitorPlot *del_MonitorPlot(MonitorPlot *plot)
{
  if(plot) {
    MonitorViewer *view = plot->view;
    
    // Discard any scroll callback.
    
    mp_scroll_callback(plot, 0, NULL, 0);
    
    // Close the associated PGPLOT device.
    
    if(plot->pgplot_id > 0) {
      cpgslct(plot->pgplot_id);
      cpgclos();
    };
    
    // Delete the graphs that the plot contained.
    
    while(plot->graphs)
      rem_MonitorGraph(plot->graphs);
    
    if(plot->xdescs != 0) {
      delete plot->xdescs;
      plot->xdescs = 0;
    }

    if(plot->powSpec.dt != 0) {
      delete plot->powSpec.dt;
      plot->powSpec.dt = 0;
    }

    if(plot->powSpec.dv != 0) {
      delete plot->powSpec.dv;
      plot->powSpec.dv = 0;
    }

    if(plot->powSpec.xdataLinear != 0) {
      delete plot->powSpec.xdataLinear;
      plot->powSpec.xdataLinear = 0;
    }

    if(plot->powSpec.xdataLog != 0) {
      delete plot->powSpec.xdataLog;
      plot->powSpec.xdataLog = 0;
    }
    
    // Return the container to the free-list.
    
    plot = (MonitorPlot* )del_FreeListNode("del_MonitorPlot", view->plot_mem, 
					   plot);
  };

  return NULL;
}

/*.......................................................................
 * Add a plot to a monitor viewer.
 *
 * Input:
 *  view  MonitorViewer *  The viewer to add the plot to.
 *  device         char *  The PGPLOT specification of the device
 *                         to assign to the plot.
 * Output:
 *  return  MonitorPlot *  The plot that was added, or NULL on error.
 */
MonitorPlot *add_MonitorPlot(MonitorViewer *view, char *device, char* type)
{
  MonitorPlot *plot;  /* The object to be added to the list */
  
  // Check arguments.

  if(!view || !device) {
    lprintf(stderr, "add_MonitorPlot: NULL argument.\n");
    return NULL;
  };
  
  // Allocate the new plot object.
  
  plot = new_MonitorPlot(view, device);
  if(!plot)
    return NULL;
  
  // Append to the end of the list of plots in the viewer.

  if(view->tail_plot)
    view->tail_plot->next = plot;
  else
    view->plots = plot;

  view->tail_plot = plot;

  return plot;
}

/*.......................................................................
 * Remove a given plot from a monitor viewer and delete it.
 *
 * Input:
 *  plot   MonitorPlot *  The plot to be removed.
 * Output:
 *  return MonitorPlot *  The removed and deleted plot (ie. NULL).
 */
MonitorPlot *rem_MonitorPlot(MonitorPlot *plot)
{
  if(plot) {
    MonitorViewer *view = plot->view; /* The viewer to which the plot belongs */
    MonitorPlot *node;         /* The node being compared against 'plot' */
    MonitorPlot *prev = NULL;  /* The node that precedes 'node' */
    for(node=view->plots; node && node != plot; prev=node,node=node->next)
      ;
    if(!node) {
      lprintf(stderr, "rem_MonitorPlot: Plot not found.\n");
      return NULL;
    };
    /*
     * Remove the plot from the list by linking around it.
     */
    if(prev)
      prev->next = plot->next;
    else
      view->plots = plot->next;
    /*
     * If the plot was at the end of the list, make the tail point
     * at the previous node in the list.
     */
    if(plot == view->tail_plot)
      view->tail_plot = prev;
    /*
     * Delete the plot.
     */
    plot = del_MonitorPlot(plot);
    /*
     * Mark the viewer as needing to have its register selection updated.
     */
    flag_MonitorViewer(view, 0);
  };
  return NULL;
}

/*.......................................................................
 * Reconfigure an existing plot.
 *
 * Input:
 *  plot         MonitorPlot *  The plot to be configured.
 *  title               char *  The title of the plot.
 *  xleft             double    The leftmost limit of the x-axis.
 *  xright            double    The rightmost limit of the x-axis.
 *  marker_size     unsigned    The relative size of a plotted point.
 *  join                 int    If true, join neighboring points with lines.
 *  scroll_mode MpScrollMode    The scrolling mode, chosen from:
 *                                disabled - Disable scrolling.
 *                                maximum  - Scroll to keep the maximum
 *                                           X value visible.
 *                                minimum  - Scroll to keep the mininum
 *                                           X value visible.
 *  scroll_margin     double     The extra amount to scroll past the min
 *                               or max to reduce the frequency of scrolling.
 *                               This is specified as a percentage of the
 *                               X-axis length.
 *  xreg                char *   The specification of the x-axis register.
 *  xlabel              char *   The label to plot below the x-axis.
 * Output:
 *  return               int     0 - OK.
 *                               1 - Error.
 */
int config_MonitorPlot(MonitorPlot *plot, char *title, double xleft,
		       double xright, unsigned marker_size, int join,
		       MpScrollMode scroll_mode, double scroll_margin,
		       char *xreg, char *xlabel,
		       char* type, int npt, double dt, bool linAxis)
{
  // Check arguments.
  
  if(!plot) {
    lprintf(stderr, "config_MonitorPlot: NULL argument.\n");
    return 1;
  };
  
  // Record the type
  
  if(strcasecmp(type, "normal")==0)
    plot->type = PLOT_NORM;
  else {
    plot->type = PLOT_POWSPEC;
  }

  // Is the x-axis register being changed?
  
  if(strcmp(plot->regname, xreg) !=0) {
    
    // Check the syntax of the register specification.
    
    InputStream *stream = plot->view->input;
    
    if(open_StringInputStream(stream, 0, xreg)) {
      lprintf(stderr, "config_MonitorPlot InputStream failure.");
      return 1;
    };
    
    gcp::util::RegParser parser;
    
    try {
      (void)parser.inputReg(stream, true, REG_INPUT_RANGE, true);
    } catch(...) {}
    
    // We only care about the validity if this is a normal plot

    if(plot->type == PLOT_NORM) {
      if(parser.validity() == REG_INVALID){
	close_InputStream(stream);
	return 1;
      };
    }
    
    close_InputStream(stream);
    
    // Record the new register name.
    
    if(strlen(xreg) > MP_REG_LEN) {
      lprintf(stderr, "Register name too long: %s\n", stream->work);
      return 1;
    };
    
    strcpy(plot->regname, xreg);
    
    // Invalidate the previously recorded register-map specification.
    
    plot->xdescs->resize(0);
    plot->regname_invalid = 0;
    
    // Mark the viewer as needing to update the selection of monitored
    // registers.
    
    flag_MonitorViewer(plot->view, 0);
  };
  
  // Record the new title.
  
  strncpy(plot->title, title ? title : "", AXLAB_LEN);
  plot->title[AXLAB_LEN] = '\0';
  
  // Record the npt and dt

  plot->initPowSpec(npt, dt, xleft, xright, linAxis);

  // Sort the limits so that xright > xleft, and widen the range if
  // necessary to accommodate precision restrictions.
  
  vet_axis_limits(&xleft, &xright);
  
  // Install the new range.
  
  plot->xleft   = xleft;
  plot->xright  = xright;
  plot->xlength = xright - xleft;
  plot->wxa     = xleft;
  plot->wxb     = xright;
  
  // Record the new marker size.
  
  plot->marker_size = marker_size > 0 ? marker_size : 1;
  
  // Should lines be drawn between points?
  
  plot->join = join ? 1:0;
  
  // Check and record the new scrolling mode.
  
  switch(scroll_mode) {
  case SCROLL_DISABLED:
    plot->wxa = plot->xleft;
    plot->wxb = plot->xright;
    break;
  case SCROLL_MAXIMUM:
  case SCROLL_MINIMUM:
    break;
  default:
    lprintf(stderr, "config_MonitorPlot: Unknown scrolling mode.\n");
    return 1;
  };
 
  plot->scroll.mode = scroll_mode;
  
  // Record the scrolling margin.
  
  plot->scroll.margin = MP_TICK_BORDER +
    (scroll_margin < MP_MIN_SCROLL ? MP_MIN_SCROLL : (scroll_margin/100.0));
  
  // Record the x-axis label.
  
  strncpy(plot->xlabel, xlabel ? xlabel : "", AXLAB_LEN);
  plot->xlabel[AXLAB_LEN] = '\0';
  
  // Flag the plot as needing to be replotted.
  
  flag_MonitorPlot(plot);

  return 0;
}

/**.......................................................................
 * Identify peaks on a power spectrum plot
 */
int powSpecPkIdent(MonitorGraph* graph, int npk, double xleft, double xright, bool fullRange)
{
  // Check arguments.
  
  if(!graph) {
    lprintf(stderr, "config_MonitorPlot: NULL argument.\n");
    return 1;
  };
  
  if(graph->plot->type != PLOT_POWSPEC)
    return 1;

  graph->powSpec.npk     = npk;
  graph->powSpec.pkIdent = npk > 0;
  graph->powSpec.pkXmin  = xleft;
  graph->powSpec.pkXmax  = xright;
  graph->powSpec.fullXrange = fullRange;

  // Flag the graph as needing to be replotted.
  
  flag_MonitorGraph(graph);

  return 0;
}

/*.......................................................................
 * Delete the graphs of a given plot along with their configurations.
 *
 * Input:
 *  plot  MonitorPlot *  The plot to be modified.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error.
 */
int rem_MonitorPlot_graphs(MonitorPlot *plot)
{
  // Check arguments.
  
  if(!plot) {
    lprintf(stderr, "rem_MonitorPlot_graphs: NULL argument.\n");
    return 1;
  };
  
  // Delete the graphs that the plot contained.
  
  while(plot->graphs)
    rem_MonitorGraph(plot->graphs);
  
  // Mark the parent plot as needing to be redrawn, and mark the
  // viewer as needing to have its register selection updated.
  
  flag_MonitorPlot(plot);
  flag_MonitorViewer(plot->view, 0);
  return 0;
}

/*.......................................................................
 * Limit the given plot to only display subsequently received data.
 *
 * Input:
 *  plot   MonitorPlot *   The plot to be limited.
 * Output:
 *  return         int     0 - OK.
 *                         1 - Error.
 */
int limit_MonitorPlot(MonitorPlot *plot)
{
  MonitorViewer *view;
  if(!plot) {
    lprintf(stderr, "limit_MonitorPlot: NULL plot.\n");
    return 1;
  };
  
  // Record the next snapshot index as the first to be displayed in
  // the redrawn plot.
  
  view = plot->view;
  plot->oldest = (view->mb && view->mb->map) ? (view->mb->map->newest + 1) : 0;
  plot->stats.empty = 1;
  
  // Prevent further plotting until the plot is redrawn.
  
  flag_MonitorPlot(plot);
  return 0;
}

/*.......................................................................
 * Clear the register specifications of a given plot and its graphs.
 *
 * Input:
 *  plot   MonitorPlot *  The plot to be removed.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
static int clr_MonitorPlot_regs(MonitorPlot *plot)
{
  MonitorGraph *graph;
  if(!plot) {
    lprintf(stderr, "clr_MonitorPlot_regs: NULL plot.\n");
    return 1;
  };
  
  // Clear the X-axis register specification.
  
  plot->xdescs->resize(0);
  plot->regname_invalid = 0;
  
  // Clear the Y-axis register specifications of the plot's graphs.
  
  for(graph=plot->graphs; graph; graph=graph->next) {
    MonitorTrace *trace;
    for(trace=graph->traces; trace; trace=trace->next) {
      trace->descs->resize(0);
      trace->regname_invalid = 0;
    };
  };
  
  // Mark the plot as needing to be redrawn, and mark the viewer as
  // needing to have its register selection updated.
  
  flag_MonitorPlot(plot);
  flag_MonitorViewer(plot->view, 0);
  return 0;
}

/*.......................................................................
 * Find a monitor plot of a given viewer via its unique plot
 * identification tag.
 *
 * Input:
 *  view  MonitorViewer *  The viewer that contains the plot.
 *  tag        unsigned    The tag to identify the plot by.
 * Output:
 *  return MonitorPlot *  The located plot, or NULL if not found.
 */
MonitorPlot *find_MonitorPlot(MonitorViewer *view, unsigned tag)
{
  MonitorPlot *plot;  /* The plot being checked */
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "find_MonitorPlot: NULL viewer argument.\n");
    return NULL;
  };
  for(plot=view->plots; plot; plot=plot->next) {
    if(plot->tag == tag)
      return plot;
  };
  lprintf(stderr, "find_MonitorPlot: Plot not found.\n");
  return NULL;
}


/*.......................................................................
 * If needed and currently enabled, scroll the X-axis of the specified
 * plot to make either the min or the max X-axis value received so far
 * lie to the inside of the currently selected scroll edge of the X-axis.
 *
 * Input:
 *  plot   MonitorPlot *  The plot to scroll.
 * Output:
 *  return         int    0 - No scrolling was necessary.
 *                        1 - The plot was scrolled.
 */
static int scroll_MonitorPlot(MonitorPlot *plot)
{
  MpScroll *scroll = &plot->scroll;
  MonitorGraph *graph; /* A graph whose axes are being drawn or erased */
  double shift=0.0;    /* The amount to shift the X axis (world coordinates) */
  float xa, xb, ya, yb;/* The scrolled world coordinates */
  /*
   * Is scrolling currently inappropriate?
   */
  if(plot->update_needed)
    return 0;
  /*
   * Determine the shift needed to bring the specified X value into
   * view (plus a margin).
   */
  switch(scroll->mode) {
  case SCROLL_DISABLED:
    return 0;
  case SCROLL_MAXIMUM:
    if(plot->stats.xmax > plot->wxb - plot->xlength * MP_TICK_BORDER) {
      shift = plot->stats.xmax + plot->xlength * plot->scroll.margin -
	plot->wxb;
    } else {
      return 0;
    };
    break;
  case SCROLL_MINIMUM:
    if(plot->stats.xmin < plot->wxa + plot->xlength * MP_TICK_BORDER) {
      shift = plot->stats.xmin - plot->xlength * plot->scroll.margin -
	plot->wxa;
    } else {
      return 0;
    };
    break;
  };
  /*
   * Double buffer to avoid flicker.
   */
  cpgslct(plot->pgplot_id);
  cpgbbuf();
  /*
   * Erase the current axes.
   */
  for(graph=plot->graphs; graph; graph=graph->next)
    draw_MonitorGraph_axes(graph, 1);
  /*
   * When scrolling is first enabled, the required shift may be greater
   * than the size of the window. In this case simply change the world
   * coordinates and redisplay.
   */
  if(fabs(shift) > fabs(plot->xlength)) {
    update_MonitorPlot(plot, 1);
  } else {
    /*
     * Place a viewport around all of the graphs so that they can be
     * scrolled simultaneously by 'shift' world-coordinates to the right.
     */
    cpgsvp(plot->vxa, plot->vxb, plot->vya, plot->vyb);
    cpgswin(0.0, plot->wxb - plot->wxa, 0.0f, 1.0f);
    cpgscrl(-shift, 0.0f);
    /*
     * Determine what the scrolled world coordinates are.
     */
    cpgqwin(&xa, &xb, &ya, &yb);
    plot->wxb -= xa;
    plot->wxa -= xa;
    /*
     * Redraw the shifted axes.
     */
    for(graph=plot->graphs; graph; graph=graph->next)
      draw_MonitorGraph_axes(graph, 0);
  };
  /*
   * Reveal the result.
   */
  cpgebuf();
  return 1;
}

/*.......................................................................
 * Flag a monitor plot and its contents as needing to be updated. No
 * further plotting will be performed to the plot until
 * update_MonitorPlot(plot) is called.
 *
 * Input:
 *  plot   MonitorPlot *  The plot to be marked for update.
 */
static void flag_MonitorPlot(MonitorPlot *plot)
{
  MonitorGraph *graph;      /* A graph of the plot */
  /*
   * Don't flag the hierarchy redundantly.
   */
  if(!plot->update_needed) {
    plot->update_needed = 1;
    for(graph=plot->graphs; graph; graph=graph->next)
      flag_MonitorGraph(graph);
  };
}

/*.......................................................................
 * Add the registers of a plot to the set of registers to be monitored
 * and buffered.
 *
 * Input:
 *  plot   MonitorPlot *  The plot to accomodate in the register set.
 *  regmap      RegMap *  The register map to use when looking up
 *                        registers.
 *  map MonitorBuffMap *  The buffer map to add the registers to.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
static int add_MonitorPlot_regs(MonitorPlot *plot, ArrayMap *arraymap,
				MonitorBuffMap *map)
{
  int plot_active = 0;  // True if the plot contains any traces 
  
  // Select the Y-axis register ranges.
  
  for(MonitorGraph* graph=plot->graphs; graph; graph=graph->next) {

    for(MonitorTrace* trace=graph->traces; trace; trace=trace->next) {

      if(trace->regname_invalid)
	continue;

      // We are about to add the registers specified in regname.  If
      // the plot was configured before attaching to a source of data,
      // these will not yet have been checked for consistency.  Do it
      // now.

      if(plot->type == PLOT_NORM)
	checkXaxisValidity(plot->regname, trace->regname, map);

      if(trace->addRegs()) {
	return 1;
      }

      if(trace->descs->size() > 0)
	plot_active = 1;
      else
	trace->regname_invalid = 1;

      // If this is a power spectrum plot, check that all registers
      // specified in this trace have the same number of elements

      if(trace->checkRegConsistency()) {
	return true;
      }
    };
  };
  
  // If the plot has any active traces then include the X-axis
  // register of the plot. Doing this prematurely would trigger an
  // unnecessary register-selection update.
  
  if(plot_active && !plot->regname_invalid) {

    // Only add the x-register if this is a normal plot.  Otherwise,
    // we don't use an x-register

    if(plot->type == PLOT_NORM) {

      if(add_MonitorPlot_regs(plot->view, map, plot->regname, plot->xdescs))
	return 1;
      
      if(plot->xdescs->size() == 0)
	plot->regname_invalid = 1;

    }

  };

  return 0;
}

/*.......................................................................
 * This is a private function of add_MonitorPlot_regs(), used to parse
 * the specification of a plotted register and add the resulting register
 * elements to the set of buffered registers to be read from the monitor
 * stream.
 *
 * Input:
 *  view  MonitorViewer *  The parent viewer.
 *  map  MonitorBuffMap *  The buffer map to prepare, or NULL if the
 *                         register is not to be buffered.
 *  regname        char *  The specification of the register.
 * Input/Output:
 *  reg       RegMapReg *  The parsed version of regname[] will be
 *                         assigned to *reg.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static int add_MonitorPlot_reg(MonitorViewer* view, MonitorBuffMap* map,
			       char* regname, gcp::util::RegDescription* desc)
{
  MonitorStream *ms = view->ms;      /* The current data source */
  InputStream *stream = view->input; /* The stream to use to parse regname[] */
  
  if(desc==0)
    return 1;
  
  // Delete any previous register specification.
  
  desc->reset();
  
  // Parse the register name.
  
  if(open_StringInputStream(stream, 0, regname))
    return 1;
  
  gcp::util::RegParser parser(view->ms->archivedOnly_);
  
  try {
    *desc = parser.inputReg(stream, true, REG_INPUT_RANGE, true, map->arraymap);
  } catch(gcp::util::Exception& err) {
    lprintf(stderr, err.what());
  }
  
  if(parser.validity() == REG_VALID){
    if(input_skip_white(stream, 1, 0)) {
      desc->reset();
    } else if(stream->nextc != EOF) {
      lprintf(stderr, "Unexpected trailing characters in register name: %s\n",
	      regname);
      desc->reset();
    } else if(desc->iSlot() < 0) {
      lprintf(stderr, "%s is not an archived register.\n", regname);
    };
  };
  close_InputStream(stream);
  
  // If the register doesn't exist, don't attempt to add it to the
  // register selection set.
  
  if(desc->iSlot() < 0)
    return 0;
  
  // Add the selected registers to the set that is to be provided by
  // the monitor stream.
  
  try {
    ms_prep_RegSet(ms)->addRegister(*desc);
  } catch(...) {
    return 1;
  }
  
  // Add the register elements to the buffer map.
  
  for(desc->begin(); !desc->isEnd(); ++(*desc)) {
    map->offset[desc->currentSlot()].a[desc->aspect()] = 1;

    //    std::cout << "Adding register: " 
    //    	      << desc << "(" << desc->currentSlot() << ", " << desc->aspect() << ") to the buffer map" << std::endl;
  }
  
  return 0;
}

/*.......................................................................
 * This is a private function of add_MonitorPlot_regs(), used to parse
 * the specification of a plotted register and add the resulting register
 * elements to the set of buffered registers to be read from the monitor
 * stream.
 *
 * Input:
 *  view  MonitorViewer *  The parent viewer.
 *  map  MonitorBuffMap *  The buffer map to prepare, or NULL if the
 *                         register is not to be buffered.
 *  regname        char *  The specification of the register.
 * Input/Output:
 *  reg       RegMapReg *  The parsed version of regname[] will be
 *                         assigned to *reg.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static int add_MonitorPlot_regs(MonitorViewer* view, MonitorBuffMap* map,
				char* regname, 
				std::vector<gcp::util::RegDescription>* descs,
				bool split)
{
  MonitorStream *ms = view->ms;      /* The current data source */
  InputStream *stream = view->input; /* The stream to use to parse regname[] */
  
  if(descs==0)
    return 1;
  
  // Delete any previous register specification.
  
  descs->resize(0);
  
  // Parse the register name.
  
  if(open_StringInputStream(stream, 0, regname))
    return 1;
  
  gcp::util::RegParser parser(view->ms->archivedOnly_);
  
  try {

    *descs = parser.inputRegs(stream, true, map->arraymap, 
			      REG_INPUT_RANGE, true, split);

  } catch(gcp::util::Exception& err) {
    lprintf(stderr, err.what());
  }
  
  if(parser.validity() == REG_VALID){
    if(input_skip_white(stream, 1, 0)) {
      descs->resize(0);
    } else if(stream->nextc != EOF) {
      lprintf(stderr, "Unexpected trailing characters in register name: %s\n",
	      regname);
      descs->resize(0);
    } else if(descs->size() == 0) {
      lprintf(stderr, "%s is not an archived register.\n", regname);
    };
  };

  close_InputStream(stream);
  
  // If the register doesn't exist, don't attempt to add it to the
  // register selection set.
  
  if(descs->size() == 0)
    return 0;

  // Add the selected registers to the set that is to be provided by
  // the monitor stream.
  
  try {
    ms_prep_RegSet(ms)->addRegisters(*descs);
  } catch(...) {
    return 1;
  }
  
  // Add the register elements to the buffer map.
  
  for(unsigned iReg=0; iReg < descs->size(); iReg++) {
    gcp::util::RegDescription& desc = descs->at(iReg);
    
    for(desc.begin(); !desc.isEnd(); ++desc) {
      map->offset[desc.currentSlot()].a[desc.aspect()] = 1;
      map->integrationFlags[desc.currentSlot()] = desc.block()->flags_;
    }
  }
  
  return 0;
}

/*.......................................................................
 * Allocate the resource object of the monitor viewer.
 *
 * Input:
 *  buff_size       size_t    The size of the circular buffer that is
 *                            used to record historical register values
 *                            for plotting. The actual amount of memory
 *                            used by the buffer is:
 *
 *                              bytes = buff_size * sizeof(double)
 *
 * Output:
 *  return   MonitorViewer *  The new object, or NULL on error.
 */
MonitorViewer *new_MonitorViewer(size_t buff_size)
{
  MonitorViewer *view;   /* The object to be returned */
  int i;
  /*
   * Allocate the container.
   */
  view = (MonitorViewer *) malloc(sizeof(MonitorViewer));
  if(!view) {
    lprintf(stderr, "new_MonitorViewer: Insufficient memory.\n");
    return NULL;
  };
  /*
   * Before attempting any operation that might fail, initialize the
   * object at least up to the point at which it can safely be passed
   * to del_MonitorViewer().
   */
  view->trace_mem = NULL;
  view->graph_mem = NULL;
  view->plot_mem = NULL;
  view->page_mem = NULL;
  view->field_mem = NULL;
  view->input = NULL;
  view->pages = NULL;
  view->tail_page = NULL;
  view->plots = NULL;
  view->tail_plot = NULL;
  view->mb = NULL;
  view->ms = NULL;
  view->ims = NULL;
  view->imageHandler_ = NULL;
  view->next_page_tag = 1;
  view->next_plot_tag = 1;
  view->defer_display = 0;
  view->update_needed = 1;
  view->send_in_progress = 0;
  view->im_send_in_progress = 0;
  view->interval = 1;
  view->calfile = NULL;
  /*
   * Allocate a free-list from which to allocate trace objects.
   */
  view->trace_mem = new_FreeList("new_MonitorViewer", sizeof(MonitorTrace),
				 TRACE_BLK_FACT);
  if(!view->trace_mem)
    return del_MonitorViewer(view);
  /*
   * Allocate a free-list from which to allocate graph objects.
   */
  view->graph_mem = new_FreeList("new_MonitorViewer", sizeof(MonitorGraph),
				 GRAPH_BLK_FACT);
  if(!view->graph_mem)
    return del_MonitorViewer(view);
  /*
   * Allocate a free-list from which to allocate plot objects.
   */
  view->plot_mem = new_FreeList("new_MonitorViewer", sizeof(MonitorPlot),
				PLOT_BLK_FACT);
  if(!view->plot_mem)
    return del_MonitorViewer(view);
  /*
   * Allocate a free-list from which to allocate page objects.
   */
  view->page_mem = new_FreeList("new_MonitorViewer", sizeof(MonitorPage),
				PAGE_BLK_FACT);
  if(!view->page_mem)
    return del_MonitorViewer(view);
  /*
   * Allocate a free-list from which to allocate field objects.
   */
  view->field_mem = new_FreeList("new_MonitorViewer", sizeof(MonitorField),
				 FIELD_BLK_FACT);
  if(!view->field_mem)
    return del_MonitorViewer(view);
  /*
   * Create an input stream to use when parsing register names.
   */
  view->input = new_InputStream();
  if(!view->input)
    return del_MonitorViewer(view);
  /*
   * Create the snapshot buffer.
   */
  view->mb = new_MonitorBuff(buff_size);
  if(!view->mb)
    return del_MonitorViewer(view);
  /*
   * Create the output stream used to format text field values.
   */
  {
    MpFormatBuf *fmt = &view->fmt;
    fmt->stream = new_OutputStream();
    if(!fmt->stream ||
       open_StringOutputStream(fmt->stream, 1,
			       fmt->string, sizeof(fmt->string)))
      return del_MonitorViewer(view);
  };

  view->imageHandler_ = new gcp::util::ImageHandler();
  if(view->imageHandler_==0)
    return del_MonitorViewer(view);

  /*
   * Initialize the image pgplot device id.
   */
  view->image.pgplot_id = -1;
  for(i=0;i < GRABBER_IM_SIZE;i++)
    view->image.data[i] = 0.0;
  
  return view;
}

/*.......................................................................
 * Delete the resource object of the monitor viewer.
 *
 * Input:
 *  view   MonitorViewer *  The object to be deleted.
 * Output:
 *  return MonitorViewer *  The deleted object (Always NULL).
 */
MonitorViewer *del_MonitorViewer(MonitorViewer *view)
{
  if(view) {
    /*
     * Before deleting the free-lists of containers, delete the lists
     * that they support.
     */
    while(view->pages)
      rem_MonitorPage(view->pages);
    while(view->plots)
      rem_MonitorPlot(view->plots);
    /*
     * Discard the now idle free-lists.
     */
    view->trace_mem = del_FreeList("del_MonitorViewer", view->trace_mem, 1);
    view->graph_mem = del_FreeList("del_MonitorViewer", view->graph_mem, 1);
    view->plot_mem = del_FreeList("del_MonitorViewer", view->plot_mem, 1);
    view->page_mem = del_FreeList("del_MonitorViewer", view->page_mem, 1);
    view->field_mem = del_FreeList("del_MonitorViewer", view->field_mem, 1);
    /*
     * Delete the input stream.
     */
    view->input = del_InputStream(view->input);
    /*
     * Delete the monitor stream.
     */
    view->ms = del_MonitorStream(view->ms);
    /*
     * Delete the image monitor stream.
     */
    view->ims = del_ImMonitorStream(view->ims);
    /*
     * Delete the format buffer.
     */
    view->fmt.stream = del_OutputStream(view->fmt.stream);

    if(view->imageHandler_) {
      delete view->imageHandler_;
      view->imageHandler_ = 0;
    }
    
    /*
     * Finally, delete the container.
     */
    free(view);

  };
  return NULL;
}

/*.......................................................................
 * Flag a monitor viewer and its contents as needing to be updated. No
 * further plotting or data buffering will be performed until
 * update_MonitorViewer(view) is called.
 *
 * Input:
 *  view   MonitorViewer *  The view to be marked for update.
 *  propagate        int    True to flag all child plots and their graphs.
 *                          This should only be true if the cause of the
 *                          flag invalidates all plots.
 */
static void flag_MonitorViewer(MonitorViewer *view, int propagate)
{
  /*
   * Don't flag the hierarchy redundantly.
   */
  if(!view->update_needed) {
    view->update_needed = 1;
    if(propagate) {
      MonitorPlot *plot;
      for(plot=view->plots; plot; plot=plot->next)
	flag_MonitorPlot(plot);
    };
  };
}

/*.......................................................................
 * Delete the current plots and their configurations.
 *
 * Input:
 *  view  MonitorViewer *  The monitor viewer resource object.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
int rem_MonitorViewer_plots(MonitorViewer *view)
{
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "rem_MonitorViewer_plots: NULL argument.\n");
    return 1;
  };
  /*
   * Delete the current list of plots.
   */
  while(view->plots)
    rem_MonitorPlot(view->plots);
  /*
   * Mark the monitor viewer as being inconsistent with the
   * current register selection.
   */
  flag_MonitorViewer(view, 1);
  return 0;
}

/*.......................................................................
 * Clear the register specifications of all plots and their graphs.
 * This must be called whenever the register map is changed.
 *
 * Input:
 *  view  MonitorViewer *  The monitor viewer resource object.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
int clr_MonitorViewer_regs(MonitorViewer *view)
{
  MonitorPage *page;   /* A page being cleared */
  MonitorPlot *plot;   /* A plot that is being cleared */
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "clr_MonitorViewer_regs: NULL argument.\n");
    return 1;
  };
  /*
   * Clear the registers of each page.
   */
  for(page=view->pages; page; page=page->next)
    clr_MonitorPage_regs(page);
  /*
   * Clear the registers of each plot.
   */
  for(plot=view->plots; plot; plot=plot->next)
    clr_MonitorPlot_regs(plot);
  /*
   * Mark the monitor viewer as having an outdated register selection.
   */
  flag_MonitorViewer(view, 1);
  return 0;
}

/*.......................................................................
 * Change the source of monitor data. If the register map of the new
 * source differs from the last source, or an error occurs, the
 * register selections of the current plots and graphs will be
 * discarded.
 *
 * Input:
 *  view  MonitorViewer *  The monitor viewer resource object.
 *  ms    MonitorStream *  The new monitor source, or NULL to just
 *                         delete the current one.
 *  defer           int    If true, defer on-the-fly display while loading
 *                         until the end of the stream is reached. While
 *                         loading the user can still call
 *                         update_MonitorPlot(plot,force=1) to see what
 *                         has been loaded so far.
 * Output:
 *  return          int    0 - No reconfiguration required.
 *                         1 - The register map changed, so the current
 *                             register selections were deleted. No more
 *                             plotting will occur until valid register
 *                             specifications have been received, followed
 *                             by a call to update_MonitorViewer().
 */
int change_MonitorStream(MonitorViewer *view, MonitorStream *ms, int defer)
{
  int changed = 0;  /* True if the new register map differs from the last */
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "change_MonitorStream: NULL argument.\n");
    ms = del_MonitorStream(ms);
    return 1;
  };
  /*
   * See if the current data-source and the new one have compatible
   * register maps.
   */
  if(!view->ms || !ms || !equiv_ArrayMap(ms_ArrayMap(ms), 
					 ms_ArrayMap(view->ms)))
    changed = 1;
  
  // Discard the previous data-source and its calibrations.
  
  view->ms = del_MonitorStream(view->ms);
  mp_set_calfile(view, NULL);
  /*
   * Install the new data-source.
   */
  view->ms = ms;
  view->defer_display = defer;
  view->page_preview = time(NULL);
  view->send_in_progress = 0;
  view->interval = 1;
  /*
   * If the new register map was found to be incompatible, clear the
   * register specifications of the current set of plots and allocate
   * new buffer allocation maps.
   */
  if(changed)
    (void) remap_MonitorBuff(view);
  /*
   * Mark the current configuration as needing updating.
   */
  flag_MonitorViewer(view, 0);
  /*
   * Tell the caller whether the register map changed.
   */
  return changed;
}
/*.......................................................................
 * Change the source of image monitor data. 
 *
 * Input:
 *  view  MonitorViewer *  The monitor viewer resource object.
 *  ims   ImMonitorStream *The new image monitor source, or NULL to just
 *                         delete the current one.
 * Output:
 *  return          int    0 - OK.No reconfiguration required.
 *                         1 - Error.
 */
int change_ImMonitorStream(MonitorViewer *view, ImMonitorStream *ims)
{
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "change_ImMonitorStream: NULL argument.\n");
    ims = del_ImMonitorStream(ims);
    return 1;
  };
  /*
   * Discard the previous data-source.
   */
  view->ims = del_ImMonitorStream(view->ims);
  view->ims = ims;
  /*
   * Install the new (or deleted) stream.
   */
  return 0;
}

/*.......................................................................
 * Return the register map that is currently being used by the
 * monitor viewer. Note that the register map can potentially change
 * whenever change_MonitorStream() is called. If no stream is currently
 * open then NULL will be returned.
 *
 * Input:
 *  view  MonitorViewer *  The resource object of the monitor viewer.
 * Output:
 *  return       RegMap *  The current register map, or NULL if no
 *                         monitor-stream is currently open.
 */
ArrayMap *MonitorViewer_ArrayMap(MonitorViewer *view)
{
  if(!view || !view->ms)
    return NULL;
  return ms_ArrayMap(view->ms);
}

/*.......................................................................
 * Create a buffer in which to cache plotted data so that it can be
 * replotted, searched etc..
 *
 * Input:
 *  size         size_t    The number of register values that can be
 *                         displayed simultaneously.
 * Output:
 *  return  MonitorBuff *  The new buffer, or NULL on error.
 */
static MonitorBuff *new_MonitorBuff(size_t size)
{
  MonitorBuff *mb;   /* The object to be returned */
  /*
   * Check arguments.
   */
  if(size < 1) {
    lprintf(stderr, "new_MonitorBuff: size < 1\n");
    return NULL;
  };
  /*
   * Allocate the container.
   */
  mb = (MonitorBuff *) malloc(sizeof(MonitorBuff));
  if(!mb) {
    lprintf(stderr, "new_MonitorBuff: Insufficient memory.\n");
    return NULL;
  };
  /*
   * Before attempting any operation that might fail, initialize the
   * the container at least up to the point at which it can safely
   * be passed to del_MonitorBuff().
   */
  mb->data = NULL;
  mb->data_dim = size;
  mb->old = NULL;
  mb->map = NULL;
  /*
   * Allocate the buffer.
   */
  mb->data = (double *) malloc(sizeof(double) * mb->data_dim);
  if(!mb->data) {
    lprintf(stderr, "new_MonitorBuff: Insufficient memory for buffer.\n");
    return del_MonitorBuff(mb);
  };
  return mb;
}

/*.......................................................................
 * Delete a monitor data-buffer object.
 *
 * Input:
 *  mb       MonitorBuff *  The object to be deleted.
 * Output:
 *  return   MonitorBuff *  The deleted object (always NULL).
 */
static MonitorBuff *del_MonitorBuff(MonitorBuff *mb)
{
  if(mb) {
    if(mb->data)
      free(mb->data);
    mb->map = del_MonitorBuffMap(mb->map);
    free(mb);
  };
  return NULL;
}

/*.......................................................................
 * Create a monitor-buffer layout object suitable for a given selection
 * of registers.
 *
 * Input:
 *  view    MonitorViewer *  The parent viewer.
 *  ms      MonitorStream *  The stream to which the map pertains.
 * Output:
 *  return MonitorBuffMap *  The new layout object, or NULL on error.
 */
static MonitorBuffMap *new_MonitorBuffMap(MonitorViewer *view)
{
  MonitorStream *ms = view->ms; /* The current data source */
  MonitorBuffMap *map;          /* The new object */
  ArrayMap *arraymap;           /* The current array map of the stream */
  int i,j;
  /*
   * Get the register map of the stream.
   */
  arraymap = ms_ArrayMap(ms);
  /*
   * Would there be too many registers per snapshot to fit a single snapshot
   * in the buffer?
   if(regmap->nreg > view->mb->data_dim) {
   lprintf(stderr, "new_MonitorBuffMap: Register buffer too small.\n");
   return NULL;
   };
  */
  /*
   * Allocate the container.
   */
  map = (MonitorBuffMap *) malloc(sizeof(MonitorBuffMap));
  if(!map) {
    lprintf(stderr, "new_MonitorBuffMap: Insufficient memory.\n");
    return NULL;
  };
  /*
   * Before attempting any operation that might fail, initialize the
   * container at least up to the point at which it can safely be passed
   * to del_MonitorBuffMap().
   */
  map->arraymap = NULL;
  map->max_snap = 0;
  map->nsnap = 0;
  map->oldest = 0;
  map->newest = -1;
  map->arraymap_size = arraymap->nreg;
  for(i=0; i<REG_NASPECT; i++)
    map->nbuf[i] = 0;
  map->regs = NULL;
  map->regs_dim = 0;
  map->nregs = 0;
  map->integrationFlags = NULL;

  /*
   * Allocate an alias of the stream register map.
   */
  map->arraymap = alias_ArrayMap(arraymap);
  if(!map->arraymap)
    return del_MonitorBuffMap(map);
  /*
   * Allocate the array that records details of the currently buffered
   * registers. Allow room for arraymap_size plain registers, and
   * arraymap_size/2 of each of the derived aspects of complex and utc
   * registers.
   */
  //  map->regs_dim = map->arraymap_size + (map->arraymap_size+1)/2 * (REG_NASPECT - 1);

  // My complex registers are now stored as complex types, so each
  // aspect is derived from only a single register.  Thus we will now make
  // the buffer map large enough for arraymap_size * NASPECT

  map->regs_dim = map->arraymap_size * REG_NASPECT;

  map->regs = (MonitorBuffReg *) malloc(sizeof(MonitorBuffReg) * map->regs_dim);
  if(!map->regs) {
    lprintf(stderr, "new_MonitorBuffMap: Insufficient memory.\n");
    return del_MonitorBuffMap(map);
  };
  for(i=0; i<map->regs_dim; i++) {
    MonitorBuffReg *reg = map->regs + i;
    reg->oldest         =  0;
    reg->slot           = -1;
    reg->aspect         =  REG_PLAIN;
    reg->intType        =  REG_NONE;

    reg->lastComplexVal =  0;
    reg->lastDate       =  0;
    reg->lastComplexVal =  new gcp::util::Complex<float>();
    reg->lastDate       =  new gcp::util::RegDate();
    reg->nInt           =  0;
  };
  /*
   * Allocate the array that maps register map elements to buffer regs.
   */
  map->offset = (MonitorBuffOff *) malloc(sizeof(MonitorBuffOff) * map->arraymap_size);
  if(!map->offset) {
    lprintf(stderr, "new_MonitorBuffMap: Insufficient memory.\n");
    return del_MonitorBuffMap(map);
  };
  for(i=0; i<map->arraymap_size; i++) {
    MonitorBuffOff *offset = map->offset + i;
    for(j = 0; j < REG_NASPECT; j++)
      offset->a[j] = -1;
  };

  // Allocate the array the stores the integration type of each
  // register in the map

  map->integrationFlags = 
    (unsigned *) malloc(sizeof(unsigned) * map->arraymap_size);

  if(!map->integrationFlags) {
    lprintf(stderr, "new_MonitorBuffMap: Insufficient memory.\n");
    return del_MonitorBuffMap(map);
  };

  // Default to no integration for any register

  for(i=0; i<map->arraymap_size; i++) 
    map->integrationFlags[i] = REG_NONE;

  return map;
}

/*.......................................................................
 * Fill in a previously allocated buffer-assignment map to match the
 * current register selections.
 *
 * Input:
 *  view    MonitorViewer *  The parent viewer.
 *  map    MonitorBuffMap *  The map to be filled.
 * Output:
 *  return            int    0 - OK.
 *                           1 - An error occured.
 */
static int fill_MonitorBuffMap(MonitorViewer *view, MonitorBuffMap *map)
{
  MonitorStream *ms = view->ms;  /* The current data stream */
  MonitorPlot *plot;             /* A plot being queried for registers */
  MonitorPage *page;             /* A page being queried for registers */
  int i,j;
  
  // Determine the user's selection of registers.
  
  ms_prep_RegSet(ms)->reset();
  
  // Clear any legacy register selections.
  
  for(i=0; i<map->arraymap_size; i++) {
    MonitorBuffOff *offset = map->offset + i;
    for(j = 0; j < REG_NASPECT; j++)
      offset->a[j] = -1;
  };
  
  map->max_snap = 0;
  map->nsnap = 0;
  map->oldest = 0;
  map->newest = -1;
  for(i=0; i<REG_NASPECT; i++)
    map->nbuf[i] = 0;
  map->nregs = 0;
  
  // Add the register selections of all plots to the register set and
  // to the buffer map.
  
  for(plot=view->plots; plot; plot=plot->next) {

    try {
      if(add_MonitorPlot_regs(plot, map->arraymap, map))
	return 1;
    } catch(gcp::util::Exception& err) {
      lprintf(stderr, "%s", err.what());
    }
  };
  
  // Add the register selections of all pages to the register set, but
  // not to the buffer map. Page registers aren't buffered.
  
  for(page=view->pages; page; page=page->next) {
    if(add_MonitorPage_regs(page, map->arraymap))
      return 1;
  };
  
  // Allocate buffer locations to each register that has been marked
  // for buffering. Allocate all of the register locations for one
  // aspect at a time, so that elsewhere we can have tailored loops
  // over register entries for each aspect.
  
  for(i=0; i<REG_NASPECT; i++) {
    int old_nregs = map->nregs;
    for(j=0; j<map->arraymap_size; j++) {
      int *offset = &map->offset[j].a[i];
      if(*offset != -1) {

	MonitorBuffReg *reg = (MonitorBuffReg* )(map->regs + map->nregs);
	reg->slot    = j;
	reg->aspect  = (RegAspect)i;
	reg->nInt    = 0;
	reg->intType = REG_NONE;
	reg->lastComplexVal->initialize();
	reg->lastDate->initialize();

	*offset = map->nregs++;
      };
    };
    map->nbuf[i] = map->nregs - old_nregs;
  };
  
  // Is there enough space to buffer at least one frame?
  
  if(map->nregs > view->mb->data_dim) {
    lprintf(stderr,
	    "The buffer size is too small for the number of registers selected.\n");
    return 1;
  };
  
  // Compute the number of complete snapshots that will fit in the
  // buffer.
  
  map->max_snap = view->mb->data_dim / (map->nregs > 0 ? map->nregs : 1);
  return 0;
}

/*.......................................................................
 * Delete a monitor-buffer layout object.
 *
 * Input:
 *  map    MonitorBuffMap *  The object to be deleted.
 * Output:
 *  return MonitorBuffMap *  The deleted object (always NULL).
 */
static MonitorBuffMap *del_MonitorBuffMap(MonitorBuffMap *map)
{
  if(map) {
    map->arraymap = del_ArrayMap(map->arraymap);
    if(map->offset)
      free(map->offset);
    if(map->integrationFlags)
      free(map->integrationFlags); 
   if(map->regs) {

      // Delete any members which were allocated per MonitorBuffReg

      for(unsigned ireg=0; ireg < map->regs_dim; ireg++) {
	if(map->regs[ireg].lastComplexVal != 0)
	  delete map->regs[ireg].lastComplexVal;
	if(map->regs[ireg].lastDate != 0)
	  delete map->regs[ireg].lastDate;
      }

      free(map->regs);
    }
    free(map);
  };
  return NULL;
}

/*.......................................................................
 * Reconfigure the monitor buffer for a new selection of registers.
 *
 * Input:
 *  view  MonitorViewer * The resource object of the monitor viewer.
 *  ms   MonitorStream *  The stream to configure.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
static int config_MonitorBuff(MonitorViewer *view)
{
  MonitorBuff *mb = view->mb; /* The buffer to be reconfigured */
  MonitorBuffMap *newmap;     /* The new buffer layout object */
  MonitorBuffMap *oldmap;     /* The old buffer layout object */
  MonitorPlot *plot;          /* A plot whose 'oldest' index is being updated */
  int ncommon;               /* The number of registers that are common to */
                              /*  both the old and new buffer maps */
  int i;
  /*
   * Allocate new buffer assignment maps?
   */
  if(!mb->map) {
    if(!(mb->old = new_MonitorBuffMap(view)) ||
       !(mb->map = new_MonitorBuffMap(view))) {
      mb->old = del_MonitorBuffMap(mb->old);
      mb->map = del_MonitorBuffMap(mb->map);
      flag_MonitorViewer(view, 0);
      return 1;
    };
  } else {
    /*
     * Demote the current map, and reuse the previous old map to
     * record the new mapping.
     */
    MonitorBuffMap *tmp_map = mb->map;
    mb->map = mb->old;
    mb->old = tmp_map;
  };
  /*
   * Construct the new buffer-layout map.
   */
  if(fill_MonitorBuffMap(view, mb->map)) {
    mb->map = del_MonitorBuffMap(mb->map);
    mb->old = del_MonitorBuffMap(mb->old);
    flag_MonitorViewer(view, 0);
    return 1;
  };
  /*
   * Get local aliases to the two buffer maps.
   */
  newmap = mb->map;
  oldmap = mb->old;
  /*
   * If either the new or the old maps contain no registers then clearly
   * there are no registers in common.
   */
  if(newmap->nregs < 1 || oldmap->nregs < 1) {
    /*
     * Discard the previously buffered data.
     */
    if(clr_MonitorBuff(view))
      return 1;
    /*
     * If the new map contains any registers then arrange for their values
     * to be loaded by rewinding the stream where possible.
     */
    if(newmap->nregs > 0 && queue_MonitorViewer_rewind(view) == MS_SEND_ERROR)
      return 1;
    return 0;
  };
  /*
   * See if the two buffer mappings contain identical register selections.
   */
  if(newmap->nregs == oldmap->nregs) {
    int identical = 1;
    /*
     * Are the buffered registers identically divided between aspects?
     */
    for(i=0; i<REG_NASPECT; i++) {
      if(oldmap->nbuf[i] != newmap->nbuf[i])
	identical = 0;
    };
    /*
     * Now see if the register entries refer to the same registers.
     */
    if(identical) {
      for(i=0; i<oldmap->nregs; i++) {
	if(oldmap->regs[i].slot != newmap->regs[i].slot)
	  identical = 0;
      };
    };
    /*
     * If the two maps contain identical register selections then
     * reinstate the old map (the old map also records other things, which
     * would otherwise have to be copied to the new map).
     */
    if(identical) {
      MonitorBuffMap *tmp_map = mb->map;
      mb->map = mb->old;
      mb->old = tmp_map;
      return 0;
    };
  };
  /*
   * Position the first of the new frames just after the last of the old frames.
   * Note that the buffer is treated as circular, so this gives the most room
   * to copy legacy frames before potentially overwriting old frames.
   */
  newmap->oldest = (oldmap->nregs * ((oldmap->newest+1) % oldmap->max_snap) +
		    newmap->nregs - 1) / newmap->nregs;
  /*
   * Determine how many of the legacy frames can be copied to new frames
   * (starting at newmap->oldest), such that the last new frame can be
   * written without impinging on the last old frame. This ensures that
   * none of the legacy frames that are to be copied are overwritten by
   * the copies of older frames before they themselves can be copied.
   */
  newmap->nsnap = (mb->data_dim - oldmap->nregs) / newmap->nregs;
  /*
   * Limit the number of snapshots to the number of legacy frames that are
   * actually available.
   */
  if(newmap->nsnap > oldmap->nsnap)
    newmap->nsnap = oldmap->nsnap;
  /*
   * Compute the frame index of the new frame that will correspond to
   * the newest legacy frame.
   */
  newmap->newest = newmap->oldest + newmap->nsnap - 1;
  /*
   * If no legacy frames can be copied, discard the old data.
   */
  if(newmap->nsnap < 1)
    return clr_MonitorBuff(view);
  /*
   * Determine the oldest buffered snapshot of each of the newly selected
   * registers.
   */
  for(i=0; i<newmap->nregs; i++) {
    MonitorBuffReg *newreg = newmap->regs + i;
    int old_offset = oldmap->offset[newreg->slot].a[newreg->aspect];
    if(old_offset < 0) {
      newreg->oldest = newmap->newest + 1;
    } else {
      MonitorBuffReg *oldreg = oldmap->regs + old_offset;
      int nold = (oldmap->newest - oldreg->oldest);
      if(nold <= newmap->nsnap)
	newreg->oldest = newmap->newest - nold;
      else
	newreg->oldest = newmap->oldest;
    };
  };
  /*
   * Some of the newly selected registers may have also been recorded in
   * the previous selection. If so, then we must preserve as much as
   * possible of their historical values by copying them into the newly
   * dimensioned data array. In order to do this we need a map of where
   * each of these registers appear in the new buffer layout. In order not
   * to have to allocate a special re-mapping array, the oldmap->regs[]
   * array will be reused. Variable 'ncommon' will record the
   * number of registers that are the same in the old and new selections
   * and for each of these registers oldmap->regs[].oldest will record the
   * the original buffer-frame offset and .slot will record the new
   * buffer-frame offset.
   */
  for(i=0,ncommon=0; i<oldmap->nregs; i++) {
    MonitorBuffReg *oldreg = oldmap->regs + i;
    int newoff = newmap->offset[oldreg->slot].a[oldreg->aspect];
    if(newoff >= 0) {
      oldmap->regs[ncommon].oldest = i;
      oldmap->regs[ncommon].slot = newoff;
      ncommon++;
    };
  };
  /*
   * If the stream can be rewound and there are previously unbuffered
   * registers in the new map, rewind the stream to reread the data.
   */
  if(ncommon < newmap->nregs && can_MonitorViewer_rewind(view)) {
    /*
     * Discard the previously buffered data before rewinding so that
     * we don't read duplicate data.
     */
    if(clr_MonitorBuff(view) ||
       queue_MonitorViewer_rewind(view) == MS_SEND_ERROR)
      return 1;
    /*
     * If the old and new mappings have no registers in common, discard the old
     * data.
     */
  } else if(ncommon < 1) {
    if(clr_MonitorBuff(view))
      return 1;
    /*
     * Copy the legacy values of common registers to their new
     * buffer-frames.
     */
  } else {
    int old_snap;  /* The snapshot index of the original frame */
    int new_snap;  /* The snapshot index of the destination frame */
    for(old_snap=oldmap->newest - newmap->nsnap + 1, new_snap=newmap->oldest;
	old_snap <= oldmap->newest;
	old_snap++, new_snap++) {
      /*
       * Get pointers to the source and destination frames.
       */
      double *old_frame = mb->data +
	(old_snap % oldmap->max_snap) * oldmap->nregs;
      double *new_frame = mb->data +
	(new_snap % newmap->max_snap) * newmap->nregs;
      /*
       * Copy common registers from the old frame to the new frame.
       */
      for(i=0; i<ncommon; i++)
	new_frame[oldmap->regs[i].slot] = old_frame[oldmap->regs[i].oldest];
    };
    /*
     * Convert the 'oldest' indexes of each plot to the new numbering scheme.
     */
    for(plot=view->plots; plot; plot=plot->next) {
      int nold = (oldmap->newest - plot->oldest);
      if(nold <= newmap->nsnap)
	plot->oldest = newmap->newest - nold;
      else
	plot->oldest = newmap->oldest;
    };
    /*
     * Refine the plot->oldest indexes of each plot so that they don't
     * point to data that precedes the first buffered point of the traces
     * in their child graphs.
     */
    for(plot=view->plots; plot; plot=plot->next)
      refine_MonitorPlot_oldest(plot);
  };
  return 0;
}

/*.......................................................................
 * This is a private function of config_MonitorBuff().
 *
 * If the x-axis register of a plot is buffered further back in time than
 * any of the y-axis registers of any of its graphs then plot->oldest
 * may point unnecessarily far back in the buffer. Check for this condition
 * and correct plot->oldest if needed.
 *
 * Input:
 *  plot    MonitorPlot *  The plot to be sanitized.
 */
static int refine_MonitorPlot_oldest(MonitorPlot *plot)
{
  int oldest = 0;     // The index of the oldest pertinent buffer record 
  int first = 1;       // True until 'oldest' has been initialized 
  
  // Get the buffer layout map.
  
  MonitorViewer *view = plot->view;    // The parent viewer of the plot 
  MonitorBuffMap *map = view->mb->map; // The buffer configuration object 
  
  // Find the oldest buffer index that is pertinent to the traces of
  // all of the graphs in the specified plot.
  
  for(MonitorGraph* graph=plot->graphs; graph; graph=graph->next) {
    
    for(MonitorTrace* trace=graph->traces; trace; trace=trace->next) {
      
      for(unsigned iReg=0; iReg < trace->descs->size(); iReg++) {
	
	gcp::util::RegDescription& desc = trace->descs->at(iReg);
	
	if(desc.iSlot() >= 0) {
	  
	  for(desc.begin(); !desc.isEnd(); ++desc) {

	    int y_offset = 
	      map->offset[desc.currentSlot()].a[desc.aspect()];
	    int y_oldest = map->regs[y_offset].oldest;

	    if(first || y_oldest < oldest) {
	      oldest = y_oldest;
	      first = 0;
	    };

	  };
	};
      };
    };
  };
  
  // We can't plot anything older than the oldest buffered value of
  // the x-axis register.
  
  for(unsigned iReg=0; iReg < plot->xdescs->size(); iReg++) {
	
    gcp::util::RegDescription& xdesc = plot->xdescs->at(iReg);
	
    if(xdesc.iSlot() >= 0) {

      for(xdesc.begin(); !xdesc.isEnd(); ++xdesc) {
	int x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
	int x_oldest = map->regs[x_offset].oldest;
	if(x_oldest > oldest)
	  oldest = x_oldest;
      }
    };
  }

  // If plot->oldest points unnecessarily far back in the buffer, make
  // it point to the first record that is used by any of the child
  // graphs of the plot.
  
  if(oldest > plot->oldest)
    plot->oldest = oldest;
  
  return 0;
}

/*.......................................................................
 * It resets the buffer indexes, and plot->oldest indexes to discard
 * legacy data.
 *
 * Input:
 *  view    MonitorViewer *  The resource container of the monitor viewer.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
int clr_MonitorBuff(MonitorViewer *view)
{
  MonitorBuffMap *map;
  MonitorPlot *plot;
  int i;
  /*
   * Check the arguments.
   */
  if(!view) {
    lprintf(stderr, "clr_MonitorBuff: NULL argument(s).\n");
    return 1;
  };
  /*
   * Get the map to be cleared.
   */
  map = view->mb ? view->mb->map : NULL;
  /*
   * If there is no buffer-map at the moment, nothing needs to be done
   * because the next time that a buffer-map is created, it will automatically
   * cleared.
   */
  if(!map)
    return 0;
  /*
   * Reset the buffer-map to represent an empty buffer.
   */
  map->oldest = 0;
  map->newest = -1;
  map->nsnap = 0;
  for(i=0; i<map->nregs; i++)
    map->regs[i].oldest = 0;
  /*
   * Reset the start times of each plot.
   */
  for(plot=view->plots; plot; plot=plot->next)
    limit_MonitorPlot(plot);
  return 0;
}

/*.......................................................................
 * Read part or all of the next snapshot of register values. If a
 * complete snapshot is received, record it in the monitor buffer
 * and display it.
 *
 * Input:
 *  view   MonitorViewer *  The resource container of the monitor viewer.
 * Output:
 *  return   MsReadState    The status of the operation, from:
 *                            MS_READ_ENDED - The end of the stream
 *                                            was reached before reading
 *                                            another register frame.
 *                            MS_READ_AGAIN - Call this function again
 *                                            to complete the transaction.
 *                            MS_READ_BREAK - This is a warning that the
 *                                            next call may return
 *                                            MS_READ_REGMAP.
 *                            MS_READ_REGMAP- The register map has changed
 *                                            and all register selections
 *                                            have been cleared. The new
 *                                            register map can be queried
 *                                            by calling MonitorViewer_RegMap().
 *                            MS_READ_DONE  - The transaction completed.
 */
MsReadState read_MonitorViewer_frame(MonitorViewer *view)
{
  MonitorStream *ms;    /* The monitor data stream */
  MonitorBuff *mb;      /* The monitor buffer */
  MonitorPage *page;    /* The plot being processed */
  MonitorPlot *plot;    /* The plot being processed */
  MonitorBuffMap *map;  /* The buffer-layout configuration object */
  MsReadState state;    /* The monitor-stream input-completion status */
  int read_size;       /* The max number of frames to read during this call */
  int nread;           /* The number of frames read */
  int i;
  
  // Check arguments.

  if(!view) {
    lprintf(stderr, "read_MonitorViewer_frame: NULL argument.\n");
    return MS_READ_ENDED;
  };
  
  // Get a local alias for the monitor-viewer input stream.

  ms = view->ms;

  if(!ms)
    return MS_READ_ENDED;

  // Get the buffer map.

  mb = view->mb;
  map = mb->map;
  
  // If no registers have been selected yet, read and discard the
  // frame.

  if(!map) {
    return ms_read_frame(ms, 0);
  }
  
  // Determine the max number of frames to read in this call.

  read_size = MP_READ_SIZE;
  if(read_size >= map->max_snap)
    read_size = map->max_snap/2;
  if(read_size < 1)
    read_size = 1;

  // Erase any plotted data whose locations in the ring buffer will be
  // overwritten by this read
  
  view->eraseOldDataIfNecessary(read_size);

  // Read data from the input stream until either read_size frames
  // have been read, the input stream blocks, or the register map
  // changes.

  state = MS_READ_AGAIN;

  for(nread=0; nread < read_size &&
	(state=ms_read_frame(ms, 0)) == MS_READ_DONE; nread++) {
    
    // Copy the newly read registers into the buffer, calibrating as
    // we go

    view->updateCalibratedRegisters();
  }

  // If we encountered a new register map, reconfigure the buffer to
  // reflect the contents and ordering of the new register map. Note
  // that because MS_READ_BREAK is always returned before
  // MS_READ_REGMAP and MS_READ_BREAK will abort the above loop, we
  // are guaranteed that MS_READ_REGMAP will be returned on the first
  // iteration of the above loop so nread should be 0.
  
  if(state == MS_READ_REGMAP) {
    (void) remap_MonitorBuff(view);
    if(view->calfile)
      (void) ms_load_cal_file(view->ms, "", view->calfile);
    return state;
  };

  // Plot the new snapshots if on-the-fly display is enabled and at
  // least one new frame was read.

  if(nread > 0 && !view->defer_display) {

    // Update any plots

    view->updatePlots(nread);

    // Update text display pages to show the values of the last
    // complete frame that was received.

    view->updatePages();
  };

  // When on-the-fly display is disabled display the loaded data when
  // the end of the stream is reached.

  if(state == MS_READ_ENDED && view->defer_display) {
    gcp::util::RegCal::RegCalData *cal = ms_RegCalData(ms);
    if(!cal->isEmpty()) {
      for(page=view->pages; page; page=page->next)
	draw_MonitorPage(page, cal, 0);
    };
    for(plot=view->plots; plot; plot=plot->next)
      update_MonitorPlot(plot, 1);
  };

  // When on-the-fly display is disabled update monitor pages just
  // before the register map changes (provided that anything has been
  // read since the previous register map change). Also update them
  // every MP_PAGE_SYNC seconds.

  if(view->defer_display && (state == MS_READ_BREAK ||
			     page_preview_pending(view))) {
    gcp::util::RegCal::RegCalData *cal = ms_RegCalData(ms);
    if(!cal->isEmpty()) {
      for(page=view->pages; page; page=page->next)
	draw_MonitorPage(page, cal, 0);
    };
    view->page_preview = time(NULL);
  };

  return state;
}

/*.......................................................................
 * Read part or all of the next frame grabber frame. If a
 * complete frame is received, record it in the monitor buffer
 * and display it.
 *
 * Input:
 *  view   MonitorViewer *  The resource container of the monitor viewer.
 * Output:
 *  return   MsReadState    The status of the operation, from:
 *                            MS_READ_ENDED - The end of the stream
 *                                            was reached before reading
 *                                            another register frame.
 *                            MS_READ_AGAIN - Call this function again
 *                                            to complete the transaction.
 *                            MS_READ_BREAK - This is a warning that the
 *                                            next call may return
 *                                            MS_READ_REGMAP.
 *                            MS_READ_REGMAP- The register map has changed
 *                                            and all register selections
 *                                            have been cleared. The new
 *                                            register map can be queried
 *                                            by calling MonitorViewer_RegMap().
 *                            MS_READ_DONE  - The transaction completed.
 */
ImsReadState read_MonitorViewer_image(MonitorViewer *view)
{
  ImMonitorStream *ims; /* The image monitor data stream */
  ImsReadState state;   /* The image monitor-stream input-completion status */
  unsigned short *image;
  int i;

  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "read_MonitorViewer_image: NULL argument.\n");
    return IMS_READ_ENDED;
  };
  /*
   * Get a local alias for the image monitor-viewer input stream.
   */
  ims = view->ims;
  if(!ims)
    return IMS_READ_ENDED;

  /*
   * Read the image.
   */
  state = ims_read_image(ims, 0);
  image = ims_get_image(ims);

  /*
   * And copy the new image into the display buffer.
   */
  unsigned indFrom, indTo, ix, iy;
  for(iy=0; iy < GRABBER_YNPIX; iy++) {
    for(ix=0; ix < GRABBER_XNPIX; ix++) {
      indFrom = iy * GRABBER_XNPIX + ix;
      indTo   = (GRABBER_YNPIX - iy - 1) * GRABBER_XNPIX + ix;
      view->image.data[indTo] = (float) image[indFrom];
    }
  }

  view->imageHandler_->installNewImage(image);

  /*
   * If we finished reading an image on this call, update the image display.
   */
  if(state==IMS_READ_DONE) {
    init_MonitorImage(&view->image);
    draw_MonitorImage_data(view);
  }
  
  return state;
}

/*.......................................................................
 * Return true if it is time to update text pages while loading data
 * with defered display enabled.
 *
 * Input:
 *  view  MonitorViewer *  The monitor viewer resource object.
 * Output:
 *  return          int    0 - There is no need to update.
 *                         1 - It is time to update text pages.
 */
static int page_preview_pending(MonitorViewer *view)
{
  time_t now = time(NULL);
  return now >= 0 && view->page_preview >= 0 &&
    difftime(now, view->page_preview) >= MP_PAGE_SYNC;
}

/*.......................................................................
 * After traces, plots, or graphs have been modified, added or removed,
 * this function should be called to reconfigure the viewer. This
 * includes initiating an update of the MonitorStream register set,
 * clearing and reconfiguring the plot buffer, recomputing graph
 * viewports, clearing the plot pages and drawing new axes. If changes
 * have not been made, then this function does nothing.
 *
 * Input:
 *  view  MonitorViewer *  The monitor viewer resource object.
 * Output:
 *  return  MsSendState    The state of the transaction, from:
 *                           MS_SEND_ERROR - An unrecoverable error occurred.
 *                           MS_SEND_AGAIN - To complete the transaction,
 *                                           call send_MonitorViewer_msg(view).
 *                           MS_SEND_DONE  - The transaction is complete.
 */
MsSendState update_MonitorViewer(MonitorViewer *view)
{
  MonitorPlot *plot;  /* A plot being updated */
  MsSendState state;  /* The status of the monitor-stream regset message */
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "update_MonitorViewer: NULL argument.\n");
    return MS_SEND_ERROR;
  };
  /*
   * Do nothing if no monitor stream is currently available.
   */
  if(!view->ms)
    return MS_SEND_DONE;
  /*
   * Do we need to reconfigure?
   */
  if(view->update_needed) {
    /*
     * Reconfigure the plot buffer for the plot register selections.
     */
    if(config_MonitorBuff(view))
      return MS_SEND_ERROR;
    /*
     * Initiate sending the new register set to the monitor stream.
     */
    state = ms_queue_regset(view->ms);
    /*
     * The local configuration update is complete.
     */
    view->update_needed = 0;
    /*
     * See whether the monitor-stream transaction completed, or whether
     * more sends will be needed.
     */
    switch(state) {
    case MS_SEND_ERROR:
      flag_MonitorViewer(view, 1); /* Mark the viewer as invalid */
      return MS_SEND_ERROR;
      break;
    case MS_SEND_AGAIN:         /* More sends will be required */
      view->send_in_progress = 1;
      break;
    case MS_SEND_DONE:          /* The register set has been sent */
      break;
    };
  } else {
    state = view->send_in_progress ? MS_SEND_AGAIN : MS_SEND_DONE;
  };
  /*
   * Update the plots of the viewer.
   */
  for(plot=view->plots; plot; plot=plot->next)
    update_MonitorPlot(plot, 0);

  return state;
}

/*.......................................................................
 * Redraw a monitor graph without erasing other graphs on the same plot.
 *
 * Input:
 *  graph   MonitorGraph *   The graph to be re-drawn.
 *  force            int     If true redraw the graph, even if it hasn't
 *                           been marked as being out of date.
 * Output:
 *  return           int     0 - OK.
 *                           1 - Error.
 */
int update_MonitorGraph(MonitorGraph *graph, int force)
{
  MonitorBuffMap *map; /* The container of buffered monitor data */
  MonitorPlot *plot;   /* The parent plot of the graph */

  // Check the arguments.

  if(!graph) {
    lprintf(stderr, "update_MonitorGraph: NULL argument.\n");
    return 1;
  };
  
  // Get the parent plot of the graph.

  plot = graph->plot;
  
  // Do nothing if the parent plot also needs to be updated.

  if(plot->update_needed || plot->view->update_needed)
    return 0;

  // Is a redraw actually needed?

  if(!force && !graph->update_needed)
    return 0;
  
  // Direct PGPLOT output to the parent plot.

  cpgslct(plot->pgplot_id);
  cpgbbuf();
  
  // If the graph has already been drawn, clear the area of the graph
  // by overdrawing it with a rectangle that is filled with the
  // background color.  Note that the rectangle must erase axis labels
  // as well as the graph because the X and Y axis labels may have
  // changed.

  if(graph->started) {
    cpgsfs(PGCFS);
    cpgsci(0);
    if(!graph->next)
      cpgsvp(0.0f, graph->vxb, 0.0f, graph->vyb);
    else
      cpgsvp(0.0f, graph->vxb, graph->vya, graph->vyb);
    cpgswin(0.0f, 1.0f, 0.0f, 1.0f);
    cpgrect(0.0f, 1.0f, 0.0f, 1.0f);
  };
  
  // Get the monitor buffer layout configuration.

  map = graph->plot->view->mb->map;

  if(!map) {
    cpgebuf();
    return 0;
  };
  
  // Mark the graph as up to date to prevent draw_MonitorGraph_axes()
  // and draw_MonitorGraph_data() from complaining. The update_needed
  // flag will be reasserted if an error occurs.

  graph->update_needed = 0;
  graph->started = 1;

  drawMonitorGraphData(graph, map->oldest, map->newest, 0);
  draw_MonitorGraph_axes(graph, 0);
  
  // Re-establish the coordinate system used by external cursor
  // functions.

  cpgsvp(0.0f, 1.0f, 0.0f, 1.0f);
  cpgswin(0.0f, 1.0f, 0.0f, 1.0f);
  
  // Reveal the updated graph.

  cpgebuf();

  return 0;
}

/*.......................................................................
 * Return the autoscale Y-axis range for the current X-axis range.
 *
 * Input:
 *  graph  MonitorGraph *   The graph to query.
 * Input/Output:
 *  ymin,ymax    double *   The requested Y-axis range.
 * Output:
 *  return          int     0 - OK.
 *                          1 - Error.
 */
int auto_MonitorGraph_yrange(MonitorGraph *graph, double *ymin, double *ymax)
{
  double min,max;         /* The measured range of Y values */
  MonitorRegStats stats;  /* The statistics of one recorded register */
  MonitorPlot *plot;      /* The parent plot of the specified graph */
  MonitorTrace *trace;    /* A child plot-trace of the specified graph */
  int first;              /* True until the first trace has been processed */
  
  // Check the arguments.
  
  if(!graph || !ymin || !ymax) {
    lprintf(stderr, "auto_MonitorGraph_yrange: NULL argument(s).\n");
    return 1;
  };
  
  // Get the parent plot of the specified graph.
  
  plot = graph->plot;
  
  // If the viewer isn't configured, return the current axis limits.
  
  if(plot->view->update_needed) {
    *ymin = graph->wya;
    *ymax = graph->wyb;
  };
  
  // Get the min and max of all the traces of the specified graph.
  
  min = max = 0;
  first = 1;
  
  // If we are plotting logic levels, simply determine the range of
  // bit levels selected for plotting.
  
  if(graph->bits) {
    int bit;
    for(bit=0; bit<32; bit++) {
      if((graph->bits >> bit) & 1U) {
	if(first) {
	  first = 0;
	  min = max = bit;
	} else if(bit < min) {
	  min = bit;
	} else if(bit > max) {
	  max = bit;
	};
      };
    };
    
    // The high and low values of each bit are repsented by the bit
    // number plus or minus the bit number.
    
    min -= 0.25;
    max += 0.25;
    
    // Get the range of register values plotted in the graph.
    
  } else {
    
    for(trace=graph->traces; trace; trace=trace->next) {
      
      // Get statistics for this trace

      if(mpTraceRegStats(plot, trace, plot->wxa, plot->wxb, &stats))
	return 1;
	    
      if(first) {
	first = false;
	min = stats.min;
	max = stats.max;
      } else {

	if(stats.min < min)
	  min = stats.min;
	if(stats.max > max) 
	  max = stats.max;
      };
    };
  };
  
  // If nothing has been plotted yet, simply return the current plot
  // limits.
  
  if(first) {
    *ymin = graph->wya;
    *ymax = graph->wyb;
    
    // Add a margin to prevent the lowest and highest points from
    // being occulted by the frame of the graph.
    
  } else {
    double margin = (max-min) * MP_Y_MARGIN;
    *ymin = min - margin;
    *ymax = max + margin;
  };
  
  // Widen the range if necessary to accomodate precision
  // restrictions.
  
  vet_axis_limits(ymin, ymax);
  
  return 0;
}

/*.......................................................................
 * Redraw a monitor plot.
 *
 * Input:
 *  plot   MonitorPlot *   The plot to be re-drawn.
 *  force          int     If true redraw the plot, even if it hasn't
 *                         been marked as being out of date.
 * Output:
 *  return         int     0 - OK.
 *                         1 - Error.
 */
int update_MonitorPlot(MonitorPlot *plot, int force)
{
  MonitorGraph *graph;              /* A graph being redrawn */
  
  // Check the arguments.

  if(!plot) {
    lprintf(stderr, "update_MonitorPlot: NULL argument.\n");
    return 1;
  };
  
  // Do nothing if the plot has not been assigned an archivable X-axis
  // register, or the parent viewer needs to be updated.

  if(plot->updateNeeded()) {
    
    cpgslct(plot->pgplot_id);
    
    cpgsvp(0.0f, 1.0f, 0.0f, 1.0f);
    cpgswin(0.0f, 1.0f, 0.0f, 1.0f);
    cpgsch(1.2f);
    cpgsci(5);

    if(plot->graphs == 0)
      cpgptxt(0.5f, 0.5f, 0.0f, 0.5f,
	      "Please use the Configure menu to add a graph.");
    
    return 0;
  }
  
  // Do we actually need to redraw the whole plot?

  if(force || plot->update_needed) {
    
    // Compute the buffered range of the X-axis register and record it
    // in plot->stats.
    
    if(get_MonitorPlot_stats(plot))
      return 1;
    
    // Set up the axis limits.
    
    if(!plot->stats.empty) {
      switch(plot->scroll.mode) {
      case SCROLL_DISABLED:
	break;
      case SCROLL_MAXIMUM:
	{
	  double offset = plot->stats.xmax +
	    plot->xlength * plot->scroll.margin - plot->xright;
	  plot->wxa = plot->xleft  + offset;
	  plot->wxb = plot->xright + offset;
	};
	break;
      case SCROLL_MINIMUM:
	{
	  double offset = plot->stats.xmin -
	    plot->xlength * plot->scroll.margin - plot->xleft;
	  plot->wxa = plot->xleft  + offset;
	  plot->wxb = plot->xright + offset;
	};
	break;
      };
    };
    
    // Direct PGPLOT output to the target plot.
    
    cpgslct(plot->pgplot_id);
    cpgbbuf();
    
    // Clear the plot by drawing over it in black. This is buffered
    // whereas pgpage isn't, so it avoids update-flicker.
    
    cpgsfs(PGCFS);
    cpgsci(0);
    cpgsvp(0.0f, 1.0f, 0.0f, 1.0f);
    cpgswin(0.0f, 1.0f, 0.0f, 1.0f);
    cpgrect(0.0f, 1.0f, 0.0f, 1.0f);
    
    // Mark the plot as up to date.
    
    plot->update_needed = 0;
    
    // Draw the title of the plot.
    
    draw_MonitorPlot_title(plot, 0);
    
    // Calculate the viewports of each graph and mark each graph as
    // undrawn.
    
    if(plot->graphs) {
      int igraph;           // The sequential number of a graph 
      float dy;             // The vertical size of each graph 
      
      // Give each graph 1/plot->ngraph times the vertical extent of
      // the plot viewport.
      
      dy = (plot->vyb - plot->vya) / plot->ngraph;
      
      // Tile the viewports vertically, starting from the top of the
      // viewport.
      
      for(igraph=0,graph=plot->graphs; graph; graph=graph->next, igraph++) {
	graph->vxa = plot->vxa;
	graph->vxb = plot->vxb;
	graph->vya = plot->vyb - (igraph+1) * dy;
	graph->vyb = plot->vyb - igraph * dy;
	graph->started = 0;
	flag_MonitorGraph(graph);
      };
      
      // Redraw the individual graphs.
      
      for(graph=plot->graphs; graph; graph=graph->next) {
	if(update_MonitorGraph(graph, force)) {
	  cpgebuf();
	  return 1;
	};
      };

      // No graphs defined.
      
    } else {
      
      // Erase the title (if any)
      
      draw_MonitorPlot_title(plot, 1);
      
      cpgsvp(0.0f, 1.0f, 0.0f, 1.0f);
      cpgswin(0.0f, 1.0f, 0.0f, 1.0f);
      cpgsch(1.2f);
      cpgsci(5);
      cpgptxt(0.5f, 0.5f, 0.0f, 0.5f,
	      "Please use the Configure menu to add a graph.");
    };
    
    // Re-establish the coordinate system used by external cursor
    // functions.
    
    cpgsvp(0.0f, 1.0f, 0.0f, 1.0f);
    cpgswin(0.0f, 1.0f, 0.0f, 1.0f);
    
    // Reveal the updated plot.
    
    cpgebuf();
  } else {
    
    // Even if the plot isn't marked for update, one of more of its
    // graphs may be.
    
    cpgslct(plot->pgplot_id);
    cpgbbuf();
    for(graph=plot->graphs; graph; graph=graph->next) {
      if(update_MonitorGraph(graph, force)) {
	cpgebuf();
	return 1;
      };
    };
    cpgebuf();
  };
  return 0;
}

/*.......................................................................
 * Draw the title of a monitor plot.
 *
 * Input:
 *  plot   MonitorPlot *  The plot being drawn.
 *  erase          int    True to erase, false to plot.
 */
static void draw_MonitorPlot_title(MonitorPlot *plot, int erase) {
  /*
   * Do nothing if the plot isn't up to date with its configuration.
   */
  if(plot->update_needed)
    return;
  cpgslct(plot->pgplot_id);
  cpgsci(erase ? 0:1);
  cpgsvp(plot->vxa, plot->vxb, plot->vya, plot->vyb);
  cpgslw(1);
  cpgscf(1);
  cpgsch(MP_TITLE_SIZE);
  cpgmtxt("T", 1.0f, 0.5f, 0.5f, plot->title);
}

/*.......................................................................
 * This is a private function of update_MonitorPlot(), used to compute
 * the statistics of the buffered and plotted extent of the plot's
 * X-axis register(s).
 *
 * Input:
 *  plot   MonitorPlot *  The plot to be characterised.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
static int get_MonitorPlot_stats(MonitorPlot *plot)
{
  MonitorBuff *mb = plot->view->mb; // The snapshot buffer 
  MonitorBuffMap *map = mb->map;    // The buffer configuration object 
  int isnap;                       // The index of a snapshot 
  int x_offset;                    // Snapshot offset of the X-axis register 
  int oldest;                      // The index of the start snapshot 
  int newest;                      // The index of the last snapshot 
  
  // The x-axis doesn't change if this is a power spectrum plot

  if(plot->type == PLOT_POWSPEC)
    return 0;

  // The caller is not supposed to invoke this function if no X-axis
  // register has been selected.
  
  if(plot->xdescs->size()== 0) {
    lprintf(stderr, "get_MonitorPlot_stats: No register.\n");
    return 1;
  };
  
  // If either the viewer or the plot isn't configured, return the
  // current axis limits.
  
  if(plot->updateNeeded()) {
    plot->stats.empty = 1;
    return 0;
  };

  // Iterate over all x-axes in this plot

  plot->stats.empty = 1;
  
  for(unsigned iReg=0; iReg < plot->xdescs->size(); iReg++) {

    gcp::util::RegDescription& xdesc = plot->xdescs->at(iReg);
	
    if(xdesc.iSlot() >= 0) {

      for(xdesc.begin(); !xdesc.isEnd(); ++xdesc) {

	// Find the offset of the X-axis register within buffered snapshots.
  
	x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
  
	// Limit 'oldest' to the constraints of the plot and buffer.
	
	oldest = map->oldest > plot->oldest ? map->oldest : plot->oldest;
	
	// Also constrain 'oldest' to the extent of buffered X-axis data.
	
	int oldest_xreg = map->regs[x_offset].oldest;
	if(oldest_xreg > oldest)
	  oldest = oldest_xreg;
	
	// Get the index of the last snapshot to be characterised.
	
	newest = map->newest;
	
	// Accumulate the statistics of the X-axis register.
	
	for(isnap=oldest; isnap <= newest; isnap++) {
	  double xval = mb->data[(isnap % map->max_snap) * map->nregs + x_offset];
	  if(plot->stats.empty) {
	    plot->stats.empty = 0;
	    plot->stats.xmin = plot->stats.xmax = xval;
	  } else if(xval > plot->stats.xmax) {
	    plot->stats.xmax = xval;
	  } else if(xval < plot->stats.xmin) {
	    plot->stats.xmin = xval;
	  };
	};
      }
    }
  }

  return 0;
}

/*.......................................................................
 * Return the X-axis range that would be needed to display all of the
 * buffered data in a given plot.
 *
 * Input:
 *  plot   MonitorPlot *   The plot to query.
 * Input/Output:
 *  xmin,xmax   double *   The requested X-axis range.
 * Output:
 *  return         int     0 - OK.
 *                         1 - Error.
 */
int full_MonitorPlot_xrange(MonitorPlot *plot, double *xmin, double *xmax)
{
  
  // Check the arguments.
  
  if(!plot || !xmin || !xmax) {
    lprintf(stderr, "full_MonitorPlot_xrange: NULL argument(s).\n");
    return 1;
  };
  
  // Update the x-axis statistics of the specified plot.
  
  if(get_MonitorPlot_stats(plot))
    return 1;
  
  // If nothing has been plotted yet, simply return the current plot
  // limits.
  
  if(plot->stats.empty) {
    *xmin = plot->xleft;
    *xmax = plot->xright;
    
    // If scrolling is disabled return exactly the limits of the
    // buffered data.
    
  } else if(plot->scroll.mode == SCROLL_DISABLED ||
	    plot->scroll.margin >= 1.0) {
    *xmin = plot->stats.xmin;
    *xmax = plot->stats.xmax;
    
    // If scrolling is enabled, return limits that include the scroll
    // margin.
    
  } else {
    *xmin = 0.0;
    *xmax = (plot->stats.xmax - plot->stats.xmin) / (1.0 - plot->scroll.margin);
  };
  
  return 0;
}

/*.......................................................................
 * Respond to a change in device size by starting a new page and redrawing
 * the associated plot.
 *
 * Input:
 *  plot   MonitorPlot *   The plot to be re-drawn.
 * Output:
 *  return         int     0 - OK.
 *                         1 - Error.
 */
int resize_MonitorPlot(MonitorPlot *plot)
{
  /*
   * Check the arguments.
   */
  if(!plot) {
    lprintf(stderr, "resize_MonitorPlot: NULL argument.\n");
    return 1;
  };
  /*
   * Start a new page so that the graphics driver accomodates to the
   * current size of the device.
   */
  cpgslct(plot->pgplot_id);
  cpgpage();
  return update_MonitorPlot(plot, 1);
}

/*.......................................................................
 * Send more of an incomplete monitor-viewer message to the monitor
 * stream.
 *
 * Input:
 *  view  MonitorViewer *  The resource container of the viewer.
 *  dowait          int    If true, don't return until the message
 *                         has been sent or an error occurs.
 * Output:
 *  return  MsSendState    The completion status of the transaction,
 *                         from:
 *                           MS_SEND_ERROR - An unrecoverable error occurred.
 *                           MS_SEND_AGAIN - Another call to this function
 *                                           will be needed to complete the
 *                                           transaction (dowait = 0).
 *                           MS_SEND_DONE  - The transaction is complete.
 */
MsSendState send_MonitorViewer_msg(MonitorViewer *view, int dowait)
{
  MsSendState state;  /* The completion status of the transaction */
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "send_MonitorViewer: NULL argument.\n");
    return MS_SEND_ERROR;
  };
  /*
   * See if the transaction is already marked as complete.
   */
  if(!view->send_in_progress)
    return MS_SEND_DONE;
  /*
   * Send as much of the unsent part of the message as possible.
   */
  state = ms_send_msg(view->ms, dowait);
  switch(state) {
  case MS_SEND_ERROR:
    flag_MonitorViewer(view, 1);
    break;
  case MS_SEND_AGAIN:         /* More sends will be required */
    break;
  case MS_SEND_DONE:          /* The message has been sent */
    view->send_in_progress = 0;
    break;
  };
  return state;
}
/*.......................................................................
 * Send more of an incomplete monitor-viewer message to the image monitor
 * stream.
 *
 * Input:
 *  view  MonitorViewer *  The resource container of the viewer.
 *  dowait          int    If true, don't return until the message
 *                         has been sent or an error occurs.
 * Output:
 *  return  ImsSendState    The completion status of the transaction,
 *                         from:
 *                           IMS_SEND_ERROR - An unrecoverable error occurred.
 *                           IMS_SEND_AGAIN - Another call to this function
 *                                           will be needed to complete the
 *                                           transaction (dowait = 0).
 *                           IMS_SEND_DONE  - The transaction is complete.
 */
ImsSendState send_MonitorViewer_im_msg(MonitorViewer *view, int dowait)
{
  return IMS_SEND_DONE;
}

/*.......................................................................
 * Return the file descriptor that is currently attached to the
 * monitor I/O stream. Given that the file descriptor of a stream
 * can potentially change at the end of each completed read or written
 * message, if the fd is to be used with select(), this function should
 * be called before every call to select() to get the correct fd to
 * listen on.
 *
 * Input:
 *  view  MonitorViewer *  The monitor viewer resource object.
 * Output:
 *  return          int    The current fd, or -1 if no stream is connected.
 */
int MonitorViewer_fd(MonitorViewer *view)
{
  return view ? ms_select_fd(view->ms) : -1;
}
/*.......................................................................
 * Return the file descriptor that is currently attached to the
 * image monitor I/O stream. Given that the file descriptor of a stream
 * can potentially change at the end of each completed read or written
 * message, if the fd is to be used with select(), this function should
 * be called before every call to select() to get the correct fd to
 * listen on.
 *
 * Input:
 *  view  MonitorViewer *  The monitor viewer resource object.
 * Output:
 *  return          int    The current fd, or -1 if no stream is connected.
 */
int MonitorViewer_im_fd(MonitorViewer *view)
{
  return view ? ims_select_fd(view->ims) : -1;
}

/*.......................................................................
 * Load a new calibration file.
 *
 * Input:
 *  view   MonitorViewer *  The monitor viewer resource object.
 *  name            char *  The pathname of the calibration file, or
 *                          NULL to remove all calibrations.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
int mp_set_calfile(MonitorViewer *view, char *calfile)
{
  if(!view) {
    lprintf(stderr, "input_MonitorViewer_cal: NULL argument.\n");
    return 1;
  };
  /*
   * Discard any existing calibrations.
   */
  if(view->ms)
    ms_reset_cal(view->ms);
  if(view->calfile) {
    free(view->calfile);
    view->calfile = NULL;
  };
  /*
   * If a new file name was provided, record it, so that we can reread it
   * whenever either a new stream is started, or a new register map is
   * encountered.
   */
  if(calfile && calfile[0] != '\0') {
    view->calfile = (char* )malloc(strlen(calfile)+1);
    if(!view->calfile) {
      lprintf(stderr, "Insufficient memory to record calibration file name.\n");
      return 1;
    };
    strcpy(view->calfile, calfile);
    /*
     * If we currently have a data-stream, load the calibration file.
     */
    if(view->ms)
      return ms_load_cal_file(view->ms, "", calfile);
  };
  return 0;
}

/*.......................................................................
 * Request a change in the sampling interval of the monitor stream.
 *
 * Input:
 *  view   MonitorViewer *  The monitor viewer resource object.
 *  interval    unsigned    The sampling interval. Initially this is
 *                          zero, which halts monitoring.
 * Output:
 *  return  MsSendState    The state of the transaction, from:
 *                           MS_SEND_ERROR - An unrecoverable error occurred.
 *                           MS_SEND_AGAIN - To complete the transaction,
 *                                           call send_MonitorViewer_msg(view).
 *                           MS_SEND_DONE  - The transaction is complete.
 */
MsSendState set_MonitorViewer_interval(MonitorViewer *view, unsigned interval)
{
  MsSendState state;  /* The transaction status */
  /*
   * Ignore the request if there is no stream connected.
   */
  if(!view->ms)
    return MS_SEND_DONE;
  /*
   * Record the interval.
   */
  view->interval = interval >= 1 ? interval : 1;
  /*
   * Attempt to send the interval to the monitor stream.
   */
  state = ms_queue_interval(view->ms, view->interval);
  if(state == MS_SEND_AGAIN)
    view->send_in_progress = 1;
  return state;
}

/*.......................................................................
 * Return true if the current stream can be rewound.
 * If false, calls to queue_MonitorViewer_rewind() will do nothing
 * more than return MS_SEND_DONE.
 *
 * Input:
 *  view   MonitorViewer *  The monitor viewer resource object.
 * Output:
 *  return           int    0 - Not rewindable.
 *                          1 - Rewindable.
 */
int can_MonitorViewer_rewind(MonitorViewer *view)
{
  return view->ms && ms_can_be_rewound(view->ms);
}

/*.......................................................................
 * Request a rewind of the stream. This does nothing if the stream isn't
 * rewindable.
 *
 * Input:
 *  view   MonitorViewer *  The monitor viewer resource object.
 * Output:
 *  return  MsSendState    The state of the transaction, from:
 *                           MS_SEND_ERROR - An unrecoverable error occurred.
 *                           MS_SEND_AGAIN - To complete the transaction,
 *                                           call send_MonitorViewer_msg(view).
 *                           MS_SEND_DONE  - The transaction is complete.
 */
MsSendState queue_MonitorViewer_rewind(MonitorViewer *view)
{
  MsSendState state;  /* The transaction status */
  /*
   * Ignore the request if there is no stream connected.
   */
  if(!view->ms)
    return MS_SEND_DONE;
  /*
   * Attempt to send the interval to the monitor stream.
   */
  state = ms_queue_rewind(view->ms);
  if(state == MS_SEND_AGAIN)
    view->send_in_progress = 1;
  return state;
}

/*.......................................................................
 * Establish or remove a callback function to be invoked whenever the
 * specified plot is scrolled. If an callback has previously been
 * registered, then the new one replaces it.
 *
 * Input:
 *  plot      MonitorPlot *   The plot to report on.
 *  user_fn  MP_SCROLL_FN(*)  The function to call whenever the plot is
 *                            scrolled. Send 0 to delete a previous
 *                            callback without registering a new one.
 *  user_data        void *   Optional data to be passed to user_fn()
 *                            whenever it is called.
 *  del_fn      MP_DEL_FN(*)  If user_data should be deleted when the
 *                            callback is replaced or discarded, then
 *                            del_fn() should be a destructor function
 *                            for user_data.
 * Output:
 *  return            int     0 - OK.
 *                            1 - Error [del_fn(user_data) will have
 *                                       been invoked if del_fn != 0].
 */
int mp_scroll_callback(MonitorPlot *plot, MP_SCROLL_FN(*user_fn),
		       void *user_data, MP_DEL_FN(del_fn))
{
  MpScroll *scroll;
  /*
   * Check arguments.
   */
  if(!plot) {
    lprintf(stderr, "mp_scroll_callback: NULL plot.\n");
    return 1;
  };
  /*
   * Remove any existing handler.
   */
  scroll = &plot->scroll;
  scroll->user_fn = 0;
  scroll->user_data = scroll->del_fn ? scroll->del_fn(scroll->user_data) : NULL;
  scroll->del_fn = 0;
  /*
   * Install the new handler.
   */
  scroll->user_fn = user_fn;
  scroll->user_data = user_data;
  scroll->del_fn = del_fn;
  return 0;
}

/*.......................................................................
 * Given an X-axis normalized device coordinate and a graph, return
 * the equivalent graph coordinate.
 *
 * Input:
 *  graph   MonitorGraph *  The graph whose coordinate is to be returned.
 *  ndc_x         double    The X-axis plot coordinate (NDC).
 * Output:
 *  return        double    The graph coordinate corresponding to ndc_x.
 */
static double x_ndc_to_graph(MonitorGraph *graph, double ndc_x)
{
  if(!graph) {
    lprintf(stderr, "x_ndc_to_graph: NULL graph.\n");
    return 0.0;
  } else {
    MonitorPlot *plot = graph->plot;

    if(plot->type != PLOT_POWSPEC) {

      return plot->wxa + (plot->wxb - plot->wxa) * 
	((ndc_x - graph->vxa) / (graph->vxb - graph->vxa));

    } else {

      float xa = graph->lastPlot.xa;
      float xb = graph->lastPlot.xb;
      float ya = graph->lastPlot.ya;
      float yb = graph->lastPlot.yb;
      
      if(graph->lastPlot.xIsLinear) {

	return xa + (xb - xa) * 
	  ((ndc_x - graph->vxa) / (graph->vxb - graph->vxa));

	} else {

	  double xval = log10(xa) + (log10(xb) - log10(xa)) * 
	    ((ndc_x - graph->vxa) / (graph->vxb - graph->vxa));

	  return pow(10.0, xval);
	}
    }

  }
}

/*.......................................................................
 * Given a Y-axis normalized device coordinate and a graph, return
 * the equivalent graph coordinate.
 *
 * Input:
 *  graph   MonitorGraph *  The graph whose coordinate is to be returned.
 *  ndc_y         double    The Y-axis plot coordinate (NDC).
 * Output:
 *  return        double    The graph coordinate corresponding to ndc_y.
 */
static double y_ndc_to_graph(MonitorGraph *graph, double ndc_y)
{
  if(!graph) {
    lprintf(stderr, "y_ndc_to_graph: NULL graph.\n");
    return 0.0;
  } else {

    if(graph->plot->type != PLOT_POWSPEC) {

      return graph->wya + (graph->wyb - graph->wya) * 
	((ndc_y - graph->vya) / (graph->vyb - graph->vya));

    } else {

      float xa = graph->lastPlot.xa;
      float xb = graph->lastPlot.xb;
      float ya = graph->lastPlot.ya;
      float yb = graph->lastPlot.yb;
      
      if(graph->lastPlot.yIsLinear) {

	return ya + (yb - ya) * 
	  ((ndc_y - graph->vya) / (graph->vyb - graph->vya));

	} else {

	  double yval = log10(ya) + (log10(yb) - log10(ya)) * 
	    ((ndc_y - graph->vya) / (graph->vyb - graph->vya));

	  return pow(10.0, yval);
	}
    }

  }
}

/*.......................................................................
 * Given an X-axis coordinate in graph units, return the normalized device
 * coordinate at which that coordinate appears.
 *
 * Input:
 *  graph   MonitorGraph *  The graph in which world_x is defined.
 *  world_x       double    The X-axis graph coordinate.
 * Output:
 *  return        double    The NDC coordinate corresponding to world_x.
 */
static double x_graph_to_ndc(MonitorGraph *graph, double world_x)
{
  if(!graph) {
    lprintf(stderr, "x_ndc_to_graph: NULL graph.\n");
    return 0.0;
  } else {
    MonitorPlot *plot = graph->plot;

    if(plot->type != PLOT_POWSPEC) {

      return graph->vxa + (graph->vxb - graph->vxa) * 
	((world_x - plot->wxa) / (plot->wxb - plot->wxa));

    } else {

      float xa = graph->lastPlot.xa;
      float xb = graph->lastPlot.xb;
      float ya = graph->lastPlot.ya;
      float yb = graph->lastPlot.yb;
      
      if(graph->lastPlot.xIsLinear) {

	return graph->vxa + (graph->vxb - graph->vxa) * 
	  ((world_x - xa) / (xb - xa));
    
      } else {

	return graph->vxa + (graph->vxb - graph->vxa) * 
	  ((log10(world_x) - log10(xa)) / (log10(xb) - log10(xa)));

      }
    }
  };
}

/*.......................................................................
 * Given an Y-axis coordinate in graph units, return the normalized device
 * coordinate at which that coordinate appears.
 *
 * Input:
 *  graph   MonitorGraph *  The graph in which world_y is defined.
 *  world_y       double    The Y-axis graph coordinate.
 * Output:
 *  return        double    The NDC coordinate corresponding to world_y.
 */
static double y_graph_to_ndc(MonitorGraph *graph, double world_y)
{
  if(!graph) {
    lprintf(stderr, "y_ndc_to_graph: NULL graph.\n");
    return 0.0;
  } else {

    if(graph->plot->type != PLOT_POWSPEC) {

      return graph->vya + (graph->vyb - graph->vya) * 
	((world_y - graph->wya) / (graph->wyb - graph->wya));
      
    } else {

      float xa = graph->lastPlot.xa;
      float xb = graph->lastPlot.xb;
      float ya = graph->lastPlot.ya;
      float yb = graph->lastPlot.yb;
      
      if(graph->lastPlot.yIsLinear) {

	return graph->vya + (graph->vyb - graph->vya) * 
	  ((world_y - ya) / (yb - ya));
    
      } else {

	return graph->vya + (graph->vyb - graph->vya) * 
	  ((log10(world_y) - log10(ya)) / (log10(yb) - log10(ya)));

      }
    }

  }
}

/*.......................................................................
 * Convert from cursor coordinates to graph coordinates. Cursor
 * coordinates are the same as normalized device coordinates.
 *
 * Input/Output:
 *  plot    MonitorPlot *   The plot in which the cursor 
 *  graph  MonitorGraph **  On input *graph can be NULL or point to
 *                          one of the graphs of the given 'plot'.
 *                          In the former case the graph that contains,
 *                          or is nearest to the cursor will be substituted.
 *                          On output *graph will contain the graph
 *                          to which the returned coordinates refer.
 *  x,y           double *  On input *x,*y are the cursor coordinates.
 *                          On output they are the graph coordinates.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
int mp_cursor_to_graph(MonitorPlot *plot, MonitorGraph **graph,
		       double *x, double *y)
{
  if(!graph || !x || !y) {
    lprintf(stderr, "mp_cursor_to_graph: NULL argument(s).\n");
    return 1;
  };

  // No active graphs?

  if(!plot->graphs) {
    lprintf(stderr, "mp_cursor_to_graph: No graphs are active.\n");
    return 1;
  };

  // If no graph was specified, determine which graph the cursor is
  // in, or nearest to being in.

  if(!*graph) {
    MonitorGraph *g;
    MonitorGraph *best_graph = NULL;
    double min_dy = 0.0;
    for(g=plot->graphs; g; g=g->next) {
      float a_dy = fabs(g->vya - *y);
      float b_dy = fabs(g->vyb - *y);
      int in_graph = *y >= g->vya && *y <= g->vyb;
      if(!best_graph || in_graph || a_dy < min_dy || b_dy < min_dy) {
	best_graph = g;
	min_dy = a_dy < b_dy ? a_dy : b_dy;
      };
    };
    *graph = best_graph;
  };

  *x = x_ndc_to_graph(*graph, *x);
  *y = y_ndc_to_graph(*graph, *y);

  return 0;
}

/*.......................................................................
 * Convert from graph coordinates to cursor coordinates. Cursor
 * coordinates are the same as normalized device coordinates.
 *
 * Input:
 *  graph   MonitorGraph *  The graph to which the coordinates refer.
 * Input/Output:
 *  x,y           double *  On input *x,*y are the graph coordinates.
 *                          On output they are the cursor coordinates.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
int mp_graph_to_cursor(MonitorGraph *graph, double *x, double *y)
{
  if(!graph || !x || !y) {
    lprintf(stderr, "mp_graph_to_cursor: NULL argument(s).\n");
    return 1;
  };
  *x = x_graph_to_ndc(graph, *x);
  *y = y_graph_to_ndc(graph, *y);
  return 0;
}

/*.......................................................................
 * Locate the data-point that is closest to a given graph coordinate.
 *
 * Input:
 *  graph   MonitorGraph *  The graph in which the point lies.
 *  x,y           double    The coordinate to search for in the displayed
 *                          graph units.
 * Input/Output:
 *  reg        RegMapReg *  On output, *reg will contain the details of
 *                          of register that is associated with the located
 *                          point.
 *  xo,yo         double *  The actual location of the data point.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error (Note that this is the return value
 *                              if no data have been received yet).
 */
int find_Monitor_point(MonitorGraph *graph, double x, double y,
		       gcp::util::RegDescription& retDesc, double *xo, double *yo)
{
  MonitorPlot *plot = graph->plot;  /* The parent plot of the graph */
  MonitorBuff *mb = plot->view->mb; /* The snapshot buffer */
  MonitorBuffMap *map = mb->map;    /* The buffer configuration object */
  MonitorTrace *trace;              /* The trace being searched */
  int first = 1;                    /* True until the first point is tested */
  double min_x = 0.0;               /* The x coordinate of the closest point */
  double min_y = 0.0;               /* The y coordinate of the closest point */
  double min_rsq = 0.0;             /* The min dx^2+dy^2 found so far */
  double x_to_mm;                   /* Millimeters per X-axis unit */
  double y_to_mm;                   /* Millimeters per Y-axis unit */
  gcp::util::RegDescription min_desc;/* The register of the closest point */
  int oldest;                      /* The index of the start snapshot */
  int newest;                      /* The index of the last snapshot */
  int x_offset;                    /* Snapshot offset of the X-axis register */
  
  // Check the arguments.
  
  if(!graph || !xo || !yo) {
    lprintf(stderr, "find_Monitor_point: NULL argument.\n");
    return 1;
  };
  
  // Ignore this call if the graph is not ready to be searched.
  
  if(graph->plot->type != PLOT_POWSPEC && 
     (graph->update_needed || plot->view->update_needed)) {
    lprintf(stderr, "find_Monitor_point: graph update needed.\n");
    return 1;
  } else {
    if(graph->powSpec.updateNeeded) {
      lprintf(stderr, "find_Monitor_point: graph update needed.\n");
      return 1;
    }
  }
  
  // Compute the multipliers needed to scale graph units to physical
  // device coordinates.
  
  {
    float xa, xb, ya, yb;
    cpgslct(plot->pgplot_id);
    cpgsvp(graph->vxa, graph->vxb, graph->vya, graph->vyb);
    cpgqvp(2, &xa, &xb, &ya, &yb);
    cpgsvp(0.0f, 1.0f, 0.0f, 1.0f); // Return to cursor viewport 
    x_to_mm = (xb - xa) / (plot->wxa - plot->wxb);
    y_to_mm = (yb - ya) / (graph->wya - graph->wyb);
  };




  for(unsigned iXreg=0; iXreg < plot->xdescs->size(); iXreg++) {
	
    gcp::util::RegDescription& xdesc = plot->xdescs->at(iXreg);
    
    if(xdesc.iSlot() >= 0) {
      
      for(xdesc.begin(); !xdesc.isEnd(); ++xdesc) {

	// Find the offset of the X-axis register within buffered snapshots.
  
	x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
  
	// Find the oldest snapshot of the graph.
	
	oldest = map->oldest;
	if(oldest < plot->oldest)
	  oldest = plot->oldest;
	{
	  int oldest_xreg = map->regs[x_offset].oldest;
	  if(oldest_xreg > oldest)
	    oldest = oldest_xreg;
	};
	
	// Find the newest snapshot of the graph.
	
	newest = map->newest;
	
	// Search one register at a time.
	// Iterate over all traces of this graph
	
	for(trace=graph->traces; trace; trace=trace->next) {
	  
	  // Iterate over all registers in this trace
	  
	  for(unsigned iYreg=0; iYreg < trace->descs->size(); iYreg++) {
	    
	    gcp::util::RegDescription& desc = trace->descs->at(iYreg);
	    
	    // Iterate over all requested elements of this register
	    
	    for(desc.begin(); !desc.isEnd(); ++desc) {
	      int y_offset = map->offset[desc.currentSlot()].a[desc.aspect()];
	      int min_snap=0; // The snapshot at which the closest point was found 
	      int found = 0;   // True if a new closest point was found 
	      
	      // Get the index of the oldest recorded value of the current
	      // Y-axis register.
	      
	      int y_oldest = map->regs[y_offset].oldest;
	      if(y_oldest < oldest)
		y_oldest = oldest;
	      if(y_oldest > newest)
		continue;
	      
	      // Peruse the buffered values of the register to see if any of
	      // them are closer to the target coordinate than the closest so
	      // far found.
	      
	      if(graph->bits) {
		find_logic_point(graph, mb, x, y, x_to_mm, y_to_mm,
				 x_offset, y_offset, y_oldest, newest,
				 &min_rsq, &min_snap, &first, &found);
	      } else {
		find_normal_point(graph, mb, x, y, x_to_mm, y_to_mm,
				  x_offset, y_offset, y_oldest, newest,
				  &min_rsq, &min_snap, &first, &found);
	      };
	      
	      // If a new closest register was found record its details. This
	      // is done here to reduce the amount of computation done in the
	      // the above functions.
	      
	      if(found) {
		double *frame = mb->data + (min_snap % map->max_snap) * map->nregs;
		min_x = frame[x_offset];
		min_y = frame[y_offset];
		
		min_desc = desc;
		
		// Set the coordinate for this dscriptor to the coordinate of
		// the current element
		
		gcp::util::Coord coord = desc.currentCoord();
		min_desc.setCoord(coord);
	      };
	    };
	  };
	};
      }
    }
  }
  
  // No data-point found?
  
  if(first) {
    lprintf(stderr, "find_Monitor_point: The graph contains no data.\n");
    return 1;
  };
  
  // Return the located values.
  
  *yo = min_y;
  *xo = min_x;
  
  retDesc = min_desc;
  
  return 0;
}

/*.......................................................................
 * Locate the data-point that is closest to a given graph coordinate.
 *
 * Input:
 *  graph   MonitorGraph *  The graph in which the point lies.
 *  x,y           double    The coordinate to search for in the displayed
 *                          graph units.
 * Input/Output:
 *  reg        RegMapReg *  On output, *reg will contain the details of
 *                          of register that is associated with the located
 *                          point.
 *  xo,yo         double *  The actual location of the data point.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error (Note that this is the return value
 *                              if no data have been received yet).
 */
int findMonitorPoint(MonitorGraph *graph, 
		     double x, double y,
		     gcp::util::RegDescription& closestReg, 
		     double& xo, double& yo)
{
  MonitorPlot *plot = graph->plot;  /* The parent plot of the graph */
  MonitorTrace *trace;              /* The trace being searched */
  bool first = true;                /* True until the first point is tested */
  double min_x = 0.0;               /* The x coordinate of the closest point */
  double min_y = 0.0;               /* The y coordinate of the closest point */
  double min_rsq = 0.0;             /* The min dx^2+dy^2 found so far */
  double x_to_mm;                   /* Millimeters per X-axis unit */
  double y_to_mm;                   /* Millimeters per Y-axis unit */
  
  // Check the arguments.
  
  if(!graph || !xo || !yo) {
    lprintf(stderr, "find_Monitor_point: NULL argument.\n");
    return 1;
  };
  
  // Ignore this call if the graph is not ready to be searched.
  
  if(graph->plot->type != PLOT_POWSPEC && 
     (graph->update_needed || plot->view->update_needed)) {
    lprintf(stderr, "find_Monitor_point: graph update needed.\n");
    return 1;
  } else {
    if(graph->powSpec.updateNeeded) {
      lprintf(stderr, "find_Monitor_point: graph update needed.\n");
      return 1;
    }
  }
  
  // Compute the multipliers needed to scale graph units to physical
  // device coordinates.
  
  {
    float xa, xb, ya, yb;
    cpgslct(plot->pgplot_id);
    cpgsvp(graph->vxa, graph->vxb, graph->vya, graph->vyb);
    cpgqvp(2, &xa, &xb, &ya, &yb);
    cpgsvp(0.0f, 1.0f, 0.0f, 1.0f); // Return to cursor viewport 
    x_to_mm = (xb - xa) / (plot->wxa - plot->wxb);
    y_to_mm = (yb - ya) / (graph->wya - graph->wyb);
  };
  
  for(trace=graph->traces; trace; trace=trace->next) 
    trace->findClosestPoint(x , y, x_to_mm, y_to_mm, 
			    first, min_x, min_y, min_rsq, closestReg);

  if(first) {
    lprintf(stderr, "find_Monitor_point: The graph contains no data.\n");
    return 1;
  };

  // Return the located values.
  
  yo = min_y;
  xo = min_x;

  if(graph->plot->type == PLOT_POWSPEC) {

    if(!graph->powSpec.linAxis) 
      yo = pow(10.0, yo);
    if(!graph->plot->powSpec.linAxis) 
      xo = pow(10.0, xo);

  }
  
  return 0;
}  

/*.......................................................................
 * This is a private function of find_Monitor_Point() used to find the
 * nearest plotted point of a given register to a given x,y location.
 *
 * Input:
 *  graph   MonitorGraph *  The graph in which the point lies.
 *  x,y           double    The coordinate to search for in the displayed
 *                          graph units.
 *  x_to_mm       double    Millimeters per X-axis unit.
 *  y_to_mm       double    Millimeters per Y-axis unit.
 *  x_offset        int    The offset of the x-axis register in the
 *                          each buffered frame.
 *  y_offset        int    The offset of the y-axis register in the
 *                          each buffered frame.
 *  oldest          int    The first snapshot to search.
 *  newest          int    The last snapshot to search.
 * Input/Output:
 *  min_rsq       double *  The square of the separation of the closest point.
 *  min_snap        int *  The snapshot of the closest point.
 *  first            int *  This is 0 after the first point has been
 *                          looked at.
 *  found            int *  True if a new closest point was found.
 */
static void find_normal_point(MonitorGraph *graph, MonitorBuff *mb,
			      double x, double y,
			      double x_to_mm, double y_to_mm,
			      int x_offset, int y_offset,
			      int oldest, int newest, double *min_rsq,
			      int *min_snap, int *first, int *found)
{
  MonitorBuffMap *map = mb->map;  /* The buffer layout map */
  int isnap;                      /* The snapshot being compared */
  /*
   * Search the specified range of snapshots.
   */
  for(isnap=oldest; isnap <= newest; isnap++) {
    double *frame = mb->data + (isnap % map->max_snap) * map->nregs;
    double dx = (frame[x_offset] - x) * x_to_mm;
    double dy = (frame[y_offset] - y) * y_to_mm;
    double rsq = dx*dx + dy*dy;
    if(*first || rsq < *min_rsq) {
      *first = 0;
      *found = 1;
      *min_rsq = rsq;
      *min_snap = isnap;
    };
  };
}

/*.......................................................................
 * This is a private function of find_MonitorPoint() used to find the
 * nearest plotted bit value of a given register to a given x,y location.
 *
 * Input:
 *  graph   MonitorGraph *  The graph in which the point lies.
 *  x,y           double    The coordinate to search for in the displayed
 *                          graph units.
 *  x_to_mm       double    Millimeters per X-axis unit.
 *  y_to_mm       double    Millimeters per Y-axis unit.
 *  x_offset        int    The offset of the x-axis register in the
 *                          each buffered frame.
 *  y_offset        int    The offset of the y-axis register in the
 *                          each buffered frame.
 *  oldest          int    The first snapshot to search.
 *  newest          int    The last snapshot to search.
 * Input/Output:
 *  min_rsq       double *  The square of the separation of the closest point.
 *  min_snap        int *  The snapshot of the closest point.
 *  first            int *  This is 0 after the first point has been
 *                          looked at.
 *  found            int *  True if a new closest point was found.
 */
static void find_logic_point(MonitorGraph *graph, MonitorBuff *mb,
			     double x, double y,
			     double x_to_mm, double y_to_mm,
			     int x_offset, int y_offset,
			     int oldest, int newest, double *min_rsq,
			     int *min_snap, int *first, int *found)
{
  MonitorBuffMap *map = mb->map;  /* The buffer layout map */
  int isnap;                      /* The snapshot being compared */
  /*
   * Search the specified range of snapshots.
   */
  for(isnap=oldest; isnap <= newest; isnap++) {
    double *frame = mb->data + (isnap % map->max_snap) * map->nregs;
    double dx = (frame[x_offset] - x) * x_to_mm;
    int bit;
    for(bit=0; bit < 32; bit++) {
      int on = ((unsigned) frame[y_offset] >> bit) & 1U;
      float yval = (on ? (bit+0.25):(bit-0.25)) - graph->wya;
      double dy = (yval - y) * y_to_mm;
      double rsq = dx*dx + dy*dy;
      if(*first || rsq < *min_rsq) {
	*first = 0;
	*found = 1;
	*min_rsq = rsq;
	*min_snap = isnap;
      };
    };
  };
}

/*.......................................................................
 * Return the statistics of the data buffered for a given register.
 *
 * Input:
 *  view     MonitorViewer *  The monitor viewer resource object.
 *  reg          RegMapReg *  The scalar specification of the register
 *                            to be characterised. An error will be
 *                            signalled if the register is not currently
 *                            in the set of those being buffered, or if
 *                            the specification is not scalar.
 * Input/Output:
 *  stats  MonitorRegStats *  On successful return *stats will be
 *                            initialized with the statistics of the
 *                            specified register.
 * Output:
 *  return             int    0 - OK.
 *                            1 - Error.
 */
int mp_global_RegStats(MonitorViewer *view, gcp::util::RegDescription* ydesc,
		       MonitorRegStats *stats)
{
  MonitorBuff *mb;     /* The snapshot buffer */
  MonitorBuffMap *map; /* The buffer configuration object */
  ArrayMap *arraymap;  /* The current array map */
  int reg_offset;     /* The offset of the register within each snapshot */
  int oldest,newest;  /* The range of snapshots to sample */
  int isnap;          /* The number of the snapshot being processed */
  MonitorRegStats s;   /* The statistics container to be copied for return */
  
  // Check the arguments.
  
  if(!view || !ydesc || !stats) {
    lprintf(stderr, "mp_global_regstats: NULL argument.\n");
    return 1;
  };
  
  if(ydesc->nEl() != 1) {
    lprintf(stderr, "mp_global_regstats: Invalid register specification.\n");
    return 1;
  };
  
  // Is the viewer configured?
  
  if(view->update_needed) {
    lprintf(stderr,
	    "The viewer must be configured before "
	    "statistics can be measured.\n");
    return 1;
  };
  
  // Get the current register map.
  
  arraymap = MonitorViewer_ArrayMap(view);
  if(!arraymap) {
    lprintf(stderr, "mp_global_regstats: There are no buffered registers.\n");
    return 1;
  };
  
  // Get local aliases for the snapshot buffer and its configuration
  // map.
  
  mb = view->mb;
  map = mb->map;
  
  // See if the specified register is one of those being monitored.
  
  reg_offset = (ydesc->iSlot() < 0 || ydesc->iSlot() >= map->arraymap_size) ? -1 :
    map->offset[ydesc->iSlot()].a[ydesc->aspect()];
  
  if(reg_offset < 0) {
    lprintf(stderr,
	    "mp_global_regstats: "
	    "The specified register is not being monitored.\n");
    return 1;
  };
  
  // Find the oldest snapshot of the register.
  
  oldest = map->regs[reg_offset].oldest;
  if(oldest < map->oldest)
    oldest = map->oldest;
  
  // Find the number of the newest snapshot.
  
  newest = map->newest;
  
  // Accumulate the initial round of statistics.
  
  s.min = s.max = s.mean = s.rms = 0.0;
  s.npoint = 0;
  if(oldest <= newest) {
    for(isnap=oldest; isnap <= newest; isnap++) {
      double *frame = mb->data + (isnap % map->max_snap) * map->nregs;
      double value = frame[reg_offset];
      if(s.npoint++) {
	if(value < s.min)
	  s.min = value;
	else if(value > s.max)
	  s.max = value;
	s.mean += (value - s.mean) / (double) s.npoint;
      } else {
	s.min = s.max = s.mean = value;
      };
    };
    
    // Now take a second pass through the data to compute the rms from
    // the mean.
    
    s.npoint = 0;
    for(isnap=oldest; isnap <= newest; isnap++) {
      double *frame = mb->data + (isnap % map->max_snap) * map->nregs;
      double value = frame[reg_offset];
      double dv = (value - s.mean);
      s.rms += (dv*dv - s.rms) / (double) ++s.npoint;
    };
    
    // Convert the mean square deviation to an rms deviation.
    
    s.rms = sqrt(s.rms);
  };
  
  // Return the statistics.
  
  *stats = s;
  
  return 0;
}

/*.......................................................................
 * Return the statistics a of a subset of the data buffered for a given
 * Y-axis register. The subset that is used is determined by the value of
 * the X-axis register of the source plot. Each sample in which the X-axis
 * register is within given limits will have the target register value
 * extracted and included in the statistics calculation.
 *
 * Input:
 *  plot       MonitorPlot *  The plot whose X-axis register is to
 *                            be used to select the subset of the
 *                            data to be used. xa and xb will be
 *                            limited to the extent of the X-axis of
 *                            the plot and the oldest snapshot used
 *                            will be the oldest one used by the
 *                            plot.
 *  yreg         RegMapReg *  The scalar specification of a Y-axis
 *                            register to be characterised.
 *  xa, xb          double    The range of values of the X-axis
 *                            register for which yreg should be
 *                            sampled.
 * Input/Output:
 *  stats  MonitorRegStats *  On successful return *stats will be
 *                            initialized with the statistics of the
 *                            specified register.
 * Output:
 *  return             int    0 - OK.
 *                            1 - Error.
 */
int mpRegStats(MonitorPlot *plot, 
	       double xa, double xb, 
	       gcp::util::RegDescription& statDesc,
	       MonitorRegStats *stats)
{
  // Find the trace to which this descriptor belongs
  
  MonitorTrace* trace = findTrace(plot, statDesc);
  if(trace != 0)
    return mpTraceRegStats(plot, trace, xa, xb, stats, &statDesc);
  else {
    lprintf(stderr, "No graph contains the specified register\n");
    return 1;
  }
}

static MonitorTrace* 
findTrace(MonitorPlot* plot, gcp::util::RegDescription& desc)
{
  for(MonitorGraph* graph=plot->graphs; graph; graph=graph->next){
    for(MonitorTrace* trace=graph->traces; trace; trace=trace->next) {
      for(unsigned iReg=0; iReg < trace->descs->size(); iReg++) {
	
	// Get the descriptor for this register
	
	gcp::util::RegDescription& tdesc = trace->descs->at(iReg);
	
	if(tdesc.contains(desc))
	  return trace;
      }
    }
  }
  return 0;
}

/**.......................................................................
 * Get statistics on a register managed by the specified trace object
 */
static int mpTraceRegStats(MonitorPlot *plot, MonitorTrace* trace,
			   double xa, double xb, 
			   MonitorRegStats *stats,
			   gcp::util::RegDescription* desc)
{
  MonitorBuff *mb;     /* The snapshot buffer */
  MonitorBuffMap *map; /* The buffer configuration object */
  ArrayMap *arraymap;  /* The current array map */
  MonitorRegStats s;   /* The statistics container to be copied for return */
  
  // Check the arguments.
  
  if(!plot || !trace || !stats) {
    lprintf(stderr, "mpTraceRegStats: NULL argument.\n");
    return 1;
  };
  
  // Get the current register map.
  
  arraymap = MonitorViewer_ArrayMap(plot->view);
  if(!arraymap) {
    lprintf(stderr, "mpTraceRegStats: There are no buffered registers.\n");
    return 1;
  };
  
  // Is the viewer configured?
  
  if(plot->view->update_needed) {
    lprintf(stderr,
	    "The viewer must be configured before statistics"
	    " can be measured.\n");
    return 1;
  };
  
  // Get local aliases for the snapshot buffer and its configuration
  // map.
  
  mb = plot->view->mb;
  map = mb->map;

  // Sort xa and xb into ascending order.
  
  if(xa > xb) {
    double xtmp = xa;
    xa = xb;
    xb = xtmp;
  };
  
  // Constrain xa and xb to lie within the bounds of the plot X-axis.
  // Note that if the range xa..xb is not visible then after the
  // following xa will be greater than xb. This is tested for later.
  
  if(xa < plot->wxa)
    xa = plot->wxa;
  if(xb > plot->wxb)
    xb = plot->wxb;
  
  s.npoint = 0;
  s.state = MRS_UNINIT;

  // Get statistics now

  trace->getStats(xa, xb, s, desc);

  // Convert the mean square deviation to an rms deviation.
  
  s.rms = sqrt(s.rms);
  
  // Calc the number of sigma deviation from zero.
  
  s.nsig = s.mean / (s.rms/sqrt((double)s.npoint));
  
  // Return the statistics.
  
  *stats = s;
  
  return 0;
}

/*.......................................................................
 * Make a hardcopy version of a given plot.
 *
 * Input:
 *  plot     MonitorPlot *  The plot to be saved.
 *  device          char *  The PGPLOT specification of the hardcopy
 *                          device to be used.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
int mp_hardcopy(MonitorPlot *plot, char *device)
  {
  MonitorGraph *graph;  /* A child graph of the plot */
  MonitorBuffMap *map;  /* The container of buffered monitor data */
  int softcopy_id;      /* The preserved PGPLOT id of the current plot device */
  int hardcopy_id;      /* The PGPLOT id of the hardcopy device */
  int update_needed;    /* True if the plot is not ready for plotting */
  /*
   * Check the arguments.
   */
  if(!plot || !device) {
    lprintf(stderr, "mp_hardcopy: NULL argument.\n");
    return 1;
  };
  /*
   * Is the plot configuration ready?
   */
  update_needed = plot->update_needed || plot->view->update_needed;
  for(graph=plot->graphs; graph; graph=graph->next) {
    if(graph->update_needed)
      update_needed = 1;
  };
  if(update_needed) {
    lprintf(stderr, "mp_hardcopy: The plot configuration isn't ready.\n");
    return 1;
  };
  /*
   * Attempt to open the hardcopy device.
   */
  hardcopy_id = cpgopen(device);
  if(hardcopy_id <= 0)
    return 1;
  cpgask(0);
  cpgpage();
  cpgbbuf();
  
  // Temporarily replace the current plot device with the hardcopy
  // device.

  softcopy_id = plot->pgplot_id;
  plot->pgplot_id = hardcopy_id;
  
  // Plot the title of the plot.

  draw_MonitorPlot_title(plot, 0);
  
  // Draw the graphs.

  map = plot->view->mb->map;
  for(graph=plot->graphs; graph; graph=graph->next) {
    drawMonitorGraphData(graph, map->oldest, map->newest, 0);
    draw_MonitorGraph_axes(graph, 0);
  };

  cpgebuf();

  // Close the hardcopy device and reinstate the orgininal PGPLOT
  // device.

  cpgclos();
  plot->pgplot_id = softcopy_id;
  return 0;
}

/*.......................................................................
 * When ms_read_frame() indicates that a new register map has been
 * adopted by a stream, this function must be called. It clears all
 * register selections, allocates new buffer mapping objects and attempts
 * to rearrange the monitor buffer to preserve the data of previously
 * buffered registers that are common to the old and new register maps.
 *
 * After this function returns, its caller must also inform the
 * user-interface layer that the register map has changed. The user
 * interface layer can then re-negotiate its register selections.
 *
 * Input:
 *  view    MonitorViewer *  The viewer resource object.
 * Output:
 *  return            int    0 - OK.
 *                           1 - Error.
 */
static int remap_MonitorBuff(MonitorViewer *view)
{
  MonitorBuff *mb = view->mb;  /* The buffer to be reconfigured */
  ArrayMap *old_arraymap;      /* The previous register map */
  ArrayMap *new_arraymap;      /* The new register map */
  MonitorBuffMap *old_map;     /* The old buffer-layout map */
  MonitorBuffMap *new_map;     /* The new buffer-layout map */
  MonitorBuffOff *old_offset;  /* The old {slot -> buffer offset} mapping */
                               /*  array */
  MonitorBuffOff *new_offset;  /* The new {slot -> buffer offset} mapping */
                               /*  array */
  int old_snap;               /* The snapshot index of a legacy frame */
  int new_snap;               /* The snapshot index of a copied frame */
  int new_regs_dim;           /* The number of buffer elements per snapshot */
                               /*  that can be accomodated by new_map. */
  int regmap;                  /* The index of a register map of an array map */
  int board;                   /* The index of a board of a register map */
  int block;                   /* The index of a register block of a board */
  int i,j;
  /*
   * Is there a buffer to be reconfigured?
   */
  if(!mb->map)
    return 0;
  /*
   * The register selections associated with each plot and page are
   * now potentially invalid, so discard them.
   */
  clr_MonitorViewer_regs(view);
  /*
   * Record an alias of the previous (now invalid) buffer layout object.
   */
  old_map = mb->map;
  mb->old = del_MonitorBuffMap(mb->old);
  mb->map = NULL;
  /*
   * Allocate two new buffer layout maps for use with the new register map.
   */
  mb->old = new_MonitorBuffMap(view);
  mb->map = new_MonitorBuffMap(view);
  if(!mb->old || !mb->map) {
    mb->old = del_MonitorBuffMap(mb->old);
    mb->map = del_MonitorBuffMap(mb->map);
    old_map = del_MonitorBuffMap(old_map);
    return 1;
  };
  /*
   * Get a pointer to the new buffer layout map that is to be initialized.
   */
  new_map = mb->map;
  /*
   * Get the old and new register maps.
   */
  old_arraymap = old_map->arraymap;
  new_arraymap = new_map->arraymap;
  /*
   * Get the old and new arrays that map from archive slot index to
   * buffer offset.
   */
  old_offset = old_map->offset;
  new_offset = new_map->offset;
  /*
   * Get the max number of datums that can be buffered per register frame
   * from the new register map.
   */
  new_regs_dim = new_map->regs_dim;
  /*
   * Start by marking all of the registers in the new register map as
   * unbuffered.
   */
  for(j=0; j<REG_NASPECT; j++) {
    for(i=0; i<new_map->arraymap_size; i++)
      new_offset[i].a[j] = -1;
  };
  
  // See which of the registers that are currently contained in the
  // monitor buffer exist in the new register map. For each of these
  // registers in the new register map, record the buffer offset of
  // the register in new_offset[] indexed by its new archive slot
  // index.
  
  for(regmap=0; regmap < old_arraymap->nregmap; regmap++) {
    /*
     * Get the next register map of the old array map.
     */
    ArrRegMap* old_arregmap = old_arraymap->regmaps[regmap];
    /*
     * See if a register map of the same name exists in the new array map.
     */
    ArrRegMap* new_arregmap = find_ArrRegMap(new_arraymap, old_arregmap->name);
    
    if(new_arregmap) {
      
      // Traverse the register blocks of the old board and compare
      // them with those in the new board.
      
      for(board=0; board < old_arregmap->regmap->nboard_; board++) {
	
	// Get the next board of the old register map.
	
	RegMapBoard* old_brd = old_arregmap->regmap->boards_[board];
	
	// See if a board of the same name exists in the new register
	// map.
	
	RegMapBoard* new_brd = find_RegMapBoard(new_arregmap->regmap, 
						old_brd->name);
	if(new_brd) {
	  
	  // Traverse the register blocks of the old board and compare
	  // them with those in the new board.
	  
	  for(block=0; block < old_brd->nblock; block++) {
	    
	    // Get the next block of the old register map.
	    
	    RegMapBlock *old_blk = old_brd->blocks[block];
	    
	    // See if a block of the same name exists in the new
	    // register map.
	    
	    RegMapBlock *new_blk = find_RegMapBoard_Block(new_brd, 
							  old_blk->name_);
	    if(new_blk) {
	      
	      // Was this an archived register and is it still
	      // archived?
	      
	      if(old_blk->slot_ > 0 && new_blk->slot_ > 0) {
		
		// Determine the number of archived slots in the old
		// mapping that can be preserved in the new mapping.
		
		int nslot = old_blk->nreg_ <= new_blk->nreg_ ? old_blk->nreg_ :
		  new_blk->nreg_;
		
		// Get the starting archive slot indexes of the old
		// and new blocks.
		
		int old_slot = old_blk->slot_;
		int new_slot = new_blk->slot_;
		
		// If the register is marked as a complex or utc pair
		// in both register maps then we should attempt to
		// preserve all of its aspects. Otherwise we should
		// only preserve the plain aspect.
		
		if(new_blk->flags_ & old_blk->flags_ & REG_COMPLEX ||
		   new_blk->flags_ & old_blk->flags_ & REG_UTC) {
		  for(j=0; j<REG_NASPECT; j++) {
		    for(i=0; i < nslot; i++)
		      new_offset[new_slot+i].a[j] = old_offset[old_slot+i].a[j];
		  };
		} else {
		  for(i=0; i < nslot; i++) {
		    new_offset[new_slot+i].a[REG_PLAIN] =
		      old_offset[old_slot+i].a[REG_PLAIN];
		  };
		};
	      };
	    };
	  };
	};
      };
    };
  };
  
  // Clear the array that maps from buffer-location to archived
  // register slot.
  
  for(i=0; i<new_regs_dim; i++)
    new_map->regs[i].slot = -1;
  
  // Now traverse the newly constructed offset[] mapping array to work
  // out its reverse mapping. This maps from buffer-frame offset to
  // slot number. For the moment override the normal use of the
  // 'oldest' member to record the buffer offset. The offset is
  // currently the same as the index of the reverse mapping element,
  // but we are about to reorder the register entries, so we need to
  // keep a record of which is which.
  
  for(j=0; j<REG_NASPECT; j++) {
    for(i=0; i<new_map->arraymap_size; i++) {
      int offset = new_offset[i].a[j];
      if(offset >= 0) {
	if(offset < new_regs_dim) {
	  MonitorBuffReg *old_reg = old_map->regs + offset;
	  MonitorBuffReg *new_reg = new_map->regs + offset;
	  new_reg->oldest = offset;
	  new_reg->slot = i;
	  new_reg->aspect = old_reg->aspect;
	} else {
	  new_offset[i].a[j] = -1;
	};
      };
    };
  };
  /*
   * Sort the register entries into ascending order of archive slot number
   * followed by the aspect. Treat slot numbers of -1 as greater than any other
   * number. This will move unarchived registers to the end of the array.
   */
  {
    int nregs = old_map->nregs > new_regs_dim ? new_regs_dim : old_map->nregs;
    qsort(new_map->regs, nregs, sizeof(MonitorBuffReg), cmp_MonitorBuffReg);
    /*
     * Count the remaining entries and the number of entries per aspect.
     */
    for(i=0; i < nregs && new_map->regs[i].slot >= 0; i++)
      new_map->nbuf[new_map->regs[i].aspect]++;
    new_map->nregs = i;
  };
  /*
   * The above sort changed the locations at which bufferd archive slots
   * will be recorded in the monitor buffer. Update the offset[] array
   * to record the corrected buffer offsets.
   */
  for(i=0; i<new_map->nregs; i++) {
    MonitorBuffReg *new_reg = new_map->regs + i;
    new_offset[new_reg->slot].a[new_reg->aspect] = i;
  };
  /*
   * If there are no preservable registers in the buffer, clear the
   * new buffer layout object.
   */
  if(new_map->nregs < 1) {
    clr_MonitorBuff(view);
  } else {
    /*
     * Arrange for the oldest of the preserved frames to be copied to just
     * after the oldest of the currently buffered frames. Since the buffer
     * is treated as circular, this will provide the most room
     * to copy legacy frames before potentially overwriting old frames.
     */
    new_map->oldest = (old_map->nregs *
		       ((old_map->newest+1) % old_map->max_snap) +
		       new_map->nregs - 1) / new_map->nregs;
    /*
     * Determine how many of the legacy frames can be copied to new frames
     * (starting at new_map->oldest), such that the last new frame can be
     * written without impinging on the last old frame. This ensures that
     * none of the legacy frames that are to be copied are overwritten by
     * the copies of older frames before they themselves can be copied.
     */
    new_map->nsnap = ((old_map->max_snap-1) * old_map->nregs) / new_map->nregs;
    /*
     * Limit the number of snapshots to preserve to the number of legacy
     * frames that are actually available.
     */
    if(new_map->nsnap > old_map->nsnap)
      new_map->nsnap = old_map->nsnap;
    /*
     * Compute the frame index of the new frame that will correspond to
     * the newest legacy frame.
     */
    new_map->newest = new_map->oldest + new_map->nsnap - 1;
    /*
     * Compute the number of complete snapshots that will fit in the
     * buffer.
     */
    new_map->max_snap = view->mb->data_dim / new_map->nregs;
    /*
     * Copy the legacy values of common registers to their new
     * buffer-frames.
     */
    for(old_snap=old_map->newest - new_map->nsnap + 1, new_snap=new_map->oldest;
	old_snap <= old_map->newest;
	old_snap++, new_snap++) {
      /*
       * Get pointers to the source and destination frames.
       */
      double *old_frame = mb->data +
	(old_snap % old_map->max_snap) * old_map->nregs;
      double *new_frame = mb->data +
	(new_snap % new_map->max_snap) * new_map->nregs;
      /*
       * Copy common registers from the old frame to the new frame.
       * Note that new_reg->oldest is currently being used to record the
       * old buffer position of the register associated with the new entry.
       */
      for(i=0; i<new_map->nregs; i++) {
	MonitorBuffReg *new_reg = new_map->regs + i;
	new_frame[i] = old_frame[new_reg->oldest];
      };
    };
    /*
     * Compute the frame indexes of the oldest buffered registers.
     */
    for(i=0; i<new_map->nregs; i++) {
      MonitorBuffReg *new_reg = new_map->regs + i;
      new_reg->oldest = old_map->regs[new_reg->oldest].oldest +
	new_map->newest - old_map->newest;
    };
    /*
     * Convert the 'oldest' indexes of each plot to the new numbering scheme.
     */
    {
      MonitorPlot *plot;
      for(plot=view->plots; plot; plot=plot->next)
	plot->oldest += new_map->newest - old_map->newest;
    };
  };
  /*
   * Discard the old monitor buffer map.
   */
  old_map = del_MonitorBuffMap(old_map);
  return 0;
}

/*.......................................................................
 * This is a private function of remap_MonitorBuff(). It is used with
 * qsort to sort MonitorBuffReg elements into ascending order of archive slot
 * number and derived aspect.
 */
static int cmp_MonitorBuffReg(const void *v1, const void *v2)
{
  const MonitorBuffReg *b1 = (const MonitorBuffReg* )v1;
  const MonitorBuffReg *b2 = (const MonitorBuffReg* )v2;
  /*
   * Arrange for unarchived slots to be sorted to the end of the array, by
   * implying that -1 is greater than any other number.
   */
  if(b1->slot < 0)
    return 1;
  if(b2->slot < 0)
    return -1;
  /*
   * If the slot indexes differ then their order determine the return
   * value.
   */
  if(b1->slot < b2->slot)
    return -1;
  if(b1->slot > b2->slot)
    return 1;
  /*
   * b1->slot == b2->slot
   *
   * Since the slot indexes are the same, use the aspect to determine
   * the return value. Put the most likely case first.
   */
  if(b1->aspect == b2->aspect)
    return 0;
  if(b1->aspect < b2->aspect)
    return -1;
  /*
   * b1->aspect > b2->aspect.
   */
  return 1;
}

/*.......................................................................
 * If there is insufficient precision to divide a given axis range into
 * at least MP_AXIS_PREC distinct parts, widen it accordingly. Also sort
 * the axis limits into ascending order.
 *
 * Axis labels are computed between amin..amax using double's, whereas
 * plotting is done with float's between 0..max-min, so the above
 * requirement translates to us needing to be able to split the
 * double precision range min..max into MP_AXIS_PREC parts, and
 * to be able to split the single precision range 0..max-min into
 * MP_AXIS_PREC.
 *
 * Input/Output:
 *  a1,a2    double *  On input provide the two ends of the axis range
 *                     in any order.
 *                     On output *a1 will be less than *a2 by at
 *                     least the required precision.
 */
static void vet_axis_limits(double *a1, double *a2)
{
  double amin, amax;  /* The vetted axis range */
  double ymag;        /* The largest magnitude on the y-axis */
  double ydiff;       /* The y-axis range */
  
  // Sort the limits so that amin > amax.

  if(*a1 < *a2) {
    amin = *a1;
    amax = *a2;
  } else {
    amin = *a2;
    amax = *a1;
  };
  
  // Determine the largest magnitude on the axis.

  ymag = fabs(amin) > fabs(amax) ? fabs(amin) : fabs(amax);
  
  // Get the range covered by the axis.

  ydiff = amax - amin;
  
  // If double's don't have sufficient precision to divide the range
  // amin..amax into MP_AXIS_PREC parts, or float's don't have
  // sufficient precision to divide the range 0..amax-amin into
  // MP_AXIS_PREC parts, widen the range accordingly.

  if(ydiff <= FLT_MIN * MP_AXIS_PREC ||
     ydiff <= ymag * DBL_EPSILON * MP_AXIS_PREC) {
    double dbl_margin = ymag * DBL_EPSILON * MP_AXIS_PREC/2.0;
    double flt_margin = FLT_MIN * MP_AXIS_PREC/2.0;
    double margin = dbl_margin > flt_margin ? dbl_margin : flt_margin;
    double ymid = (amin + amax) / 2.0;
    *a1 = ymid - margin;
    *a2 = ymid + margin;
  } else {
    *a1 = amin;
    *a2 = amax;
  };

  return;
}

/*.......................................................................
 * Return the tag ID of a given monitor plot.
 *
 * Input:
 *  plot   MonitorPlot *  The plot to identify.
 * Output:
 *  return    unsigned    The tag ID of the plot (0 if plot==NULL).
 */
unsigned tag_of_MonitorPlot(MonitorPlot *plot)
{
  return plot ? plot->tag : 0;
}

/*.......................................................................
 * Return the tag ID of a given monitor graph.
 *
 * Input:
 *  graph   MonitorGraph *  The graph to identify.
 * Output:
 *  return    unsigned    The tag ID of the graph (0 if graph==NULL).
 */
unsigned tag_of_MonitorGraph(MonitorGraph *graph)
{
  return graph ? graph->tag : 0;
}

/*.......................................................................
 * Return the tag ID of a given monitor page.
 *
 * Input:
 *  page   MonitorPage *  The page to identify.
 * Output:
 *  return    unsigned    The tag ID of the page (0 if page==NULL).
 */
unsigned tag_of_MonitorPage(MonitorPage *page)
{
  return page ? page->tag : 0;
}

/*.......................................................................
 * Return the tag ID of a given monitor field.
 *
 * Input:
 *  field   MonitorField *  The field to identify.
 * Output:
 *  return      unsigned    The tag ID of the field (0 if field==NULL).
 */
unsigned tag_of_MonitorField(MonitorField *field)
{
  return field ? field->tag : 0;
}

/*.......................................................................
 * Attempt to resize the monitor buffer.
 *
 * Input:
 *  view  MonitorViewer *  The viewer resource object.
 *  size         size_t    The new size.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
int mp_resize_buffer(MonitorViewer *view, size_t size)
{
  MonitorBuff *mb;     /* The container of the buffer to be resized */
  MonitorBuffMap *map; /* The current buffer layout map (if any) */
  /*
   * Check arguments.
   */
  if(!view) {
    lprintf(stderr, "mp_resize_buffer: NULL argument.\n");
    return 1;
  };
  if(size <= 1) {
    lprintf(stderr, "Unexpected 0 or negative buffer size.\n");
    return 1;
  };
  /*
   * Get the buffer container.
   */
  mb = view->mb;
  /* 
   * Get the buffer layout map. This will be NULL if no register selection
   * is currently registered.
   */
  map = mb->map;
  /*
   * Does the buffer already have the desired size?
   */
  if((int)size == mb->data_dim) {
    return 0;
    /*
     * If no register selection has been established, simply resize the
     * buffer.
     */
  } else if(!map || map->nregs==0) {
    double *data = (double* )realloc(mb->data, size * sizeof(double));
    if(!data) {
      lprintf(stderr, "Insufficient memory to resize the monitor buffer.\n");
      return 1;
    };
    mb->data = data;
    mb->data_dim = size;
    /*
     * If the new size is too small to contain at least one frame of the
     * currently selected registers, complain.
     */
  } else if((int)size < map->nregs) {
    lprintf(stderr, "A buffer size of at least %ld is needed for the currently selected registers.\n", map->nregs);
    return 1;
  } else if((int)size > mb->data_dim) {
    return expand_MonitorBuff(view, size);
  } else {
    return shrink_MonitorBuff(view, size);
  };
  return 0;
}

/*.......................................................................
 * This is a private function of mp_resize_buffer(), used to reduce the
 * size of the monitoring buffer to a specified number of elements.
 * It attempts to preserve as many frames as possible of the previously
 * buffered data.
 *
 * Input:
 *  view       MonitorViewer *   The viewer resource object.
 *  size                int     The number of elements in the shrunk
 *                               buffer.
 * Output:
 *  return               int     0 - OK.
 *                               1 - Error (the buffer will be left
 *                                   with its original size).
 */
static int shrink_MonitorBuff(MonitorViewer *view, int size)
{
  MonitorBuff *mb=view->mb;    /* The container of the monitoring buffer */
  MonitorBuffMap *map=mb->map; /* The buffer layout map */
  double *data;                /* The resized buffer */
  MonitorPlot *plot;           /* A plot having its age updated */
  int reg;                     /* The buffer offset of a register */
  /*
   * Compute the number of snapshots that will fit in the resized buffer.
   */
  int max_snap = size / map->nregs;
  /*
   * Compute the number of already buffered snapshots to preserve.
   */
  int nwanted = map->nsnap > max_snap ? max_snap : map->nsnap;
  /*
   * Work out the start and end indexes of the part of the
   * circular buffer that is to be preserved.
   */
  int oldest = (map->newest - (nwanted-1)) % map->max_snap;
  int newest = map->newest % map->max_snap;
  /*
   * If the buffered data is contiguous, move it to the start of the
   * buffer, where it will be preserved when the buffer is shrunk.
   */
  if(oldest <= newest) {
    memmove(mb->data, mb->data + oldest * map->nregs,
	    nwanted * map->nregs * sizeof(double));
    /*
     * If the circularly buffered data straddles the end of the physical buffer
     * copy the old data at the end of the current buffer, to what will be
     * the end of the shrunken buffer.
     */
  } else {
    memmove(mb->data + (size - (nwanted - newest - 1)) * map->nregs,
	    mb->data + oldest * map->nregs,
	    (nwanted - newest - 1) * map->nregs * sizeof(double));
  };
  /*
   * Attempt to resize the buffer.
   */
  data = (double *)realloc(mb->data, size * sizeof(double));
  /*
   * If the allocation failed, we must restore the buffer layout to
   * a consistent state, without discarding any more data than we
   * had to do above.
   */
  if(!data) {
    lprintf(stderr, "Insufficient memory to resize the monitor buffer.\n");
    if(oldest < newest)
      memmove(mb->data + oldest * map->nregs, mb->data,
	      nwanted * map->nregs * sizeof(double));
    else
      memmove(mb->data + oldest * map->nregs,
	      mb->data + (size - (nwanted - newest - 1)) * map->nregs,
	      (nwanted - newest - 1) * map->nregs * sizeof(double));
    map->oldest = map->newest - nwanted;
    return 1;
  };
  /*
   * Compute the limits of the preserved data in the resized buffer,
   * with newest = oldest + nwanted.
   */
  if(oldest < newest) {
    oldest = 0;
    newest = nwanted - 1;
  } else {
    newest += size;
    oldest = newest - nwanted;
  };
  /*
   * Convert all buffer indexes to the new scheme.
   */
  for(plot=view->plots; plot; plot=plot->next) {
    int age = map->newest - plot->oldest + 1;
    plot->oldest = age < nwanted ? newest - age : oldest;
  };
  for(reg=0; reg<map->nregs; reg++) {
    int age = map->newest - map->regs[reg].oldest + 1;
    map->regs[reg].oldest = age < nwanted ? newest - age : oldest;
  };
  map->newest = newest;
  map->oldest = oldest;
  map->nsnap = nwanted;
  map->max_snap = max_snap;
  /*
   * Install the new buffer.
   */
  mb->data = data;
  mb->data_dim = size;
  /*
   * Redraw all plots.
   */
  for(plot=view->plots; plot; plot=plot->next)
    update_MonitorPlot(plot, 1);
  return 0;
}

/*.......................................................................
 * This is a private function of mp_resize_buffer(), used to increase the
 * size of the monitoring buffer to a specified number of elements.
 * It preserves all currently buffered register frames.
 *
 * Input:
 *  view       MonitorViewer *   The viewer resource object.
 *  size                int     The number of elements in the shrunk
 *                               buffer.
 * Output:
 *  return               int     0 - OK.
 *                               1 - Error (the buffer will be left
 *                                   with its original size).
 */
static int expand_MonitorBuff(MonitorViewer *view, int size)
{
  MonitorBuff *mb=view->mb;    /* The container of the monitoring buffer */
  MonitorBuffMap *map=mb->map; /* The buffer layout map */
  double *data;                /* The resized buffer */
  MonitorPlot *plot;           /* A plot having its age updated */
  int reg;                     /* The buffer offset of a register */
  /*
   * Compute the number of snapshots that will fit in the resized buffer.
   */
  int max_snap = size / map->nregs;
  /*
   * Work out the extent of the buffered data in the resized buffer,
   * modulo its new size.
   */
  int oldest = map->oldest % map->max_snap;
  int newest = oldest + map->nsnap - 1;
  /*
   * Attempt to expand the buffer to the new size.
   */
  data = (double* )realloc(mb->data, size * sizeof(double));
  if(!data) {
    lprintf(stderr, "Insufficient memory to resize the monitor buffer.\n");
    return 1;
  };
  /*
   * If the buffered data wraps around the end of the old buffer,
   * copy the old frames that were at the end of the buffer to
   * the end of the new buffer.
   */
  if(newest >= map->max_snap) {
    int n = map->max_snap - oldest;
    memmove(data + (max_snap - n) * map->nregs, data + oldest * map->nregs,
	    n * map->nregs * sizeof(double));
    oldest = max_snap - n;
    newest = oldest + map->nsnap - 1;
  };
  /*
   * Convert all buffer indexes to the new scheme.
   */
  for(plot=view->plots; plot; plot=plot->next) {
    int age = map->newest - plot->oldest + 1;
    plot->oldest = age < map->nsnap ? newest - age : oldest;
  };
  for(reg=0; reg<map->nregs; reg++) {
    int age = map->newest - map->regs[reg].oldest + 1;
    map->regs[reg].oldest = age < map->nsnap ? newest - age : oldest;
  };
  map->newest = newest;
  map->oldest = oldest;
  map->max_snap = max_snap;
  /*
   * Install the new buffer.
   */
  mb->data = data;
  mb->data_dim = size;
  return 0;
}

/*.......................................................................
 * Return the current X-axis limits of a given plot.
 *
 * Input:
 *  plot   MonitorPlot *   The plot to query.
 * Input/Output:
 *  xmin,xmax   double *   The requested X-axis range.
 * Output:
 *  return         int     0 - OK.
 *                         1 - Error.
 */
int mp_xaxis_limits(MonitorPlot *plot, double *xmin, double *xmax)
{
  // Check the arguments.

  if(!plot || !xmin || !xmax) {
    lprintf(stderr, "mp_xaxis_limits: NULL argument(s).\n");
    return 1;
  };

  *xmin = plot->wxa;
  *xmax = plot->wxb;

  return 0;
}
/*.......................................................................
 * Create a new image object. This contains a single frame for display
 * of a frame grabber image.
 *
 * Input:
 *  view   MonitorViewer *  The resource object of the monitor viewer.
 *  device          char *  The PGPLOT specification of the graphics
 *                          device to assign to the plot.
 * Output:
 *  return                  0 -- OK.
 *                          1 -- error.
 */
int open_MonitorImage(MonitorViewer *view, char *device)
{
  MonitorImage *image = &view->image;
  int i;
  /*
   * Check arguments.
   */
  if(!view || !device) {
    lprintf(stderr, "open_MonitorImage: NULL argument.\n");
    return 1;
  };
  /*
   * Attempt to open the specified PGPLOT device, but only if it's not already
   * open.
   */
  if(image->pgplot_id <= 0)
    image->pgplot_id = cpgopen(device);
  if(image->pgplot_id <= 0)
    return 1;
  /*
   * Initialize the container with sensible starting values.
   */
  memset(image->title, '\0', AXLAB_LEN);
  memset(image->xlab, '\0', AXLAB_LEN);
  image->zoomed = 0;
  image->display = 0;
  /*
   * Initialize the plot boundaries in arcseconds.
   */
  image->aspect =  0.8; /* The ratio of height to width of camera fov */
  image->fov    = 12.0;  

  image->wxa = image->xa = 
    - ((double)(GRABBER_XNPIX)/2 + 0.5) * image->fov/GRABBER_XNPIX * 60;

  image->wxb = image->xb = 
      ((double)(GRABBER_XNPIX)/2 - 0.5) * image->fov/GRABBER_XNPIX * 60;

  /*
   * Y-limits are foreshortened by 480/512 (NTSC standard is 512 columns by
   * 480 lines), and the display monitor aspect ratio is even more distended.
   */

  image->wya = image->ya = 
    - ((double)(GRABBER_YNPIX)/2 + 0.5) * image->fov * image->aspect/GRABBER_YNPIX * 60;

  image->wyb = image->yb = 
      ((double)(GRABBER_YNPIX)/2 - 0.5) * image->fov * image->aspect/GRABBER_YNPIX * 60;
  
  image->dogrid = image->dobull = 0;
  image->docompass = 1;
  image->docrosshair = 1;
  image->havePeak = false;
  image->backoff = 10;
  image->xpeak = 0.0;
  image->ypeak = 0.0;
  image->interval = 30; /* In arcseconds */
  image->wza = image->za = 0.0;
  image->wzb = image->zb = 1.0;
  image->vxa = MP_LEFT_MARGIN;
  image->vxb = 1.0f - MP_RIGHT_MARGIN;
  image->vya = MP_BOT_MARGIN;
  image->vyb = 1.0f - MP_TOP_MARGIN;
  image->contra = -1.0;
  image->bright = 0.5;
  
  image->nx = GRABBER_XNPIX;
  image->ny = GRABBER_YNPIX;

  image->view = view;
  
  image->cmap = NULL;
  for(i=0;i < n_std_cmap;i++) {
    if(strcmp(std_cmaps[i].name,"grey")==0)
      image->cmap = &std_cmaps[i];
  }
  /*
   * And set all pixel values in the image to 0.
   */
  for(i=0;i < image->nx*image->ny;i++)
    image->data[i] = 0.0;
  /*
   * Turn off new page prompting.
   */
  cpgvstd();
  cpgask(0);
  cpgpage();
  
  cpgsvp(0,1,0,1);
  
  cpgswin(image->wxa, image->wxb, image->wya, image->wyb);
  cpgwnad(image->wxa, image->wxb, image->wya, image->wyb);
  
  return 0;
}

/*.......................................................................
 * After receiving an image from the control program, this function will 
 * be called to set up intial values for needed plot variables.
 *
 * Input:
 *  view   MonitorViewer *  The resource object of the monitor viewer.
 *
 * Output:
 *  return                  0 -- OK.
 *                          1 -- error.
 */
static void init_MonitorImage(MonitorImage *image)
{
  /*
   * Set the display flag to true.
   */
  image->display = 1;
  image->havePeak = false;

  /*
   * Initialize the container with sensible starting values.
   */
  get_MonitorImage_greyscale(image);
  
  return;
}

/**.......................................................................
 * Display an image from the frame grabber.
 *
 * Input:
 *  graph   MonitorGraph *  The graph to update.
 *  oldest          int    The first snapshot to draw.
 *  newest          int    The last snapshot to draw.
 *  erase            int    If true, draw the snapshot data in the
 *                          background color to erase it.
 */
void draw_MonitorImage_data(MonitorViewer *view)
{
  MonitorImage *image = &view->image;
  float tr[6],dx,dy;

  /*
   * Direct PGPLOT output to the parent plot.
   */
  if(image->pgplot_id <= 0)
    return;

  /**
   * Only draw the data array if we've received an image from the
   * control program.
   */
  if(image->display) {

  cpgslct(image->pgplot_id);

  /*
   * Don't show the imagea until it has been redrawn.
   */
  cpgbbuf();

  /*
   * Set up the transformation matrix for the data array.
   */
  dx = (image->xb - image->xa)/(image->nx);
  dy = (image->yb - image->ya)/(image->ny);

  /*
   * Set the transformation matrix for the data array.
   */
  tr[0]=image->xa;
  tr[1]=dx;
  tr[2]=0.0;
  tr[3]=image->ya;
  tr[4]=0.0;
  tr[5]=dy;

  /*
   * Install the viewport and world coordinates of the graph.
   */
  cpgsci(1);
  cpgask(0);
  cpgpage();  
  cpgsch(1.0f);
  
  
  cpgsvp(0,1,(1.0-image->aspect)/2,1.0-(1.0-image->aspect)/2);
  
  cpgswin(image->wxa,image->wxb, image->wya, image->wyb);
  cpgwnad(image->wxa,image->wxb, image->wya, image->wyb);
  
  cpgpage();  

  /*
   * Only draw the data array if we've received an image from the control
   * program.
   */
  if(image->display) {
    /*
     * Compute the greyscale range of the currently displayed data.
     */
    get_MonitorImage_greyscale(image);
    
    cpgctab(image->cmap->l,image->cmap->r,image->cmap->g,image->cmap->b,
	    image->cmap->n,image->contra,image->bright);

    cpgimag(image->data, image->nx, image->ny, 1, image->nx, 1, image->ny,
	    image->wzb, image->wza,tr); 
  }
  /*
   * Draw the grid of requested width
   */
  if(image->dogrid) {
    int i,j,ngridx,ngridy;
    cpgsci(10);
    if(image->interval > 0) {
      ngridx = (int)((image->xb)/image->interval+1);
      ngridy = (int)((image->yb)/image->interval+1);
      for(i=0;i < ngridx;i++) {
	cpgmove( i*image->interval, image->ya);
	cpgdraw( i*image->interval, image->yb);
	cpgmove(-i*image->interval, image->ya);
	cpgdraw(-i*image->interval, image->yb);
      }
      for(j=0;j < ngridy;j++) {
	cpgmove(image->xa,  j*image->interval);
	cpgdraw(image->xb,  j*image->interval);
	cpgmove(image->xa, -j*image->interval);
	cpgdraw(image->xb, -j*image->interval);
      }
    }
  }
  /*
   * Draw the largest number of concentric rings which will fit in the window.
   */
  if(image->dobull) {
    int i,nring;
    
    cpgsci(7);
    if(image->interval > 0) {
      nring = (int)(image->yb/image->interval);
      cpgsfs(2);
      for(i=0;i < nring;i++) 
	cpgcirc(0.0,0.0,(i+1)*image->interval);
    }
  }
  /*
   * Finally, draw the compass lines, if requested.  Elevation axis is a vector
   * at an angle clockwise from vertical.
   */

  if(image->docompass) {
    
    float len = (image->wyb-image->wya)/3,xel,yel,xaz,yaz;

    xel = len*sin(image->angle);
    yel = len*cos(image->angle);
    xaz = len*sin(image->angle+M_PI/2);
    yaz = len*cos(image->angle+M_PI/2);
    
    cpgsci(1);

    xel = (image->ximdir == 1 ? xel : -xel);
    xaz = (image->ximdir == 1 ? xaz : -xaz);

    yel = (image->yimdir == 1 ? yel : -yel);
    yaz = (image->yimdir == 1 ? yaz : -yaz);
    
    cpgsci(5);
    cpgarro(0.0,0.0,xel,yel);
    cpgsci(1);

    cpgarro(0.0,0.0,xaz,yaz);

    /*
     * And write text 2 character heights away from the tips of the vectors.
     */
    len = len + 2.0*(image->wyb-image->wya)/40;
    xel = len*sin(image->angle);
    yel = len*cos(image->angle);
    xaz = len*sin(image->angle+M_PI/2);
    yaz = len*cos(image->angle+M_PI/2);

    xel = (image->ximdir == 1 ? xel : -xel);
    xaz = (image->ximdir == 1 ? xaz : -xaz);

    yel = (image->yimdir == 1 ? yel : -yel);
    yaz = (image->yimdir == 1 ? yaz : -yaz);

    /*
     * Write only if the coordinates ar inside the current display window.
     */
    if(xel >= image->wxa && xel <= image->wxb && 
       yel >= image->wya && yel <= image->wyb)
      cpgsci(5);
      cpgtext(xel,yel,"El");
      cpgsci(1);
    if(xaz >= image->wxa && xaz <= image->wxb && 
       yaz >= image->wya && yaz <= image->wyb)
      cpgtext(xaz,yaz,"Az");
  }
  
  // If requested to draw a cross-hair, do so now

  if(image->docrosshair && image->havePeak) {
    int ci, lw;
    cpgqci(&ci);
    cpgqlw(&lw);
    cpgsci(5);
    cpgslw(1);

    double xhalf = (image->wxa+image->wxb)/2;
    double yhalf = (image->wya+image->wyb)/2;

    // Draw a cross-hair cursor with a center region excised

    cpgmove(image->xpeak, image->wya);
    cpgdraw(image->xpeak, image->ypeak - image->backoff);

    cpgmove(image->xpeak, image->wyb);
    cpgdraw(image->xpeak, image->ypeak + image->backoff);

    cpgmove(image->wxa, image->ypeak);
    cpgdraw(image->xpeak - image->backoff, image->ypeak);

    cpgmove(image->wxb, image->ypeak);
    cpgdraw(image->xpeak + image->backoff, image->ypeak);

    cpgsci(ci);
    cpgslw(lw);
  }

  cpgebuf();
  }  
  return;
}

/*.......................................................................
 * Change the x-image direction
 */
void change_MonitorImage_ximdir(MonitorViewer* view, int dir)
{
  MonitorImage *image = &view->image;
  image->ximdir = dir;

  // If we are displaying the compass, update the image to redraw it

  if(image->docompass)
    draw_MonitorImage_data(view);
}

/*.......................................................................
 * Change the y-image direction
 */
void change_MonitorImage_yimdir(MonitorViewer* view, int dir)
{
  MonitorImage *image = &view->image;
  image->yimdir = dir;

  // If we are displaying the compass, update the image to redraw it

  if(image->docompass)
    draw_MonitorImage_data(view);
}

/*.......................................................................
 * Compute the image centroid | peak.
 *
 * Input:
 *  graph   MonitorGraph *  The graph to update.
 *  oldest          int    The first snapshot to draw.
 *  newest          int    The last snapshot to draw.
 *  erase            int    If true, draw the snapshot data in the
 *                          background color to erase it.
 */
void find_MonitorImage_centroid(MonitorViewer *view, double& xcntr, 
				double& ycntr, int centroid)
{
  MonitorImage *image = &view->image;
  double xmean=0.0,ymean=0.0,amean=0.0;
  double xmax=0.0,ymax=0.0,max=0.0,min=0.0;
  double xsum=0.0,ysum=0.0,asum=0.0;
  double dx,dy,x,y;
  int i,j,ind,n,first=1,maxind=0,maxi,maxj;
  /*
   * Only compute a centroid if we've received an image from the control
   * program.
   */
  dx = (image->xb - image->xa)/(image->nx);
  dy = (image->yb - image->ya)/(image->ny);
  
  if(image->display) {
    xsum = ysum = asum = 0.0;
    for(n=0,j=0;j < image->ny;j++)
      for(i=0;i < image->nx;i++) {
	ind = j*image->nx + i;
	x = image->xa + dx*i;
	y = image->ya + dy*j;
	/*
	 * Only compute the peak intensity for pixels within the currently
	 * displayed limits.
	 */
	if(x <= image->wxb && x >= image->wxa &&
	   y <= image->wyb && y >= image->wya) {
	  /* 
	   * Compute the brightness-weighted mean x and mean y.
	   */
	  if(first) {
	    xmax = x;
	    ymax = y;
	    min = max = image->data[ind];
	    maxi = i;
	    maxj = j;
	    maxind = ind;
	    first = 0;
	  }
	  if(image->data[ind] > max) {
	    max = image->data[ind];
	    xmax = x;
	    ymax = y;
	    maxi = i;
	    maxj = j;
	    maxind = ind;
	  }
	  
	  min = (image->data[ind] < min) ? image->data[ind] : min;
	  
	  xmean += (x*image->data[ind] - xmean)/(n+1);
	  ymean += (y*image->data[ind] - ymean)/(n+1);
	  amean += (image->data[ind] - amean)/(n+1);
	  n++;
	}
      }
    /*
     * Return the peak or centroid
     */
    x = centroid ? xmean/amean : xmax;
    y = centroid ? ymean/amean : ymax;

    image->havePeak = true;

    image->xpeak = x;
    image->ypeak = y;

    if(image->docrosshair)
      draw_MonitorImage_data(view);
  }
  else {
    x = 0.0;
    y = 0.0;
  }


  // Assign return values

  xcntr = x;
  ycntr = y;
  
  return;
}

/**.......................................................................
 * Set the centroid
 */
void set_MonitorImage_centroid(MonitorViewer *view, unsigned ipeak, unsigned jpeak)
{
  MonitorImage *image = &view->image;

  double dx = (image->xb - image->xa)/(image->nx);
  double dy = (image->yb - image->ya)/(image->ny);

  image->havePeak = true;

  image->xpeak =   image->xa + dx*ipeak;
  image->ypeak = -(image->ya + dy*jpeak);

  if(image->docrosshair)
    draw_MonitorImage_data(view);
}

/*.......................................................................
 * Fiddle the contrast and brightness of a frame grabber image.
 *
 * Input:
 *  graph   MonitorGraph *  The graph to update.
 *  oldest          int    The first snapshot to draw.
 *  newest          int    The last snapshot to draw.
 *  erase            int    If true, draw the snapshot data in the
 *                          background color to erase it.
 */
void fid_MonitorImage_contrast(MonitorViewer *view, double xa, double ya)
{
  MonitorImage *image = &view->image;
  
  double contra,bright;
  double ymid = (image->wyb+image->wya)/2;
  /*
   * Compute the contrast and brightness from the cursor position.
   */
  contra = 5.0 * (ya-ymid)/(ya < ymid ? (image->wya-ymid) : -(image->wyb-ymid));
  bright = 0.5 + 1.0 * (fabs(contra)+1.0) *
    ((xa - image->wxb)/(image->wxa - image->wxb) - 0.5);
  
  if(image->cmap)
    cpgctab(image->cmap->l,image->cmap->r,image->cmap->g,image->cmap->b,
	    image->cmap->n,contra,bright);
  else 
    fprintf(stderr,"fid_MonitorImage_contrast: Intercepted NULL colormap.\n");
  
  image->contra = contra;
  image->bright = bright;
  
  return;
};
/*.......................................................................
 * Change the colormap of a frame grabber image display.
 *
 * Input:
 *  image   MonitorImage *  The image to update.
 *  name    char         *  The name of the colormap to install.
 */
int install_MonitorImage_colormap(MonitorViewer *view, char *name)
{
  MonitorImage *image = &view->image;
  
  int i;
  for(i=0;i < n_std_cmap;i++)
    if(strcmp(name,std_cmaps[i].name)==0) {
      image->cmap = &std_cmaps[i];
      cpgctab(image->cmap->l,image->cmap->r,image->cmap->g,image->cmap->b,
	      image->cmap->n,image->contra,image->bright);
      return 0;
    }
  lprintf(stderr,"Unrecognized colormap: %s.\n",name);
  return 1;
};
/*.......................................................................
 * Change the colormap of a frame grabber image display.
 *
 * Input:
 *  image   MonitorImage *  The image to update.
 *  name    char         *  The name of the colormap to install.
 */
int reset_MonitorImage_contrast(MonitorViewer *view)
{
  MonitorImage *image = &view->image;
  
  image->contra = -1.0;
  image->bright = 0.5;
  
  return 0;
};
/*.......................................................................
 * Reset the colormap of a frame grabber image display.
 *
 * Input:
 *  image   MonitorImage *  The image to update.
 *  name    char         *  The name of the colormap to install.
 */
int reset_MonitorImage_colormap(MonitorViewer *view)
{
  MonitorImage *image = &view->image;
  
  if(image->cmap!=NULL)
    cpgctab(image->cmap->l,image->cmap->r,image->cmap->g,image->cmap->b,
	    image->cmap->n,image->contra,image->bright);
  else {
    lprintf(stderr,"Received NULL colormap.\n");
    return 1;
  }
  return 0;
};
/*.......................................................................
 * Change the range limits of the image display.
 *
 * Input:
 *  image   MonitorImage *  The image to update.
 *  name    char         *  The name of the colormap to install.
 */
void set_MonitorImage_range(MonitorViewer *view, double xa, double xb, double ya, double yb)
{
  MonitorImage *image = &view->image;
  
  double threshold=1.0/256;
  
  // Install the requested x and y limits.

  image->wxa = (fabs(xa-xb) < threshold) ? image->xa : (xa < xb ? xa : xb);
  image->wxb = (fabs(xa-xb) < threshold) ? image->xb : (xb > xa ? xb : xa);
  image->wya = (fabs(ya-yb) < threshold) ? image->ya : (ya < yb ? ya : yb);
  image->wyb = (fabs(ya-yb) < threshold) ? image->yb : (yb > ya ? yb : ya);
  
  return;
};
/*.......................................................................
 * Compute the range of greyscale levels to be displayed for the new plot
 * boundaries.
 *
 * Input:
 *  image   MonitorImage *  The image to update.
 */ 
void get_MonitorImage_greyscale(MonitorImage *image)
{
  /*
   * Compute the new displayed greylevel range.
   */
  float zmin=0.0,zmax=0.0,range;
  float dx,dy,x,y;
  int i,j,first=1;
  int index;
  /*
   * Set up the transformation matrix for the data array.
   */
  dx = (image->xb - image->xa)/(image->nx);
  dy = (image->yb - image->ya)/(image->ny);
  /*
   * Use a border of 5 pixels around the frame, since the camera appears to 
   * return junk on the left-hand side of the image.
   */  
  for(i=OPTCAM_IMAGE_BORDER;i < image->nx-OPTCAM_IMAGE_BORDER;i++) 
    for(j=0;j < image->ny;j++) {
      x = image->xa + i*dx;
      y = image->ya + j*dy;
      if(x <= image->wxb && x >= image->wxa &&
	 y <= image->wyb && y >= image->wya) {
	index = i+image->nx*j;
	if(first) {
	  zmin = zmax = image->data[index];
	  first = 0;
	}
	zmin = zmin < image->data[index] ? zmin : image->data[index];
	zmax = zmax > image->data[index] ? zmax : image->data[index];
      }
    }
  range = zmax-zmin;
  
  image->za = image->wza = zmin - 0.2*range;
  image->zb = image->wzb = zmax + 0.2*range;
  
  
  return;
};
/*.......................................................................
 * Compute statistics on the requested image boundaries.
 *
 * Input:
 *  image   MonitorImage  *     The image for which we want statistics.
 *  xa      double              Min x.
 *  xb      double              Max x.
 *  ya      double              Min y.
 *  yb      double              Max y.
 *
 * Input/Output:
 *  s       MonitorRegStats  *  The container for results.
 */ 
void get_MonitorImage_stats(MonitorViewer *view, double xmin, double xmax,
			    double ymin, double ymax, double *min, double *max,
			    double *mean, double *rms, int *npoint)
{
  MonitorImage *image = &view->image;
  /*
   * Compute the new displayed greylevel range.
   */
  double dx,dy,x,y,v,vs;
  int i,j,first=1;
  int index,dofull=0;
  
  dofull = (xmin == xmax && ymin == ymax);
  
  /*
   * Compute the pixel dimensions.
   */
  dx = (image->xb - image->xa)/(image->nx);
  dy = (image->yb - image->ya)/(image->ny);
  /*
   * Calculate stats here, ignoring the bottom 32 rows of the image.
   * We compute the mean and second moment on one pass, then compute the rms
   * from the two moments.
   */  
  *npoint = 0;
  for(i=0;i < image->nx;i++) 
    for(j=0;j < image->ny;j++) {
      x = image->xa + i*dx;
      y = image->ya + j*dy;
      if((x <= xmax && x >= xmin && y <= ymax && y >= ymin) || dofull) {
	index = i+j*image->nx;
	if(first) {
	  *min = *max = image->data[index];
	  first = 0;
	}
	*min = *min < image->data[index]? *min:image->data[index];
	*max = *max > image->data[index]? *max:image->data[index];
	v = image->data[index];
	vs = v*v;
	*mean += (v - *mean) / (double) ++(*npoint);
	*rms += (vs - *rms) / (double) *npoint;
      }
    }
  /*
   * Now convert the second moment to an rms.
   */
  *rms = sqrt(*rms - (*mean)*(*mean));
  
  return;
};
/*.......................................................................
 * Change the interval betwen bullseye rings.
 *
 * Input:
 *  view     MonitorViewer *  The parent viewer.
 *  interval double           The interval to install.
 */
void set_MonitorImage_step(MonitorViewer *view, double interval)
{
  MonitorImage *image = &view->image;
  
  image->interval = interval;
  
  return;
};

/*.......................................................................
 * Change the viewport size
 *
 * Input:
 *  view     MonitorViewer *  The parent viewer.
 *  fov      double           The new fov (arcseconds)
 */
void set_MonitorImage_fov(MonitorViewer *view, double fov)
{
  MonitorImage *image = &view->image;
  double fovold;
  fovold = image->fov;

  /*
   * Store the new field of view.
   */
  image->fov = fov;

  /*
   * And rescale the plot boundaries to the new fov.
   */
  image->xa  *= fov/fovold;
  image->xb  *= fov/fovold;
  image->wxa *= fov/fovold;
  image->wxb *= fov/fovold;
  image->ya  *= fov/fovold;
  image->yb  *= fov/fovold;
  image->wya *= fov/fovold;
  image->wyb *= fov/fovold;
  
  return;
};

/*.......................................................................
 * Change the aspect ratio
 *
 * Input:
 *  view     MonitorViewer *  The parent viewer.
 *  fov      double           The new fov (arcseconds)
 */
void set_MonitorImage_aspect(MonitorViewer *view, double aspect)
{
  MonitorImage *image = &view->image;
  double aspectold;
  
  aspectold = image->aspect;
  /*
   * Store the new aspect ratio
   */
  image->aspect = aspect;
  /*
   * And rescale the plot boundaries to the new fov.
   */
  image->ya  *= aspect/aspectold;
  image->yb  *= aspect/aspectold;
  image->wya *= aspect/aspectold;
  image->wyb *= aspect/aspectold;
  
  return;
}

/*.......................................................................
 * Update whether or not to draw a grid.
 *
 * Input:
 *  view     MonitorViewer *  The parent viewer.
 */
void toggle_MonitorImage_grid(MonitorViewer *view)
{
  MonitorImage *image = &view->image;
  
  image->dogrid = !image->dogrid;
  
  return;
};
/*.......................................................................
 * Update whether or not to draw a bullseye.
 *
 * Input:
 *  view     MonitorViewer *  The parent viewer.
 */
void toggle_MonitorImage_bullseye(MonitorViewer *view)
{
  MonitorImage *image = &view->image;
  
  image->dobull = !image->dobull;
  
  return;
};

/*.......................................................................
 * Update whether or not to draw a crosshair
 *
 * Input:
 *  view     MonitorViewer *  The parent viewer.
 */
void toggle_MonitorImage_crosshair(MonitorViewer *view)
{
  MonitorImage *image = &view->image;
  
  image->docrosshair = !image->docrosshair;
  
  return;
};

/*.......................................................................
 * Update whether or not to draw a compass
 *
 * Input:
 *  view     MonitorViewer *  The parent viewer.
 */
void toggle_MonitorImage_compass(MonitorViewer *view)
{
  MonitorImage *image = &view->image;
  
  image->docompass = !image->docompass;
  
  return;
};
/*.......................................................................
 * Update the compass angle
 *
 * Input:
 *  view     MonitorViewer *  The parent viewer.
 */
void set_MonitorImage_compass(MonitorViewer *view, double angle)
{
  MonitorImage *image = &view->image;
  
  image->angle = angle*M_PI/180;
  
  return;
};
/*.......................................................................
 * Reset the counters for all page-enabled registers
 *
 * This function is called whenever the ::allow_paging variable is reset to
 * 1.  Whenever this happens, we will also redraw any pages which may contain 
 * paging registers, so that the paging foreground color will be reset to the
 * default.
 */
int reset_MonitorField_counters(MonitorViewer *view)
{
  MonitorPage *page=NULL;
  MonitorField *field=NULL;
  int redraw;
  gcp::util::RegCal::RegCalData *cal = ms_RegCalData(view->ms);
  
  // Iterate through all fields of all pages, looking for registers
  // which are page-enabled.
  
  for(page=view->pages;page != NULL; page=page->next) {
    redraw = 0;
    for(field=page->fields;field != NULL; field=field->next) {
      if(field->dopage) {
	field->counter = 0;
	redraw = 1;
      }
    }
    
    // If any page-enabled registers were found, redraw the page to
    // update the foreground colors.
    
    if(redraw) 
      draw_MonitorPage(page, cal, 1);
  }
  return 0;
}

bool is_MonitorViewer_archivedOnly(MonitorViewer* view)
{
  return view->ms->archivedOnly_;
}

void outputRegName(OutputStream* output, 
		   MonitorGraph* graph, 
		   gcp::util::RegDescription& desc)
{
  // Find the trace to which this descriptor belongs
  
  MonitorTrace* trace = findTrace(graph->plot, desc);
  
  if(trace==0) 
    ThrowError("Register does not belong to any trace");

  // Figure out how to present this register:

  // If this is a traditional single-element trace, just print the
  // register specification

  if(graph->plot->isSingleXAxis()) {

    desc.output(output, outputStatMode(graph->plot));
    
    // Else we have multiple x-axes
    
  } else {
    
    // If these are single-element axes, print the
    // trace-specification, but with just the element that's relevant
    // to the stat'd register
    
    if(graph->plot->xAxisIsSingleElement()) {
      
      std::string tmpName(trace->regname);
      std::ostringstream os("");;
      
      std::string work = tmpName.substr(tmpName.find('.')+1);

      // Get the register-map name

      std::string regMapName = work.substr(0, work.find('.'));

      // Get the register name

      std::string regName = work.substr(work.find('.')+1);

      std::string::size_type idx = regName.find('[');
      
      if(idx == std::string::npos) 
	os << tmpName;
      else {
	gcp::util::Coord coord = desc.currentCoord();
	os << regMapName << "." << regName.substr(0, idx) << coord;
      }
      
      if(write_OutputStream(output, os.str().c_str()))
	ThrowError("Error writing to output stream");
      
    } else {
      desc.output(output, REG_OUTPUT_BLOCK);
    }
  }
}

/**.......................................................................
 * Return whether or not this plot needs to
 * be updated
 */
bool MonitorPlot::updateNeeded()
{
  if(type == PLOT_NORM)
    return (view->update_needed || xdescs->size() == 0);
  else
    return (view->update_needed);
}

/**.......................................................................
 * Return true if this plot has a valid x-axis
 */
bool MonitorPlot::xAxisIsValid()
{
  if(type == PLOT_NORM) {
    return xdescs->size() > 0;
  } else if(type == PLOT_POWSPEC) {
    return true;
  } else {
    ThrowError("Unhandled plot type");
  }
}

/**.......................................................................
 * Return true if this plot has a single-element x-axis
 */
bool MonitorPlot::xAxisIsSingleElement()
{
  if(type == PLOT_NORM) {
    return (xdescs->at(0).nEl() == 1);
  } else if(type == PLOT_POWSPEC) {
    return false;
  } else {
    ThrowError("Unhandled plot type");
    return false;
  }
}

/**.......................................................................
 * Update the x-axis statistics after new data have been read
 */
void MonitorPlot::updateXaxisStats(unsigned nread)
{
  if(type == PLOT_POWSPEC)
    return;

  // Get the buffer map.

  MonitorBuff*    mb  = view->mb;
  MonitorBuffMap* map = mb->map;

  // Update the x-axis statistics of the plot to account for the
  // new data.
  
  for(unsigned iReg=0; iReg < xdescs->size(); iReg++) {
    
    gcp::util::RegDescription& xdesc = xdescs->at(iReg);
    
    if(xdesc.iSlot() >= 0) {
      
      for(xdesc.begin(); !xdesc .isEnd(); ++xdesc) {

	int x_offset = map->offset[xdesc.currentSlot()].a[xdesc.aspect()];
	int snap;

	for(snap=map->newest-nread+1; snap<=map->newest; snap++) {

	  double *frame = mb->data + (snap % map->max_snap) * map->nregs;
	  double xvalue = frame[x_offset];

	  if(stats.empty) {
	    stats.empty = 0;
	    stats.xmin = stats.xmax = xvalue;
	  } else if(xvalue < stats.xmin) {
	    stats.xmin = xvalue;
	  } else if(xvalue > stats.xmax) {
	    stats.xmax = xvalue;
	  }

	} // End for(snap=map...
      } // End for(xdecs.begin()...
    } // End if(xdesc.iSlot >= 0)
  } // End for(unsigned iReg=0...

}

// Count the number of active traces in this plot

unsigned MonitorPlot::countTraces()
{
  unsigned count=0;

  for(MonitorGraph* graph = graphs; graph != 0; graph = graph->next)
    for(MonitorTrace* trace = graph->traces; trace != 0; trace = trace->next) 
      ++count;

  return count;
}

/**.......................................................................
 * Initialize power spectrum members.
 */
void MonitorPlot::initPowSpec(unsigned n, double dtInMilliSeconds, 
			      double& xleft, double& xright, bool linAxis) 
{
  // Convert from milliSeconds to nanoSeconds and set the delta T
  
  unsigned nanoSec = (unsigned)(dtInMilliSeconds * 1000000);

  bool changed = (powSpec.nSec != nanoSec || powSpec.npt != n);

  // If nothing changed, just return without doing anything

  if(changed) {

    powSpec.nSec = nanoSec;
    powSpec.dt->setTime(0, 0, nanoSec);
    
    // Get the frequency resolution corresponding to this transform
    
    *powSpec.dv = gcp::util::Dft::getFrequencyResolution(n, *powSpec.dt);
    
    // Allocate/resize the object to store the frequency axis of pow spec plots
    
    powSpec.npt    = n;
    powSpec.ntrans = Dft::transformSize(n);
    
    // And iterate over any existing traces to reinitialize the spectra
    
    for(MonitorGraph* graph=graphs; graph != 0; graph = graph->next)
      for(MonitorTrace* trace=graph->traces; trace != 0; trace = trace->next) {
	trace->powSpec->resize(powSpec.npt, powSpec.ntrans);
      }
    
    powSpec.xdataLinear->resize(powSpec.ntrans);
    powSpec.xdataLog->resize(powSpec.ntrans);
    
    // Now iterate over the xaxis, setting the values

    for(unsigned i=0; i < powSpec.ntrans; i++) {
      powSpec.xdataLinear->at(i) = i*powSpec.dv->Hz();
      powSpec.xdataLog->at(i) = 
	(i > 0) ? log10(powSpec.xdataLinear->at(i)) : 0.0;
    }
  }
    
  // If this is a power spectrum plot, the x limits come from the data
  // themselves, unless the user explicitly set the limits

  if(xright < 0.0 && xleft < 0.0 && type == PLOT_POWSPEC) {

    // If this is a log plot, set the left limit to the first non-zero
    // x-value, else set to 0

    if(!linAxis) {
      xleft  = powSpec.dv->Hz();
    } else {
      xleft  = 0.0;
    }

    xright = powSpec.ntrans * powSpec.dv->Hz();

    if(linAxis) {

      // Expand by a fraction of the total range covered

      double range = xright-xleft;
      
      xleft  -= 0.1 * range;
      xright += 0.1 * range;
    } else {

      // Expand by a fraction of the total range covered

      double range = log10(xright) - log10(xleft);
      
      xleft  = pow(10.0, log10(xleft)  - 0.1 * range);
      xright = pow(10.0, log10(xright) + 0.1 * range);
    }
  }

  powSpec.linAxis = linAxis;
}

//=======================================================================
// Methods of MonitorViewer
//=======================================================================

/**.......................................................................
 * Method called whenever new data are available to be read, to
 * erase any data from current plots whose slots in the ring buffer
 * will be overwritten by the new read
 */
void MonitorViewer::eraseOldDataIfNecessary(unsigned read_size)
{
  MonitorBuffMap* map = mb->map;

  // Make sure that at least read_size frames are free in the buffer
  // by erasing old frames.
  
  if(map->max_snap - map->nsnap < read_size) {
    int nold = read_size - (map->max_snap - map->nsnap);
    
    // If on-the-fly plotting is enabled for the current data stream
    // erase previously plotted data.

    if(!defer_display) {
      for(MonitorPlot* plot=plots; plot; plot=plot->next) {

	// Do nothing for power spectrum plots -- they don't need any
	// special treatment, regardless of the state of the ring buffer

	if(plot->type==PLOT_POWSPEC)
	  continue;

	MonitorGraph *graph;
	if(!plot->join || map->nsnap<2) { /* Erase points */
	  for(MonitorGraph* graph=plot->graphs; graph; graph=graph->next){
	    drawMonitorGraphData(graph, map->oldest, map->oldest+nold-1,1);

	    draw_MonitorGraph_axes(graph, 0);
	  }
    	} else {		/* Erase lines */
	  for(MonitorGraph* graph=plot->graphs; graph; graph=graph->next) {
	    drawMonitorGraphData(graph, map->oldest, map->oldest+nold, 1);

	    draw_MonitorGraph_axes(graph, 0);
	  }
	};
      };
    };
    
    // Mark the relinquished buffer space as empty.

    map->oldest += nold;
    map->nsnap  -= nold;
  }
}  

/**.......................................................................
 * After a new frame has been read from the stream, thie method is
 * called to copy calibrated (and possibly integrated) values into the
 * buffer
 */
void MonitorViewer::updateCalibratedRegisters() 
{
  MonitorBuffMap* map = mb->map;

  // Get the calibrated register frame that has just been read.
  
  gcp::util::RegCal::RegCalData* cal = ms_RegCalData(ms);
  
  // Record the existence of a new snapshot in the buffer.
  
  unsigned last = map->newest;
  map->newest++;
  
  // Copy the values of the currently selected registers into the
  // monitor buffer.
  
  {
    double *frame       = mb->data + (map->newest % map->max_snap)*map->nregs;
    double *lastFrame   = mb->data + (last % map->max_snap)*map->nregs;
    MonitorBuffReg *reg = map->regs; // The buffer entry being filled 
    int aspect;		// The aspect to be derived 
    double lastVal, newVal, result;
    
    // The register entries are grouped by how their values are to
    // be derived.
    
    for(aspect = 0; aspect < REG_NASPECT; aspect++) {
      int nregs = map->nbuf[aspect];
      switch(aspect) {
	
      case REG_PLAIN:
	for(unsigned i=0; i<nregs; i++,reg++) {
	  
	  // Plain regs will just be integrated as doubles
	  
	  lastVal = *lastFrame++;
	  newVal  = cal->slots_[reg->slot];
	  
	  switch(reg->intType & (REG_SUM|REG_UNION|REG_POSTAVG|REG_PREAVG)) {
	  case REG_NONE:
	    result = newVal;
	    break;
	  case REG_UNION:
	    result = 
	      (double)((unsigned int)newVal | (unsigned int)lastVal);
	    break;
	  case REG_SUM:
	    result = lastVal + newVal;
	    break;
	  default:
	    result = lastVal + (newVal - lastVal)/(++reg->nInt);
	    break;
	  }
	  
	  *frame++ = result;
	}
	break;
	
      case REG_REAL:
	{
	  gcp::util::Complex<float> cVal(0.0,0.0);
	  
	  for(unsigned i=0; i<nregs; i++,reg++) {
	    *cVal.data() = 
	      *((gcp::util::Complex<float>::Data*)&cal->slots_[reg->slot]);
	    
	    lastVal = *lastFrame++;
	    lastVal = reg->nInt==0 ? 0.0 : lastVal;
	    newVal  = cVal.real();
	    
	    switch(reg->intType & (REG_SUM|REG_UNION|REG_POSTAVG|REG_PREAVG)) {
	    case REG_NONE:
	      result = newVal;
	      break;
	    case REG_SUM:
	      result = lastVal + newVal;
	      break;
	    default:
	      result = lastVal + (newVal - lastVal)/(++reg->nInt);
	      break;
	    }
	    
	    *frame++ = result;
	  }
	}
	break;
	
      case REG_IMAG:
	{
	  gcp::util::Complex<float> cVal(0.0,0.0);
	  
	  for(unsigned i=0; i<nregs; i++,reg++) {
	    *cVal.data() = 
	      *((gcp::util::Complex<float>::Data*)&cal->slots_[reg->slot]);
	    
	    lastVal = *lastFrame++;
	    lastVal = reg->nInt==0 ? 0.0 : lastVal;
	    newVal  = cVal.imag();
	    
	    switch(reg->intType & (REG_SUM|REG_UNION|REG_POSTAVG|REG_PREAVG)) {
	    case REG_NONE:
	      result = newVal;
	      break;
	    case REG_SUM:
	      result = lastVal + newVal;
	      break;
	    default:
	      result = lastVal + (newVal - lastVal)/(++reg->nInt);
	      break;
	    }
	    
	    *frame++ = result;
	  }
	}
	break;
	
      case REG_AMP:
	{
	  gcp::util::Complex<float> cVal(0.0,0.0);
	  
	  for(unsigned i=0; i<nregs; i++,reg++) {
	    *cVal.data() = 
	      *((gcp::util::Complex<float>::Data*)&cal->slots_[reg->slot]);
	    
	    switch(reg->intType & (REG_SUM|REG_UNION|REG_POSTAVG|REG_PREAVG)) {
	    case REG_NONE:
	      *(reg->lastComplexVal) = cVal;
	      break;
	    case REG_SUM:
	      *(reg->lastComplexVal) += cVal;
	      break;
	    default:
	      *(reg->lastComplexVal) += 
		(cVal - *(reg->lastComplexVal))/(++reg->nInt);
	      break;
	    }
	    
	    *frame++ = reg->lastComplexVal->amp();
	  }
	}
	break;
	
      case REG_PHASE:
	{
	  gcp::util::Complex<float> cVal(0.0,0.0);
	  
	  for(unsigned i=0; i<nregs; i++,reg++) {
	    *cVal.data() = 
	      *((gcp::util::Complex<float>::Data*)&cal->slots_[reg->slot]);
	    
	    switch(reg->intType & (REG_SUM|REG_UNION|REG_POSTAVG|REG_PREAVG)) {
	    case REG_NONE:
	      *(reg->lastComplexVal) = cVal;
	      break;
	    case REG_SUM:
	      *(reg->lastComplexVal) += cVal;
	      break;
	    default:
	      *(reg->lastComplexVal) += 
		(cVal - *(reg->lastComplexVal))/(++reg->nInt);
	      break;
	    }
	    
	    *frame++ = reg->lastComplexVal->phaseInDegrees();
	  }
	}
	break;
	
      case REG_DATE:
	{
	  gcp::util::RegDate dVal;
	  
	  for(unsigned i=0; i<nregs; i++,reg++) {
	    *dVal.data() = 
	      *((gcp::util::RegDate::Data*)&cal->slots_[reg->slot]);
	    
	    switch(reg->intType & (REG_SUM|REG_UNION|REG_POSTAVG|REG_PREAVG)) {
	    case REG_NONE:
	      *(reg->lastDate) = dVal;
	      break;
	    case REG_SUM:
	      *(reg->lastDate) += dVal;
	      break;
	    default:
	      *(reg->lastDate) += 
		(dVal - *(reg->lastDate))/(++reg->nInt);
	      break;
	    }
	    
	    *frame++ = reg->lastDate->mjd();
	  }
	}
	break;
	
      case REG_TIME:
	{
	  gcp::util::RegDate dVal;
	  
	  for(unsigned i=0; i<nregs; i++,reg++) {
	    *dVal.data() = 
	      *((gcp::util::RegDate::Data*)&cal->slots_[reg->slot]);
	    
	    switch(reg->intType & (REG_SUM|REG_UNION|REG_POSTAVG|REG_PREAVG)) {
	    case REG_NONE:
	      *(reg->lastDate) = dVal;
	      break;
	    case REG_SUM:
	      *(reg->lastDate) += dVal;
	      break;
	    default:
	      *(reg->lastDate) += 
		(dVal - *(reg->lastDate))/(++reg->nInt);
	      break;
	    }
	    
	    *frame++ = reg->lastDate->timeInHours();
	  }
	}
	break;
      }
    };
  };
  
  // Keep a record of the number of buffered snapshots.
  
  map->nsnap++;
}

/**.......................................................................
 * Called after new data have been read to update pages
 */
void MonitorViewer::updatePages()
{
  for(MonitorPage* page=pages; page; page=page->next)
    draw_MonitorPage(page, ms_RegCalData(ms), 0);
}

/**.......................................................................
 * Called after new data have been read to update plots
 */
void MonitorViewer::updatePlots(unsigned nread)
{
  for(MonitorPlot* plot=plots; plot; plot=plot->next)
    plot->update(nread);
}

//=======================================================================
// MonitorPlot methods
//=======================================================================

/**.......................................................................
 * Called after new data have been read to update plots
 */
void MonitorPlot::update(unsigned nread)
{
  if(type == PLOT_NORM) {
    updateNormal(nread); 
  } else if(type == PLOT_POWSPEC) {
    updatePowSpec(nread); 
  } else {
    ThrowError("Unhandled plot type");
  }
}

/**.......................................................................
 * Iterate over all traces, pushing new data into the power spectra
 * containers.
 */
void MonitorPlot::pushPowSpecData() 
{ 
  // Iterate over graphs, calculating the new min/max for any power
  // spectra that were calculated

  for(MonitorGraph* graph=graphs; graph != 0; graph = graph->next) {

    // Initialize first to true, but set ymin and ymax to the current
    // min/max so that on return, if no transforms have been
    // calculated, they will be left unchanged

    bool first = true;
    double ymin = graph->powSpec.ymin;
    double ymax = graph->powSpec.ymax;

    for(MonitorTrace* trace=graph->traces; trace != 0; trace = trace->next) {
      trace->pushPowSpecData(first, ymin, ymax);
    }

    graph->powSpec.ymin  = ymin;
    graph->powSpec.ymax  = ymax;
  }
}

/**.......................................................................
 * Called after new data have been read to update plots
 */
void MonitorPlot::updatePowSpec(unsigned nread)
{
  pushPowSpecData();
  replotPowSpec();
}

/**.......................................................................
 * Iterate over all traces in this plot, replotting as necessary
 */
void MonitorPlot::replotPowSpec()
{ 
  // Only replot if one of this plot's graphs was updated

  if(powSpec.updateNeeded) {

    beginBufferedPowSpecAxisRedraw();
    
    // Iterate over all graphs, redrawing as necessary

    for(MonitorGraph* graph=graphs; graph != 0; graph = graph->next) {

      // Only replot if one of this graphs traces was  updated

      if(graph->powSpec.updateNeeded) {

	// First erase the old axis by overplotting in black

	draw_MonitorGraph_axes(graph, true);

	// Now iterate over all traces, erasing data for any that have changed

	int ci=0;
	for(MonitorTrace* trace=graph->traces; trace != 0;trace = trace->next) 
	  if(trace->powSpec->updateNeeded) 
	    trace->stepPowSpecState(ci, true);

	// Now reset the axis limits to the new data and redraw the
	// axis

	vet_axis_limits(&graph->powSpec.ymin, &graph->powSpec.ymax);

	// If tracking the spectrum, set the new range, else leave it
	// set to whatever the user specified

	if(graph->track) {
	  double wya, wyb, range;

	  if(graph->powSpec.linAxis) {
	    range = graph->powSpec.ymax - graph->powSpec.ymin;
	    graph->wya = graph->powSpec.ymin - 0.1 * range;
	    graph->wyb = graph->powSpec.ymax + 0.1 * range;
	  } else {
	    range = log10(graph->powSpec.ymax) - log10(graph->powSpec.ymin);
	    graph->wya = pow(10.0, log10(graph->powSpec.ymin) - 0.1 * range);
	    graph->wyb = pow(10.0, log10(graph->powSpec.ymax) + 0.1 * range);
	  }

	} 

	draw_MonitorGraph_axes(graph, false);

	// Finally draw the new data

	ci=MP_MIN_CI;
	for(MonitorTrace* trace=graph->traces; trace != 0;trace = trace->next) {

	  if(trace->powSpec->updateNeeded) {
	    trace->stepPowSpecState(ci, false);
	  }

	  powSpec.updateNeeded = false;
	}

	graph->powSpec.updateNeeded = false;
      }
    }

    // Reset the coordinate system and reveal the plot

    endBufferedPowSpecAxisRedraw();

    powSpec.updateNeeded = false;
  }
}

/**.......................................................................
 * Start a buffered redraw of the axes for this plot
 */
void MonitorPlot::beginBufferedPowSpecAxisRedraw()
{
  // Direct PGPLOT output to the target plot.
  
  cpgslct(pgplot_id);
  cpgbbuf();
}

/**.......................................................................
 * Called to end a buffered redraw of a power spectrum plot
 */
void MonitorPlot::endBufferedPowSpecAxisRedraw()
{
  // Re-establish the coordinate system used by external cursor
  // functions.
  
  cpgsvp( 0.0f, 1.0f, 0.0f, 1.0f);
  cpgswin(0.0f, 1.0f, 0.0f, 1.0f);
  
  // Reveal the updated plot.
  
  cpgebuf();
}

/**.......................................................................
 * Update this plot
 */
void MonitorPlot::updateNormal(unsigned nread)
{
  MonitorBuffMap* map = view->mb->map;

  int was_scrolled = 0;
  
  if(update_needed || stats.empty) {
    
    if(update_MonitorPlot(this, 1) == 0)
      was_scrolled = 1;
    
  } else {
    
    updateXaxisStats(nread);
    
    // Buffer the update.
    
    cpgslct(pgplot_id);
    cpgbbuf();
    
    // See if the plot should be scrolled.
    
    was_scrolled = scroll_MonitorPlot(this);
    
    // Draw the contents of the new frames in each of the graphs.
    
    MonitorGraph *graph;
    float ya, yb;
    if(!join || map->nsnap==1) {
      for(MonitorGraph* graph=graphs; graph; graph=graph->next) {
	
	if(checkYAxis(graph, map->newest-nread+1, map->newest, &ya, &yb)) {
	  drawMonitorGraphData(graph, map->oldest, map->newest, 1);
	  draw_MonitorGraph_axes(graph, 1);
	  graph->wya = ya;
	  graph->wyb = yb;
	  drawMonitorGraphData(graph, map->oldest, map->newest, 0);
	} else {
	  drawMonitorGraphData(graph, map->newest-nread+1, map->newest, 0);
	}
	
	draw_MonitorGraph_axes(graph, 0);
      }
      
    } else {		/* Draw lines from the last sample */
      for(MonitorGraph* graph=graphs; graph; graph=graph->next) {
	
	if(checkYAxis(graph, map->newest-nread+1, map->newest, &ya, &yb)) {
	  drawMonitorGraphData(graph, map->oldest, map->newest, 1);
	  draw_MonitorGraph_axes(graph, 1);
	  graph->wya = ya;
	  graph->wyb = yb;
	  drawMonitorGraphData(graph, map->oldest, map->newest, 0);
	} else {
	  drawMonitorGraphData(graph, map->newest-nread, map->newest, 0);
	}
	
	draw_MonitorGraph_axes(graph, 0);
      }
    };
    
    cpgebuf();
    
    /*
     * Re-establish the coordinate system used by external cursor
     * functions.
     */
    cpgsvp(0.0f, 1.0f, 0.0f, 1.0f);
    cpgswin(0.0f, 1.0f, 0.0f, 1.0f);
    
    /*
     * If any scrolling was needed, report the new X-axis limits.
     */
    if(was_scrolled && scroll.user_fn)
      scroll.user_fn(scroll.user_data,this,wxa,wxb);
  };
};

/**.......................................................................
 * Method to parse the xregister specification of a plot
 */
bool MonitorPlot::checkXregisters(std::vector<RegDescription>& descs)
{
  InputStream* stream = view->input;
  MonitorBuffMap* map = view->mb->map;
  RegParser parser;

  if(type != PLOT_POWSPEC) {
    
    // We will start by getting the x-axis registers -- with multiple
    // axes, we need to be able to check that the same number of axes
    // are specified in each multi-axis y-trace
    
    if(open_StringInputStream(stream, 0, regname)) {
      lprintf(stderr, "checkXregisters: InputStream failure.");
      return true;
    };
    
    try {
      descs = parser.inputRegs(stream, true, (map ? map->arraymap : 0), 
			       REG_INPUT_RANGE, true);
    } catch(...) {
      return false;
    }
    
    close_InputStream(stream);
  }

  return false;
}

/**.......................................................................
 * Check a set of yregisters for syntax and consistency with the
 * x-registers
 */
bool MonitorPlot::checkYregisters(char* yregs, 
				  std::vector<RegDescription>& descs,
				  std::vector<RegDescription>& ydescs)
{
  InputStream* stream = view->input;
  MonitorBuffMap* map = view->mb->map;
  RegParser parser;

  // Now read y-register specifications
  
  if(open_StringInputStream(stream, 0, yregs)) {
    lprintf(stderr, "checkYregisters: InputStream failure.");
    return true;
  };
  
  while(input_skip_white(stream, 1, 0)==0 && stream->nextc != EOF) {
    
    try {
      ydescs = parser.inputRegs(stream, true, (map ? map->arraymap : 0), 
				REG_INPUT_RANGE, true, (type == PLOT_POWSPEC));
    } catch(...) {}
    
    if(parser.validity() == REG_INVALID) {
      close_InputStream(stream);
      return true;
    }
    
    // For power spectrum plots, there is no x-axis.  We will however
    // re-use the logic of the normal plotting register checking by
    // copying the first y-axis register specified into the x-register
    // name place holder.  
    //
    // Hence the check if this is not a power spectrum plot, or if
    // this is the first power spectrum trace to be specified.
    
    if(type == PLOT_NORM || (type == PLOT_POWSPEC && !powSpec.first)) {
      
      // Check that the y-axes make sense for the other axes that have
      // been specified for this plot
      
      checkXaxisValidity(regname, descs, ydescs);
      
      // If this is a power spectrum plot, and it is the first
      // trace specified for this plot, copy it into xdescs, so
      // that subsequent register specifications will be checked
      // against it.
      
    } else {

      if(type==PLOT_POWSPEC && ydescs.size() > 0) {

	// Store the only number of elements that will be allowed for
	// the other traces of this plot

	powSpec.nel   = ydescs[0].nEl();
	powSpec.first = false;
      }

    }
    
  }
  
  close_InputStream(stream);

  return false;
}

//=======================================================================
// Methods of MonitorGraph
//=======================================================================

/**.......................................................................
 * Read trace specifications from a char array and add traces to
 * correspond
 */
bool MonitorGraph::addTraces(char* yregs)
{
  // Get the pointer to the first of the list of traces that currently
  // belong to the plot.
  
  MonitorTrace** link = &traces;
  MonitorTrace* trace=0;

  // Prepare to read register specifications from yregs[].
  
  InputStream* stream = plot->view->input;

  if(open_StringInputStream(stream, 0, yregs)) {
    lprintf(stderr, "addTraces: InputStream failure.");
    return true;
  };
  
  while(input_skip_white(stream, 1, 0)==0 && stream->nextc != EOF) {
    
    // Read the next register specification into stream->work[].
    
    if(input_word(stream, true, false)) {
      close_InputStream(stream);
      return true;
    };
    
    // If needed add a new trace to the list of traces belonging to
    // the current graph.
    
    if(!*link) {
      *link = new_MonitorTrace(this);
      if(!*link)
	return true;
    };
    
    trace = *link;
    link = &trace->next;
    
    // If the new register name differs from the one that is currently
    // registered to the trace, mark the viewer as needing to update
    // its register selection, and record the new register name.
    
    if(strcmp(stream->work, trace->regname) != 0) {

      flag_MonitorViewer(plot->view, 0);

      if(strlen(stream->work) > MP_REG_LEN) {
	lprintf(stderr, "Register name too long: %s\n", stream->work);
	return true;
      };
      
      strcpy(trace->regname, stream->work);
      
      // Invalidate the previously recorded register-map
      // specification.
      
      trace->descs->resize(0);
      
      // We don't yet know whether regname[] is invalid.
      
      trace->regname_invalid = 0;
    };
  };
  
  close_InputStream(stream);
  
  // If there are any remaining traces, delete them, and tell the
  // viewer that it will have to update its current register selection
  // to account for the deleted traces.
  
  if(*link) {
    MonitorTrace *next = *link;  // The next trace to be deleted 
    
    // Terminate the list of wanted traces.
    
    *link = NULL;
    
    // Delete the trailing list of unwanted traces.
    
    while(next) {
      trace = next;
      next  = trace->next;
      trace = del_MonitorTrace(trace);
    };
    
    // If the deletion left the parent plot with no more active
    // traces, reset the

    if(plot->type == PLOT_POWSPEC && plot->countTraces()==0) {
    }

    // Tell the viewer that a register update is needed.
    
    flag_MonitorViewer(plot->view, 0);
  }

  return false;
}

/**.......................................................................
 * Get the aspect of the x-axis register
 */
RegAspect MonitorGraph::getXAspect()
{
  switch (plot->type) {
  case PLOT_NORM:
    {

      RegAspect aspect = REG_DATE;
      
      for(unsigned iXreg=0; iXreg < plot->xdescs->size(); iXreg++)
	if(plot->xdescs->at(iXreg).aspect() != REG_DATE) {
	  aspect = plot->xdescs->at(iXreg).aspect();
	  break;
	}
      return aspect;
    }
    break;
  case PLOT_POWSPEC:
    return REG_PLAIN;;
    break;
  default:
    ThrowError("Unhandled plot type");
    break;
  }
}
  
//=======================================================================
// Methods of MonitorTrace
//=======================================================================

void MonitorTrace::
getStats(double xa, double xb, MonitorRegStats& s, RegDescription* desc)
{
  switch (graph->plot->type) {
  case PLOT_NORM:
    getStatsNorm(xa, xb, s, desc);
    break;
  case PLOT_POWSPEC:
    getStatsPowSpec(xa, xb, s, desc);
    break;
  default:
    ThrowError("Unhandled plot type");
    break;
  }
}

void  MonitorTrace::
getStatsNorm(double xa, double xb, MonitorRegStats& s, RegDescription* desc)
{
  MonitorPlot* plot = graph->plot;
  
  // Conduct a first pass to accumulate the mean
  
  if(plot->xdescs->size() == 1) {
    getStatsSa(this, xa, xb, s, desc);
    
    // Else we have multiple x-axes
    
  } else {
    
    // If these are single-element axes
    
    if(plot->xdescs->at(0).nEl() == 1) {
      getStatsMaSe(this, xa, xb, s, desc);
    } else {
      getStatsMaMe(this, xa, xb, s, desc);
    }
  }
  
  // Now take a second pass through the data to compute the rms from
  // the mean.
  
  s.npoint = 0;
  s.state = MRS_RMS;
  
  if(plot->xdescs->size() == 1) {
    getStatsSa(this, xa, xb, s, desc);
    
    // Else we have multiple x-axes
    
  } else {
    
    // If these are single-element axes
    
    if(plot->xdescs->at(0).nEl() == 1) 
      getStatsMaSe(this, xa, xb, s, desc);
    else 
      getStatsMaMe(this, xa, xb, s, desc);
  }
}

void  MonitorTrace::
getStatsPowSpec(double xa, double xb, MonitorRegStats& s, RegDescription* desc)
{
  MonitorPlot* plot = graph->plot;
  
  // Conduct a first pass to accumulate the mean
  
  getStatsSpectrum(xa, xb, s, desc);
  
  // Now take a second pass through the data to compute the rms from
  // the mean.
  
  s.npoint = 0;
  s.state = MRS_RMS;
  
  getStatsSpectrum(xa, xb, s, desc);
}

void  MonitorTrace::
getStatsSpectrum(double xa, double xb, MonitorRegStats& s, RegDescription* desc)
{
  double xval, yval;
  bool isLinear = graph->powSpec.linAxis;

  // Note that we skip the first element (DC component) in all
  // calculations

  float* xptr = graph->plot->powSpec.linAxis ? 
    &graph->plot->powSpec.xdataLinear->at(1) :
    &graph->plot->powSpec.xdataLog->at(1);

  unsigned npt = graph->plot->powSpec.ntrans-1;

  if(!graph->plot->powSpec.linAxis) {
    xa = log10(xa);
    xb = log10(xb);
  }

  for(unsigned iReg=0; iReg < descs->size(); iReg++) {

    // If we are checking stats for one trace only, skip if this one
    // doesn't match
    
    if(desc != 0 && !descs->at(iReg).contains(*desc))
      continue;

    float* yptr = powSpec->spectra[iReg].ydata->getReadBuffer()+1;

    for(unsigned i=0; i < npt; i++) {

      xval = xptr[i];
      yval = yptr[i];

      if(xval >= xa && xval <= xb) {
	s.insert(xval, isLinear ? yval : pow(10.0, yval));
      }
    }

    powSpec->spectra[iReg].ydata->releaseReadBuffer();

  }
}

void MonitorTrace::
findClosestPoint(double x, double y, double x_to_mm, double y_to_mm, 
		 bool& first, 
		 double& min_x, double& min_y, double& min_rsq,
		 RegDescription& closestReg)
{
  switch (graph->plot->type) {
  case PLOT_NORM:
    findClosestPointNorm(x, y, x_to_mm, y_to_mm, first, 
			 min_x, min_y, min_rsq, closestReg);
    break;
  case PLOT_POWSPEC:
    findClosestPointPowSpec(x, y, x_to_mm, y_to_mm, first, 
			    min_x, min_y, min_rsq, closestReg);
    break;
  default:
    ThrowError("Unhandled plot type");
    break;
  }
}

/**.......................................................................
 * Find the closest point for normal plots
 */
void MonitorTrace::
findClosestPointNorm(double x, double y, double x_to_mm, double y_to_mm, 
		     bool& first, double& min_x, double& min_y, double& min_rsq,
		     RegDescription& closestReg)
{
  MonitorPlot* plot = graph->plot;
  
  // Either we have a single axis
  
  if(plot->xdescs->size() == 1) {
    findClosestPointSa(this, x, y, x_to_mm, y_to_mm, 
		       closestReg, 
		       min_x, min_y, min_rsq, first);
    
    // Or we have multiple x-axes
    
  } else {
    
    // Either we have single-element axes
    
    if(plot->xdescs->at(0).nEl() == 1) 
      findClosestPointMaSe(this, x, y, x_to_mm, y_to_mm, 
			   closestReg, 
			   min_x, min_y, min_rsq, first);
    
    // Or multiple-element axes
    
    else 
      findClosestPointMaMe(this, x, y, x_to_mm, y_to_mm, 
			   closestReg, 
			   min_x, min_y, min_rsq, first);
  }
}

/**.......................................................................
 * Find the closest point for normal plots
 */
void MonitorTrace::
findClosestPointPowSpec(double x, double y, double x_to_mm, double y_to_mm, 
			bool& first, 
			double& min_x, double& min_y, double& min_rsq,
			RegDescription& closestReg)
{
  MonitorPlot* plot = graph->plot;

  bool eitherIsLog = !(graph->powSpec.linAxis && graph->plot->powSpec.linAxis);

  float* xptr = graph->plot->powSpec.linAxis ? 
    &plot->powSpec.xdataLinear->at(1) :
    &plot->powSpec.xdataLog->at(1);
  float* yptr=0;
  double dy, dx, rsq;
  unsigned minRegInd=0, minInd=0;
  bool found=false;

  unsigned npt = graph->plot->powSpec.ntrans-1;

  // Iterate over all registers that are part of this trace

  for(unsigned iReg=0; iReg < descs->size(); iReg++) {

    // Skip this spectrum if it hasn't been plotted yet

    if(powSpec->spectra[iReg].state != Spectrum::PS_DATA_PLOTTED)
      continue;

    yptr = powSpec->spectra[iReg].ydata->getReadBuffer()+1;

    // Iterate over all points in the transform, looking for the
    // shortest distance

    for(unsigned i=0; i < npt; i++) {

      dy = (yptr[i] - y) * y_to_mm;
      dx = (xptr[i] - x) * x_to_mm;
      rsq = dx*dx + dy*dy;

      if(first || rsq < min_rsq) {
	first = false;
	found = true;
	minRegInd = iReg;
	min_x = xptr[i];
	min_y = yptr[i];
	min_rsq = rsq;
      }

    }

    powSpec->spectra[iReg].ydata->releaseReadBuffer();

  }

  if(found) 
    closestReg = descs->at(minRegInd);
}

/**.......................................................................
 * Add the registers of this trace to the list to be monitored
 */
bool MonitorTrace::addRegs()
{
  MonitorViewer* view = graph->plot->view;

  // If this is a power-spectrum trace, split multi-element register
  // specifications into separate reg descriptors

  bool split = (graph->plot->type == PLOT_POWSPEC);

  // Attempt to add our registers to the map

  if(add_MonitorPlot_regs(view, view->mb->map, regname, descs, split))
    return true;

  // If this is a power spectrum plot, set up a PowSpec vector.  We
  // might want to do this in a separate thread, but for now, just try
  // it here.

  if(graph->plot->type == PLOT_POWSPEC) {

    // Resize the array of DFTs we are managing

    powSpec->resize(descs->size(),
		    graph->plot->powSpec.npt, 
		    graph->plot->powSpec.ntrans,
		    graph->powSpec.apodType);
  }

  return false;
}

/**.......................................................................
 * Check that the registers specified for a trace are consistent
 */
bool MonitorTrace::checkRegConsistency()
{
  switch (graph->plot->type) {

  // Normal plots -- don't need to check, because they've been checked
  // against the x-register

  case PLOT_NORM:
    break;

    // Power spectrum plots -- If this is the first trace, install it
    // as the register against which others will be compared.
    // Otherwise, check it against the other registers.

  case PLOT_POWSPEC:
    {
      for(unsigned iReg=0; iReg < descs->size(); iReg++) {
	
	if(graph->plot->powSpec.first) {

	  graph->plot->powSpec.nel   = descs->at(iReg).nEl();
	  graph->plot->powSpec.first = false;

	} else {
	  
	  if(descs->at(iReg).nEl() != graph->plot->powSpec.nel) {
	    ThrowSimpleError("Inconsistent register specification:" 
			     << endl << descs->at(iReg)
			     << " " << graph->plot->powSpec.nel);
	    return true;
	  }
	}
      }
    }
    break;

    // Future types should be handled on a case-by-case basis

  default:
    ThrowError("Unhandled plot type");
    return true;
  }

  return false;
}

/**.......................................................................
 * Push power spectrum data into the Dft container for this trace
 */
void MonitorTrace::pushPowSpecData(bool& first, double& ymin, double& ymax)
{
  MonitorViewer*  view = graph->plot->view;
  MonitorBuffMap* map  = view->mb->map;

  // Get the frame of data corresponding to the latest sample read

  double* frame = view->mb->data + (map->newest % map->max_snap) * map->nregs;

  unsigned aspect, offset;

  for(unsigned i=0; i < descs->size(); i++) {

    RegDescription& desc = descs->at(i);
    aspect = desc.aspect();

    for(desc.begin(); !desc.isEnd(); ++desc) {
      offset = map->offset[desc.currentSlot()].a[aspect];

#if 1
      powSpec->spectra[i].pushSample(frame[offset], first, 
				     ymin, ymax,
      				     !graph->powSpec.linAxis);
#else
      powSpec->spectra[i].pushSample(((double)rand())/RAND_MAX - 0.5,
				     first, ymin, ymax,
      				     !graph->powSpec.linAxis);
#endif

    }

    // Flag the whole trace a needing updating if any of its spectra
    // were updated on this call

    if(powSpec->spectra[i].updateNeeded) {

      powSpec->updateNeeded = true;
      graph->powSpec.updateNeeded = true;
      graph->plot->powSpec.updateNeeded = true;

      powSpec->spectra[i].updateNeeded = false;
    }
  }
}

/**.......................................................................
 * Draw, stepping the state of a power spectrum plot accordingly
 */
void MonitorTrace::stepPowSpecState(int& ci, int erase)
{
  for(unsigned iSpec=0; iSpec < powSpec->spectra.size(); iSpec++) {
    powSpec->spectra[iSpec].stepPowSpecState(graph, ci, erase);

    // Increment the color index

    if(!erase) {
      if(++ci > MP_MAX_CI)
	ci = MP_MIN_CI;
    }
  }
}

//=======================================================================
// Methods of Spectrum class
//=======================================================================

/**.......................................................................
 * Method of Spectrum class to draw power spectrum data and step the
 * state machine of a power spectrum plot
 */
void Spectrum::stepPowSpecState(MonitorGraph* graph, int ci, bool erase)
{
  switch (state) {
  case Spectrum::PS_FIRST: // No data plotted and no data ready, just return
    return;
    break;

  case Spectrum::PS_FIRST_READY: // First data set is ready, but not plotted
    if(erase) {
      return;
    } else {
      ydata->switchBuffers();
      redrawPowSpecData(graph, ci, false);
      state = Spectrum::PS_DATA_PLOTTED;
    }
    break;

    // Next data set is ready, but not plotted.  If erase is true,
    // just erase the old data.  If erase is false, then first erase
    // the previous data set, advance the buffer and replot

  case Spectrum::PS_DATA_READY:  
    if(erase) {
      redrawPowSpecData(graph, ci, true);
      return;
    } else {
      ydata->switchBuffers();
      redrawPowSpecData(graph, ci, false);
      state = Spectrum::PS_DATA_PLOTTED;
    }
    break;

    // Current data set is already plotted.  If erase is true, erase,
    // else replot.

  case Spectrum::PS_DATA_PLOTTED:
    redrawPowSpecData(graph, ci, erase);
    break;
  }
}

/**.......................................................................
 * Method of Spectrum to push a single sample onto the dft ring buffer
 */
void Spectrum::
pushSample(double sample, bool& first, double& ymin, double& ymax, bool doLog)
{
  // Push the newest sample onto the dft ring buffer

  dft->pushSample(sample);
  
  // If the push resulted in a transform being recalculated, fill the
  // output array with the data, and mark the hierarchy as needing to
  // be updated.

  if(dft->transformIsReady()) {

    float* fptr = ydata->getWriteBuffer();
    dft->abs(fptr, first, ymin, ymax, doLog);

    ydata->releaseWriteBuffer();

    // Set the state accordingly

    state = (state==Spectrum::PS_FIRST) ? 
      Spectrum::PS_FIRST_READY : Spectrum::PS_DATA_READY;

    // Flag the hierarchy of power spectrum plots as needing to be
    // redrawn

    updateNeeded = true;
  }
}

/**.......................................................................
 * Draw the power spectrum data
 */
void Spectrum::redrawPowSpecData(MonitorGraph* graph, int ci, bool erase)
{
  MonitorPlot* plot = graph->plot;

  bool xIsLinear = erase ? graph->lastPlot.xIsLinear : 
    graph->plot->powSpec.linAxis;

  bool yIsLinear = erase ? graph->lastPlot.yIsLinear : 
    graph->powSpec.linAxis;

  bool eitherIsLog = !(xIsLinear && yIsLinear);

  // Dont plot DC component -- hence the 1-offset

  float* xptr = xIsLinear ? 
    &graph->plot->powSpec.xdataLinear->at(1) :
    &graph->plot->powSpec.xdataLog->at(1);

  float* yptr = ydata->getReadBuffer()+1;

  unsigned npt = graph->plot->powSpec.ntrans-1;

  cpgsci(erase ? 0 : ci);

  cpgsvp(graph->vxa, graph->vxb, graph->vya, graph->vyb);

  if(erase) {
    cpgswin(0.0f, 1.0f, 0.0f, 1.0f);
    cpgrect(0.0f, 1.0f, 0.0f, 1.0f);
  } else {

    float xa = xIsLinear ? plot->wxa  : log10(plot->wxa);
    float xb = xIsLinear ? plot->wxb  : log10(plot->wxb);
    float ya = yIsLinear ? graph->wya : log10(graph->wya);
    float yb = yIsLinear ? graph->wyb : log10(graph->wyb);

    cpgswin(xa, xb, ya, yb);
    
    if(graph->plot->join) {
      cpgslw(graph->plot->marker_size);

      // Only draw if axis is linear, or the data are non-zero.
      // (Taking the log of zero is not a good thing...)

      //      if(yIsLinear || ymin > 0.0) {
	cpgline(npt, xptr, yptr);
	//      }
    } else {
      //      if(yIsLinear || ymin > 0.0) {
	cpgpt(npt, xptr, yptr, MP_POINT_SYMBOL);
	//      }
    }
    
    identifyPeaks(graph, npt, xptr, yptr);
    ydata->releaseReadBuffer();
  }

}

struct Pt {
  unsigned i_;
  float x_;
  float y_;
  
  Pt(float x, float y, unsigned ind) {
    x_ = x;
    y_ = y;
    i_ = ind;
  };
};

static bool compPt(Pt pt1, Pt pt2) {
  return pt1.y_ > pt2.y_;
};

/**.......................................................................
 * Set the apodization type
 */
void Spectrum::setApodizationType(gcp::util::Dft::Apodization type)
{
  if(dft) {
    dft->setApodizationType(type);
  }
}

/**.......................................................................
 * If peak identification was requested, plot them now
 */
void identifyPeaks(MonitorGraph* graph, unsigned npt, float* xptr, float* yptr) 
{
  //------------------------------------------------------------
  // See if peak identification is turned on
  //------------------------------------------------------------
  
  if(!graph->powSpec.pkIdent)
    return;
    
  MonitorPlot* plot = graph->plot;

  float ya = graph->powSpec.linAxis ? graph->wya : log10(graph->wya);
  float yb = graph->powSpec.linAxis ? graph->wyb : log10(graph->wyb);
  float dy = (yb - ya) * 0.03;
  
  float xa = plot->powSpec.linAxis ? graph->powSpec.pkXmin : log10(graph->powSpec.pkXmin);
  float xb = plot->powSpec.linAxis ? graph->powSpec.pkXmax : log10(graph->powSpec.pkXmax);

  float xaz = plot->powSpec.linAxis ? plot->wxa  : log10(plot->wxa);
  float xbz = plot->powSpec.linAxis ? plot->wxb  : log10(plot->wxb);

  if(graph->powSpec.fullXrange) {
    xa = xaz;
    xb = xbz;
  } else {
    if(xa < xaz)
      xa = xaz;
    
    if(xb > xbz)
      xb = xbz;
  }

  unsigned npk = graph->powSpec.npk;
  
  std::vector<Pt> pts;
  
  double ymax;
  bool first=true;
  unsigned nfound=0;


  for(unsigned i=0; i < npt; i++) {
    if(xptr[i] <= xb && xptr[i] >= xa) {
      pts.push_back(Pt(xptr[i], yptr[i], i));
      ++nfound;
    }
  }
  
  std::sort(pts.begin(), pts.end(), compPt);

  npk = npk > nfound ? nfound : npk;

  float y;
  char xstr[10];
  int ci;
  cpgqci(&ci);
  cpgsci(5);
  for(unsigned i=0; i < npk; i++) {
    sprintf(xstr, "%.2f", graph->plot->powSpec.linAxis ? pts[i].x_ : pow(10.0, (double)pts[i].x_));
    cpgptxt(pts[i].x_, pts[i].y_ + dy, 0.0, 0.5, xstr);
  }
  cpgsci(ci);
}

RegInputMode inputStatMode(MonitorPlot* plot)
{
  switch (plot->type) {
  case PLOT_NORM:
    return REG_INPUT_ELEMENT;
    break;
  case PLOT_POWSPEC:
    return REG_INPUT_RANGE;
    break;
  default:
    ThrowError("Unhandled plot type");
    break;
  }
}

RegOutputMode outputStatMode(MonitorPlot* plot)
{
  switch (plot->type) {
  case PLOT_NORM:
    return REG_OUTPUT_ELEMENT;
    break;
  case PLOT_POWSPEC:
    return REG_OUTPUT_RANGE;
    break;
  default:
    ThrowError("Unhandled plot type");
    break;
  }
}

bool MonitorPlot::isSingleXAxis() 
{
  switch (type) {
  case PLOT_NORM:
    return (xdescs->size() == 1);
    break;
  case PLOT_POWSPEC:
    return true;
    break;
  default:
    ThrowError("Unhandled plot type");
    return false;
    break;
  }
}
    
