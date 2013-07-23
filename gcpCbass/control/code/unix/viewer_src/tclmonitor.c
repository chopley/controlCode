/*
 * This file implements the TCL "monitor" command that provides the
 * interface between TCL and the monitor-viewer code in monitor_viewer.c.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <math.h>

#include <tcl.h>

#include "tclmonitor.h"
#include "monitor_viewer.h"
#include "hash.h"
#include "astrom.h"
#include "lprintf.h"

#include "gcp/util/common/Directives.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/PeriodicTimer.h"
#include "gcp/util/common/PipeQ.h"
#include "gcp/util/common/RegParser.h"
#include "gcp/util/common/SshTunnel.h"
#include "gcp/util/common/Sort.h"

#include <string>
#include <vector>

using namespace gcp::control;
using namespace std;

#if TCL_MAJOR_VERSION < 8 || (TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION==0 && TCL_RELEASE_LEVEL==0)
#define USE_TCLFILE
#endif

/**
 * Does the current version of tcl use (const char *) declarations or
 * vanilla (char *) 
 */
#if TCL_MAJOR_VERSION < 8 || (TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION < 4) 
#define USE_CHAR_DECL 
#endif

struct Retry {

  std::string host_;
  std::string gateway_;
  unsigned timeout_;
  unsigned interval_;
  bool enabled_;

  gcp::util::Mutex guard_;

  Retry() 
  {
    enabled_  = false;
    interval_ = 1;
  }
  
  ~Retry() {}

  void setTo(std::string host, std::string gateway, 
	     unsigned timeout, bool enabled) 
  {
    guard_.lock();

    host_    = host;
    gateway_ = gateway;
    timeout_ = timeout;
    enabled_ = enabled;

    guard_.unlock();
  }

  void setInterval(unsigned interval) 
  {
    guard_.lock();
  
    interval_ = interval;

    guard_.unlock();
  }

  void copyArgs(std::string& host, std::string& gateway, unsigned& timeout, unsigned& interval)
  {
    guard_.lock();

    host     = host_;
    gateway  = gateway_;
    timeout  = timeout_;
    interval = interval_;

    guard_.unlock();
  }

  unsigned interval()
  {
    unsigned retval = 1;

    guard_.lock();
    retval = interval_;
    guard_.unlock();

    return retval;
  }

  bool enabled() 
  {
    bool retval=false;

    guard_.lock();
    retval = enabled_;
    guard_.unlock();

    return retval;
  }
};

struct TclMonitorMsg {
  enum MsgType {
    MSG_CONNECT,
  };

  MsgType type_;
};

/*
 * Define the resource object of the interface.
 */
typedef struct {
  Tcl_Interp *interp;    /* The tcl-interpretter */
  FreeList *tmc_mem;     /* Memory for TmCallback containers */
  FreeList *tmf_mem;     /* Memory for TmField containers */
  HashMemory *hash_mem;  /* Memory from which to allocate hash tables */
  HashTable *services;   /* A symbol table of service functions */
  char* regmap_script;   /* The Tcl code to evaluate at register map updates */
  char* eos_script;      /* The Tcl code to evaluate on reaching the end of */
                         /*  the current stream. */

  char* reconnect_script; // The Tcl code to evaluate when it is time
			  // to attempt to reconnect to the monitor
			  // stream

  MonitorViewer *view;   /* The monitor-viewer resource object */
  int fd;                /* The data-stream file descriptor */
  int im_fd;             /* The image data-stream file descriptor */
  int send_in_progress;  /* True when a monitor-stream send is in progress */
  int im_send_in_progress;/* True when an image monitor-stream send is in 
			     progress */
#ifdef USE_TCLFILE
  Tcl_File tclfile;      /* A Tcl wrapper around fd */
  Tcl_File im_tclfile;   /* A Tcl wrapper around im_fd */
  Tcl_File msgq_tclfile;   /* A Tcl wrapper around msgq.fd() */
#else
  int fd_registered;     /* True if a file handler has been created for fd */
  int im_fd_registered;  /* True if a file handler has been created 
			    for im_fd */
  int msgq_fd_registered;  /* True if a file handler has been created 
			      for msgq_fd */
#endif
  int fd_event_mask;     /* The current event mask associated with 'fd' */
  int im_fd_event_mask;  /* The current event mask associated with 'im_fd' */
  int msgq_fd_event_mask;/* The current event mask associated with 'msgq_fd' */
  char buffer[256];      /* A work buffer for constructing result strings */
  InputStream *input;    /* An input stream to parse register names in */
  OutputStream *output;  /* An output stream to parse register names in */
  Tcl_DString stderr_output;/* A resizable string containing stderr output */

  gcp::util::SshTunnel* controlTunnel_;
  gcp::util::SshTunnel* monitorTunnel_; 
  gcp::util::SshTunnel* imageTunnel_;

  gcp::util::PeriodicTimer* connectTimer_;
  Retry* retry_;
  gcp::util::PipeQ<TclMonitorMsg>* msgq_;

} TclMonitor;

static PERIODIC_TIMER_HANDLER(retryConnectionHandler);
static void enableRetryTimer(TclMonitor* tm, bool enable);

static TclMonitor *new_TclMonitor(Tcl_Interp *interp);
static TclMonitor *del_TclMonitor(TclMonitor *tm);
static void delete_TclMonitor(ClientData context);

#ifdef USE_CHAR_DECL
static int service_TclMonitor(ClientData context, Tcl_Interp *interp,
			      int argc, char *argv[]);
#else
static int service_TclMonitor(ClientData context, Tcl_Interp *interp,
			      int argc, const char *argv[]);
#endif
static MonitorPage *tm_find_MonitorPage(TclMonitor *tm, char *tag_s);
static MonitorPlot *tm_find_MonitorPlot(TclMonitor *tm, char *tag_s);
static MonitorGraph *tm_find_MonitorGraph(TclMonitor *tm, MonitorPlot *plot,
					  char *tag_s);
static void tm_file_handler(ClientData context, int mask);
static void tm_im_file_handler(ClientData context, int mask);
static void tm_msgq_file_handler(ClientData context, int mask);
static void createMsgqFileHandler(TclMonitor* tm);

static void tm_update_TclFile(TclMonitor *tm, int fd, int send_in_progress);
static void tm_update_im_TclFile(TclMonitor *tm, int fd, int send_in_progress);

static void tm_suspend_TclFile(TclMonitor *tm);
static void tm_suspend_im_TclFile(TclMonitor *tm);

static void tm_resume_TclFile(TclMonitor *tm);
static void tm_resume_im_TclFile(TclMonitor *tm);

static int parse_reg(Tcl_Interp *interp, TclMonitor *tm, char *spec,
		     RegInputMode mode, int extend, 
		     gcp::util::RegDescription* desc);
static int tm_parse_date_and_time(TclMonitor *tm, char *string, double *utc);
static int tm_parse_format(TclMonitor *tm, char *name, MonitorFormat *fmt);

static MP_FIELD_FN(tm_field_dispatcher);
static void call_regmap_callback(TclMonitor *tm);
static void call_eos_callback(TclMonitor *tm);
static void call_reconnect_callback(TclMonitor *tm);

static LOG_DISPATCHER(tm_log_dispatcher);

static int tm_connect(TclMonitor* tm, Tcl_Interp* interp, 
		      char* host, char* gateway, unsigned timeout);
static int tm_retry_connection(TclMonitor* tm);

/*
 * Define an object to encapsulate a copy of a Tcl callback script
 * along with the resource object of the monitor interface.
 */
typedef struct {
  TclMonitor *tm;   /* The resource object of the monitor interface */
  char *script;     /* The callback script. */
} TmCallback;

static TmCallback *new_TmCallback(TclMonitor *tm, char *script);
static void *del_TmCallback(void *user_data);

/*
 * Declare the callback dispatcher for scroll events.
 */
static MP_SCROLL_FN(scroll_dispatch_fn);

/*
 * Create a container for text-fields. The values of text fields are
 * communicated to Tcl via Tcl variables, one variable per field.
 */
enum {TMF_VAR_LEN=32};     /* Max length of a field variable name */
typedef struct {
  TclMonitor *tm;          /* The resource object of the monitor interface */
  char var[TMF_VAR_LEN+1]; /* The name of the Tcl field-value variable */
  char warn[TMF_VAR_LEN+1];/* The name of the Tcl value-out-of-range variable */
  char activate_pager[TMF_VAR_LEN+1];/* The name of the Tcl activate pager variable */
  enum {                   /* The status of the last reported field value */
    TMF_WAS_BAD,           /* Out of range */
    TMF_WAS_OK,            /* In range */
    TMF_UNKNOWN            /* No value has been reported yet */
  } status;
} TmField;

static TmField *new_TmField(TclMonitor *tm, char *value_var, char *warn_var,
			    char *page_var);
static void *del_TmField(void *user_data);

/*
 * Create a container for describing service functions.
 */
#define SERVICE_FN(fn) int (fn)(Tcl_Interp *interp, TclMonitor *tm, \
				 int argc, char *argv[])
typedef struct {
  char *name;          /* The name of the service */
  SERVICE_FN(*fn);     /* The function that implements the service */
  unsigned narg;       /* The expected value of argc to be passed to fn() */
} ServiceFn;

/*
 * Provide prototypes for service functions.
 */
static SERVICE_FN(tm_list_regmaps);
static SERVICE_FN(tm_list_boards);
static SERVICE_FN(tm_list_regs);
static SERVICE_FN(tm_delete_pages);
static SERVICE_FN(tm_delete_fields);
static SERVICE_FN(tm_delete_plots);
static SERVICE_FN(tm_delete_graphs);
static SERVICE_FN(tm_add_page);
static SERVICE_FN(tm_add_plot);
static SERVICE_FN(tm_add_graph);
static SERVICE_FN(tm_open_image);
static SERVICE_FN(tm_rem_page);
static SERVICE_FN(tm_rem_field);
static SERVICE_FN(tm_rem_plot);
static SERVICE_FN(tm_rem_graph);
static SERVICE_FN(tm_int_graph);
static SERVICE_FN(tm_get_xlimits);
static SERVICE_FN(tm_get_x_axis_limits);
static SERVICE_FN(tm_get_y_autoscale);
static SERVICE_FN(tm_limit_plot);
static SERVICE_FN(tm_redraw_plot);
static SERVICE_FN(tm_redraw_graph);
static SERVICE_FN(tm_resize_plot);
static SERVICE_FN(tm_host);
static SERVICE_FN(tm_imhost);
static SERVICE_FN(tm_im_disconnect);
static SERVICE_FN(tm_disk);
static SERVICE_FN(tm_reconfigure);
static SERVICE_FN(tm_load_cal);
static SERVICE_FN(tm_have_stream);
static SERVICE_FN(tm_set_interval);
static SERVICE_FN(tm_queue_rewind);
static SERVICE_FN(tm_scroll_callback);
static SERVICE_FN(tm_cursor_to_graph);
static SERVICE_FN(tm_graph_to_cursor);
static SERVICE_FN(tm_find_point);
static SERVICE_FN(tm_global_stats);
static SERVICE_FN(tm_subset_stats);
static SERVICE_FN(tm_plot_hardcopy);
static SERVICE_FN(tm_add_field);
static SERVICE_FN(tm_configure_field);
static SERVICE_FN(tm_field_variables);
static SERVICE_FN(tm_clr_buffer);
static SERVICE_FN(tm_regmap_callback);
static SERVICE_FN(tm_eos_callback);
static SERVICE_FN(tm_reconnect_callback);
static SERVICE_FN(tm_freeze_page);
static SERVICE_FN(tm_unfreeze_page);
static SERVICE_FN(tm_configure_plot);
static SERVICE_FN(tm_configure_graph);
static SERVICE_FN(tm_resize_buffer);
static SERVICE_FN(tm_mjd_to_date);
static SERVICE_FN(tm_date_to_mjd);
static SERVICE_FN(tm_hours_to_interval);
static SERVICE_FN(tm_interval_to_hours);
static SERVICE_FN(tm_im_redraw);
static SERVICE_FN(tm_im_setrange);
static SERVICE_FN(tm_im_stat);
static SERVICE_FN(tm_im_contrast);
static SERVICE_FN(tm_im_colormap);
static SERVICE_FN(tm_im_step);
static SERVICE_FN(tm_im_fov);
static SERVICE_FN(tm_im_aspect);
static SERVICE_FN(tm_im_docompass);
static SERVICE_FN(tm_im_compass);
static SERVICE_FN(tm_im_centroid);
static SERVICE_FN(tm_im_set_centroid);
static SERVICE_FN(tm_im_toggle_grid);
static SERVICE_FN(tm_im_toggle_bullseye);
static SERVICE_FN(tm_im_toggle_crosshair);
static SERVICE_FN(tm_im_toggle_compass);
static SERVICE_FN(tm_im_reset_contrast);
static SERVICE_FN(tm_reset_counters);

static SERVICE_FN(tm_im_ximdir);
static SERVICE_FN(tm_im_yimdir);

static SERVICE_FN(tm_pkident);

/*
 * List top-level service functions.
 */
static ServiceFn service_fns[] = {
  
  /* Return a list of register maps. */
  {"list_regmaps",      tm_list_regmaps,      0},
  
  /* Return a list of register boards. */
  {"list_boards",       tm_list_boards,       1},
  
  /* Return a list of registers on a given board */
  {"list_regs",         tm_list_regs,         2},
  
  /* Delete all active pages */
  {"delete_pages",      tm_delete_pages,      0},
  
  /* Delete all fields of a given page */
  {"delete_fields",     tm_delete_fields,     1},
  
  /* Delete all active plots */
  {"delete_plots",      tm_delete_plots,      0},
  
  /* Delete all graphs of a given plot */
  {"delete_graphs",     tm_delete_graphs,     1},
  
  /* Add a text page to the viewer hierarchy */
  {"add_page",          tm_add_page,          0},
  
  /* Add a plot to the viewer hierarchy and assign it a PGPLOT device */
  {"add_plot",          tm_add_plot,          2},
  
  /* Add a graph to a given plot */
  {"add_graph",         tm_add_graph,         1},
  
  /* Add an image display to the viewer and assign it a PGPLOT device */
  {"open_image",        tm_open_image,        1},
  
  /* Remove a text page from the viewer hierarchy */
  {"remove_page",       tm_rem_page,          1},
  
  /* Remove a field from a given page */
  {"remove_field",      tm_rem_field,         2},
  
  /* Remove a plot from the viewer hierarchy */
  {"remove_plot",       tm_rem_plot,          1},
  
  /* Remove a graph from a given plot */
  {"remove_graph",      tm_rem_graph,         2},
  
  /* Integrate a graph from a given plot */
  {"integrate_graph",   tm_int_graph,         3},
  
  /* Return the X-axis extent of the buffered data */
  {"get_xlimits",       tm_get_xlimits,       3},
  
  /* Return the current x-axis limits */
  {"get_x_axis_limits", tm_get_x_axis_limits, 3},
  
  /* Return the Y-axis extent of the buffered data */
  {"get_y_autoscale",   tm_get_y_autoscale,   4},
  
  /* Limit the given plot to only display subsequently received data. */
  {"limit_plot",        tm_limit_plot,        1},
  
  /* Redraw a given plot with updated axes */
  {"redraw_plot",       tm_redraw_plot,       1},
  
  /* Redraw a given graph with updated axes */
  {"redraw_graph",      tm_redraw_graph,      2},
  
  /* Re-synchronize the size of a plot with its plot device */
  {"resize_plot",       tm_resize_plot,       1},
  
  /* Change the input stream to take real-time data from the control-program */
  {"host",              tm_host,              4},
  
  /* Change the image input stream to take real-time data from the 
     control-program */
  {"imhost",            tm_imhost,            1},
  
  /* Change the input stream to take archived data from disk */
  {"im_disconnect",     tm_im_disconnect,     0},
  
  /* Change the input stream to take archived data from disk */
  {"disk",              tm_disk,              4},
  
  /* Update the viewer to take account of configuration changes */
  {"reconfigure",       tm_reconfigure,       0},
  
  /* Replace the current calibration with the contents of a cal file */
  {"load_cal",          tm_load_cal,          1},
  
  /* Return error + message if no data-stream is connected */
  {"have_stream",       tm_have_stream,       0},
  
  /* Request a change in the sampling interval of the monitor stream */
  {"set_interval",      tm_set_interval,      1},
  
  /* Request that the monitor stream be rewound, if pertinent */
  {"queue_rewind",      tm_queue_rewind,      0},
  
  /* Register a script to be called whenever a given plot is scrolled */
  {"scroll_callback",   tm_scroll_callback,   2},
  
  /* Convert from cursor coordinates to graph coordinates */
  {"cursor_to_graph",   tm_cursor_to_graph,   7},
  
  /* Convert from graph coordinates to cursor coordinates */
  {"graph_to_cursor",   tm_graph_to_cursor,   6},
  
  /* Return details of the nearest point to a given graph location */
  {"find_point",        tm_find_point,        8},
  
  /* Return the statistics of one of the registers that is being monitored */
  {"global_stats",      tm_global_stats,      1},
  
  /* Return the statistics of one of the registers that is being monitored */
  {"subset_stats",      tm_subset_stats,      10},
  
  /* Make a hardcopy version of a given plot */
  {"plot_hardcopy",     tm_plot_hardcopy,     2},
  
  /* Add a register field to a page */
  {"add_field",         tm_add_field,         1},
  
  /* Reconfigure a text output field */
  {"configure_field",   tm_configure_field,   14},
  
  /* Specify Tcl variables to receive register values and statuses */
  {"field_variables",   tm_field_variables,   5},
  
  /* Discard buffered data */
  {"clr_buffer",        tm_clr_buffer,        0},
  
  /* Register a script to be called whenever the register map changes */
  {"regmap_callback",   tm_regmap_callback,   1},
  
  /* Register a script to be called whenever a monitor stream ends */
  {"eos_callback",      tm_eos_callback,      1},
  
  // Register a script to be called whenever it is time to reconnect

  {"reconnect_callback",tm_reconnect_callback,1},
  
  /* Stop updating the fields of a given page */
  {"freeze_page",       tm_freeze_page,       1},
  
  /* Stop updating the fields of a given page */
  {"unfreeze_page",     tm_unfreeze_page,     1},
  
  /* Reconfigure the characteristics of a given plot */
  {"configure_plot",    tm_configure_plot,    14},
  
  /* Reconfigure the characteristics of a given graph */
  {"configure_graph",   tm_configure_graph,   11},
  
  /* Resize the monitor buffer */
  {"resize_buffer",     tm_resize_buffer,     1},
  
  /* Convert a Modified Julian Date to Gregorian date and time */
  {"mjd_to_date",       tm_mjd_to_date,       1},
  
  /* Convert a Gregorian date and time to Modified Julian Date */
  {"date_to_mjd",       tm_date_to_mjd,       1},
  
  /* Convert a time in decimal hours to a time of day */
  {"hours_to_interval", tm_hours_to_interval, 1},
  
  /* Convert a time of day to decimal hours */
  {"interval_to_hours", tm_interval_to_hours, 1},
  
  /* Change the axis limits of a given image */
  {"im_setrange",       tm_im_setrange,       4},
  
  /* Compute statistics on a given image */
  {"im_stat",           tm_im_stat,           4},
  
  /* Redraw an image */
  {"im_redraw",         tm_im_redraw,         0},
  
  /* Change the contrast and brightness of a given image */
  {"im_contrast",       tm_im_contrast,       2},
  
  /* Change the colormap of an image */
  {"im_colormap",       tm_im_colormap,       1},
  
  /* Change the step interval for the bullseye display */
  {"im_change_step",    tm_im_step,           1},
  
  /* Change the image viewport size. */
  {"im_change_fov",      tm_im_fov,             1},

  /* Change the image aspect ratio. */
  {"im_change_aspect",      tm_im_aspect,     1},

  /* Change whether displaying the crosshair or not */
  {"im_toggle_crosshair", tm_im_toggle_crosshair,    0},
  
  /* Change whether displaying the compass or not */
  {"im_toggle_compass",   tm_im_toggle_compass,      0},
  
  /* Change the step interval for the bullseye display */
  {"im_compass",        tm_im_compass,         1},
  
  /* Compute the image centroid */
  {"im_centroid",       tm_im_centroid,        0},
  
  /* Set the image centroid */
  {"im_set_centroid",   tm_im_set_centroid,    2},

  /* Toggle drawing the grid on the frame grabber display */
  {"im_toggle_grid",    tm_im_toggle_grid,     0},
  
  /* Toggle drawing the bullseye on the frame grabber display */
  {"im_toggle_bullseye",tm_im_toggle_bullseye, 0},
  
  /* Reset the image contrast/brightness to the default values */
  {"im_reset_contrast", tm_im_reset_contrast,  0},

  /* Reset the image contrast/brightness to the default values */
  {"im_ximdir",         tm_im_ximdir,          1},

  /* Reset the image contrast/brightness to the default values */
  {"im_yimdir",         tm_im_yimdir,          1},

  /* Reconfigure a text output field */
  {"reset_counters",    tm_reset_counters,     0},

  /* Reconfigure the characteristics of a given plot */
  {"pkident",           tm_pkident,            6},
};

