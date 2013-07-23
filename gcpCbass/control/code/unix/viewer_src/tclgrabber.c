/**.......................................................................
 * This file implements the TCL grabber command that provides the
 * interface between TCL and the frame grabber code in
 * grabber.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <math.h>

#include <tcl.h>

#include "hash.h"
#include "astrom.h"
#include "lprintf.h"

#include "gcp/util/common/RegParser.h"
#include "gcp/util/common/Sort.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/PointingTelescopes.h"

#include "gcp/pgutil/common/ImageReader.h"
#include "gcp/pgutil/common/MultipleImageReader.h"

#include <string>
#include <vector>

using namespace gcp::control;
using namespace gcp::grabber;
using namespace gcp::util;
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

/*
 * Define the resource object of the interface.
 */
struct TclGrabber {
  Tcl_Interp *interp;    /* The tcl-interpretter */
  FreeList *tgc_mem;     /* Memory for TgCallback containers */
  HashMemory *hash_mem;  /* Memory from which to allocate hash tables */
  HashTable *services;   /* A symbol table of service functions */

  char *eos_script;      /* The Tcl code to evaluate on reaching the end of */
                         /*  the current stream. */
  int im_fd;             /* The image data-stream file descriptor */
  int im_send_in_progress;/* True when an image monitor-stream send is in 
			     progress */
#ifdef USE_TCLFILE
  Tcl_File im_tclfile;   /* A Tcl wrapper around im_fd */
#else
  int im_fd_registered;  /* True if a file handler has been created 
			    for im_fd */
#endif
  int im_fd_event_mask;  /* The current event mask associated with 'im_fd' */
  char buffer[256];      /* A work buffer for constructing result strings */
  InputStream *input;    /* An input stream to parse register names in */
  OutputStream *output;  /* An output stream to parse register names in */
  Tcl_DString stderr_output;/* A resizable string containing stderr output */

  gcp::grabber::MultipleImageReader* image_;

  struct {
    char* channel_var;
    char* flatfield_var;
    char* aspect_var;
    char* fov_var;
    char* collimation_var;
    char* combine_var;
    char* ximdir_var;
    char* yimdir_var;
    char* dkRotSense_var;
    char* xpeak_var;
    char* ypeak_var;
    char* grid_var;
    char* bull_var;
    char* comp_var;
    char* cross_var;
    char* box_var;
    char* cmap_var;
    char* chan0_var;
    char* chan1_var;
    char* chan2_var;
    char* chan3_var;
  } grabberVars;
};

static int is_empty_string(char *s);

/**.......................................................................
 * After a change to the application's register map this function is
 * called to inform the Tcl layer via a previously registered callback.
 */
static void call_eos_callback(TclGrabber *tg);


static TclGrabber *new_TclGrabber(Tcl_Interp *interp);
static TclGrabber *del_TclGrabber(TclGrabber *tg);
static void delete_TclGrabber(ClientData context);

#ifdef USE_CHAR_DECL
static int service_TclGrabber(ClientData context, Tcl_Interp *interp,
			      int argc, char *argv[]);
#else
static int service_TclGrabber(ClientData context, Tcl_Interp *interp,
			      int argc, const char *argv[]);
#endif

static void tg_im_file_handler(ClientData context, int mask);
static void tg_update_im_TclFile(TclGrabber *tg, int fd, int send_in_progress);
static void tg_suspend_im_TclFile(TclGrabber *tg);
static void tg_resume_im_TclFile(TclGrabber *tg);

static LOG_DISPATCHER(tg_log_dispatcher);

/*
 * Define an object to encapsulate a copy of a Tcl callback script
 * along with the resource object of the monitor interface.
 */
typedef struct {
  TclGrabber *tg;   /* The resource object of the monitor interface */
  char *script;     /* The callback script. */
} TgCallback;

static TgCallback *new_TgCallback(TclGrabber *tg, char *script);
static void *del_TgCallback(void *user_data);

/*
 * Create a container for describing service functions.
 */
#define SERVICE_FN(fn) int (fn)(Tcl_Interp *interp, TclGrabber *tg, \
				 int argc, char *argv[])
typedef struct {
  char *name;          /* The name of the service */
  SERVICE_FN(*fn);     /* The function that implements the service */
  unsigned narg;       /* The expected value of argc to be passed to fn() */
} ServiceFn;

/*
 * Provide prototypes for service functions.
 */
static SERVICE_FN(tg_imhost);
static SERVICE_FN(tg_im_disconnect);
static SERVICE_FN(tg_im_reconfigure);
static SERVICE_FN(tg_im_redraw);
static SERVICE_FN(tg_im_redraw2);
static SERVICE_FN(tg_im_setrange);
static SERVICE_FN(tg_im_setrange_full);
static SERVICE_FN(tg_im_stat);
static SERVICE_FN(tg_im_inc);
static SERVICE_FN(tg_im_exc);
static SERVICE_FN(tg_im_del_box);
static SERVICE_FN(tg_im_del_all_box);
static SERVICE_FN(tg_im_contrast);
static SERVICE_FN(tg_im_colormap);
static SERVICE_FN(tg_im_step);
static SERVICE_FN(tg_im_fov);
static SERVICE_FN(tg_im_aspect);
static SERVICE_FN(tg_im_docompass);
static SERVICE_FN(tg_im_rotationAngle);
static SERVICE_FN(tg_im_centroid);
static SERVICE_FN(tg_im_set_centroid);
static SERVICE_FN(tg_im_set_combine);
static SERVICE_FN(tg_im_set_ptel);
static SERVICE_FN(tg_im_set_flatfield);
static SERVICE_FN(tg_im_toggle_grid);
static SERVICE_FN(tg_im_toggle_bullseye);
static SERVICE_FN(tg_im_toggle_crosshair);
static SERVICE_FN(tg_im_toggle_boxes);
static SERVICE_FN(tg_im_toggle_compass);
static SERVICE_FN(tg_im_reset_contrast);
static SERVICE_FN(tg_im_ximdir);
static SERVICE_FN(tg_im_yimdir);
static SERVICE_FN(tg_im_select_channel);
static SERVICE_FN(tg_im_worldToSky);
static SERVICE_FN(tg_im_worldToPixel);
static SERVICE_FN(tg_open_image);
static SERVICE_FN(tg_vars);
static SERVICE_FN(tg_eos_callback);

/*
 * List top-level service functions.
 */