/*.......................................................................
 * This is the TCL monitor-viewer package-initialization function.
 *
 * Input:
 *  interp  Tcl_Interp *  The TCL interpretter.
 * Output:
 *  return         int    TCL_OK    - Success.
 *                        TCL_ERROR - Failure.           
 */
int TclMonitor_Init(Tcl_Interp *interp)
{
  TclMonitor *tm;   /* The resource object of the tcl_db() function */
  
  // Check arguments.

  if(!interp) {
    fprintf(stderr, "TclMonitor_Init: NULL interpreter.\n");
    return TCL_ERROR;
  };
  
  // Allocate a resource object for the service_TclMonitor() command.

  tm = new_TclMonitor(interp);
  if(!tm)
    return TCL_ERROR;

  // Create a file handler to service messages received on our message
  // queue

  createMsgqFileHandler(tm);

  // Create the TCL command that will control the monitor viewer.

  Tcl_CreateCommand(interp, "monitor", service_TclMonitor, (ClientData) tm,
		    delete_TclMonitor);
  return TCL_OK;
}

/**.......................................................................
 * Create the file handler to service messages received on our message
 * queue
 */
static void createMsgqFileHandler(TclMonitor* tm) 
{
  tm->msgq_fd_event_mask = TCL_READABLE;

#ifdef USE_TCLFILE
  if((tm->msgq_tclfile = Tcl_GetFile((ClientData) tm->msgq_->fd(), TCL_UNIX_FD)) != NULL) {
    Tcl_CreateFileHandler(tm->msgq_tclfile, tm->msgq_fd_event_mask, tm_msgq_file_handler,
			  (ClientData) tm);
  }
#else
  Tcl_CreateFileHandler(tm->msgq_->fd(), tm->msgq_fd_event_mask, tm_msgq_file_handler,
			(ClientData) tm);
  tm->msgq_fd_registered = 1;
#endif
}

/*.......................................................................
 * Create the resource object of the Tcl monitor viewer interface.
 *
 * Input:
 *  interp  Tcl_Interp *  The Tcl interpretter.
 * Output:
 *  return  TclMonitor *  The resource object, or NULL on error.
 */
static TclMonitor *new_TclMonitor(Tcl_Interp *interp)
{
  TclMonitor *tm;  /* The object to be returned */
  int i;
  /*
   * Check arguments.
   */
  if(!interp) {
    fprintf(stderr, "new_TclMonitor: NULL interp argument.\n");
    return NULL;
  };
  /*
   * Allocate the container.
   */
  tm = (TclMonitor *) malloc(sizeof(TclMonitor));
  if(!tm) {
    fprintf(stderr, "new_TclMonitor: Insufficient memory.\n");
    return NULL;
  };
  /*
   * Before attempting any operation that might fail, initialize the
   * container at least up to the point at which it can safely be passed
   * to del_TclMonitor().
   */
  tm->interp = interp;
  tm->tmc_mem = NULL;
  tm->tmf_mem = NULL;
  tm->hash_mem = NULL;
  tm->services = NULL;
  tm->regmap_script    = NULL;
  tm->eos_script       = NULL;
  tm->reconnect_script = NULL;
  tm->view = NULL;
  tm->fd = -1;
  tm->im_fd = -1;
  tm->send_in_progress = 0;
  tm->im_send_in_progress = 0;
#if USE_TCLFILE
  tm->tclfile = NULL;
  tm->im_tclfile = NULL;
  tm->msgq_tclfile = NULL;
#else
  tm->fd_registered      = 0;
  tm->im_fd_registered   = 0;
  tm->msgq_fd_registered = 0;
#endif
  tm->fd_event_mask      = 0;
  tm->im_fd_event_mask   = 0;
  tm->msgq_fd_event_mask = 0;
  tm->input = NULL;
  tm->output = NULL;

  tm->controlTunnel_ = 0;
  tm->monitorTunnel_ = 0;
  tm->imageTunnel_   = 0;
  tm->connectTimer_  = 0;
  tm->retry_         = 0;
  tm->msgq_          = 0;

  Tcl_DStringInit(&tm->stderr_output);

  // Create a free-list of TmCallback containers.

  tm->tmc_mem = new_FreeList("new_TclMonitor", sizeof(TmCallback), 20);
  if(!tm->tmc_mem)
    return del_TclMonitor(tm);
  
  // Create a free-list of TmField containers.

  tm->tmf_mem = new_FreeList("new_TclMonitor", sizeof(TmField), 100);
  if(!tm->tmf_mem)
    return del_TclMonitor(tm);
  /*
   * Create memory for allocating hash tables.
   */
  tm->hash_mem = new_HashMemory(10, 20);
  if(!tm->hash_mem)
    return del_TclMonitor(tm);
  /*
   * Create and populate the symbol table of services.
   */
  tm->services = new_HashTable(tm->hash_mem, 53, IGNORE_CASE, NULL, 0);
  if(!tm->services)
    return del_TclMonitor(tm);
  for(i=0; i < (int)(sizeof(service_fns)/sizeof(service_fns[0])); i++) {
    ServiceFn *srv = service_fns + i;
    if(new_HashSymbol(tm->services, srv->name, srv->narg,
		      (void (*)(void)) srv->fn, NULL, 0) == NULL)
      return del_TclMonitor(tm);
  };
  /*
   * Create the monitor viewer resource object.
   */
  tm->view = new_MonitorViewer(500000);
  if(!tm->view)
    return del_TclMonitor(tm);
  /*
   * Create unassigned input and output streams.
   */
  tm->input = new_InputStream();
  if(!tm->input)
    return del_TclMonitor(tm);
  tm->output = new_OutputStream();
  if(!tm->output)
    return del_TclMonitor(tm);

  tm->retry_ = new Retry();
  if(!tm->retry_)
    return del_TclMonitor(tm);

  tm->msgq_ = new gcp::util::PipeQ<TclMonitorMsg>();
  if(!tm->msgq_)
    return del_TclMonitor(tm);

  tm->connectTimer_ = new gcp::util::PeriodicTimer();
  if(!tm->connectTimer_)
    return del_TclMonitor(tm);

  tm->connectTimer_->spawn();

  return tm;
}

/*.......................................................................
 * Delete the resource object of the Tcl monitor viewer interface.
 *
 * Input:
 *  tm     TclMonitor *  The object to be deleted.
 * Output:
 *  return TclMonitor *  The deleted object (Always NULL).
 */
static TclMonitor *del_TclMonitor(TclMonitor *tm)
{
  if(tm) {
    /*
     * Delete the hash-table of services.
     */
    tm->services = del_HashTable(tm->services);
    /*
     * Having deleted all its client hash-tables, delete the hash-table
     * memory allocator.
     */
    tm->hash_mem = del_HashMemory(tm->hash_mem, 1);
    /*
     * Delete the register-map callback script.
     */
    if(tm->regmap_script)
      free(tm->regmap_script);
    tm->regmap_script = NULL;
    /*
     * Delete the end-of-stream callback script.
     */
    if(tm->eos_script)
      free(tm->eos_script);
    tm->eos_script = NULL;
    /*
     * Delete the reconnect callback script.
     */
    if(tm->reconnect_script)
      free(tm->reconnect_script);
    tm->reconnect_script = NULL;
    /*
     * Delete the Tcl file wrapper around tm->fd.
     */
    tm_update_TclFile(tm, -1, 0);
    /*
     * Delete the Tcl file wrapper around tm->im_fd.
     */
    tm_update_im_TclFile(tm, -1, 0);
    /*
     * Delete the monitor viewer resource object.
     */
    tm->view = del_MonitorViewer(tm->view);
    /*
     * Delete the callback containers. This is left until after
     * del_MonitorViewer(), so that all of the callback containers
     * will have been returned to the free-list.
     */
    tm->tmc_mem = del_FreeList("del_TclMonitor", tm->tmc_mem, 1);
    /*
     * Delete all text field contexts.
     */
    tm->tmf_mem = del_FreeList("del_TclMonitor", tm->tmf_mem, 1);
    /*
     * Delete the streams.
     */
    tm->input = del_InputStream(tm->input);
    tm->output = del_OutputStream(tm->output);

    if(tm->controlTunnel_) {
      delete tm->controlTunnel_;
      tm->controlTunnel_ = 0;
    }

    if(tm->monitorTunnel_) {
      delete tm->monitorTunnel_;
      tm->monitorTunnel_ = 0;
    }

    if(tm->imageTunnel_) {
      delete tm->imageTunnel_;
      tm->imageTunnel_ = 0;
    }

    if(tm->connectTimer_) {
      delete tm->connectTimer_;
      tm->connectTimer_ = 0;
    }

    if(tm->retry_) {
      delete tm->retry_;
      tm->retry_ = 0;
    }

    if(tm->msgq_) {
      delete tm->msgq_;
      tm->msgq_ = 0;
    }

    // Delete the container.

    free(tm);
  };

  return NULL;
}

/*.......................................................................
 * This is a wrapper around del_TclMonitor() that can be called with a
 * ClientData version of the (TclMonitor *) pointer.
 *
 * Input:
 *  context    ClientData  A TclMonitor pointer cast to ClientData.
 */
static void delete_TclMonitor(ClientData context)
{
  del_TclMonitor((TclMonitor *)context);
}

/*.......................................................................
 * This is the service function of the Tcl monitor command. It dispatches
 * the appropriate function for the service that is named by the
 * first argument.
 *
 * Input:
 *  context   ClientData    The TclMonitor resource object cast to
 *                          ClientData.
 *  interp    Tcl_Interp *  The Tcl interpretter instance.
 *  argc             int    The number of arguments in argv[].
 *  argv            char ** The arguments of the command. The first
 *                          argument specifies what service is required.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Error.
 */
#ifdef USE_CHAR_DECL
static int service_TclMonitor(ClientData context, Tcl_Interp *interp,
			      int argc, char *argv[])
#else
  static int service_TclMonitor(ClientData context, Tcl_Interp *interp,
				int argc, const char *argv[])
#endif
{
  TclMonitor *tm = (TclMonitor *) context;
  Symbol *sym;    /* The symbol table entry of a given service */
  int status;     /* The return status of the service function */
  LOG_DISPATCHER(*old_dispatcher);/* The previous lprintf() dispatch function */
  void *old_context;              /* The data attached to old_dispatcher */
  /*
   * Check arguments.
   */
  if(!tm || !interp || !argv) {
    fprintf(stderr, "service_TclMonitor: NULL arguments.\n");
    return TCL_ERROR;
  };
  if(argc < 2) {
    Tcl_AppendResult(interp, "monitor service name missing.", NULL);
    return TCL_ERROR;
  };
  /*
   * Lookup the named service.
   */
  sym = find_HashSymbol(tm->services, (char* )argv[1]);
  if(!sym) {
    Tcl_AppendResult(interp, "monitor service '", argv[1], "' unknown.", NULL);
    return TCL_ERROR;
  };
  /*
   * Check the number of argument matches that required by the service.
   */
  if(argc-2 != sym->code) {
    Tcl_AppendResult(interp, "Wrong number of arguments to the 'monitor ",
		     argv[1], "' command.", NULL);
    return TCL_ERROR;
  };
  /*
   * Arrange for error messages to be collected in tm->stderr_output.
   */
  divert_lprintf(stderr, tm_log_dispatcher, tm, &old_dispatcher, &old_context);
  /*
   * Pass the rest of the arguments to the service function.
   */
  status = ((SERVICE_FN(*))sym->fn)(interp, tm, argc-2, (char**)(argv+2));
  /*
   * Revert to the previous dispatch function for stderr, if any,
   * or to sending stderr to the controlling terminal otherwise.
   */
  divert_lprintf(stderr, old_dispatcher, old_context, NULL, NULL);
  /*
   * If any error messages were collected from lprintf(), assign them to
   * the interpretter's result object, discard the message object,
   * and return the Tcl error code.
   */
  if(*Tcl_DStringValue(&tm->stderr_output)) {
    Tcl_DStringResult(interp, &tm->stderr_output);
    return TCL_ERROR;
  };
  return status;
}

/*.......................................................................
 * This is an lprintf() callback function for stderr. It creates
 * tm->stderr_output for the first line of an error message, then
 * appends that line and subsequent lines to it.
 */
static LOG_DISPATCHER(tm_log_dispatcher)
{
  TclMonitor *tm = (TclMonitor *) context;
  /*
   * Append the message to tm->stderr_output.
   */
  Tcl_DStringAppend(&tm->stderr_output, message, -1);
  /*
   * Append a newline to separate it from subsequent messages.
   */
  Tcl_DStringAppend(&tm->stderr_output, "\n", -1);
  return 0;
}

/*.......................................................................
 * Decode a stringized monitor-page tag and return the corresponding
 * page.
 *
 * Input:
 *  tm        TclMonitor *  The Tcl monitor-interface resource object.
 *  tag_s           char *  A textual representation of the page tag.
 * Output:
 *  return   MonitorPage *  The located page, or NULL if not found.
 *                          In the latter case a lookup failure
 *                          message will have been appended to the
 *                          result string of tm->interp.
 */
static MonitorPage *tm_find_MonitorPage(TclMonitor *tm, char *tag_s)
{
  MonitorPage *page; /* The page identified by the tag */
  char *endp;        /* The pointer to the undecoded part of tag_s */
  unsigned tag;      /* The decoded tag */
  /*
   * Decode the tag from the given string. Note that Tcl_GetInt()
   * decodes signed integers whereas tags are unsigned, so we have
   * to decompose the string ourself.
   */
  tag = strtoul(tag_s, &endp, 0);
  if(*tag_s == '\0' || !endp || *endp != '\0') {
    Tcl_AppendResult(tm->interp, "Malformed tag id: '", tag_s, "'", NULL);
    return NULL;
  };
  /*
   * Lookup the page associated with the specified tag.
   */
  page = find_MonitorPage(tm->view, tag);
  if(!page) {
    Tcl_AppendResult(tm->interp, "Page with tag ", tag_s, " not found.", NULL);
    return NULL;
  };
  return page;
}

/*.......................................................................
 * Decode a stringized monitor-field tag and return the corresponding
 * field.
 *
 * Input:
 *  tm        TclMonitor *  The Tcl monitor-interface resource object.
 *  page     MonitorPage *  The parent page of the field.
 *  tag_s           char *  A textual representation of the field tag.
 * Output:
 *  return  MonitorField *  The located field, or NULL if not found.
 *                          In the latter case a lookup failure
 *                          message will have been appended to the
 *                          result string of tm->interp.
 */