static ServiceFn service_fns[] = {
  
  /* Change the image input stream to take real-time data from the 
     control-program */

  {"imhost",              tg_imhost,            1},
  
  /* Change the input stream to take archived data from disk */

  {"im_disconnect",       tg_im_disconnect,     0},
  
  // Select the current image channel

  {"im_select_channel",   tg_im_select_channel, 1},

  //------------------------------------------------------------ 
  // These commands are per-channel, so the first argument will always
  // be the channel to which they apply
  //------------------------------------------------------------

  // Change the axis limits of a given image 

  {"im_setrange",         tg_im_setrange,       5},

  /* Change the axis limits of a given image */

  {"im_setrange_full",    tg_im_setrange_full,  1},
  
  /* Compute statistics on a given image */

  {"im_stat",             tg_im_stat,           5},

  {"im_inc",              tg_im_inc,            5},
  {"im_exc",              tg_im_exc,            5},
  {"im_del_box",          tg_im_del_box,        3},
  {"im_del_all_box",      tg_im_del_all_box,    1},
  
  // Reconfigure all dialog boxes for the current channel

  {"im_reconfigure",      tg_im_reconfigure,    2},

  /* Redraw an image */

  {"im_redraw",           tg_im_redraw,         0},

  /* Redraw an image */

  {"im_redraw2",          tg_im_redraw2,        0},
  
  /* Change the contrast and brightness of a given image */

  {"im_contrast",         tg_im_contrast,       3},
  
  /* Change the colormap of an image */

  {"im_colormap",         tg_im_colormap,       2},
  
  /* Change the step interval for the bullseye display */

  {"im_change_step",      tg_im_step,           2},
  
  /* Change the image viewport size. */

  {"im_change_fov",       tg_im_fov,            2},

  /* Change the image rotation angle. */

  {"im_change_rotAngle",  tg_im_rotationAngle,  2},

  /* Change the image aspect ratio. */

  {"im_change_aspect",    tg_im_aspect,         2},

  /* Change whether displaying the crosshair or not */

  {"im_toggle_crosshair", tg_im_toggle_crosshair,1},
  
  /* Change whether displaying boxes or not */

  {"im_toggle_boxes",     tg_im_toggle_boxes,    1},
  
  /* Change whether displaying the compass or not */

  {"im_toggle_compass",   tg_im_toggle_compass,  1},
  
  /* Compute the image centroid */

  {"im_centroid",         tg_im_centroid,        1},
  
  /* Set the image centroid */

  {"im_set_centroid",     tg_im_set_centroid,    3},

  // Set the number of images to combine

  {"im_set_combine",      tg_im_set_combine,     2},

  // Set the pointing telescope associated with this channel

  {"im_set_ptel",         tg_im_set_ptel,        2},

  // Set the flatfielding type

  {"im_set_flatfield",    tg_im_set_flatfield,   2},

  /* Toggle drawing the grid on the frame grabber display */

  {"im_toggle_grid",      tg_im_toggle_grid,     1},
  
  /* Toggle drawing the bullseye on the frame grabber display */

  {"im_toggle_bullseye",  tg_im_toggle_bullseye, 1},
  
  /* Reset the image contrast/brightness to the default values */

  {"im_reset_contrast",   tg_im_reset_contrast,  1},

  /* Reset the image contrast/brightness to the default values */

  {"im_ximdir",           tg_im_ximdir,          2},

  /* Reset the image contrast/brightness to the default values */

  {"im_yimdir",           tg_im_yimdir,          2},

  {"worldToSky",          tg_im_worldToSky,      3},

  {"worldToPixel",        tg_im_worldToPixel,    3},

  //------------------------------------------------------------
  // The rest of these commands apply globally
  //------------------------------------------------------------

  // Associate a pgplot widget with the frame grabber

  {"open_image",          tg_open_image,         1},

  {"im_vars",             tg_vars,               2},

  // Register a script to be called whenever an image monitor stream ends

  {"eos_callback",        tg_eos_callback,       1},
};

/**.......................................................................
 * This is the TCL grabber package-initialization function.
 *
 * Input:
 *  interp  Tcl_Interp *  The TCL interpretter.
 * Output:
 *  return         int    TCL_OK    - Success.
 *                        TCL_ERROR - Failure.           
 */
int TclGrabber_Init(Tcl_Interp *interp)
{
  TclGrabber *tg;   /* The resource object of the tcl_db() function */
  
  // Check arguments.

  if(!interp) {
    fprintf(stderr, "TclGrabber_Init: NULL interpreter.\n");
    return TCL_ERROR;
  };
  
  // Allocate a resource object for the service_TclGrabber() command.

  tg = new_TclGrabber(interp);

  if(!tg)
    return TCL_ERROR;
  
  // Create the TCL command that will control the grabber

  Tcl_CreateCommand(interp, "grabber", service_TclGrabber, (ClientData) tg,
		    delete_TclGrabber);
  return TCL_OK;
}

/**.......................................................................
 * Create the resource object of the Tcl grabber interface.
 *
 * Input:
 *  interp  Tcl_Interp *  The Tcl interpretter.
 * Output:
 *  return  TclGrabber *  The resource object, or NULL on error.
 */
static TclGrabber *new_TclGrabber(Tcl_Interp *interp)
{
  TclGrabber *tg;  /* The object to be returned */
  int i;
  
  // Check arguments.

  if(!interp) {
    fprintf(stderr, "new_TclGrabber: NULL interp argument.\n");
    return NULL;
  };
  
  // Allocate the container.

  tg = (TclGrabber *) malloc(sizeof(TclGrabber));
  if(!tg) {
    fprintf(stderr, "new_TclGrabber: Insufficient memory.\n");
    return NULL;
  };
  
  // Before attempting any operation that might fail, initialize the
  // container at least up to the point at which it can safely be
  // passed to del_TclGrabber().

  tg->interp     = interp;

  tg->tgc_mem    = NULL;
  tg->hash_mem   = NULL;
  tg->services   = NULL;
  tg->eos_script = NULL;
  
  tg->im_fd = -1;
  tg->im_send_in_progress = 0;

#if USE_TCLFILE
  tg->im_tclfile = NULL;
#else
  tg->im_fd_registered = 0;
#endif

  tg->im_fd_event_mask = 0;
  tg->input = NULL;
  tg->output = NULL;
  tg->image_ = 0;

  Tcl_DStringInit(&tg->stderr_output);

  // Create a free-list of TgCallback containers.

  tg->tgc_mem = new_FreeList("new_TclMonitor", sizeof(TgCallback), 20);
  if(!tg->tgc_mem)
    return del_TclGrabber(tg);

  // Create memory for allocating hash tables.

  tg->hash_mem = new_HashMemory(10, 20);
  if(!tg->hash_mem)
    return del_TclGrabber(tg);
  
  // Create and populate the symbol table of services.

  tg->services = new_HashTable(tg->hash_mem, 53, IGNORE_CASE, NULL, 0);
  if(!tg->services)
    return del_TclGrabber(tg);

  for(i=0; i < (int)(sizeof(service_fns)/sizeof(service_fns[0])); i++) {
    ServiceFn *srv = service_fns + i;
    if(new_HashSymbol(tg->services, srv->name, srv->narg,
		      (void (*)(void)) srv->fn, NULL, 0) == NULL)
      return del_TclGrabber(tg);
  };
  
  // Create unassigned input and output streams.

  tg->input = new_InputStream();
  if(!tg->input)
    return del_TclGrabber(tg);

  tg->output = new_OutputStream();
  if(!tg->output)
    return del_TclGrabber(tg);

  tg->image_ = new MultipleImageReader();

  if(!tg->image_)
    return del_TclGrabber(tg);

  // Initialize variables used to communicate with the TCL layer

  tg->grabberVars.channel_var     = NULL;
  tg->grabberVars.flatfield_var   = NULL;
  tg->grabberVars.aspect_var      = NULL;
  tg->grabberVars.fov_var         = NULL;
  tg->grabberVars.collimation_var = NULL;
  tg->grabberVars.combine_var     = NULL;
  tg->grabberVars.ximdir_var      = NULL;
  tg->grabberVars.yimdir_var      = NULL;
  tg->grabberVars.dkRotSense_var  = NULL;
  tg->grabberVars.xpeak_var       = NULL;
  tg->grabberVars.ypeak_var       = NULL;
  tg->grabberVars.grid_var        = NULL;
  tg->grabberVars.bull_var        = NULL;
  tg->grabberVars.comp_var        = NULL;
  tg->grabberVars.cross_var       = NULL;
  tg->grabberVars.box_var         = NULL;
  tg->grabberVars.cmap_var        = NULL;

  tg->grabberVars.chan0_var = NULL;
  tg->grabberVars.chan1_var = NULL;
  tg->grabberVars.chan2_var = NULL;
  tg->grabberVars.chan3_var = NULL;

  return tg;
}