static MonitorField *tm_find_MonitorField(TclMonitor *tm, MonitorPage *page,
					  char *tag_s)
{
  MonitorField *field; /* The field identified by the tag */
  char *endp;        /* The pointer to the undecoded part of tag_s */
  unsigned tag;      /* The decoded tag */
  /*
   * Decode the tag from the given string. Note that Tcl_GetInt()
   * decodes signed integers whereas tags are unsigned, so we have
   * to decompose the string ourself.
   */
  tag = strtoul(tag_s, &endp, 0);
  if(*tag_s == '\0' || !endp || *endp != '\0') {
    Tcl_AppendResult(tm->interp, "Malformed tag id: '", tag_s, "'", NULL);
    return NULL;
  };
  /*
   * Lookup the field associated with the specified tag.
   */
  field = find_MonitorField(page, tag);
  if(!field) {
    Tcl_AppendResult(tm->interp, "Field with tag ", tag_s, " not found.", NULL);
    return NULL;
  };
  return field;
}

/*.......................................................................
 * Decode a stringized monitor-plot tag and return the corresponding
 * plot.
 *
 * Input:
 *  tm        TclMonitor *  The Tcl monitor-interface resource object.
 *  tag_s           char *  A textual representation of the plot tag.
 * Output:
 *  return   MonitorPlot *  The located plot, or NULL if not found.
 *                          In the latter case a lookup failure
 *                          message will have been appended to the
 *                          result string of tm->interp.
 */
static MonitorPlot *tm_find_MonitorPlot(TclMonitor *tm, char *tag_s)
{
  MonitorPlot *plot; /* The plot identified by the tag */
  char *endp;        /* The pointer to the undecoded part of tag_s */
  unsigned tag;      /* The decoded tag */
  /*
   * Decode the tag from the given string. Note that Tcl_GetInt()
   * decodes signed integers whereas tags are unsigned, so we have
   * to decompose the string ourself.
   */
  tag = strtoul(tag_s, &endp, 0);
  if(*tag_s == '\0' || !endp || *endp != '\0') {
    Tcl_AppendResult(tm->interp, "Malformed tag id: '", tag_s, "'", NULL);
    return NULL;
  };
  /*
   * Lookup the plot associated with the specified tag.
   */
  plot = find_MonitorPlot(tm->view, tag);
  if(!plot) {
    Tcl_AppendResult(tm->interp, "Plot with tag ", tag_s, " not found.", NULL);
    return NULL;
  };
  return plot;
}

/*.......................................................................
 * Decode a stringized monitor-graph tag and return the corresponding
 * graph.
 *
 * Input:
 *  tm        TclMonitor *  The Tcl monitor-interface resource object.
 *  plot     MonitorPlot *  The parent plot of the graph.
 *  tag_s           char *  A textual representation of the graph tag.
 * Output:
 *  return  MonitorGraph *  The located graph, or NULL if not found.
 *                          In the latter case a lookup failure
 *                          message will have been appended to the
 *                          result string of tm->interp.
 */
static MonitorGraph *tm_find_MonitorGraph(TclMonitor *tm, MonitorPlot *plot,
					  char *tag_s)
{
  MonitorGraph *graph; /* The graph identified by the tag */
  char *endp;        /* The pointer to the undecoded part of tag_s */
  unsigned tag;      /* The decoded tag */
  /*
   * Decode the tag from the given string. Note that Tcl_GetInt()
   * decodes signed integers whereas tags are unsigned, so we have
   * to decompose the string ourself.
   */
  tag = strtoul(tag_s, &endp, 0);
  if(*tag_s == '\0' || !endp || *endp != '\0') {
    Tcl_AppendResult(tm->interp, "Malformed tag id: '", tag_s, "'", NULL);
    return NULL;
  };
  /*
   * Lookup the graph associated with the specified tag.
   */
  graph = find_MonitorGraph(plot, tag);
  if(!graph) {
    Tcl_AppendResult(tm->interp, "Graph with tag ", tag_s, " not found.", NULL);
    return NULL;
  };
  return graph;
}

/*.......................................................................
 * The following function is called by the TCL event loop whenever the
 * monitor-stream file-descriptor becomes readable, or (when selected)
 * writable.
 *
 * Input:
 *  context   ClientData    The TclMonitor resource container cast to
 *                          ClientData.
 *  mask             int    TCL_READABLE and/or TCL_WRITABLE.
 */
static void tm_file_handler(ClientData context, int mask)
{
  TclMonitor *tm = (TclMonitor *) context;
  int send_in_progress = tm->send_in_progress;
  int disconnect = 0;                  // True if the stream should be closed 
  LOG_DISPATCHER(*old_dispatcher) = 0; // The previous lprintf() dispatch function 
  void* old_context = 0;               // The data attached to old_dispatcher 

  // Arrange for error messages to be collected in tm->stderr_output.

  divert_lprintf(stderr, tm_log_dispatcher, tm, &old_dispatcher, &old_context);
  
  // Check for incoming data.

  if(mask | TCL_READABLE) {
    switch(read_MonitorViewer_frame(tm->view)) {
    case MS_READ_REGMAP:
      call_regmap_callback(tm);
      break;
    case MS_READ_ENDED:
      disconnect = 1;
      break;
    case MS_READ_BREAK:
    case MS_READ_AGAIN:
    case MS_READ_DONE:
      break;
    };
  };

  if(mask | TCL_WRITABLE) {
    switch(send_MonitorViewer_msg(tm->view, 0)) {
    case MS_SEND_ERROR:
      disconnect = 1;
      break;
    case MS_SEND_AGAIN:
      break;
    case MS_SEND_DONE:
      send_in_progress = 0;
      break;
    };
  };
  
  // Update the TCL file handle for changes in the events to be
  // selected, and for potential changes to the fd.

  if(disconnect)
    tm_update_TclFile(tm, -1, 0);
  else
    tm_update_TclFile(tm, MonitorViewer_fd(tm->view), send_in_progress);
  
  // Flush other event types so that we don't hog the event loop.

  Tcl_Sleep(10);
  while(Tcl_DoOneEvent(TCL_WINDOW_EVENTS | TCL_TIMER_EVENTS |
		       TCL_IDLE_EVENTS | TCL_DONT_WAIT))
    ;
  
  // Revert to the previous dispatch function for stderr, if any, or
  // to sending stderr to the controlling terminal otherwise.

  divert_lprintf(stderr, old_dispatcher, old_context, NULL, NULL);
  
  // If any error messages were collected from lprintf(), have the
  // viewer display them and discard the message object.

  if(*Tcl_DStringValue(&tm->stderr_output)) {
    Tcl_VarEval(tm->interp, "bgerror {", Tcl_DStringValue(&tm->stderr_output),
		"}", NULL);
    Tcl_DStringInit(&tm->stderr_output);
  };
  
  // If we reached the end of the stream evaluate any end-of-stream
  // callback that the user registered. Note that this has to be done
  // after the above reporting and clearing of tm->stderr_output
  // because otherwise if the event that triggered the disconnection
  // wrote anything to stderr, that error message would get associated
  // with the first call to a monitor service in the callback script.

  if(disconnect)
    call_eos_callback(tm);
}

/*.......................................................................
 * The following function is called by the TCL event loop whenever the
 * message queue becomes readable
 */
static void tm_msgq_file_handler(ClientData context, int mask)
{
  TclMonitor *tm = (TclMonitor *) context;
  TclMonitorMsg msg;

  if(mask | TCL_READABLE) {
    tm->msgq_->readMsg(&msg);

    switch (msg.type_) {
    case TclMonitorMsg::MSG_CONNECT:
      tm_retry_connection(tm);
      break;
    default:
      break;
    }
  }
    
  // Flush other event types so that we don't hog the event loop.
  
  Tcl_Sleep(10);
  while(Tcl_DoOneEvent(TCL_WINDOW_EVENTS | TCL_TIMER_EVENTS |
		       TCL_IDLE_EVENTS | TCL_DONT_WAIT));
}

/*.......................................................................
 * The following function is called by the TCL event loop whenever the
 * image monitor-stream file-descriptor becomes readable, or (when selected)
 * writable.
 *
 * Input:
 *  context   ClientData    The TclMonitor resource container cast to
 *                          ClientData.
 *  mask             int    TCL_READABLE and/or TCL_WRITABLE.
 */
static void tm_im_file_handler(ClientData context, int mask)
{
  TclMonitor *tm = (TclMonitor *) context;
  int send_in_progress = tm->im_send_in_progress;
  int disconnect = 0;  /* True if the stream should be closed */
  LOG_DISPATCHER(*old_dispatcher);/* The previous lprintf() dispatch function */
  void *old_context;              /* The data attached to old_dispatcher */
  /*
   * Arrange for error messages to be collected in tm->stderr_output.
   */
  divert_lprintf(stderr, tm_log_dispatcher, tm, &old_dispatcher, &old_context);
  /*
   * Check for incoming data.
   */
  if(mask | TCL_READABLE) {
    switch(read_MonitorViewer_image(tm->view)) {
    case IMS_READ_ENDED:
      disconnect = 1;
      return;
      break;
    case IMS_READ_AGAIN:
    case IMS_READ_DONE:
      break;
    };
  };
  if(mask | TCL_WRITABLE) {
    switch(send_MonitorViewer_im_msg(tm->view, 0)) {
    case IMS_SEND_ERROR:
      disconnect = 1;
      return;
      break;
    case IMS_SEND_AGAIN:
      break;
    case IMS_SEND_DONE:
      send_in_progress = 0;
      break;
    };
  };
  /*
   * Update the TCL file handle for changes in the events to be
   * selected for potential changes to the fd.
   */
  if(disconnect)
    tm_update_im_TclFile(tm, -1, 0); /* Disconnect the file handler */
  else 
    tm_update_im_TclFile(tm, MonitorViewer_im_fd(tm->view), send_in_progress);
  /*
   * Flush other event types so that we don't hog the event loop.
   */
  Tcl_Sleep(10);
  while(Tcl_DoOneEvent(TCL_WINDOW_EVENTS | TCL_TIMER_EVENTS |
		       TCL_IDLE_EVENTS | TCL_DONT_WAIT))
    ;
  /*
   * Revert to the previous dispatch function for stderr, if any,
   * or to sending stderr to the controlling terminal otherwise.
   */
  divert_lprintf(stderr, old_dispatcher, old_context, NULL, NULL);
  /*
   * If any error messages were collected from lprintf(), have the viewer
   * display them and discard the message object.
   */
  if(*Tcl_DStringValue(&tm->stderr_output)) {
    Tcl_VarEval(tm->interp, "bgerror {", Tcl_DStringValue(&tm->stderr_output),
		"}", NULL);
    Tcl_DStringInit(&tm->stderr_output);
  };
  /*
   * If we reached the end of the stream evaluate any end-of-stream
   * callback that the user registered. Note that this has to be done
   * after the above reporting and clearing of tm->stderr_output because
   * otherwise if the event that triggered the disconnection wrote anything
   * to stderr, that error message would get associated with the first
   * call to a monitor service in the callback script.
   */
  if(disconnect)
    call_eos_callback(tm);
}

/*.......................................................................
 * Parse a register specification.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  spec            char *   The scalar register specification to be
 *                           decoded.
 *  mode    RegInputMode     The type of specification to expect:
 *                             REG_INPUT_BLOCK - A register block.
 *                             REG_INPUT_ELEMENT A register element.
 *                             REG_INPUT_RANGE   A range of register
 *                                               elements.
 *  extend            int    If true allow the user to append, where
 *                           appropriate, one of the following components
 *                           to complex and utc register specifications:
 *                            .amp    -  The amplitude of a complex pair.
 *                            .phase  -  The phase of a complex pair.
 *                            .real   -  The real part of a complex pair.
 *                            .imag   -  The imaginary part of a complex pair.
 *                            .mjd    -  The Modified Julian Date of a utc pair.
 *                           If the user uses these notations then the
 *                           selected attribute will be recorded in
 *                           RegMapReg::aspect.
 * Input/Output:
 *  reg        RegMapReg *   On output the decoded register specification
 *                           will be left in *reg.
 * Output:
 *  return           int     TCL_OK    -  The register specification was
 *                                        valid.
 *                           TCL_ERROR -  The specification was bad. A
 *                                        message will already have been
 *                                        printed or will be returned in
 *                                        interp->result.
 */
static int parse_reg(Tcl_Interp *interp, TclMonitor *tm, char *spec,
		     RegInputMode mode, int extend, 
		     gcp::util::RegDescription* desc)
{
  ArrayMap *arraymap;   /* The current array map */
  InputStream *stream;  /* An input stream wrapped around spec[] */
  
  // Get the register map of the current data-stream.
  
  arraymap = MonitorViewer_ArrayMap(tm->view);
  if(!arraymap) {
    Tcl_AppendResult(interp, "No register map is currently available.", NULL);
    return TCL_ERROR;
  };
  
  // Attach an input stream to the specification string.
  
  stream = tm->input;
  if(open_StringInputStream(stream, 0, spec)) {
    Tcl_AppendResult(interp, "parse_xreg stream failure.", NULL);
    return TCL_ERROR;
  };
  /*
   * Skip leading space to check that the string is not empty.
   */
  if(input_skip_space(stream, 1, 0) || stream->nextc == EOF) {
    Tcl_AppendResult(interp, "X-axis register specification missing.", NULL);
    close_InputStream(stream);
    return TCL_ERROR;
  };
  /*
   * Parse the scalar register specification.
   */
  gcp::util::RegParser parser;
  try {
    *desc = parser.inputReg(stream, true, mode, extend, arraymap);
  } catch(...) {
    close_InputStream(stream);
    return TCL_ERROR;
  }
  
  if(input_skip_white(stream, 1, 0)) {
    close_InputStream(stream);
    return TCL_ERROR;
  };
  
  // Check for unexpected characters following the specification.
  
  if(input_skip_white(stream, 1, 0) || stream->nextc != EOF) {
    Tcl_AppendResult(interp,
		     "Unexpected characters at end of \"", spec, "\"", NULL);
    close_InputStream(stream);
    return TCL_ERROR;
  };
  
  // The stream is now redundant.
  
  close_InputStream(stream);
  
  // Check that the register is monitorable.
  
  if(desc->iSlot() < 0) {
    Tcl_AppendResult(interp, spec,
		     " is not a monitorable register.", NULL);
    return TCL_ERROR;
  };
  return TCL_OK;
}

/*.......................................................................
 * Update the monitor-stream file handler to cater for a change of fd
 * or a change in which events to select for. Note that the monitor-stream
 * file fd is liable to change at the end of each I/O transaction (eg.
 * The file input-stream can span many files). When this happens this
 * function allocates a new Tcl file wrapper.
 *
 * Input:
 *  tm        TclMonitor *  The Tcl monitor-interface resource object.
 *  fd               int    The current file descriptor of the monitor
 *                          stream, or -1 to delete the current file
 *                          handle and handler.
 *  send_in_progress int    True to select for writeability.
 */
static void tm_update_TclFile(TclMonitor *tm, int fd, int send_in_progress)
{
  int update_handler = 0;  /* True to (re)-install the file handler */
  
  // See if the required events have changed.

  update_handler = update_handler || send_in_progress != tm->send_in_progress;
  
  // See if the file-descriptor of the monitor stream has changed.

  if(tm->fd == -1 || tm->fd != fd ||
#ifdef USE_TCLFILE
     !tm->tclfile
#else
     !tm->fd_registered
#endif
     ) {
    
    // Delete the current file handle and event handler.

#ifdef USE_TCLFILE
    if(tm->tclfile) {
      Tcl_DeleteFileHandler(tm->tclfile);
      Tcl_FreeFile(tm->tclfile);
      tm->tclfile = NULL;
    };
#else
    if(tm->fd_registered) {
      Tcl_DeleteFileHandler(tm->fd);
      tm->fd_registered = 0;
    };
#endif
    
    // Record the new fd and attempt to create its Tcl file handle.

    if(fd != -1
#ifdef USE_TCLFILE
       && (tm->tclfile = Tcl_GetFile((ClientData) fd, TCL_UNIX_FD)) != NULL
#endif
       ) {
      tm->fd = fd;
      update_handler = 1;
    } else {
      tm->fd = -1;
      tm->send_in_progress = 0;
      update_handler = 0;
    };
  };
  
  // See if the file handler needs to be updated.

  if(update_handler) {
    tm->fd_event_mask = TCL_READABLE;
    if(send_in_progress)
      tm->fd_event_mask |= TCL_WRITABLE;
#ifdef USE_TCLFILE
    Tcl_CreateFileHandler(tm->tclfile, tm->fd_event_mask, tm_file_handler,
			  (ClientData) tm);
#else
    Tcl_CreateFileHandler(tm->fd, tm->fd_event_mask, tm_file_handler,
			  (ClientData) tm);
    tm->fd_registered = 1;
#endif
    tm->send_in_progress = send_in_progress;
  };
}

/*.......................................................................
 * Update the image monitor-stream file handler to cater for a change of fd
 * or a change in which events to select for. Note that the monitor-stream
 * file fd is liable to change at the end of each I/O transaction (eg.
 * The file input-stream can span many files). When this happens this
 * function allocates a new Tcl file wrapper.
 *
 * Input:
 *  tm        TclMonitor *  The Tcl monitor-interface resource object.
 *  fd               int    The current file descriptor of the monitor
 *                          stream, or -1 to delete the current file
 *                          handle and handler.
 *  send_in_progress int    True to select for writeability.
 */