/*.......................................................................
 * Delete the resource object of the Tcl grabber interface.
 */
static TclGrabber *del_TclGrabber(TclGrabber *tg)
{
  if(tg) {
    
    // Delete the hash-table of services.

    tg->services = del_HashTable(tg->services);
    
    // Having deleted all its client hash-tables, delete the
    // hash-table memory allocator.

    tg->hash_mem = del_HashMemory(tg->hash_mem, 1);
    
    // Delete the end-of-stream callback script.

    if(tg->eos_script)
      free(tg->eos_script);
    tg->eos_script = NULL;
    
    // Delete the Tcl file wrapper around tg->im_fd.

    tg_update_im_TclFile(tg, -1, 0);
    
    // Delete the callback containers. This is left until after
    // del_MonitorViewer(), so that all of the callback containers
    // will have been returned to the free-list.

    tg->tgc_mem = del_FreeList("del_TclGrabber", tg->tgc_mem, 1);
    
    // Delete the streams.

    tg->input = del_InputStream(tg->input);
    tg->output = del_OutputStream(tg->output);

    if(tg->image_) {
      delete tg->image_;
      tg->image_ = 0;
    }
    
    // Initialize variables used to communicate with the TCL layer

    if(tg->grabberVars.channel_var) {
      free(tg->grabberVars.channel_var);
      tg->grabberVars.channel_var = 0;
    }

    if(tg->grabberVars.flatfield_var) {
      free(tg->grabberVars.flatfield_var);
      tg->grabberVars.flatfield_var = 0;
    }

    if(tg->grabberVars.aspect_var) {
      free(tg->grabberVars.aspect_var);
      tg->grabberVars.aspect_var = 0;
    }

    if(tg->grabberVars.fov_var) {
      free(tg->grabberVars.fov_var);
      tg->grabberVars.fov_var = 0;
    }

    if(tg->grabberVars.collimation_var) {
      free(tg->grabberVars.collimation_var);
      tg->grabberVars.collimation_var = 0;
    }

    if(tg->grabberVars.combine_var) {
      free(tg->grabberVars.combine_var);
      tg->grabberVars.combine_var = 0;
    }

    if(tg->grabberVars.ximdir_var) {
      free(tg->grabberVars.ximdir_var);
      tg->grabberVars.ximdir_var = 0;
    }

    if(tg->grabberVars.yimdir_var) {
      free(tg->grabberVars.yimdir_var);
      tg->grabberVars.yimdir_var = 0;
    }

    if(tg->grabberVars.dkRotSense_var) {
      free(tg->grabberVars.dkRotSense_var);
      tg->grabberVars.dkRotSense_var = 0;
    }

    if(tg->grabberVars.xpeak_var) {
      free(tg->grabberVars.xpeak_var);
      tg->grabberVars.xpeak_var = 0;
    }

    if(tg->grabberVars.ypeak_var) {
      free(tg->grabberVars.ypeak_var);
      tg->grabberVars.ypeak_var = 0;
    }

    if(tg->grabberVars.grid_var) {
      free(tg->grabberVars.grid_var);
      tg->grabberVars.grid_var = 0;
    }

    if(tg->grabberVars.bull_var) {
      free(tg->grabberVars.bull_var);
      tg->grabberVars.bull_var = 0;
    }

    if(tg->grabberVars.comp_var) {
      free(tg->grabberVars.comp_var);
      tg->grabberVars.comp_var = 0;
    }

    if(tg->grabberVars.cross_var) {
      free(tg->grabberVars.cross_var);
      tg->grabberVars.cross_var = 0;
    }

    if(tg->grabberVars.box_var) {
      free(tg->grabberVars.box_var);
      tg->grabberVars.box_var = 0;
    }

    if(tg->grabberVars.cmap_var) {
      free(tg->grabberVars.cmap_var);
      tg->grabberVars.cmap_var = 0;
    }

    if(tg->grabberVars.chan0_var) {
      free(tg->grabberVars.chan0_var);
      tg->grabberVars.chan0_var = 0;
    }

    if(tg->grabberVars.chan1_var) {
      free(tg->grabberVars.chan1_var);
      tg->grabberVars.chan1_var = 0;
    }

    if(tg->grabberVars.chan2_var) {
      free(tg->grabberVars.chan2_var);
      tg->grabberVars.chan2_var = 0;
    }

    if(tg->grabberVars.chan3_var) {
      free(tg->grabberVars.chan3_var);
      tg->grabberVars.chan3_var = 0;
    }

    // Delete the container.

    free(tg);
  };

  return NULL;
}

/*.......................................................................
 * This is a wrapper around del_TclGrabber() that can be called with a
 * ClientData version of the (TclGrabber *) pointer.
 */
static void delete_TclGrabber(ClientData context)
{
  del_TclGrabber((TclGrabber *)context);
}

/*.......................................................................
 * This is the service function of the Tcl monitor command. It dispatches
 * the appropriate function for the service that is named by the
 * first argument.
 */
#ifdef USE_CHAR_DECL
static int service_TclGrabber(ClientData context, Tcl_Interp *interp,
			      int argc, char *argv[])
#else
     static int service_TclGrabber(ClientData context, Tcl_Interp *interp,
				   int argc, const char *argv[])