static void tm_update_im_TclFile(TclMonitor *tm, int fd, int send_in_progress)
{
  int update_handler = 0;  /* True to (re)-install the file handler */
  /*
   * See if the required events have changed.
   */
  update_handler = update_handler || send_in_progress != 
    tm->im_send_in_progress;
  /*
   * See if the file-descriptor of the monitor stream has changed.
   */
  if(tm->im_fd == -1 || tm->im_fd != fd ||
#ifdef USE_TCLFILE
     !tm->im_tclfile
#else
     !tm->im_fd_registered
#endif
     ) {
    /*
     * Delete the current file handle and event handler.
     */
#ifdef USE_TCLFILE
    if(tm->im_tclfile) {
      Tcl_DeleteFileHandler(tm->im_tclfile);
      Tcl_FreeFile(tm->im_tclfile);
      tm->im_tclfile = NULL;
    };
#else
    if(tm->im_fd_registered) {
      Tcl_DeleteFileHandler(tm->im_fd);
      tm->im_fd_registered = 0;
    };
#endif
    /*
     * Record the new fd and attempt to create its Tcl file handle.
     */
    if(fd != -1
#ifdef USE_TCLFILE
       && (tm->im_tclfile = Tcl_GetFile((ClientData) fd, TCL_UNIX_FD)) != NULL
#endif
       ) {
      tm->im_fd = fd;
      update_handler = 1;
    } else {
      tm->im_fd = -1;
      tm->im_send_in_progress = 0;
      update_handler = 0;
    };
  };
  /*
   * See if the file handler needs to be updated.
   */
  if(update_handler) {
    tm->im_fd_event_mask = TCL_READABLE;
    if(send_in_progress)
      tm->im_fd_event_mask |= TCL_WRITABLE;
#ifdef USE_TCLFILE
    Tcl_CreateFileHandler(tm->im_tclfile, tm->im_fd_event_mask, 
			  tm_im_file_handler,
			  (ClientData) tm);
#else
    Tcl_CreateFileHandler(tm->im_fd, tm->im_fd_event_mask, tm_im_file_handler,
			  (ClientData) tm);
    tm->im_fd_registered = 1;
#endif
    tm->im_send_in_progress = send_in_progress;
  };
}

/*.......................................................................
 * Arrange to temporarily stop listening for the readability and
 * writability of the data-stream file descriptor.
 *
 * Input:
 *  tm        TclMonitor *   The resource object of the tcl interface.
 */
static void tm_suspend_TclFile(TclMonitor *tm)
{
  /*
   * If a file handler is currently registered, clear its event mask.
   */
#ifdef USE_TCLFILE
  if(tm->tclfile)
    Tcl_CreateFileHandler(tm->tclfile, 0, tm_file_handler, (ClientData) tm);
#else
  if(tm->fd_registered)
    Tcl_CreateFileHandler(tm->fd, 0, tm_file_handler, (ClientData) tm);
#endif
}
/*.......................................................................
 * Arrange to temporarily stop listening for the readability and
 * writability of the image data-stream file descriptor.
 *
 * Input:
 *  tm        TclMonitor *   The resource object of the tcl interface.
 */
static void tm_suspend_im_TclFile(TclMonitor *tm)
{
  /*
   * If a file handler is currently registered, clear its event mask.
   */
#ifdef USE_TCLFILE
  if(tm->im_tclfile)
    Tcl_CreateFileHandler(tm->im_tclfile, 0, tm_im_file_handler, 
			  (ClientData) tm);
#else
  if(tm->im_fd_registered)
    Tcl_CreateFileHandler(tm->im_fd, 0, tm_im_file_handler, (ClientData) tm);
#endif
}

/*.......................................................................
 * Resume listening to events on the current data stream socket having
 * previously called tm_suspend_TclFile().
 *
 * Input:
 *  tm        TclMonitor *   The resource object of the tcl interface.
 */
static void tm_resume_TclFile(TclMonitor *tm)
{
  /*
   * Reinstate the event mask.
   */
#ifdef USE_TCLFILE
  if(tm->tclfile)
    Tcl_CreateFileHandler(tm->tclfile, tm->fd_event_mask, tm_file_handler,
			  (ClientData) tm); 
#else
  if(tm->fd_registered)
    Tcl_CreateFileHandler(tm->fd, tm->fd_event_mask, tm_file_handler,
			  (ClientData) tm);
#endif
}
/*.......................................................................
 * Resume listening to events on the current image data stream socket having
 * previously called tm_suspend_im_TclFile().
 *
 * Input:
 *  tm        TclMonitor *   The resource object of the tcl interface.
 */
static void tm_resume_im_TclFile(TclMonitor *tm)
{
  /*
   * Reinstate the event mask.
   */
#ifdef USE_TCLFILE
  if(tm->im_tclfile)
    Tcl_CreateFileHandler(tm->im_tclfile, tm->im_fd_event_mask, 
			  tm_im_file_handler,
			  (ClientData) tm); 
#else
  if(tm->im_fd_registered)
    Tcl_CreateFileHandler(tm->im_fd, tm->im_fd_event_mask, tm_im_file_handler,
			  (ClientData) tm);
#endif
}

/*.......................................................................
 * Return a list of register maps
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_list_regmaps)
{
  ArrayMap *arraymap; // The current array map 
  int iregmap;         // The index of the regmap being reported 
  
  // Get the register map of the current data-stream.
  
  arraymap = MonitorViewer_ArrayMap(tm->view);
  if(!arraymap) {
    Tcl_AppendResult(interp, "No array map is currently available.", NULL);
    return TCL_ERROR;
  };
  
  // Append each of the names of archived regmaps to the return
  // string.
  
  std::vector<string> regmapNames;

  for(iregmap=0; iregmap < arraymap->nregmap; iregmap++) {
    ArrRegMap* regmap = arraymap->regmaps[iregmap];

    if(!is_MonitorViewer_archivedOnly(tm->view) 
       || regmap->regmap->narchive_ > 0) {
      regmapNames.push_back(regmap->name);
    }
  }

  regmapNames = gcp::util::Sort::sort(regmapNames);

  for(unsigned i=0; i < regmapNames.size(); i++) {
    Tcl_AppendResult(interp, regmapNames[i].c_str(), " ", NULL);
  };

  return TCL_OK;
}

/*.......................................................................
 * Return a list of register boards.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_list_boards)
{
  ArrayMap *arraymap; // The current array map 
  char* regmap_name;  // The name of the register map to look up
  int board;          // The index of the board being reported 
  ArrRegMap* arregmap=NULL;
  RegMap* regmap=NULL;
  
  // Get the array map of the current data-stream.
  
  arraymap = MonitorViewer_ArrayMap(tm->view);
  if(!arraymap) {
    Tcl_AppendResult(interp, "No array map is currently available.", NULL);
    return TCL_ERROR;
  };
  
  // Look up the named register map
  
  regmap_name = argv[0];
  arregmap = find_ArrRegMap(arraymap, regmap_name);
  if(!arregmap || !arregmap->regmap) {
    Tcl_AppendResult(interp, "There is no register map named: ", regmap_name, 
		     NULL);
    return TCL_ERROR;
  };
  
  regmap = arregmap->regmap;
  
  // Append each of the names of archived boards to the return string.
  
  std::vector<string> boardNames;
  for(board=0; board < regmap->nboard_; board++) {

    RegMapBoard* brd = regmap->boards_[board];
    
    if(!is_MonitorViewer_archivedOnly(tm->view) || brd->narchive > 0)
      boardNames.push_back(brd->name);
  }

  boardNames = gcp::util::Sort::sort(boardNames);

  for(unsigned i=0; i < boardNames.size(); i++) {
    Tcl_AppendResult(interp, boardNames[i].c_str(), " ", NULL);
  };

  return TCL_OK;
}

/*.......................................................................
 * Return a list of the registers of a given board.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                            [1] - The name of the board.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_list_regs)
{
  RegMapBoard *brd;  /* The description of the board */
  char *board_name;  /* The name of the board */
  char *regmap_name; /* The name of the register map */
  int block;         /* The index of the register-block being reported */
  ArrRegMap *arregmap;    /* The current register map */
  RegMap *regmap;    /* The current register map */
  ArrayMap *arraymap;/* The current array map */
  
  // Get the array map of the current data-stream.
  
  arraymap = MonitorViewer_ArrayMap(tm->view);
  if(!arraymap) {
    Tcl_AppendResult(interp, "No array map is currently available.", NULL);
    return TCL_ERROR;
  };
  
  // Look up the named register map
  
  regmap_name = argv[0];
  arregmap = find_ArrRegMap(arraymap, regmap_name);
  if(!arregmap || !arregmap->regmap) {
    Tcl_AppendResult(interp, "There is no register map named: ", regmap_name, 
		     NULL);
    return TCL_ERROR;
  };
  
  regmap = arregmap->regmap;
  
  // Look up the named board.
  
  board_name = argv[1];
  brd = find_RegMapBoard(regmap, board_name);
  if(!brd) {
    Tcl_AppendResult(interp, "There is no board named: ", board_name, NULL);
    return TCL_ERROR;
  };
  
  // Append each of the names of archived registers to the return
  // string.
  
 std::ostringstream os;

 std::vector<string> regNames;
 
 for(block=0; block < brd->nblock; block++) {
   RegMapBlock* blk = brd->blocks[block];
   
   if(!is_MonitorViewer_archivedOnly(tm->view) || blk->isArchived()) {
     
     os.str("");
     os << blk->name_;
     os << *(blk->axes_);
     
     regNames.push_back(os.str());
   }
 }

 regNames = gcp::util::Sort::sort(regNames);

 for(unsigned i=0; i < regNames.size(); i++) {
   Tcl_AppendResult(interp, regNames[i].c_str(), " ", NULL);
 }

 return TCL_OK;
}

/*.......................................................................
 * Delete all active pages.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_delete_pages)
{
  rem_MonitorViewer_pages(tm->view);
  return TCL_OK;
}

/*.......................................................................
 * Delete all active plots.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_delete_plots)
{
  rem_MonitorViewer_plots(tm->view);
  return TCL_OK;
}

/*.......................................................................
 * Delete all graphs of a given plot.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the plot to
 *                                 modify.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_delete_graphs)
{
  MonitorPlot *plot;   /* The plot to be modified */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  rem_MonitorPlot_graphs(plot);
  return TCL_OK;
}

/*.......................................................................
 * Delete all fields of a given page.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the page to
 *                                 modify.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_delete_fields)
{
  MonitorPage *page;   /* The page to be modified */
  /*
   * Locate the page whose tag is named in the first argument.
   */
  page = tm_find_MonitorPage(tm, argv[0]);
  if(!page)
    return TCL_ERROR;
  rem_MonitorPage_fields(page);
  return TCL_OK;
}

/*.......................................................................
 * Add a new text page to the viewer hierarchy.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments (Not used).
 * Output:
 *  return           int     TCL_OK    - The result string will contain
 *                                       the identifier tag of the new
 *                                       page, or 0 if the creation
 *                                       failed.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_add_page)
{
  MonitorPage *page = add_MonitorPage(tm->view);
  /*
   * Record the identifier tag of the new page for return.
   */
  sprintf(tm->buffer, "%u", tag_of_MonitorPage(page));
  Tcl_AppendResult(interp, tm->buffer, NULL);
  return TCL_OK;
}

/*.......................................................................
 * Add a new plot to the viewer hierarchy.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The PGPLOT device specification to
 *                                 of the device to attempt to open.
 * Output:
 *  return           int     TCL_OK    - The result string will contain
 *                                       the identifier tag of the new
 *                                       plot, or 0 if the creation
 *                                       failed.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_add_plot)
{
  MonitorPlot* plot=0;

  plot = add_MonitorPlot(tm->view, argv[0], argv[1]);

  /*
   * Record the identifier tag of the new plot for return.
   */
  sprintf(tm->buffer, "%u", tag_of_MonitorPlot(plot));
  Tcl_AppendResult(interp, tm->buffer, NULL);

  return TCL_OK;
}

/*.......................................................................
 * Add a new image display to the viewer.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The PGPLOT device specification to
 *                                 of the device to attempt to open.
 * Output:
 *  return           int     TCL_OK    - The result string will contain
 *                                       the identifier tag of the new
 *                                       plot, or 0 if the creation
 *                                       failed.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_open_image)
{
  if(open_MonitorImage(tm->view, argv[0]))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Add a graph of a given register list to a plot.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the plot to
 *                                 modify.
 * Output:
 *  return           int     TCL_OK    - The identifier tag of the newly
 *                                       created graph, or 0 if the creation
 *                                       failed.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_add_graph)
{
  MonitorPlot *plot;   /* The parent plot of the graph */
  MonitorGraph *graph; /* The newly created graph */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Attempt to create the new graph.
   */
  graph = add_MonitorGraph(plot);
  /*
   * Record the identifier tag of the new graph for return.
   */
  sprintf(tm->buffer, "%u", tag_of_MonitorGraph(graph));
  Tcl_AppendResult(interp, tm->buffer, NULL);
  return TCL_OK;
}

/*.......................................................................
 * Remove a page from the viewer hierarchy.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the page to
 *                                 be removed.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_rem_page)
{
  MonitorPage *page;   /* The page to be removed */
  /*
   * Locate the page whose tag is named in the first argument.
   */
  page = tm_find_MonitorPage(tm, argv[0]);
  if(!page)
    return TCL_ERROR;
  /*
   * Zap it.
   */
  rem_MonitorPage(page);
  return TCL_OK;
}

/*.......................................................................
 * Stop updating the contents of a given page.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the page to
 *                                 be frozen.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_freeze_page)
{
  MonitorPage *page;   /* The page to be removed */
  /*
   * Locate the page whose tag is named in the first argument.
   */
  page = tm_find_MonitorPage(tm, argv[0]);
  if(!page)
    return TCL_ERROR;
  /*
   * Freeze it.
   */
  freeze_MonitorPage(page);
  return TCL_OK;
}

/*.......................................................................
 * Resume updating the contents of a given page.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the page to
 *                                 be frozen.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_unfreeze_page)
{
  MonitorPage *page;   /* The page to be removed */
  /*
   * Locate the page whose tag is named in the first argument.
   */
  page = tm_find_MonitorPage(tm, argv[0]);
  if(!page)
    return TCL_ERROR;
  /*
   * Freeze it.
   */
  unfreeze_MonitorPage(page);
  return TCL_OK;
}

/*.......................................................................
 * Remove a field from a given page.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the parent
 *                                 page of the field.
 *                           [1] - The identification tag of the field to
 *                                 be removed.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_rem_field)
{
  MonitorPage *page;   /* The parent page */
  MonitorField *field; /* The field to be removed */
  /*
   * Locate the page whose tag is named in the first argument.
   */
  page = tm_find_MonitorPage(tm, argv[0]);
  if(!page)
    return TCL_ERROR;
  /*
   * Find the field to be deleted.
   */
  field = tm_find_MonitorField(tm, page, argv[1]);
  if(!field)
    return TCL_ERROR;
  /*
   * Zap it.
   */
  rem_MonitorField(field);
  return TCL_OK;
}

/*.......................................................................
 * Remove a plot from the viewer hierarchy.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the plot to
 *                                 be removed.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_rem_plot)
{
  MonitorPlot *plot;   /* The plot to be removed */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Zap it.
   */
  rem_MonitorPlot(plot);
  return TCL_OK;
}

/*.......................................................................
 * Remove a graph from a given plot.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the parent
 *                                 plot of the graph.
 *                           [1] - The identification tag of the graph to
 *                                 be removed.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_rem_graph)
{
  MonitorPlot *plot;   /* The parent plot */
  MonitorGraph *graph; /* The graph to be removed */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Find the graph to be deleted.
   */
  graph = tm_find_MonitorGraph(tm, plot, argv[1]);
  if(!graph)
    return TCL_ERROR;
  /*
   * Zap it.
   */
  rem_MonitorGraph(graph);
  return TCL_OK;
}