#endif
{
  TclGrabber *tg = (TclGrabber *) context;
  Symbol *sym;    /* The symbol table entry of a given service */
  int status;     /* The return status of the service function */
  LOG_DISPATCHER(*old_dispatcher);/* The previous lprintf() dispatch function */
  void *old_context;              /* The data attached to old_dispatcher */
  
  // Check arguments.

  if(!tg || !interp || !argv) {
    fprintf(stderr, "service_TclGrabber: NULL arguments.\n");
    return TCL_ERROR;
  };

  if(argc < 2) {
    Tcl_AppendResult(interp, "monitor service name missing.", NULL);
    return TCL_ERROR;
  };
  
  // Lookup the named service.

  sym = find_HashSymbol(tg->services, (char* )argv[1]);
  if(!sym) {
    Tcl_AppendResult(interp, "monitor service '", argv[1], "' unknown.", NULL);
    return TCL_ERROR;
  };
  
  // Check the number of argument matches that required by the
  // service.

  if(argc-2 != sym->code) {
    Tcl_AppendResult(interp, "Wrong number of arguments to the 'monitor ",
		     argv[1], "' command.", NULL);
    return TCL_ERROR;
  };
  
  // Arrange for error messages to be collected in tg->stderr_output.

  divert_lprintf(stderr, tg_log_dispatcher, tg, &old_dispatcher, &old_context);
  
  // Pass the rest of the arguments to the service function.

  try {
    status = ((SERVICE_FN(*))sym->fn)(interp, tg, argc-2, (char**)(argv+2));
  } catch(Exception& err) {
    lprintf(stderr, "%s", err.what());
  }
  
  // Revert to the previous dispatch function for stderr, if any, or
  // to sending stderr to the controlling terminal otherwise.

  divert_lprintf(stderr, old_dispatcher, old_context, NULL, NULL);
  
  // If any error messages were collected from lprintf(), assign them
  // to the interpretter's result object, discard the message object,
  // and return the Tcl error code.

  if(*Tcl_DStringValue(&tg->stderr_output)) {
    Tcl_DStringResult(interp, &tg->stderr_output);
    return TCL_ERROR;
  };

  return status;
}

/*.......................................................................
 * This is an lprintf() callback function for stderr. It creates
 * tg->stderr_output for the first line of an error message, then
 * appends that line and subsequent lines to it.
 */
static LOG_DISPATCHER(tg_log_dispatcher)
{
  TclGrabber *tg = (TclGrabber *) context;
  
  // Append the message to tg->stderr_output.

  Tcl_DStringAppend(&tg->stderr_output, message, -1);
  
  // Append a newline to separate it from subsequent messages.

  Tcl_DStringAppend(&tg->stderr_output, "\n", -1);

  return 0;
}

/**.......................................................................
 * The following function is called by the TCL event loop whenever the
 * image monitor-stream file-descriptor becomes readable, or (when selected)
 * writable.
 */
static void tg_im_file_handler(ClientData context, int mask)
{
  TclGrabber *tg = (TclGrabber *) context;
  int send_in_progress = tg->im_send_in_progress;
  int disconnect = 0;  /* True if the stream should be closed */
  LOG_DISPATCHER(*old_dispatcher);/* The previous lprintf() dispatch function */
  void *old_context;              /* The data attached to old_dispatcher */
  
  static bool first=true;

  // Arrange for error messages to be collected in tg->stderr_output.

  divert_lprintf(stderr, tg_log_dispatcher, tg, &old_dispatcher, &old_context);
  
  // Check for incoming data.

  if(mask | TCL_READABLE) {
    switch(tg->image_->read()) {
    case IMS_READ_ENDED:
      disconnect = 1;
      return;
      break;
    case IMS_READ_AGAIN:
      break;
    case IMS_READ_DONE:
      
#if 1
      char stringVar[10];
      if(tg->grabberVars.channel_var) {
	sprintf(stringVar, "%d", tg->image_->getCurrentChannel());
	Tcl_SetVar(tg->interp, tg->grabberVars.channel_var, stringVar,  
		   TCL_GLOBAL_ONLY);
      }
#else
      if(first) {
	tg->image_->drawDouble();
	first = false;
      } else {
	tg->image_->draw();
      }
#endif
      break;
    };
  };

  // Update the TCL file handle for changes in the events to be
  // selected for potential changes to the fd.

  if(disconnect)
    tg_update_im_TclFile(tg, -1, 0); /* Disconnect the file handler */
  else 
    tg_update_im_TclFile(tg, tg->image_->fd(), send_in_progress);
  
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

  if(*Tcl_DStringValue(&tg->stderr_output)) {
    Tcl_VarEval(tg->interp, "bgerror {", Tcl_DStringValue(&tg->stderr_output),
		"}", NULL);
    Tcl_DStringInit(&tg->stderr_output);
  };
  
  // If we reached the end of the stream evaluate any end-of-stream
  // callback that the user registered. Note that this has to be done
  // after the above reporting and clearing of tg->stderr_output
  // because otherwise if the event that triggered the disconnection
  // wrote anything to stderr, that error message would get associated
  // with the first call to a monitor service in the callback script.

  if(disconnect)
    call_eos_callback(tg);
}

/*.......................................................................
 * Update the image monitor-stream file handler to cater for a change of fd
 * or a change in which events to select for. Note that the monitor-stream
 * file fd is liable to change at the end of each I/O transaction (eg.
 * The file input-stream can span many files). When this happens this
 * function allocates a new Tcl file wrapper.
 *
 * Input:
 *  tg        TclGrabber *  The Tcl monitor-interface resource object.
 *  fd               int    The current file descriptor of the monitor
 *                          stream, or -1 to delete the current file
 *                          handle and handler.
 *  send_in_progress int    True to select for writeability.
 */
static void tg_update_im_TclFile(TclGrabber *tg, int fd, int send_in_progress)
{
  int update_handler = 0;  /* True to (re)-install the file handler */
  /*
   * See if the required events have changed.
   */
  update_handler = update_handler || send_in_progress != 
    tg->im_send_in_progress;
  /*
   * See if the file-descriptor of the monitor stream has changed.
   */
  if(tg->im_fd == -1 || tg->im_fd != fd ||
#ifdef USE_TCLFILE
     !tg->im_tclfile
#else
     !tg->im_fd_registered
#endif
     ) {
    /*
     * Delete the current file handle and event handler.
     */
#ifdef USE_TCLFILE
    if(tg->im_tclfile) {
      Tcl_DeleteFileHandler(tg->im_tclfile);
      Tcl_FreeFile(tg->im_tclfile);
      tg->im_tclfile = NULL;
    };
#else
    if(tg->im_fd_registered) {
      Tcl_DeleteFileHandler(tg->im_fd);
      tg->im_fd_registered = 0;
    };
#endif
    /*
     * Record the new fd and attempt to create its Tcl file handle.
     */
    if(fd != -1
#ifdef USE_TCLFILE
       && (tg->im_tclfile = Tcl_GetFile((ClientData) fd, TCL_UNIX_FD)) != NULL
#endif
       ) {
      tg->im_fd = fd;
      update_handler = 1;
    } else {
      tg->im_fd = -1;
      tg->im_send_in_progress = 0;
      update_handler = 0;
    };
  };
  /*
   * See if the file handler needs to be updated.
   */
  if(update_handler) {
    tg->im_fd_event_mask = TCL_READABLE;
    if(send_in_progress)
      tg->im_fd_event_mask |= TCL_WRITABLE;
#ifdef USE_TCLFILE
    Tcl_CreateFileHandler(tg->im_tclfile, tg->im_fd_event_mask, 
			  tg_im_file_handler,
			  (ClientData) tg);
#else
    Tcl_CreateFileHandler(tg->im_fd, tg->im_fd_event_mask, tg_im_file_handler,
			  (ClientData) tg);
    tg->im_fd_registered = 1;
#endif
    tg->im_send_in_progress = send_in_progress;
  };
}