/*.......................................................................
 * Remove a graph from a given plot.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the parent
 *                                 plot of the graph.
 *                           [1] - The identification tag of the graph to
 *                                 be removed.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_int_graph)
{
  MonitorPlot *plot;   /* The parent plot */
  MonitorGraph *graph; /* The graph to be removed */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Find the graph to be integrated
   */
  graph = tm_find_MonitorGraph(tm, plot, argv[1]);
  if(!graph)
    return TCL_ERROR;
  /*
   * Toggle its integration status it.
   */
  if(int_MonitorGraph(graph, atoi(argv[2])))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Return the X-axis extent of the buffered data.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the plot to
 *                                 query.
 *                           [1] - The name of the local TCL variable in
 *                                 which to record the lower x-axis limit.
 *                           [2] - The name of the local TCL variable in
 *                                 which to record the upper x-axis limit.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                                       the requested limits as two
 *                                       double-precision numbers.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_get_xlimits)
{
  MonitorPlot *plot;   /* The plot to be queried */
  double xmin,xmax;    /* The requested X-axis limits */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  if(full_MonitorPlot_xrange(plot, &xmin, &xmax))
    return TCL_ERROR;
  /*
   * Assign the values to the named Tcl variables.
   */
  sprintf(tm->buffer, "%.*g", DBL_DIG, xmin);
  if(!Tcl_SetVar(tm->interp, argv[1], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, xmax);
  if(!Tcl_SetVar(tm->interp, argv[2], tm->buffer, 0))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Return the current X-axis limits of a given plot.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the plot to
 *                                 query.
 *                           [1] - The name of the local TCL variable in
 *                                 which to record the lower x-axis limit.
 *                           [2] - The name of the local TCL variable in
 *                                 which to record the upper x-axis limit.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_get_x_axis_limits)
{
  MonitorPlot *plot;   /* The plot to be queried */
  double xmin,xmax;    /* The requested X-axis limits */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  if(mp_xaxis_limits(plot, &xmin, &xmax))
    return TCL_ERROR;
  /*
   * Assign the values to the named Tcl variables.
   */
  sprintf(tm->buffer, "%.*g", DBL_DIG, xmin);
  if(!Tcl_SetVar(tm->interp, argv[1], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, xmax);
  if(!Tcl_SetVar(tm->interp, argv[2], tm->buffer, 0))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Return a suitable y-axis plot range for a given graph.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the plot to
 *                                 query.
 *                           [1] - The identification tag of the graph to
 *                                 query.
 *                           [2] - The name of the local TCL variable in
 *                                 which to record the lower y-axis limit.
 *                           [3] - The name of the local TCL variable in
 *                                 which to record the upper y-axis limit.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_get_y_autoscale)
{
  MonitorPlot *plot;   /* The plot to be queried */
  MonitorGraph *graph; /* The graph to be queried */
  double ymin,ymax;    /* The requested Y-axis limits */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Locate the child graph to be queried.
   */
  graph = tm_find_MonitorGraph(tm, plot, argv[1]);
  if(!graph)
    return TCL_ERROR;
  if(auto_MonitorGraph_yrange(graph, &ymin, &ymax))
    return TCL_ERROR;
  /*
   * Assign the values to the named Tcl variables.
   */
  sprintf(tm->buffer, "%.*g", DBL_DIG, ymin);
  if(!Tcl_SetVar(tm->interp, argv[2], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, ymax);
  if(!Tcl_SetVar(tm->interp, argv[3], tm->buffer, 0))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Limit the given plot to only display subsequently received data.
 * A subsequent call to the "reconfigure" command will be required to
 * restart plotting.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the plot to
 *                                 redraw.
 * Output:
 *  return           int     TCL_OK    - OK.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_limit_plot)
{
  MonitorPlot *plot;   /* The plot to be redrawn */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Change the snapshot index limit.
   */
  (void) limit_MonitorPlot(plot);
  return TCL_OK;
}

/*.......................................................................
 * Redraw a given plot with updated axes.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the plot to
 *                                 redraw.
 * Output:
 *  return           int     TCL_OK    - OK.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_redraw_plot)
{
  MonitorPlot *plot;   /* The plot to be redrawn */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Redraw/configure the plot.
   */
  (void) update_MonitorPlot(plot, 1);
  return TCL_OK;
}

/*.......................................................................
 * Re-synchronize the size of a plot with its plot device.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the plot to
 *                                 resize.
 * Output:
 *  return           int     TCL_OK    - OK.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_resize_plot)
{
  MonitorPlot *plot;   /* The plot to be redrawn */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Redraw/configure the plot.
   */
  (void) resize_MonitorPlot(plot);
  return TCL_OK;
}

/*.......................................................................
 * Redraw a given graph with updated axes.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the parent
 *                                 plot of the graph.
 *                           [1] - The identification tag of the graph
 *                                 to be redrawn.
 * Output:
 *  return           int     TCL_OK    - OK.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_redraw_graph)
{
  MonitorPlot *plot;   /* The plot to be redrawn */
  MonitorGraph *graph; /* The graph to be redrawn */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Locate the child graph to be redrawn.
   */
  graph = tm_find_MonitorGraph(tm, plot, argv[1]);
  if(!graph)
    return TCL_ERROR;
  /*
   * Redraw/configure the graph.
   */
  (void) update_MonitorGraph(graph, 1);
  return TCL_OK;
}

/*.......................................................................
 * Change the input stream to take real-time data from the control-program.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The name of the control computer, or
 *                                 "" to disconnect.
 * Output:
 *  return           int     TCL_OK    - The result string contains
 *                                       a boolean flag which will be
 *                                       true if the current plot
 *                                       configuration had to be
 *                                       deleted to accomodate the
 *                                       register map of the new
 *                                       stream.
 *                           TCL_ERROR - Error. The configuration
 *                                       remains unchanged.
 */
static SERVICE_FN(tm_host)
{
  MonitorStream *ms; // The new monitor stream
  char* host    = 0; // The host to connect to
  char* gateway = 0; // The gateway computer to connect through
  int timeout   = 0; // The timeout in seconds
  int retry     = 0; // True if this connection should be retried

  host = argv[0];

  if(Tcl_GetInt(interp, argv[1], &retry) == TCL_ERROR)
    return TCL_ERROR;

  gateway = argv[2];

  if(Tcl_GetInt(interp, argv[3], &timeout) == TCL_ERROR)
    return TCL_ERROR;

  // Set information in the Retry object regardless of whether or
  // not auto-reconnect is enabled
  
  tm->retry_->setTo(host, gateway, timeout, retry);

  // Attempt to connect

  int retVal = tm_connect(tm, interp, host, gateway, timeout);

  // If we failed, set a timer on expiry of which we will attempt to
  // reconnect (if auto reconnect is enabled)

  enableRetryTimer(tm, (retVal == TCL_ERROR));

  return retVal;
}

/**.......................................................................
 * Activate or deactivate the retry timer
 */
void enableRetryTimer(TclMonitor* tm, bool enable)
{
  // If auto connect is not enabled, do nothing

  if(!tm->retry_->enabled()) 
    return;

  // Else enable or disable the timer

  if(enable) {
    tm->connectTimer_->addHandler(retryConnectionHandler, (void*)tm);
    tm->connectTimer_->enableTimer(true, 10);
  } else {
    tm->connectTimer_->enableTimer(false);
    tm->connectTimer_->removeHandler(retryConnectionHandler);
  }
}

/**.......................................................................
 * If requested to reconnect when the connection is lost, we set a
 * periodic timer.  On expiry of this timer, this function is called
 * to retry the connection.  If the connection succeeds, the timer is
 * disabled, else we keep trying.
 */
PERIODIC_TIMER_HANDLER(retryConnectionHandler) 
{
  TclMonitor* tm = (TclMonitor*) args;
  TclMonitorMsg msg;
  msg.type_ = TclMonitorMsg::MSG_CONNECT;
  tm->msgq_->sendMsg(&msg);
}

/**.......................................................................
 * If requested to reconnect when the connection is lost, we set a
 * periodic timer.  On expiry of this timer, this function is called
 * to retry the connection.  If the connection succeeds, the timer is
 * disabled, else we keep trying.
 */
static int tm_retry_connection(TclMonitor* tm)
{
  std::string host, gateway;
  unsigned timeout, interval;

  tm->retry_->copyArgs(host, gateway, timeout, interval);

  // Attempt to connect

  int retVal = tm_connect(tm, tm->interp, (char*)host.c_str(), (char*)gateway.c_str(), timeout);

  // If successful, reconfigure the viewer and resend the update
  // interval

  if(retVal == TCL_OK) {

    tm_reconfigure(tm->interp, tm, 0, 0);
      
    // Send the interval update.
    
    switch(set_MonitorViewer_interval(tm->view, interval)) {
    case MS_SEND_ERROR:
      tm_update_TclFile(tm, -1, 0);
      break;
    case MS_SEND_AGAIN:
      tm_update_TclFile(tm, MonitorViewer_fd(tm->view), 1);
      break;
    case MS_SEND_DONE:
      tm_update_TclFile(tm, MonitorViewer_fd(tm->view), 0);
      break;
    };
    
  }

  // And disable the retry timer on success

  enableRetryTimer(tm, (retVal == TCL_ERROR));
}

/**.......................................................................
 * Attempt to connect to the monitor stream
 */
static int tm_connect(TclMonitor* tm, Tcl_Interp* interp, 
		      char* host, char* gateway, unsigned timeout)
{
  MonitorStream* ms=0; // The new monitor stream

  // Remove any existing tunnel, if one has already been opened

  if(tm->monitorTunnel_ != 0) {
    delete tm->monitorTunnel_;
    tm->monitorTunnel_ = 0;
  }

  // Attempt to open the new stream.

  if(*host == '\0') {
    ms = NULL;
  } else {

    if(*gateway != '\0') {

      tm->monitorTunnel_ = 
	new gcp::util::SshTunnel(gateway, host, 
				 (unsigned short)CP_MONITOR_PORT, 
				 (unsigned int)timeout);
      
      if(tm->monitorTunnel_->succeeded()) {
	ms = new_NetMonitorStream("localhost");
      } else {
	Tcl_AppendResult(interp, tm->monitorTunnel_->error().c_str(), NULL);
	delete tm->monitorTunnel_;
	tm->monitorTunnel_ = 0;
	return TCL_ERROR;
      }

    } else {
      ms = new_NetMonitorStream(host);
    } 

    if(!ms) {
      Tcl_AppendResult(interp, "Unable to connect to the control program.",
		       NULL);
      tm_update_TclFile(tm, -1, 0);
      return TCL_ERROR;
    };
  };
  
  // Replace the current monitor stream with the new one. Tell the
  // viewer if this results in a change of register map.

  if(change_MonitorStream(tm->view, ms, 0)) {
    call_regmap_callback(tm);
  }

  tm_update_TclFile(tm, MonitorViewer_fd(tm->view), 0);

  return TCL_OK;
}

/*.......................................................................
 * Change the image input stream to take real-time data from the 
 * control-program.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The name of the control computer, or
 *                                 "" to disconnect.
 * Output:
 *  return           int     TCL_OK    - The result string contains
 *                                       a boolean flag which will be
 *                                       true if the current plot
 *                                       configuration had to be
 *                                       deleted to accomodate the
 *                                       register map of the new
 *                                       stream.
 *                           TCL_ERROR - Error. The configuration
 *                                       remains unchanged.
 */
static SERVICE_FN(tm_imhost)
{
  ImMonitorStream *ims;    /* The new monitor stream */
  char *host = argv[0]; /* The host to connect to */
  /*
   * Attempt to open the new stream.
   */
  if(*host == '\0') {
    ims = NULL;
  } else {
    ims = new_NetImMonitorStream(argv[0]);
    if(!ims) {
      Tcl_AppendResult(interp, "Unable to connect to the control program.",
		       NULL);
      tm_update_im_TclFile(tm, -1, 0);
      return TCL_ERROR;
    };
  };
  /*
   * Replace the current image monitor stream with the new one and record whether
   * the configuration had to be changed, in the result string.
   */
  Tcl_AppendResult(interp, change_ImMonitorStream(tm->view, ims) ? "1":"0",
		   NULL);
  tm_update_im_TclFile(tm, MonitorViewer_im_fd(tm->view), 0);
  return TCL_OK;
}
/*.......................................................................
 * Disconnect an image input stream.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The name of the control computer, or
 *                                 "" to disconnect.
 * Output:
 *  return           int     TCL_OK    - The result string contains
 *                                       a boolean flag which will be
 *                                       true if the current plot
 *                                       configuration had to be
 *                                       deleted to accomodate the
 *                                       register map of the new
 *                                       stream.
 *                           TCL_ERROR - Error. The configuration
 *                                       remains unchanged.
 */
static SERVICE_FN(tm_im_disconnect)
{
  /*
   * Replace the current image monitor stream with a NULL stream.
   */
  Tcl_AppendResult(interp, change_ImMonitorStream(tm->view, NULL) ? "1":"0",
		   NULL);
  tm_update_im_TclFile(tm, MonitorViewer_im_fd(tm->view), 0);
  return TCL_OK;
}

/*.......................................................................
 * Change the input stream to take archived data from disk.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The name of the directory in which
 *                                 to search for archive files.
 *                           [1] - A start date and time, formatted
 *                                  like "23-JAN-1997 23:34:21".
 *                           [2] - An end date and time, formatted
 *                                  like "24-JAN-1997 04:03:00".
 *                           [3] - Boolean: True if data should be
 *                                 plotted while being loaded. False
 *                                 if display should be defered until
 *                                 all of the data have been loaded.
 * Output:
 *  return           int     TCL_OK    - The result string contains
 *                                       a boolean flag which will be
 *                                       true if the current plot
 *                                       configuration had to be
 *                                       deleted to accomodate the
 *                                       register map of the new
 *                                       stream.
 *                           TCL_ERROR - Error. The configuration
 *                                       remains unchanged.
 */
static SERVICE_FN(tm_disk)
{
  MonitorStream *ms;          /* The new monitor stream */
  double ta,tb;               /* The start and end times as MJD */
  int live;                   /* True to plot data on the fly */
  /*
   * Read the starting and ending date and time.
   */
  if(tm_parse_date_and_time(tm, argv[1], &ta) == TCL_ERROR ||
     tm_parse_date_and_time(tm, argv[2], &tb) == TCL_ERROR)
    return TCL_ERROR;
  /*
   * Should we plot data on-the-fly?
   */
  if(Tcl_GetBoolean(interp, argv[3], &live) == TCL_ERROR)
    return TCL_ERROR;
  /*
   * Attempt to open the new stream.
   */
  ms = new_FileMonitorStream(argv[0], ta, tb);
  if(!ms) {
    Tcl_AppendResult(interp, "Unable to acquire archive input.", NULL);
    tm_update_TclFile(tm, -1, 0);
    return TCL_ERROR;
  };
  /*
   * Replace the current monitor stream with the new one. Tell the viewer
   * if this results in a change of register map.
   */
  if(change_MonitorStream(tm->view, ms, !live))
    call_regmap_callback(tm);
  tm_update_TclFile(tm, MonitorViewer_fd(tm->view), 0);
  return TCL_OK;
}

/*.......................................................................
 * This is a private function of tm_disk() used to parse a UTC date and
 * time of the form "23-MAY-1997 23:34:00" and convert it to a Modified
 * Julian Date.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  string          char *   The string to parse the date/time from.
 * Input/Output:
 *  utc           double *   The output UTC as a Modified Julian Date, or
 *                           0.0 if the string was empty.
 * Output:
 *  return           int     TCL_OK or TCL_ERROR.
 */
static int tm_parse_date_and_time(TclMonitor *tm, char *string, double *utc)
{
  InputStream *stream;  /* The stream to parse from */
  int waserr=0;         /* True if an error occurs while the stream is open */
  int start;            /* The index of the first non-space character */
  /*
   * Find the location of the first non-space character in the string.
   */
  for(start=0; isspace((int) string[start]); start++)
    ;
  /*
   * If the string is empty then return the wildcard utc designator.
   */
  if(string[start] == '\0') {
    *utc = 0.0;
    return TCL_OK;
  };
  /*
   * Attach an input stream to the specification string.
   */
  stream = tm->input;
  if(open_StringInputStream(stream, 0, string + start)) {
    Tcl_AppendResult(tm->interp, "parse_date_and_time stream failure.", NULL);
    return TCL_ERROR;
  };
  /*
   * Parse the date and time from the string.
   */
  waserr = input_utc(stream, 1, 0, utc);
  /*
   * The stream is now redundant.
   */
  close_InputStream(stream);
  /*
   * Report errors.
   */
  if(waserr) {
    Tcl_AppendResult(tm->interp, "Invalid date or time.", NULL);
    return TCL_ERROR;
  };
  return TCL_OK;
}

/*.......................................................................
 * Update the viewer to take account of configuration changes.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 * Output:
 *  return           int     TCL_OK    - OK.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_reconfigure)
{
  
  switch(update_MonitorViewer(tm->view)) {
  case MS_SEND_ERROR:
    tm_update_TclFile(tm, -1, 0);
    break;
  case MS_SEND_AGAIN:
    tm_update_TclFile(tm, MonitorViewer_fd(tm->view), 1);
    break;
  case MS_SEND_DONE:
    tm_update_TclFile(tm, MonitorViewer_fd(tm->view), 0);
    break;
  };
  return TCL_OK;
}

/*.......................................................................
 * Replace the current calibration with the contents of a cal file.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [1] - The path name of the calibration file.
 * Output:
 *  return           int     TCL_OK    - OK.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_load_cal)
{
  if(mp_set_calfile(tm->view, argv[0]))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Test whether a valid data-source is currently connected. If so return
 * "1" in interp->result, else "0".
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 * Output:
 *  return           int     TCL_OK
 */
static SERVICE_FN(tm_have_stream)
{
  Tcl_SetResult(interp, (char* )(MonitorViewer_ArrayMap(tm->view) ? "1":"0"), 
		TCL_STATIC);
  return TCL_OK;
}

/*.......................................................................
 * Request a change in the sampling interval of the monitor stream.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The new sampling interval (unsigned).
 * Output:
 *  return           int     TCL_OK    - A data-source is not connected.
 *                                       The result string will contain
 *                                       an explanation.
 *                           TCL_ERROR - A data-source is available.
 */
static SERVICE_FN(tm_set_interval)
{
  unsigned interval;    // The new sampling interval 
  char* interval_s = 0; // The interval argument string 
  char* endp = 0;       // The pointer to the undecoded part of interval_s 
  
  // Decode the interval from the given string. Note that Tcl_GetInt()
  // decodes signed integers whereas intervals are unsigned, so we
  // have to decompose the string ourself.

  interval_s = argv[0];
  interval = strtoul(interval_s, &endp, 0);
  if(*interval_s == '\0' || !endp || *endp != '\0') {
    Tcl_AppendResult(tm->interp, "Expected an unsigned integer, but got: '",
		     interval_s, "'", NULL);
    return TCL_ERROR;
  };
  
  // Send the interval update.

  switch(set_MonitorViewer_interval(tm->view, (unsigned)interval)) {
  case MS_SEND_ERROR:
    tm_update_TclFile(tm, -1, 0);
    break;
  case MS_SEND_AGAIN:
    tm_update_TclFile(tm, MonitorViewer_fd(tm->view), 1);
    break;
  case MS_SEND_DONE:
    tm_update_TclFile(tm, MonitorViewer_fd(tm->view), 0);
    break;
  };

  // And store the interval for future reconnection attempts

  tm->retry_->setInterval(interval);

  return TCL_OK;
}

/*.......................................................................
 * Request that the monitor stream be rewound. This does nothing if the
 * stream is not rewindable.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments: (none)
 * Output:
 *  return           int     TCL_OK    - A data-source is not connected.
 *                                       The result string will contain
 *                                       an explanation.
 *                           TCL_ERROR - A data-source is available.
 */
static SERVICE_FN(tm_queue_rewind)
{
  /*
   * Queue the rewind.
   */
  switch(queue_MonitorViewer_rewind(tm->view)) {
  case MS_SEND_ERROR:
    tm_update_TclFile(tm, -1, 0);
    break;
  case MS_SEND_AGAIN:
    tm_update_TclFile(tm, MonitorViewer_fd(tm->view), 1);
    break;
  case MS_SEND_DONE:
    tm_update_TclFile(tm, MonitorViewer_fd(tm->view), 0);
    break;
  };
  return TCL_OK;
}

/*.......................................................................
 * Make a copy of a Tcl callback and record it along with the
 * monitor-interface resource object in a container.
 *
 * Input:
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  script          char *   The callback script to be recorded.
 * Output:
 *  return    TmCallback *   The encapsulated callback, or NULL on error.
 *                           On error, an error message will have been
 *                           composed in tm->interp->result.
 */
static TmCallback *new_TmCallback(TclMonitor *tm, char *script)
{
  TmCallback *tmc;   /* The callback container to be returned */
  if(!script) {
    Tcl_AppendResult(tm->interp, "new_TmCallback: Missing script.", NULL);
    return NULL;
  };
  /*
   * Allocate the container.
   */
  tmc = (TmCallback* )new_FreeListNode(NULL, tm->tmc_mem);
  if(!tmc) {
    Tcl_AppendResult(tm->interp, "new_TmCallback: Insufficient memory.", NULL);
    return NULL;
  };
  /*
   * Before attempting any operation that might fail, initialize the container
   * at least up to the point at which it can safely be passed to
   * del_TmCallback().
   */
  tmc->tm = tm;
  tmc->script = NULL;
  /*
   * Allocate a copy of the callback script.
   */
  tmc->script = (char* )malloc(strlen(script) + 1);
  if(!tmc->script) {
    Tcl_AppendResult(tm->interp, "new_TmCallback: Insufficient memory.", NULL);
    return (TmCallback* )del_TmCallback(tmc);
  };
  strcpy(tmc->script, script);
  return tmc;
}

/*.......................................................................
 * Delete a TmCallback container that was previously allocated by
 * new_TmCallback(). This function is designed to be useable as a
 * MP_DEL_FN() destructor, so (void *) is used instead of (TmCallback *).
 *
 * Input:
 *  user_data void *   The (TmCallback *) object to be deleted.
 * Output:
 *  return    void *   The deleted container (always NULL).
 */
static void *del_TmCallback(void *user_data)
{
  TmCallback *tmc = (TmCallback* )user_data;
  if(tmc) {
    if(tmc->script)
      free(tmc->script);
    tmc = (TmCallback* )del_FreeListNode("del_TmCallback", tmc->tm->tmc_mem, tmc);
  };
  return NULL;
}

/*.......................................................................
 * Register a script to be called whenever a given plot is scrolled.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] The identification tag of the
 *                               plot.
 *                           [1] The callback script to register. When
 *                               the plot is scrolled, the following
 *                               arguments will be appended to the script
 *                               prior to it being executed:
 *                                 plot_tag xleft xright
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_scroll_callback)
{
  MonitorPlot *plot;   /* The plot to be addressed */
  TmCallback *tmc;     /* The encapsulated callback and Tcl interpretter */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Allocate a copy of the callback if needed.
   */
  if(*argv[1] == '\0') {
    tmc = NULL;
  } else {
    tmc = new_TmCallback(tm, argv[1]);
    if(!tmc)
      return TCL_ERROR;
  };
  /*
   * Register the local callback dispatcher function with the
   * viewer.
   */
  if(mp_scroll_callback(plot, scroll_dispatch_fn, tmc, del_TmCallback)) {
    Tcl_AppendResult(interp, "Failed to register scroll callback.", NULL);
    return TCL_ERROR;
  };
  return TCL_OK;
}

/*.......................................................................
 * This is a private callback of the tm_scroll_callback() service. It
 * delivers scroll events to the Tcl callback script registered by the
 * user.
 */
static MP_SCROLL_FN(scroll_dispatch_fn)
{
  if(user_data) {
    TmCallback *tmc = (TmCallback* )user_data;
    TclMonitor *tm = (TclMonitor *) tmc->tm;
    /*
     * Compose the return arguments.
     */
    sprintf(tm->buffer, " %u %.*g %.*g", tag_of_MonitorPlot(plot),
	    DBL_DIG, wxa, DBL_DIG, wxb);
    /*
     * Invoke the callback.
     */
    if(Tcl_VarEval(tm->interp, tmc->script, tm->buffer, NULL) == TCL_ERROR)
      lprintf(stderr, "%s\n", tm->interp->result);
  };
}

/*.......................................................................
 * Convert from cursor world-coordinates to graph coordinates.
 * It is assumed that cursor world coordinates are identical to
 * normalized device coordinates.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] The identification tag of the plot.
 *                           [1] The identification tag of the graph
 *                               if known. Otherwise send 0.
 *                           [2] The x-axis world coordinate of the cursor.
 *                           [3] The y-axis world coordinate of the cursor.
 * Input/Output:
 *                           [4] The name of the local Tcl variable in which
 *                               to record the tag id of the graph under the
 *                               cursor.
 *                           [5] The name of the local Tcl variable in which
 *                               to record the x-axis graph coordinate of
 *                               the cursor.
 *                           [6] The name of the local Tcl variable in which
 *                               to record the y-axis graph coordinate of
 *                               the cursor.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_cursor_to_graph)
{
  MonitorPlot *plot;   /* The plot to be addressed */
  MonitorGraph *graph; /* The input and output graph */
  double x, y;         /* The input and output coordinates */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * If a graph tag has been provided, decode it.
   */
  if(strcmp(argv[1], "0") == 0) {
    graph = NULL;
  } else {
    graph = tm_find_MonitorGraph(tm, plot, argv[1]);
    if(!graph)
      return TCL_ERROR;
  };
  /*
   * Decode the cursor coordinates that are to be converted.
   */
  if(Tcl_GetDouble(interp, argv[2], &x) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &y) == TCL_ERROR)
    return TCL_ERROR;
  /*
   * Convert the coordinates.
   */
  if(mp_cursor_to_graph(plot, &graph, &x, &y)) {
    Tcl_AppendResult(interp, "cursor_to_graph conversion error.\n", NULL);
    return TCL_ERROR;
  };
  /*
   * Assign the values to the named Tcl variables.
   */
  sprintf(tm->buffer, "%u", tag_of_MonitorGraph(graph));
  if(!Tcl_SetVar(tm->interp, argv[4], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, x);
  if(!Tcl_SetVar(tm->interp, argv[5], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, y);
  if(!Tcl_SetVar(tm->interp, argv[6], tm->buffer, 0))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Convert from graph coordinates to cursor world-coordinates.
 * The returned cursor coordinates will be identical to
 * normalized device coordinates.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] The identification tag of the plot.
 *                           [1] The identification tag of the graph.
 *                           [2] The x-axis graph coordinate.
 *                           [3] The y-axis graph coordinate.
 * Input/Output:
 *                           [4] The name of the local Tcl variable in
 *                               which to record the cursor x-axis
 *                               coordinate.
 *                           [5] The name of the local Tcl variable in
 *                               which to record the cursor y-axis
 *                               coordinate.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_graph_to_cursor)
{
  MonitorPlot *plot;   /* The plot to be addressed */
  MonitorGraph *graph; /* The graph to which the coordinates refer */
  double x, y;         /* The input and output coordinates */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Decode the graph id tag.
   */
  graph = tm_find_MonitorGraph(tm, plot, argv[1]);
  if(!graph)
    return TCL_ERROR;
  /*
   * Decode the graph coordinates that are to be converted.
   */
  if(Tcl_GetDouble(interp, argv[2], &x) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &y) == TCL_ERROR)
    return TCL_ERROR;
  /*
   * Convert the coordinates.
   */
  if(mp_graph_to_cursor(graph, &x, &y)) {
    Tcl_AppendResult(interp, "graph_to_cursor conversion error.\n", NULL);
    return TCL_ERROR;
  };
  /*
   * Assign the values to the named Tcl variables.
   */
  sprintf(tm->buffer, "%.*g", DBL_DIG, x);
  if(!Tcl_SetVar(tm->interp, argv[4], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, y);
  if(!Tcl_SetVar(tm->interp, argv[5], tm->buffer, 0))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Return details of the nearest point to a given graph location.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] The identification tag of the plot.
 *                           [1] The identification tag of the graph.
 *                           [2] The x-axis graph coordinate.
 *                           [3] The y-axis graph coordinate.
 * Input/output:
 *                           [4] The name of the local Tcl variable in
 *                               which to record the name of the
 *                               register associated with the nearest
 *                               point.
 *                           [5] The name of the local Tcl variable in
 *                               which to record the name of the
 *                               register associated with the nearest
 *                               point.
 *                           [6] The name of the local Tcl variable in
 *                               which to record the X-axis value of
 *                               the nearest point.
 *                           [7] The name of the local Tcl variable in
 *                               which to record the Y-axis value of
 *                               the nearest point.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_find_point)
{
  MonitorPlot *plot;   /* The plot to be addressed */
  MonitorGraph *graph; /* The graph to which the coordinates refer */
  gcp::util::RegDescription desc;
  double x, y;         /* The input and output coordinates */
  ArrayMap *arraymap;  /* The array map of the current monitor stream */
  
  // Get the register map of the current data-stream.
  
  arraymap = MonitorViewer_ArrayMap(tm->view);
  if(!arraymap) {
    Tcl_AppendResult(interp, "No array map is currently available.", NULL);
    return TCL_ERROR;
  };
  
  // Locate the plot whose tag is named in the first argument.
  
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  
  // Decode the graph id tag.
  
  graph = tm_find_MonitorGraph(tm, plot, argv[1]);
  if(!graph)
    return TCL_ERROR;
  
  // Decode the target coordinates.
  
  if(Tcl_GetDouble(interp, argv[2], &x) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &y) == TCL_ERROR)
    return TCL_ERROR;
  
  // Search for the closest register.
  
  if(findMonitorPoint(graph, x, y, desc, x, y)) {
    Tcl_AppendResult(interp, "Unable to locate nearest data-point.", NULL);
    return TCL_ERROR;
  };
  
  // Assign the values to the named Tcl variables.
  
  // First output the complex name of the (possibly multi-register) trace

  if(open_StringOutputStream(tm->output, 0, tm->buffer, sizeof(tm->buffer))) {
    lprintf(stderr, "Error writing statistics information.");
    close_OutputStream(tm->output);
    return TCL_ERROR;
  };

  try {
    outputRegName(tm->output, graph, desc);
  } catch(...) {
    lprintf(stderr, "Error writing statistics information.");
    close_OutputStream(tm->output);
    return TCL_ERROR;
  }

  if(!Tcl_SetVar(tm->interp, argv[4], tm->buffer, 0))
    return TCL_ERROR;

  close_OutputStream(tm->output);

  // Now output the name of the specific register we were closest to

  if(open_StringOutputStream(tm->output, 0, tm->buffer, sizeof(tm->buffer))) {
    lprintf(stderr, "Error writing statistics information.");
    close_OutputStream(tm->output);
    return TCL_ERROR;
  };

  try {
    outputStatName(tm->output, graph, desc);
  } catch(...) {
    lprintf(stderr, "Error writing statistics information.");
    close_OutputStream(tm->output);
    return TCL_ERROR;
  }

  if(!Tcl_SetVar(tm->interp, argv[5], tm->buffer, 0))
    return TCL_ERROR;

  sprintf(tm->buffer, "%.*g", DBL_DIG, x);
  if(!Tcl_SetVar(tm->interp, argv[6], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, y);
  if(!Tcl_SetVar(tm->interp, argv[7], tm->buffer, 0))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Return the statistics of one of the registers that is being monitored.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] A register element specification. This
 *                               must name a register that is currently
 *                               being monitored.
 * Output:
 *  return           int     TCL_OK    - Success. The result string will
 *                                       contain:
 *                                        min max mean rms npoint
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_global_stats)
{
  gcp::util::RegDescription desc;      /* The decoded register specification */
  MonitorRegStats stats;  /* The container of the requested statistics */
  
  // Get the register specification.

  if(parse_reg(interp, tm, argv[0], REG_INPUT_ELEMENT, 1, &desc) == TCL_ERROR)
    return TCL_ERROR;
  
  // Query the statistics of its buffered values.

  if(mp_global_RegStats(tm->view, &desc, &stats)) {
    Tcl_AppendResult(interp, "Not a monitored register element: ", argv[0],
		     NULL);
    return TCL_ERROR;
  };
  
  // Compose the return string.

  sprintf(tm->buffer, "%f %f %f %f %ld", stats.min, stats.max, stats.mean,
	  stats.rms, stats.npoint);
  Tcl_AppendResult(interp, tm->buffer, NULL);
  return TCL_OK;
}