/*.......................................................................
 * Arrange to temporarily stop listening for the readability and
 * writability of the image data-stream file descriptor.
 *
 * Input:
 *  tg        TclGrabber *   The resource object of the tcl interface.
 */
static void tg_suspend_im_TclFile(TclGrabber *tg)
{
  /*
   * If a file handler is currently registered, clear its event mask.
   */
#ifdef USE_TCLFILE
  if(tg->im_tclfile)
    Tcl_CreateFileHandler(tg->im_tclfile, 0, tg_im_file_handler, 
			  (ClientData) tg);
#else
  if(tg->im_fd_registered)
    Tcl_CreateFileHandler(tg->im_fd, 0, tg_im_file_handler, (ClientData) tg);
#endif
}

/**.......................................................................
 * Resume listening to events on the current image data stream socket having
 * previously called tg_suspend_im_TclFile().
 *
 * Input:
 *  tg        TclGrabber *   The resource object of the tcl interface.
 */
static void tg_resume_im_TclFile(TclGrabber *tg)
{
  /*
   * Reinstate the event mask.
   */
#ifdef USE_TCLFILE
  if(tg->im_tclfile)
    Tcl_CreateFileHandler(tg->im_tclfile, tg->im_fd_event_mask, 
			  tg_im_file_handler,
			  (ClientData) tg); 
#else
  if(tg->im_fd_registered)
    Tcl_CreateFileHandler(tg->im_fd, tg->im_fd_event_mask, tg_im_file_handler,
			  (ClientData) tg);
#endif
}


/*.......................................................................
 * Change the host from which we will receive images
 */
static SERVICE_FN(tg_imhost)
{
  ImMonitorStream *ims;    /* The new monitor stream */
  
  // Replace the current image monitor stream with the new one and
  // record whether the configuration had to be changed, in the result
  // string.

  tg->image_->openStream(argv[0]);
  tg_update_im_TclFile(tg, tg->image_->fd(), 0);

  return TCL_OK;
}
/*.......................................................................
 * Disconnect an image input stream.
 */
static SERVICE_FN(tg_im_disconnect)
{
  // Replace the current image monitor stream with a NULL stream.

  tg->image_->changeStream(0);
  tg_update_im_TclFile(tg, tg->image_->fd(), 0);

  return TCL_OK;
}

/*.......................................................................
 * Change the limits of a given graph.
 */
static SERVICE_FN(tg_im_setrange_full)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  // Set the new limits in the image descriptor.

  tg->image_->getImage(chan).setDisplayedRange();

  return TCL_OK;
}

/**.......................................................................
 * Change the limits of a given graph.
 */
static SERVICE_FN(tg_im_setrange)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  double xa, xb, ya, yb;  /* The new zoom limits */
  
  // Decode the range limits.

  if(Tcl_GetDouble(interp, argv[1], &xa) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &ya) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &xb) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[4], &yb) == TCL_ERROR)
    return TCL_ERROR;
  
  // Set the new limits in the image descriptor.

  tg->image_->getImage(chan).setDisplayedRange(xa, xb, ya, yb);

  return TCL_OK;
}

/**.......................................................................
 * Compute statistics on an image.
 */
static SERVICE_FN(tg_im_stat)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  double xa, xb, ya, yb, tgp; /* The limits over which to compute statistics */
  unsigned ixmin, ixmax, iymin, iymax;
  
  // Decode the range limits.

  if(Tcl_GetDouble(interp, argv[1], &xa) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &ya) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &xb) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[4], &yb) == TCL_ERROR)
    return TCL_ERROR;

  if(xa > xb) {tgp=xb;xb = xa;xa = tgp;}
  if(ya > yb) {tgp=yb;yb = ya;ya = tgp;}

  tg->image_->getImage(chan).worldToPixel(xa, ya, ixmin, iymin, true);
  tg->image_->getImage(chan).worldToPixel(xb, yb, ixmax, iymax, true);

  // Get statistics on the image.

  ImageHandler::ImStat stat = 
    tg->image_->getImage(chan).getStats(ixmin, iymin, ixmax, iymax);

  // Compose the return string.

  sprintf(tg->buffer, "%f %f %f %f %ld", stat.min_, stat.max_, stat.mean_, 
	  stat.rms_, stat.n_);

  Tcl_AppendResult(interp, tg->buffer, NULL);

  return TCL_OK;
}

/**.......................................................................
 * Define a box to include in calculations
 */
static SERVICE_FN(tg_im_inc)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  int ixmin, ixmax, iymin, iymax;
  
  // Decode the range limits.

  if(Tcl_GetInt(interp, argv[1], &ixmin) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[2], &iymin) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[3], &ixmax) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[4], &iymax) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).addIncludeBox(ixmin, iymin, ixmax, iymax);

  return TCL_OK;
}

/**.......................................................................
 * Define a box to include in calculations
 */
static SERVICE_FN(tg_im_exc)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  int ixmin, ixmax, iymin, iymax;
  
  // Decode the range limits.

  if(Tcl_GetInt(interp, argv[1], &ixmin) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[2], &iymin) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[3], &ixmax) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[4], &iymax) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).addExcludeBox(ixmin, iymin, ixmax, iymax);

  return TCL_OK;
}

/**.......................................................................
 * Delete the closest box
 */
static SERVICE_FN(tg_im_del_box)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  int ix, iy;
  
  // Decode the range limits.

  if(Tcl_GetInt(interp, argv[1], &ix) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[2], &iy) == TCL_ERROR)
    return TCL_ERROR;
  
  tg->image_->getImage(chan).deleteNearestBox(ix, iy);
  
  return TCL_OK;
}

/**.......................................................................
 * Delete all boxes
 */
static SERVICE_FN(tg_im_del_all_box)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  double x, y;
  unsigned ix, iy;
  
  tg->image_->getImage(chan).deleteAllBoxes();
  
  return TCL_OK;
}

/**.......................................................................
 * Reconfigure dialog boxes
 */
static SERVICE_FN(tg_im_reconfigure)
{
  int rmtChan;
  if(Tcl_GetInt(interp, argv[0], &rmtChan) == TCL_ERROR)
    return TCL_ERROR;

  int lclChan;
  if(Tcl_GetInt(interp, argv[0], &lclChan) == TCL_ERROR)
    return TCL_ERROR;

  char stringVar[10];


  try {

    if(rmtChan == lclChan) {

      ImagePlotter& im = tg->image_->currentImage();

      if(tg->grabberVars.flatfield_var) {
	sprintf(stringVar, "%d", im.flatfieldType_);
	if(Tcl_SetVar(interp, tg->grabberVars.flatfield_var, stringVar,  
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.aspect_var) {
	sprintf(stringVar, "%6.3f", fabs(im.aspectRatio_));
	if(Tcl_SetVar(interp, tg->grabberVars.aspect_var, stringVar,  
		      TCL_GLOBAL_ONLY) == NULL) {
	  return TCL_ERROR;
	}
      }

      if(tg->grabberVars.fov_var) {
	sprintf(stringVar, "%6.3f", fabs(im.fov_.arcmin()));
	if(Tcl_SetVar(interp, tg->grabberVars.fov_var, stringVar,  
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.collimation_var) {
	sprintf(stringVar, "%6.3f", fabs(im.rotationAngle_.degrees()));
	if(Tcl_SetVar(interp, tg->grabberVars.collimation_var, stringVar,  
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.combine_var) {
	sprintf(stringVar, "%d", im.nCombine_);
	if(Tcl_SetVar(interp, tg->grabberVars.combine_var, stringVar,  
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.ximdir_var) {
	if(Tcl_SetVar(interp, tg->grabberVars.ximdir_var, 
		      (char*)(im.xImDir_ == gcp::control::UPRIGHT ? "1" : "-1"),
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.yimdir_var) {
	if(Tcl_SetVar(interp, tg->grabberVars.yimdir_var, 
		      (char*)(im.yImDir_ == gcp::control::UPRIGHT ? "1" : "-1"),
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.grid_var) {
	if(Tcl_SetVar(interp, tg->grabberVars.grid_var, 
		      (char*)(im.doGrid() ? "on" : "off"),
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.bull_var) {
	if(Tcl_SetVar(interp, tg->grabberVars.bull_var, 
		      (char*)(im.doBullseye() ? "on" : "off"),
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.comp_var) {
	if(Tcl_SetVar(interp, tg->grabberVars.comp_var, 
		      (char*)(im.doCompass() ? "on" : "off"),
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.cross_var) {
	if(Tcl_SetVar(interp, tg->grabberVars.cross_var, 
		      (char*)(im.doCrosshair() ? "on" : "off"),
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.box_var) {
	if(Tcl_SetVar(interp, tg->grabberVars.box_var, 
		      (char*)(im.doBoxes() ? "on" : "off"),
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

      if(tg->grabberVars.cmap_var) {
	if(Tcl_SetVar(interp, tg->grabberVars.cmap_var, 
		      (char*)(im.colormapName().c_str()),
		      TCL_GLOBAL_ONLY) == NULL)
	  return TCL_ERROR;
      }

#if 0
  // Figure out what to do with these!

  char* xpeak_var;
  char* ypeak_var;
#endif


    }

    //------------------------------------------------------------
    // Reconfigure the channel labels to reflect any change in the
    // ptel associations
    //------------------------------------------------------------

    if(tg->grabberVars.chan0_var) {

      try {
	gcp::util::PointingTelescopes::Ptel ptel = 
	  gcp::util::PointingTelescopes::getSinglePtel(0);

	sprintf(stringVar, "Channel 0 (Ptel %d)", 
		gcp::util::PointingTelescopes::ptelToInt(ptel));

      } catch(...) {
	sprintf(stringVar, "Channel 0");
      }

      if(Tcl_SetVar(interp, tg->grabberVars.chan0_var, stringVar,
		    TCL_GLOBAL_ONLY) == NULL)
	return TCL_ERROR;
    }

    if(tg->grabberVars.chan1_var) {

      try {
	gcp::util::PointingTelescopes::Ptel ptel = 
	  gcp::util::PointingTelescopes::getSinglePtel(1);

	sprintf(stringVar, "Channel 1 (Ptel %d)", 
		gcp::util::PointingTelescopes::ptelToInt(ptel));

      } catch(...) {
	sprintf(stringVar, "Channel 1");
      }

      if(Tcl_SetVar(interp, tg->grabberVars.chan1_var, stringVar,
		    TCL_GLOBAL_ONLY) == NULL)
	return TCL_ERROR;
    }

    if(tg->grabberVars.chan2_var) {

      try {
	gcp::util::PointingTelescopes::Ptel ptel = 
	  gcp::util::PointingTelescopes::getSinglePtel(2);

	sprintf(stringVar, "Channel 2 (Ptel %d)", 
		gcp::util::PointingTelescopes::ptelToInt(ptel));

      } catch(...) {
	sprintf(stringVar, "Channel 2");
      }

      if(Tcl_SetVar(interp, tg->grabberVars.chan2_var, stringVar,
		    TCL_GLOBAL_ONLY) == NULL)
	return TCL_ERROR;
    }

    if(tg->grabberVars.chan3_var) {

      try {
	gcp::util::PointingTelescopes::Ptel ptel = 
	  gcp::util::PointingTelescopes::getSinglePtel(3);

	sprintf(stringVar, "Channel 3 (Ptel %d)", 
		gcp::util::PointingTelescopes::ptelToInt(ptel));

      } catch(...) {
	sprintf(stringVar, "Channel 3");
      }

      if(Tcl_SetVar(interp, tg->grabberVars.chan3_var, stringVar,
		    TCL_GLOBAL_ONLY) == NULL)
	return TCL_ERROR;
    }

  } catch(Exception& err) {
    COUT(err.what());
    return TCL_ERROR;
  }

  return TCL_OK;
}

/**.......................................................................
 * Redraw an image.
 */
static SERVICE_FN(tg_im_redraw)
{
  // Redraw the frame grabber image

  try {
    tg->image_->draw();
  } catch(...) {
    // Don't throw errors in this function, as it may be called before
    // the pgplot device is configured
  }

  return TCL_OK;
}

/**.......................................................................
 * Redraw an image.
 */
static SERVICE_FN(tg_im_redraw2)
{
  // Redraw the frame grabber image

  try {
    tg->image_->drawDouble();
  } catch(...) {
    // Don't throw errors in this function, as it may be called before
    // the pgplot device is configured
  }

  return TCL_OK;
}

/**.......................................................................
 * Change the X-direction
 */
static SERVICE_FN(tg_im_ximdir)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  // Decode the direction

  int dir;  
  if(Tcl_GetInt(interp, argv[1], &dir) == TCL_ERROR)
    return TCL_ERROR;
  
  // Redraw the monitor image.

  tg->image_->getImage(chan).setXImDir(dir==1 ? UPRIGHT : INVERTED); 

  return TCL_OK;
}

/**.......................................................................
 * Change the Y-direction
 */
static SERVICE_FN(tg_im_yimdir)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  // Decode the direction

  int dir;

  if(Tcl_GetInt(interp, argv[1], &dir) == TCL_ERROR)
    return TCL_ERROR;
  
  // Redraw the monitor image.

  tg->image_->getImage(chan).setYImDir(dir==1 ? UPRIGHT : INVERTED); 

  return TCL_OK;
}

/**.......................................................................
 * Change the currently selected channel
 */
static SERVICE_FN(tg_im_select_channel)
{
  int chan;

  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;
  
  // Set the default image to be the 

  tg->image_->setCurrentImage((unsigned short)chan);

  return TCL_OK;
}

/*.......................................................................
 * Change the contrast
 */
static SERVICE_FN(tg_im_contrast)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  double xa, ya;  /* The cursor position */
  
  // Decode the range limits.

  if(Tcl_GetDouble(interp, argv[1], &xa) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &ya) == TCL_ERROR)
    return TCL_ERROR;
  
  // Redraw the monitor image.

  tg->image_->getImage(chan).fiddleContrast(xa, ya);

  return TCL_OK;
}

/**.......................................................................
 * Install a new image colormap.
 */
static SERVICE_FN(tg_im_colormap)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  // Redraw the monitor image.

  tg->image_->getImage(chan).installColormap(argv[1]);

  return TCL_OK;
}

/**.......................................................................
 * Change the interval for bullseye display.
 */
static SERVICE_FN(tg_im_step)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  double asec;
  
  // Decode the interval (arcseconds).

  if(Tcl_GetDouble(interp, argv[1], &asec) == TCL_ERROR)
    return TCL_ERROR;

  gcp::util::Angle interval;
  interval.setArcSec(asec);

  tg->image_->getImage(chan).setGridInterval(interval);

  return TCL_OK;
}

/*.......................................................................
 * Change the FOV
 */
static SERVICE_FN(tg_im_fov)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  double amin;
  
  // Decode the viewport width (arcminutes).

  if(Tcl_GetDouble(interp, argv[1], &amin) == TCL_ERROR)
    return TCL_ERROR;

  gcp::util::Angle fov;
  fov.setArcMinutes(amin);

  tg->image_->getImage(chan).setFov(fov);

  return TCL_OK;
}

/*.......................................................................
 * Change the aspect ratio
 */
static SERVICE_FN(tg_im_aspect)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  double aspect;
  
  // Decode the aspect ratio

  if(Tcl_GetDouble(interp, argv[1], &aspect) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).setAspectRatio(aspect);

  return TCL_OK;
}

/**.......................................................................
 * Compute the image centroid.
 */
static SERVICE_FN(tg_im_centroid)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  char xstring[10],ystring[10];

  gcp::util::ImageHandler::ImStat stat = tg->image_->getImage(chan).getStats();

  double x, y;
  tg->image_->getImage(chan).pixelToWorld(stat.ixmax_, stat.iymax_, x, y);

  sprintf(xstring,"%3.3f", x);
  sprintf(ystring,"%3.3f", y);
  
  // And return the new centroid.

  Tcl_AppendResult(interp, xstring, " ", NULL);
  Tcl_AppendResult(interp, ystring, " ", NULL);

  // Finally, set the new centroid as the centroid of this image, for
  // drawing crosshairs

  tg->image_->getImage(chan).setPeak(stat.ixmax_, stat.iymax_);

  return TCL_OK;
}

/**.......................................................................
 * Convert from world coordinates to sky offset
 */
static SERVICE_FN(tg_im_worldToSky)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  // Decode the aspect ratio

  double xw, yw;
  if(Tcl_GetDouble(interp, argv[1], &xw) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &yw) == TCL_ERROR)
    return TCL_ERROR;

  gcp::util::Angle xsky, ysky;
  tg->image_->getImage(chan).worldToSkyOffset(xw, yw, xsky, ysky);

  char xstring[10], ystring[10];
  sprintf(xstring,"%s", xsky.strDegrees().c_str());
  sprintf(ystring,"%s", ysky.strDegrees().c_str());
  
  // And return the new centroid.

  Tcl_AppendResult(interp, xstring, " ", NULL);
  Tcl_AppendResult(interp, ystring, " ", NULL);

  return TCL_OK;
}

/**.......................................................................
 * Convert from world coordinates to pixel value
 */
static SERVICE_FN(tg_im_worldToPixel)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  // Decode the aspect ratio

  double xw, yw;
  if(Tcl_GetDouble(interp, argv[1], &xw) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &yw) == TCL_ERROR)
    return TCL_ERROR;

  unsigned ix, iy;
  tg->image_->getImage(chan).worldToPixel(xw, yw, ix, iy, true);

  char xstring[10], ystring[10];
  sprintf(xstring,"%d", ix);
  sprintf(ystring,"%d", iy);
  
  // And return the new coordinate

  Tcl_AppendResult(interp, xstring, " ", NULL);
  Tcl_AppendResult(interp, ystring, " ", NULL);

  return TCL_OK;
}

/**.......................................................................
 * Update whether we are drawing a grid or not.
 */
static SERVICE_FN(tg_im_toggle_grid)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).doGrid() =  !tg->image_->getImage(chan).doGrid();

  return TCL_OK;
}
/**.......................................................................
 * Update whether we are drawing a bullseye or not.
 */
static SERVICE_FN(tg_im_toggle_bullseye)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).doBullseye() =  !tg->image_->getImage(chan).doBullseye();

  return TCL_OK;
}

/**.......................................................................
 * Update whether we are drawing a crosshair or not.
 */
static SERVICE_FN(tg_im_toggle_crosshair)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).doCrosshair() =  !tg->image_->getImage(chan).doCrosshair();

  return TCL_OK;
}

/**.......................................................................
 * Update whether we are drawing a box or not.
 */
static SERVICE_FN(tg_im_toggle_boxes)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).doBoxes() =  !tg->image_->getImage(chan).doBoxes();

  return TCL_OK;
}

/**.......................................................................
 * Update whether we are drawing a compass or not.
 */
static SERVICE_FN(tg_im_toggle_compass)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).doCompass() =  !tg->image_->getImage(chan).doCompass();

  return TCL_OK;
}