/*.......................................................................
 * Return the statistics of a subset of the buffered data of one of the
 * registers that is being monitored.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] The identification tag of a plot to
 *                               use to limit the source of data to
 *                               be characterised.
 *                           [1] The scalar specification of the Y-axis
 *                               register to be characterised.
 *                           [2] One end of the limiting X-axis range.
 *                           [3] The other end of the limiting X-axis
 *                               range.
 * Input/Output:
 *                           [4] The name of a local variable in which
 *                               to record the minimum value found.
 *                           [5] The name of a local variable in which
 *                               to record the maximum value found.
 *                           [6] The name of a local variable in which
 *                               to record the mean value found.
 *                           [7] The name of a local variable in which
 *                               to record the root-mean-square value found.
 *                           [8] The name of a local variable in which
 *                               to record the number of points found.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_subset_stats)
{
  gcp::util::RegDescription ydesc;         /* The target register */
  double xa, xb;          /* The selection range */
  MonitorPlot *plot;      /* The constraining plot */
  MonitorRegStats stats;  /* The container of the requested statistics */

  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;

  /*
   * Get the target register specification.
   */
  if(parse_reg(interp, tm, argv[1], inputStatMode(plot), 1, &ydesc) == TCL_ERROR)
    return TCL_ERROR;

  /*
   * Get the limiting values of the selection register 
   */
  if(Tcl_GetDouble(interp, argv[2], &xa) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &xb) == TCL_ERROR)
    return TCL_ERROR;
  
  // Get the requested statistics.

  if(mpRegStats(plot, xa, xb, ydesc, &stats)) {
    Tcl_AppendResult(interp, "Bad scalar Y-axis register: ", argv[1], NULL);
    return TCL_ERROR;
  };
  
  // Assign the values to the named Tcl variables.

  sprintf(tm->buffer, "%.*g", DBL_DIG, stats.min);
  if(!Tcl_SetVar(tm->interp, argv[4], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, stats.max);
  if(!Tcl_SetVar(tm->interp, argv[5], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, stats.mean);
  if(!Tcl_SetVar(tm->interp, argv[6], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, stats.rms);
  if(!Tcl_SetVar(tm->interp, argv[7], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%ld", stats.npoint);
  if(!Tcl_SetVar(tm->interp, argv[8], tm->buffer, 0))
    return TCL_ERROR;
  sprintf(tm->buffer, "%.*g", DBL_DIG, stats.nsig);
  if(!Tcl_SetVar(tm->interp, argv[9], tm->buffer, 0))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Make a hardcopy version of a given plot.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] The identification tag of a plot to
 *                               be recorded.
 *                           [1] The PGPLOT specification of the
 *                               target hardcopy device.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_plot_hardcopy)
{
  MonitorPlot *plot;      /* The plot to be rendered */
  /*
   * Locate the plot whose tag is named in the first argument.
   */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
  /*
   * Render the plot.
   */
  if(mp_hardcopy(plot, argv[1])) {
    Tcl_AppendResult(interp, "Unable to make hardcopy to device: ", argv[1],
		     NULL);
    return TCL_ERROR;
  };
  return TCL_OK;
}

/*.......................................................................
 * Create a container that contains the name of the Tcl variable that is
 * used to communicate the value of a text field to Tcl.
 *
 * Input:
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  var             char *   The name of the Tcl global variable in which
 *                           to render field values.
 *  warn            char *   The name of the global boolean Tcl variable
 *                           through which to communicate the status of
 *                           the field value. This will normally be
 *                           given the value 0, but if the field value
 *                           falls outside a previously specified
 *                           range, then it is set to 1. Note that
 *                           this warning variable is only updated
 *                           when its value needs to be changed.
 *  page            char *   The name of the global boolean Tcl variable
 *                           through which to communicate the pager status of
 *                           the field value. This will normally be
 *                           given the value 0, but if the field value
 *                           falls outside a previously specified
 *                           range for long enough, then it is set to 1. 
 * Output:
 *  return       TmField *   The encapsulated field variable, or NULL on
 *                           error. On error, an error message will have been
 *                           placed in tm->interp->result.
 */
static TmField *new_TmField(TclMonitor *tm, char *var, char *warn, char *page)
{
  TmField *tmf;   /* The callback container to be returned */
  /*
   * Check arguments.
   */
  if(!var || !warn || !page) {
    Tcl_AppendResult(tm->interp, "new_TmField: Missing variable name.", NULL);
    return NULL;
  };
  if(strlen(var) > TMF_VAR_LEN || strlen(warn) > TMF_VAR_LEN || strlen(page) > TMF_VAR_LEN) {
    Tcl_AppendResult(tm->interp, "new_TmField: Variable name too long.", NULL);
    return NULL;
  };
  /*
   * Allocate the container.
   */
  tmf = (TmField* )new_FreeListNode(NULL, tm->tmf_mem);
  if(!tmf) {
    Tcl_AppendResult(tm->interp, "new_TmField: Insufficient memory.", NULL);
    return NULL;
  };
  /*
   * Before attempting any operation that might fail, initialize the container
   * at least up to the point at which it can safely be passed to
   * del_TmField().
   */
  tmf->tm = tm;
  tmf->var[0] = '\0';
  tmf->warn[0] = '\0';
  tmf->activate_pager[0] = '\0';
  tmf->status = TmField::TMF_UNKNOWN;

  /*
   * Record the variable names.
   */
  strcpy(tmf->var, var);
  strcpy(tmf->warn, warn);
  strcpy(tmf->activate_pager, page);
  return tmf;
}

/*.......................................................................
 * Delete a TmField container that was previously allocated by
 * new_TmField(). This function is designed to be useable as a
 * MP_DEL_FN() destructor, so (void *) is used instead of (TmField *).
 *
 * Input:
 *  user_data void *   The (TmField *) object to be deleted.
 * Output:
 *  return    void *   The deleted container (always NULL).
 */
static void *del_TmField(void *user_data)
{
  TmField *tmf = (TmField* )user_data;
  if(tmf) {
    tmf = (TmField* )del_FreeListNode("del_TmField", tmf->tm->tmf_mem, tmf);
  };
  return NULL;
}

/*.......................................................................
 * Add a text output field to a given page.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The page to add the field to.
 * Output:
 *  return           int     TCL_OK    - The result string will contain
 *                                       the identifier tag of the new
 *                                       field, or 0 if the creation
 *                                       failed.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_add_field)
{
  MonitorPage *page;    /* The page to add the field to */
  MonitorField *field;  /* The newly added field */
/*
 * Locate the page whose tag is named in the first argument.
 */
  page = tm_find_MonitorPage(tm, argv[0]);
  if(!page)
    return TCL_ERROR;
/*
 * Install the new field.
 */
  field = add_MonitorField(page);
  if(!field) {
    Tcl_AppendResult(interp, "Failed to add field.", NULL);
    return TCL_ERROR;
  };
/*
 * Record the identifier tag of the new field for return.
 */
  sprintf(tm->buffer, "%u", tag_of_MonitorField(field));
  Tcl_AppendResult(interp, tm->buffer, NULL);
  return TCL_OK;
}

/*.......................................................................
 * Reconfigure a given field.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The parent page of the field.
 *                           [1] - The field to be configured.
 *                           [2] - The register element to be displayed.
 *                           [3] - The output format, from:
 *                                  fixed_point -  Fixed point (%f).
 *                                  scientific  -  Scientific notation (%e).
 *                                  floating    -  Floating point (%g).
 *                                  sexagesimal -  Sexagesimal hours, minutes
 *                                                 and seconds.
 *                                  integer     -  Base 10 integer.
 *                                  hex         -  Base 16 integer.
 *                                  octal       -  Base 8 integer.
 *                                  binary      -  Base 2 integer.
 *                                  string      -  An ASCII string extracted
 *                                                 from one or more register
 *                                                 elements.
 *                                  date        -  A date and time from a
 *                                                 mjd/time register-element
 *                                                 pair.
 *                                  bit         -  The value of a single
 *                                                 bit of a register value
 *                                                 that has been cast to
 *                                                 unsigned long.
 *                                  enum        -  An enumeration in which
 *                                                 names are assigned to
 *                                                 integral values via the
 *                                                 names[] argument below.
 *                           [4] - The printf formatting flags to use, from
 *                                 [+- 0#].
 *                           [5] - The printf minimum field width attribute.
 *                           [6] - The printf precision attribute. For
 *                                 sexagesimal this is the number of
 *                                 decimal places in the seconds field.
 *  misc                     [7] - Format specific width attribute. Currently
 *                                 this is only used to set the minimum width
 *                                 of the sexagesimal hours field.
 *  names                    [8] - A space or comma-separated list of names
 *                                 to enumerate the values of bit and enum
 *                                 formats. Send an empty string for other
 *                                 formats.
 *  warn                     [9] - To have a warning border drawn for
 *                                 values outside the range min..max,
 *                                 set this to "1". Otherwise set it to
 *                                 "0".
 *  min max              [10-11] - The expected range of values. This
 *                                 will be ignored if [9]=="0".
 *
 *  dopage                  [12] - To enable paging if this register goes out of
 *                                 range, set this to "1". Otherwise set it to
 *                                 "0".
 *  min max                 [13] - The number of frames an out of range value 
 *                                 must persist before the pager is activated.
 *                                 Ignored if [12]==0
 *
 * Output:
 *  return           int     TCL_OK    - The result string will contain
 *                                       the identifier tag of the new
 *                                       field, or 0 if the creation
 *                                       failed.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_configure_field)
{
  MonitorPage *page;    /* The page to add the field to */
  MonitorField *field;  /* The field to be configured */
  MonitorFormat fmt;    /* The output format enumerator */
  char *regname;        /* The name of the register to be displayed */
  char *flags;          /* The printf flags to use */
  int width;            /* The minimum width of the field */
  int precision;        /* The output precision of float formats */
  int misc;             /* The format-specific miscellaneous width field */
  char *names;          /* The list of enumeration names */
  int warn;             /* True to enable out-of-range warnings */
  double vmin,vmax;     /* The allowed range of field-values */
  int dopage;           /* True to enable out-of-range paging */
  int nframe;           /* Number of frames after which to page */
/*
 * Locate the page whose tag is named in the first argument.
 */
  page = tm_find_MonitorPage(tm, argv[0]);
  if(!page)
    return TCL_ERROR;
/*
 * Find the field to be deleted.
 */
  field = tm_find_MonitorField(tm, page, argv[1]);
  if(!field)
    return TCL_ERROR;
/*
 * Get the register specification.
 */
  regname = argv[2];

/*
 * Decode the format keyword.
 */
  if(tm_parse_format(tm, argv[3], &fmt))
    return TCL_ERROR;
/*
 * Get the printf flags.
 */
  flags = argv[4];
/*
 * Get the width and precision
 */
  if(Tcl_GetInt(interp, argv[5], &width) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[6], &precision) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[7], &misc) == TCL_ERROR)
    return TCL_ERROR;
/*
 * Get the list of enumeration names for use with bit and enum formats.
 */
  names = argv[8];
/*
 * See whether out-of-range warnings are desired.
 */
  if(Tcl_GetInt(interp, argv[9], &warn) == TCL_ERROR)
    return TCL_ERROR;
/*
 * Get the range of values.
 */
  if(warn || dopage) {
    if(Tcl_GetDouble(interp, argv[10],  &vmin) == TCL_ERROR ||
       Tcl_GetDouble(interp, argv[11], &vmax) == TCL_ERROR)
      return TCL_ERROR;
  } else {
    vmin = vmax = 0.0;
  };
/*
 * See whether paging is desired.
 */
  if(Tcl_GetInt(interp, argv[12], &dopage) == TCL_ERROR)
    return TCL_ERROR;
/*
 * Get the frame threshold for paging
 */
  if(dopage) {
    if(Tcl_GetInt(interp, argv[13], &nframe) == TCL_ERROR)
      return TCL_ERROR;
  } else {
    nframe = 1;
  }

/*
 * Configure the field.
 */
  if(config_MonitorField(field, regname, fmt, flags, width, precision, misc,
			 names, warn, vmin, vmax, dopage, nframe))
    return TCL_ERROR;

  return TCL_OK;
}

/*.......................................................................
 * The following function is called by monitor_viewer whenever the value
 * of a register field is updated.
 */
static MP_FIELD_FN(tm_field_dispatcher)
{
  TmField *tmf = (TmField* )user_data;
  Tcl_Interp *interp = tmf->tm->interp;
/*
 * Get the existing value of this field.
 */
  char *old_value = (char* )Tcl_GetVar(interp, tmf->var, TCL_GLOBAL_ONLY);
/*
 * If the value of this field has changed, update it.
 */
  if(!old_value || strcmp(value, old_value) != 0)
    Tcl_SetVar(tmf->tm->interp, tmf->var, value, TCL_GLOBAL_ONLY);
  if(tmf->warn[0] && (tmf->status==TmField::TMF_UNKNOWN ||
		      (tmf->status==TmField::TMF_WAS_OK ? warn : !warn))) {
    Tcl_SetVar(tmf->tm->interp, tmf->warn, (char* )(warn ? "1":"0"), TCL_GLOBAL_ONLY);
  }
  /*
   * Write to the register's pager variable if we are activating the pager, 
   * or reseting the page status
   */

  if(tmf->activate_pager[0] && (dopage || reset)) {
    Tcl_SetVar(tmf->tm->interp, tmf->activate_pager, (char* )(dopage ? "1":"0"), TCL_GLOBAL_ONLY);
  }
  /*
   * And toggle the status of the register
   */
  if(tmf->warn[0] && (tmf->status==TmField::TMF_UNKNOWN ||
		      (tmf->status==TmField::TMF_WAS_OK ? warn : !warn))) {
    tmf->status = warn ? TmField::TMF_WAS_BAD : TmField::TMF_WAS_OK;
  }
}

/*.......................................................................
 * Change the names of the Tcl variable that is to receive updates of
 * the value and flag status of a given register field.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The page to add the field to.
 *                           [1] - The field to register the variable to.
 *                           [2] - The name of a global Tcl variable to copy
 *                                 the value of the field register into.
 *                           [3] - The name of a global Tcl variable to
 *                                 set to 0 or 1 depending on whether
 *                                 the field value is ok or whether a
 *                                 out-of-range warning should be
 *                                 displayed.
 *                           [4] - The name of a global Tcl variable to
 *                                 set to 0 or 1 depending on whether
 *                                 the pager should be activated
 * Output:
 *  return           int     TCL_OK - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_field_variables)
{
  MonitorPage *page;    /* The target page */
  MonitorField *field;  /* The target field */
  TmField *tmf;         /* The container that records the variable names */
/*
 * Locate the page whose tag is named in the first argument.
 */
  page = tm_find_MonitorPage(tm, argv[0]);
  if(!page)
    return TCL_ERROR;
/*
 * Find the field to be configured.
 */
  field = tm_find_MonitorField(tm, page, argv[1]);
  if(!field)
    return TCL_ERROR;
/*
 * Encapsulate copies of the TCL variable names.
 */
  tmf = new_TmField(tm, argv[2], argv[3], argv[4]);
  if(!tmf)
    return TCL_ERROR;
/*
 * Register the above container along with the field callback.
 */
  if(mf_callback_fn(field, tm_field_dispatcher, tmf, del_TmField))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Discard buffered data.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments (none).
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_clr_buffer)
{
  if(clr_MonitorBuff(tm->view)) {
    Tcl_AppendResult(interp, "Unable to clear buffer", NULL);
    return TCL_ERROR;
  };
  return TCL_OK;
}

/*.......................................................................
 * Register a script to be called whenever the register map changes.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] The callback script to register.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_regmap_callback)
{
/*
 * Discard any previous callback.
 */
  if(tm->regmap_script)
    free(tm->regmap_script);
  tm->regmap_script = NULL;
/*
 * Allocate a copy of the callback script, if one has been provided.
 */
  if(*argv[0] != '\0') {
    tm->regmap_script = (char* )malloc(strlen(argv[0])+1);
    if(!tm->regmap_script) {
      Tcl_AppendResult(interp, "Failed to allocate regmap callback.\n", NULL);
      return TCL_ERROR;
    };
    strcpy(tm->regmap_script, argv[0]);
  };
  return TCL_OK;
}

/*.......................................................................
 * After a change to the application's register map this function is
 * called to inform the Tcl layer via a previously registered callback.
 */
static void call_regmap_callback(TclMonitor *tm)
{
  // Is there a script to be evaluated?
  
  if(tm->regmap_script) {
    
    // Stop the Tcl event loop from listening for the arrival of new
    // data while the callback is active.

    tm_suspend_TclFile(tm);
    
    // Invoke the callback.

    if(Tcl_VarEval(tm->interp, tm->regmap_script, NULL)== TCL_ERROR)
      lprintf(stderr, "%s\n", tm->interp->result);
    
    // Have the Tcl event loop resume listening for the arrival of new
    // data.

    tm_resume_TclFile(tm);
  };
}

/*.......................................................................
 * Register a script to be called whenever the end of a monitor stream
 * is reached.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] The callback script to register.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_eos_callback)
{
/*
 * Discard any previous callback.
 */
  if(tm->eos_script)
    free(tm->eos_script);
  tm->eos_script = NULL;
/*
 * Allocate a copy of the callback script, if one has been provided.
 */
  if(*argv[0] != '\0') {
    tm->eos_script = (char* )malloc(strlen(argv[0])+1);
    if(!tm->eos_script) {
      Tcl_AppendResult(interp, "Failed to allocate end-of-stream callback.\n",
		       NULL);
      return TCL_ERROR;
    };
    strcpy(tm->eos_script, argv[0]);
  };
  return TCL_OK;
}

/*.......................................................................
 * After a change to the application's register map this function is
 * called to inform the Tcl layer via a previously registered callback.
 */
static void call_eos_callback(TclMonitor *tm)
{
  // If there is a script to be evaluated, this means that we expected
  // the stream to end (reading from a file) and want to now connect
  // to another stream

  if(tm->eos_script) {
    
    // Invoke the callback.

    if(Tcl_VarEval(tm->interp, tm->eos_script, NULL)== TCL_ERROR)
      lprintf(stderr, "%s\n", tm->interp->result);

    // Else this is an error.  Enable the reconnect timer if auto
    // reconnect is turned on

  } else {
    enableRetryTimer(tm, true);
  }
}

/*.......................................................................
 * Register a script to be called whenever it is time to reconnect
 */
static SERVICE_FN(tm_reconnect_callback)
{
  
  // Discard any previous callback.

  if(tm->reconnect_script)
    free(tm->reconnect_script);
  tm->reconnect_script = NULL;
  
  // Allocate a copy of the callback script, if one has been provided.

  if(*argv[0] != '\0') {
    tm->reconnect_script = (char* )malloc(strlen(argv[0])+1);
    if(!tm->reconnect_script) {
      Tcl_AppendResult(interp, "Failed to allocate end-of-stream callback.\n",
		       NULL);
      return TCL_ERROR;
    };
    strcpy(tm->reconnect_script, argv[0]);
  };
  return TCL_OK;
}

/**.......................................................................
 * When it is time to reconnect, this function is called to inform the
 * Tcl layer via a previously registered callback.
 */
static void call_reconnect_callback(TclMonitor *tm)
{
  // Is there a script to be evaluated?

  if(tm->reconnect_script) {
    
    // Invoke the callback.

    if(Tcl_VarEval(tm->interp, tm->reconnect_script, NULL)== TCL_ERROR)
      lprintf(stderr, "%s\n", tm->interp->result);
  };
}

/*.......................................................................
 * Translate from a format name to a format enumerator.
 *
 * Input:
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  name            char *   The format name to be decoded.
 * Input/Output:
 *  fmt    MonitorFormat *   On output the decoded format enumerator
 *                           will be assigned to *fmt.
 * Output:
 *  return           int     TCL_OK    -  Success.
 *                           TCL_ERROR -  The format name was not
 *                                        recognized.
 */
static int tm_parse_format(TclMonitor *tm, char *name, MonitorFormat *fmt)
{
  int i;
/*
 * List the correspondences between format names and enumerators.
 */
  struct {
    char *name;        /* The textual name of the option */
    MonitorFormat fmt; /* The corresponding format enumerator */
  } formats[] = {
    {"fixed_point",   MF_FIXED_POINT},
    {"scientific",    MF_SCIENTIFIC},
    {"floating",      MF_FLOATING},
    {"sexagesimal",   MF_SEXAGESIMAL},
    {"integer",       MF_INTEGER},
    {"hex",           MF_HEX},
    {"octal",         MF_OCTAL},
    {"binary",        MF_BINARY},
    {"string",        MF_STRING},
    {"date",          MF_DATE},
    {"bit",           MF_BIT},
    {"enum",          MF_ENUM},
    {"bool",          MF_BOOL},
    {"complex_fixed", MF_COMPLEX_FIXED},
  };
  const int nfmt = sizeof(formats)/sizeof(formats[0]);
  
  // Decode the format keyword.

  for(i=0; i<nfmt; i++) {
    if(strcmp(formats[i].name, name) == 0) {
      *fmt = formats[i].fmt;
      return TCL_OK;
    };
  };
  Tcl_AppendResult(tm->interp, "Unknown format: ", name, ".", NULL);
  return TCL_ERROR;
}

/*.......................................................................
 * Reconfigure the characteristics of a given plot.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the plot to
 *                                 modify.
 *                           [1] - The new title.
 *                           [2] - The leftmost x-axis limit.
 *                           [3] - The righmost x-axis limit.
 *                           [4] - The marker size (int >= 1).
 *                           [5] - Boolean: true to join neighboring
 *                                 points with lines.
 *                           [6] The scrolling mode, from:
 *                                disabled - Disable scrolling.
 *                                maximum  - Scroll to keep the maximum
 *                                           X value visible.
 *                                minimum  - Scroll to keep the mininum
 *                                           X value visible.
 *                           [7] The extra amount to scroll past the min
 *                               or max to reduce the frequency of scrolling.
 *                               This is specified as a percentage of the
 *                               X-axis length.
 *                           [8] A scalar register specification.
 *                           [9] The X-axis label.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_configure_plot)
{
  MonitorPlot *plot;   /* The plot to be modified */
  double xleft, xright;/* The new X-axis limits */
  int marker_size;     /* The new maker size */
  int do_join;         /* True to join neighboring data-points */
  MpScrollMode mode;   /* The new scroll mode */
  double margin;       /* The extra amount to scroll */
  int i;
/*
 * List the correspondences between scrolling mode names and
 * enumerators.
 */
  struct {
    char *name;        /* The textual name of the option */
    MpScrollMode mode; /* The corresponding scrolling mode */
  } scroll_modes[] = {
    {"disabled",  SCROLL_DISABLED},
    {"maximum",   SCROLL_MAXIMUM},
    {"minimum",   SCROLL_MINIMUM},
  };
  const int nmode = sizeof(scroll_modes)/sizeof(scroll_modes[0]);
/*
 * Locate the plot whose tag is named in the first argument.
 */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
/*
 * Decode the x-axis limits.
 */
  if(Tcl_GetDouble(interp, argv[2], &xleft) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &xright) == TCL_ERROR)
    return TCL_ERROR;
  
  // Get the new marker size.

  if(Tcl_GetInt(interp, argv[4], &marker_size) == TCL_ERROR)
    return TCL_ERROR;
  
  // Determine whether lines should be drawn between sequential points
  // of the plot.

  if(Tcl_GetBoolean(interp, argv[5], &do_join) == TCL_ERROR)
    return TCL_ERROR;
  
  // Look up the scrolling mode.

  mode = SCROLL_DISABLED;
  for(i=0; i<nmode; i++) {
    if(strcmp(scroll_modes[i].name, argv[6]) == 0) {
      mode = scroll_modes[i].mode;
      break;
    };
  };
  if(i>=nmode) {
    Tcl_AppendResult(interp, "Unknown scrolling mode: ", argv[6], ".", NULL);
    return TCL_ERROR;
  };
  
  // Get the scroll margin.

  if(Tcl_GetDouble(interp, argv[7], &margin) == TCL_ERROR)
    return TCL_ERROR;
  
  // Get the new npt

  int npt;
  if(Tcl_GetInt(interp, argv[11], &npt) == TCL_ERROR)
    return TCL_ERROR;

  // Get the new dx

  double dx;
  if(Tcl_GetDouble(interp, argv[12], &dx) == TCL_ERROR)
    return TCL_ERROR;

  // Get the new axis type

  int linAxis;
  if(Tcl_GetInt(interp, argv[13], &linAxis) == TCL_ERROR)
    return TCL_ERROR;

  // Install the new plot characteristics.

  if(config_MonitorPlot(plot, argv[1], xleft, xright, marker_size,
			do_join, mode, margin, argv[8], argv[9], 
			argv[10], npt, dx, linAxis==1)) {
    return TCL_ERROR;
  }

  return TCL_OK;
}

/**.......................................................................
 * Identify peaks on a power spectrum plot
 */
static SERVICE_FN(tm_pkident)
{
  MonitorPlot* plot=0;
  MonitorGraph* graph=0;
  int npk;
  double xleft, xright;/* The new X-axis limits */

/*
 * Locate the plot whose tag is named in the first argument.
 */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
/*
 * Locate the child graph of the plot.
 */
  graph = tm_find_MonitorGraph(tm, plot, argv[1]);
  if(!graph)
    return TCL_ERROR;

/*
 * Decode the number of peaks
 */
  if(Tcl_GetInt(interp, argv[2], &npk) == TCL_ERROR)
    return TCL_ERROR;

/*
 * Decode the x-axis limits.
 */
  if(Tcl_GetDouble(interp, argv[3], &xleft) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[4], &xright) == TCL_ERROR)
    return TCL_ERROR;
  
/*
 * Decode the range specifier
 */
  int full=0;
  if(Tcl_GetInt(interp, argv[5], &full) == TCL_ERROR)
    return TCL_ERROR;

  // Install the new plot characteristics.

  if(powSpecPkIdent(graph, npk, xleft, xright, full)) {
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*.......................................................................
 * Reconfigure the characteristics of a given graph.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The identification tag of the parent plot.
 *                           [1] - The identification tag of the graph to
 *                                 be configured.
 *                           [2] - The lower y-axis limit.
 *                           [3] - The upper y-axis limit.
 *                           [4] - The y-axis label.
 *                           [5] - The list of registers to be plotted.
 *                           [6] - A list of bits to be plotted, or {} to
 *                                 plot the composite values of the register.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_configure_graph)
{
  MonitorPlot *plot;    /* The parent plot of the graph */
  MonitorGraph *graph;  /* The graph to be configured */
  InputStream *stream;  /* The input stream used to parse bit numbers */
  double ybot, ytop;    /* The Y-axis limits */
  unsigned bits=0;      /* The bitmask of bits to be plotted */
  int waserr = 0;       /* True after a parsing error */
/*
 * Locate the plot whose tag is named in the first argument.
 */
  plot = tm_find_MonitorPlot(tm, argv[0]);
  if(!plot)
    return TCL_ERROR;
/*
 * Locate the child graph of the plot.
 */
  graph = tm_find_MonitorGraph(tm, plot, argv[1]);
  if(!graph)
    return TCL_ERROR;
/*
 * Decode the range limits.
 */
  if(Tcl_GetDouble(interp, argv[2], &ybot) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &ytop) == TCL_ERROR)
    return TCL_ERROR;

  // Decode the tracking variable

  int track=0;
  if(Tcl_GetInt(interp, argv[7], &track) == TCL_ERROR)
    return TCL_ERROR;

  // Decode the averaging variable

  int av=0;
  if(Tcl_GetInt(interp, argv[8], &av) == TCL_ERROR)
    return TCL_ERROR;

  // Decode the axis variable

  int axis=0;
  if(Tcl_GetInt(interp, argv[9], &axis) == TCL_ERROR)
    return TCL_ERROR;
  
  // Decode the axis variable

  int apodType=0;
  if(Tcl_GetInt(interp, argv[10], &apodType) == TCL_ERROR)
    return TCL_ERROR;
  
  // Form the bitwise union of the list of bits.

  stream = tm->input;
  if(open_StringInputStream(stream, 0, argv[6]))
    return TCL_ERROR;

  waserr = input_skip_space(stream, 1, 0);

  while(!waserr && isdigit(stream->nextc)) {
    unsigned int bit;
    if(input_uint(stream, 0, 0, &bit) || bit < 0 || bit > 31) {
      lprintf(stderr, "Bit number outside the available range of 0..31.\n");
      waserr = 1;
    };

    bits |= 1U<<bit;

    if(input_skip_space(stream, 1, 0))
      waserr = 1;

    while(!waserr && stream->nextc==',') {
      if(input_skip_space(stream, 1, 1))
	waserr = 1;
    };
  };

  close_InputStream(stream);

  if(waserr)
    return TCL_ERROR;
  
  // Install the new graph characteristics.

  try {

    if(config_MonitorGraph(graph, ybot, ytop, argv[4], argv[5], bits, 
			   track==1, av==1, axis==1, apodType))
      return TCL_ERROR;

  } catch(gcp::util::Exception& err) {
    lprintf(stderr, "%s", err.what());
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*.......................................................................
 * Resize the monitor buffer.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The new size (an unsigned integer).
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_resize_buffer)
{
  char *endp;   /* The next unprocessed character in argv[0] */
  int size;    /* The new buffer size */
/*
 * Get the new buffer size.
 */
  size = strtol(argv[0], &endp, 0);
  if(endp==argv[0] || *endp != '\0') {
    Tcl_AppendResult(interp, "The new buffer size must be an integer.\n", NULL);
    return TCL_ERROR;
  };
/*
 * Attempt to resize the buffer.
 */
  if(mp_resize_buffer(tm->view, size))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Convert a Modified Julian Date to a date and time string.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The Modified Julian Date to convert.
 * Output:
 *  return           int     TCL_OK    - The date and time will be
 *                                       returned in the result string.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_mjd_to_date)
{
  double mjd;   /* The Modified Julian Date to be converted */
/*
 * Get the Modified Julian Date.
 */
  if(Tcl_GetDouble(interp, argv[0], &mjd) == TCL_ERROR)
    return TCL_ERROR;
/*
 * Compose the date and time in tm->buffer[].
 */
  if(open_StringOutputStream(tm->output, 0, tm->buffer, sizeof(tm->buffer)) ||
     output_utc(tm->output, "-", 0, 3, mjd)) {
    close_OutputStream(tm->output);
    return TCL_ERROR;
  };
  close_OutputStream(tm->output);
/*
 * Copy the date into the result string.
 */
  Tcl_AppendResult(tm->interp, tm->buffer, NULL);
  return TCL_OK;
}

/*.......................................................................
 * Convert a Gregorian date and time string to Modified Julian Date.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The Gregorian date, in the format
 *                                 expected by input_utc().
 * Output:
 *  return           int     TCL_OK    - The Modified Julian Date will be
 *                                       returned in the result string.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_date_to_mjd)
{
  double mjd;   /* The Modified Julian Date to be returned */
/*
 * Perform the conversion.
 */
  if(tm_parse_date_and_time(tm, argv[0], &mjd) == TCL_ERROR)
    return TCL_ERROR;
/*
 * Install the date in the result string.
 */
  sprintf(tm->buffer, "%.*g", DBL_DIG, mjd);
  Tcl_AppendResult(tm->interp, tm->buffer, NULL);
  return TCL_OK;
}

/*.......................................................................
 * Convert a decimal hours to a sexagesimal time-interval string.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The decimal hours to convert.
 * Output:
 *  return           int     TCL_OK    - The time will be
 *                                       returned in the result string.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_hours_to_interval)
{
  double hours;   /* The decimal hours to be converted */
/*
 * Get the decimal hours.
 */
  if(Tcl_GetDouble(interp, argv[0], &hours) == TCL_ERROR)
    return TCL_ERROR;
/*
 * Compose the time in tm->buffer[].
 */
  if(open_StringOutputStream(tm->output, 0, tm->buffer, sizeof(tm->buffer)) ||
     output_sexagesimal(tm->output, "-", 0, 0, 3, hours)) {
    close_OutputStream(tm->output);
    return TCL_ERROR;
  };
  close_OutputStream(tm->output);
/*
 * Copy the time into the result string.
 */
  Tcl_AppendResult(tm->interp, tm->buffer, NULL);
  return TCL_OK;
}

/*.......................................................................
 * Convert a sexagesimal time-interval string to decimal hours.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - The interval, in the format
 *                                 expected by input_sexagesimal().
 * Output:
 *  return           int     TCL_OK    - The decimal hours will be
 *                                       returned in the result string.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_interval_to_hours)
{
  double hours;   /* The decimal hours to be returned */
/*
 * Perform the conversion.
 */
  if(open_StringInputStream(tm->input, 0, argv[0]) ||
     input_sexagesimal(tm->input, 1, &hours)) {
    close_InputStream(tm->input);
    return TCL_ERROR;
  };
  close_InputStream(tm->input);
/*
 * Install the time in the result string.
 */
  sprintf(tm->buffer, "%.*g", DBL_DIG, hours);
  Tcl_AppendResult(tm->interp, tm->buffer, NULL);
  return TCL_OK;
}
/*.......................................................................
 * Change the Y-axis limits of a given graph.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - x1
 *                           [1] - x2
 *                           [2] - y1
 *                           [3] - y2
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_setrange)
{
  double xa, xb, ya, yb;  /* The new zoom limits */
/*
 * Decode the range limits.
 */
  if(Tcl_GetDouble(interp, argv[0], &xa) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[1], &ya) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &xb) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &yb) == TCL_ERROR)
    return TCL_ERROR;
  /*
   * Set the new limits in the image descriptor.
   */
  set_MonitorImage_range(tm->view, xa,xb,ya,yb);
  return TCL_OK;
}

/*.......................................................................
 * Compute statistics on an image.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - x1
 *                           [1] - x2
 *                           [2] - y1
 *                           [3] - y2
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_stat)
{
  double xa, xb, ya, yb, tmp; /* The limits over which to compute statistics */
  double min,max,mean,rms;
  int npoint;
/*
 * Decode the range limits.
 */
  if(Tcl_GetDouble(interp, argv[0], &xa) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[1], &ya) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &xb) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &yb) == TCL_ERROR)
    return TCL_ERROR;

  if(xa > xb) {tmp=xb;xb = xa;xa = tmp;}
  if(ya > yb) {tmp=yb;yb = ya;ya = tmp;}

  /*
   * Get statistics on the image.
   */
  get_MonitorImage_stats(tm->view, xa, xb, ya, yb, &min, &max,
			 &mean, &rms, &npoint);
  /*
   * Compose the return string.
   */
  sprintf(tm->buffer, "%f %f %f %f %ld", min, max, mean, rms, npoint);
  Tcl_AppendResult(interp, tm->buffer, NULL);
  return TCL_OK;
}
/*.......................................................................
 * Redraw an image.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - x1
 *                           [1] - x2
 *                           [2] - y1
 *                           [3] - y2
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_redraw)
{
  /*
   * Redraw the monitor image.  But only if we have received data from the
   * control program.
   */
  draw_MonitorImage_data(tm->view);

  return TCL_OK;
}

/*.......................................................................
 * Change the X-direction
 */
static SERVICE_FN(tm_im_ximdir)
{
  int dir;  /* The cursor position */
  
  // Decode the range limits.

  if(Tcl_GetInt(interp, argv[0], &dir) == TCL_ERROR)
    return TCL_ERROR;
  
  // Redraw the monitor image.

  change_MonitorImage_ximdir(tm->view, dir);

  return TCL_OK;
}

/*.......................................................................
 * Change the Y-direction
 */
static SERVICE_FN(tm_im_yimdir)
{
  int dir;  /* The cursor position */
  
  // Decode the range limits.

  if(Tcl_GetInt(interp, argv[0], &dir) == TCL_ERROR)
    return TCL_ERROR;
  
  // Redraw the monitor image.

  change_MonitorImage_yimdir(tm->view, dir);

  return TCL_OK;
}

/*.......................................................................
 * Change the Y-axis limits of a given graph.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - x1
 *                           [1] - y1
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_contrast)
{
  double xa, ya;  /* The cursor position */
/*
 * Decode the range limits.
 */
  if(Tcl_GetDouble(interp, argv[0], &xa) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[1], &ya) == TCL_ERROR)
    return TCL_ERROR;
/*
 * Redraw the monitor image.
 */
  fid_MonitorImage_contrast(tm->view, xa, ya);
  return TCL_OK;
}

/*.......................................................................
 * Install a new image colormap.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - the name of the colormap to install.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_colormap)
{
/*
 * Redraw the monitor image.
 */
  if(install_MonitorImage_colormap(tm->view, argv[0]))
    return TCL_ERROR;
  return TCL_OK;
}

/*.......................................................................
 * Change the interval for bullseye display.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - the new grid interval.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_step)
{
  double interval;
/*
 * Decode the interval (arcseconds).
 */
  if(Tcl_GetDouble(interp, argv[0], &interval) == TCL_ERROR)
    return TCL_ERROR;

  set_MonitorImage_step(tm->view, interval);

  return TCL_OK;
}

/*.......................................................................
 * Change the viewport size.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - the new grid interval.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_fov)
{
  double fov;
/*
 * Decode the viewport width (arcminutes).
 */
  if(Tcl_GetDouble(interp, argv[0], &fov) == TCL_ERROR)
    return TCL_ERROR;

  set_MonitorImage_fov(tm->view, fov); 

  return TCL_OK;
}

/*.......................................................................
 * Change the aspect ratio
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - the new grid interval.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_aspect)
{
  double aspect;
/*
 * Decode the aspect ratio
 */
  if(Tcl_GetDouble(interp, argv[0], &aspect) == TCL_ERROR)
    return TCL_ERROR;

  set_MonitorImage_aspect(tm->view, aspect); 

  return TCL_OK;
}
/*.......................................................................
 * Compute the image centroid.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - the new grid interval.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_centroid)
{
  double xcntr,ycntr;
  char xstring[10],ystring[10];

  find_MonitorImage_centroid(tm->view, xcntr, ycntr, 0);

  sprintf(xstring,"%3.3f",xcntr);
  sprintf(ystring,"%3.3f",ycntr);
  /*
   * And return the new centroid.
   */
  Tcl_AppendResult(interp, xstring, " ", NULL);
  Tcl_AppendResult(interp, ystring, " ", NULL);

  return TCL_OK;
}
/*.......................................................................
 * Update whether we are drawing a grid or not.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - true if drawing a compass.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_toggle_grid)
{

  toggle_MonitorImage_grid(tm->view);

  return TCL_OK;
}

/*.......................................................................
 * Update whether we are drawing a bullseye or not.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - true if drawing a compass.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_toggle_bullseye)
{
  toggle_MonitorImage_bullseye(tm->view);

  return TCL_OK;
}

static SERVICE_FN(tm_im_toggle_crosshair)
{
  toggle_MonitorImage_crosshair(tm->view);

  return TCL_OK;
}

/*.......................................................................
 * Update whether we are drawing a compass or not.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - true if drawing a compass.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_toggle_compass)
{
  toggle_MonitorImage_compass(tm->view);

  return TCL_OK;
}
/*.......................................................................
 * Update the compass direction.
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 *  argv             char ** The array of arguments.
 *                           [0] - the new compass direction.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_compass)
{
  double angle;
/*
 * Decode the compass angle.
 */
  if(Tcl_GetDouble(interp, argv[0], &angle) == TCL_ERROR)
    return TCL_ERROR;

  set_MonitorImage_compass(tm->view, angle);

  return TCL_OK;
}

/*.......................................................................
 * Set the centroid of the image
 */
static SERVICE_FN(tm_im_set_centroid)
{
  int xpeak, ypeak;
  
  // Read the peak position

  if(Tcl_GetInt(interp, argv[0], &xpeak) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[1], &ypeak) == TCL_ERROR)
    return TCL_ERROR;

  set_MonitorImage_centroid(tm->view, xpeak, ypeak);

  return TCL_OK;
}

/*.......................................................................
 * Reset the image contrast
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_im_reset_contrast)
{
  reset_MonitorImage_contrast(tm->view);

  if(reset_MonitorImage_colormap(tm->view))
    return TCL_ERROR;

  return TCL_OK;
}

/*.......................................................................
 * Reset the counters for page-enabled registers
 *
 * Input:
 *  interp    Tcl_Interp *   The Tcl interpretter object.
 *  tm        TclMonitor *   The Tcl monitor-interface resource object.
 *  argc             int     The number of arguments in argv[]. This
 *                           is guaranteed to equal the number listed
 *                           for this function in the service_fns[]
 *                           array at the top of this file.
 * Output:
 *  return           int     TCL_OK    - Success.
 *                           TCL_ERROR - Error.
 */
static SERVICE_FN(tm_reset_counters)
{
  if(reset_MonitorField_counters(tm->view))
    return TCL_ERROR;

  return TCL_OK;
}