/**.......................................................................
 * Update the rotation angle of the camera
 */
static SERVICE_FN(tg_im_rotationAngle)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  double degrees;
  
  // Decode the rotation angle (degrees?)

  if(Tcl_GetDouble(interp, argv[1], &degrees) == TCL_ERROR)
    return TCL_ERROR;

  gcp::util::Angle angle;
  angle.setDegrees(degrees);
  tg->image_->getImage(chan).setRotationAngle(angle);

  return TCL_OK;
}

/**.......................................................................
 * Set the centroid of the image
 */
static SERVICE_FN(tg_im_set_centroid)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  int xpeak, ypeak;
  
  // Read the peak position

  if(Tcl_GetInt(interp, argv[1], &xpeak) == TCL_ERROR ||
     Tcl_GetInt(interp, argv[2], &ypeak) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).setPeak(xpeak, ypeak);

  return TCL_OK;
}

/**.......................................................................
 * Set the number of images to combine
 */
static SERVICE_FN(tg_im_set_combine)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  // Read the combine parameter

  int combine;
  if(Tcl_GetInt(interp, argv[1], &combine) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).setNCombine(combine);

  return TCL_OK;
}

/**.......................................................................
 * Set the pointing telescope association
 */
static SERVICE_FN(tg_im_set_ptel)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  // Read the combine parameter

  gcp::util::PointingTelescopes::Ptel ptel;
  if(Tcl_GetInt(interp, argv[1], (int*)&ptel) == TCL_ERROR)
    return TCL_ERROR;

  gcp::util::PointingTelescopes::
    assignFgChannel(ptel, gcp::grabber::Channel::intToChannel(chan));

  return TCL_OK;
}

/**.......................................................................
 * Set the flatfielding type
 */
static SERVICE_FN(tg_im_set_flatfield)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  // Read the flatfield parameter

  int flatfield;
  if(Tcl_GetInt(interp, argv[1], &flatfield) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).setFlatfieldType(flatfield);

  return TCL_OK;
}

/**.......................................................................
 * Reset the image contrast
 */
static SERVICE_FN(tg_im_reset_contrast)
{
  int chan;
  if(Tcl_GetInt(interp, argv[0], &chan) == TCL_ERROR)
    return TCL_ERROR;

  tg->image_->getImage(chan).setContrast();

  return TCL_OK;
}

/**.......................................................................
 * Register a script to be called whenever the end of a monitor stream
 * is reached.
 */
static SERVICE_FN(tg_eos_callback)
{
  
  // Discard any previous callback.

  if(tg->eos_script)
    free(tg->eos_script);
  tg->eos_script = NULL;
  
  // Allocate a copy of the callback script, if one has been provided.

  if(*argv[0] != '\0') {
    tg->eos_script = (char* )malloc(strlen(argv[0])+1);
    if(!tg->eos_script) {
      Tcl_AppendResult(interp, "Failed to allocate end-of-stream callback.\n",
		       NULL);
      return TCL_ERROR;
    };
    strcpy(tg->eos_script, argv[0]);
  };
  return TCL_OK;
}

/**.......................................................................
 * After a change to the application's register map this function is
 * called to inform the Tcl layer via a previously registered callback.
 */
static void call_eos_callback(TclGrabber *tg)
{
  
  // Is there a script to be evaluated?

  if(tg->eos_script) {
    
    // Invoke the callback.

    if(Tcl_VarEval(tg->interp, tg->eos_script, NULL)== TCL_ERROR)
      lprintf(stderr, "%s\n", tg->interp->result);
  }

}

/**.......................................................................
 * Add a new image display to the viewer.
 */
static SERVICE_FN(tg_open_image)
{
  tg->image_->open(argv[0]);
  
  return TCL_OK;
}

/*.......................................................................
 * After a call to this function, whenever status messages
 * are received from the grabber their contents will be recorded in
 * the specified variable.
 *
 * Input:
 *  tc      TclControl *   The package-instance resource object.
 *  interp  Tcl_Interp *   The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[].
 *  argv          char **  [0] - The global or fully qualified name of
 *                               the Tcl variable in which to record
 *                               subsequent pager messages.
 * Output:
 *  return         int     TCL_OK    - Success.
 *                         TCL_ERROR - Error. Message recorded in
 *                                     interp->result[].
 */
static SERVICE_FN(tg_vars)
{
  char* param = argv[0];
  char* name  = argv[1];
  char** var=0;

  if(strcmp(param, "channel")==0) 
    var = &tg->grabberVars.channel_var;
  else if(strcmp(param, "combine")==0) 
    var = &tg->grabberVars.combine_var;
  else if(strcmp(param, "flatfield")==0) 
    var = &tg->grabberVars.flatfield_var;
  else if(strcmp(param, "aspect")==0) 
    var = &tg->grabberVars.aspect_var;
  else if(strcmp(param, "fov")==0) 
    var = &tg->grabberVars.fov_var;
  else if(strcmp(param, "collimation")==0) 
    var = &tg->grabberVars.collimation_var;
  else if(strcmp(param, "ximdir")==0) 
    var = &tg->grabberVars.ximdir_var;
  else if(strcmp(param, "yimdir")==0) 
    var = &tg->grabberVars.yimdir_var;
  else if(strcmp(param, "dkRotSense")==0) 
    var = &tg->grabberVars.dkRotSense_var;
  else if(strcmp(param, "xpeak")==0) 
    var = &tg->grabberVars.xpeak_var;
  else if(strcmp(param, "ypeak")==0) 
    var = &tg->grabberVars.ypeak_var;
  else if(strcmp(param, "grid")==0) 
    var = &tg->grabberVars.grid_var;
  else if(strcmp(param, "bull")==0) 
    var = &tg->grabberVars.bull_var;
  else if(strcmp(param, "comp")==0) 
    var = &tg->grabberVars.comp_var;
  else if(strcmp(param, "cross")==0) 
    var = &tg->grabberVars.cross_var;
  else if(strcmp(param, "cmap")==0) 
    var = &tg->grabberVars.cmap_var;
  else if(strcmp(param, "chan0")==0) 
    var = &tg->grabberVars.chan0_var;
  else if(strcmp(param, "chan1")==0) 
    var = &tg->grabberVars.chan1_var;
  else if(strcmp(param, "chan2")==0) 
    var = &tg->grabberVars.chan2_var;
  else if(strcmp(param, "chan3")==0) 
    var = &tg->grabberVars.chan3_var;
  else if(strcmp(param, "box")==0) 
    var = &tg->grabberVars.box_var;
  else {
    Tcl_AppendResult(interp, "Unrecognized parameter", NULL);
    return TCL_ERROR;
  }
  
  // If an array has previously been registered, remove it first.

  if(*var) {
    free(*var);
    *var = NULL;
  };

  // If a non-empty array name has been specified, record a copy of
  // it.

  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
		       "pager_var: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    *var = strcpy(tmp, name);
  };
  return TCL_OK;
}

/*.......................................................................
 * Test whether a string contains anything other than white-space.
 *
 * Input:
 *  s     char *  The string to be tested.
 * Output:
 *  return int    0 - Not empty.
 *                1 - Empty string or s==NULL.
 */
static int is_empty_string(char *s)
{
  if(!s)
    return 1;
  while(isspace((int)*s))
    s++;
  return *s ? 0 : 1;
}
