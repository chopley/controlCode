// This file implements the TCL "starplot" command that provides the
// interface between TCL and the star plotter

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

#include "gcp/pgutil/common/StarPlot.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Flux.h"
#include "gcp/util/common/PeriodicTimer.h"
#include "gcp/util/common/String.h"

using namespace gcp::control;

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

struct TclStarPlotMsg {
  enum MsgType {
    MSG_REDRAW,
  };

  MsgType type_;
};

/*
 * Define the resource object of the interface.
 */
typedef struct {
  Tcl_Interp *interp;    /* The tcl-interpretter */
  FreeList *tmc_mem;     /* Memory for TmCallback containers */
  HashMemory *hash_mem;  /* Memory from which to allocate hash tables */
  HashTable *services;   /* A symbol table of service functions */

  char buffer[256];      /* A work buffer for constructing result strings */
  InputStream *input;    /* An input stream to parse register names in */
  OutputStream *output;  /* An output stream to parse register names in */
  Tcl_DString stderr_output;/* A resizable string containing stderr output */

  gcp::util::StarPlot* plotter;

#ifdef USE_TCLFILE
  Tcl_File msgq_tclfile;   // A Tcl wrapper around msgq.fd() 
#else
  int msgq_fd_registered;  // True if a file handler has been created 
#endif
  int msgq_fd_event_mask; // The current event mask associated with 'fd' 

  gcp::util::PeriodicTimer* timer;
  gcp::util::PipeQ<TclStarPlotMsg>* msgq;

} TclStarPlot;

static TclStarPlot *new_TclStarPlot(Tcl_Interp *interp);
static TclStarPlot *del_TclStarPlot(TclStarPlot *tm);
static void delete_TclStarPlot(ClientData context);

void maskObjects(TclStarPlot* ts, char* prefix, unsigned& incMask, unsigned& excMask);
std::vector<std::string> getNedSources(TclStarPlot* ts, std::string id);

#ifdef USE_CHAR_DECL
static int service_TclStarPlot(ClientData context, Tcl_Interp *interp,
			       int argc, char *argv[]);
#else
static int service_TclStarPlot(ClientData context, Tcl_Interp *interp,
			       int argc, const char *argv[]);
#endif

static LOG_DISPATCHER(ts_log_dispatcher);

/*
 * Define an object to encapsulate a copy of a Tcl callback script
 * along with the resource object of the monitor interface.
 */
typedef struct {
  TclStarPlot *ts;   /* The resource object of the monitor interface */
  char *script;     /* The callback script. */
} TmCallback;

/*
 * Create a container for describing service functions.
 */
#define SERVICE_FN(fn) int (fn)(Tcl_Interp *interp, TclStarPlot *ts, \
				 int argc, char *argv[])
typedef struct {
  char *name;          /* The name of the service */
  SERVICE_FN(*fn);     /* The function that implements the service */
  unsigned narg;       /* The expected value of argc to be passed to fn() */
} ServiceFn;

/*
 * Provide prototypes for service functions.
 */
static SERVICE_FN(ts_redraw);
static SERVICE_FN(ts_add_catalog);
static SERVICE_FN(ts_replace_catalog);
static SERVICE_FN(ts_add_cal);
static SERVICE_FN(ts_replace_cal);
static SERVICE_FN(ts_change_el);
static SERVICE_FN(ts_change_fluxlim);
static SERVICE_FN(ts_change_maglim);
static SERVICE_FN(ts_change_sunDist);
static SERVICE_FN(ts_change_moonDist);
static SERVICE_FN(ts_change_latitude);
static SERVICE_FN(ts_change_longitude);
static SERVICE_FN(ts_change_site);
static SERVICE_FN(ts_open_image);
static SERVICE_FN(ts_identify);
static SERVICE_FN(ts_add_survey);
static SERVICE_FN(ts_replace_survey);
static SERVICE_FN(ts_change_lst);
static SERVICE_FN(ts_clear_lst);
static SERVICE_FN(ts_change_mjd);
static SERVICE_FN(ts_clear_mjd);
static SERVICE_FN(ts_track);
static SERVICE_FN(ts_setrange);
static SERVICE_FN(ts_setrange_full);
static SERVICE_FN(ts_search_ned);

static SERVICE_FN(ts_listNedObjects);
static SERVICE_FN(ts_maskNedObjects);
static SERVICE_FN(ts_maskNedCatalogs);
static SERVICE_FN(ts_clearNedMasks);
static SERVICE_FN(ts_listNedCatalogs);

static SERVICE_FN(ts_setNedObjectSelection);
static SERVICE_FN(ts_setNedCatalogSelection);

void enableRedrawTimer(TclStarPlot* ts, bool enable);
static PERIODIC_TIMER_HANDLER(updateStarplot);
static void createMsgqFileHandler(TclStarPlot* ts); 
static void ts_msgq_file_handler(ClientData context, int mask);

/*
 * List top-level service functions.
 */
static ServiceFn service_fns[] = {
  
  /* Redraw a given plot with updated axes */
  {"redraw",            ts_redraw,            0},

  /* Redraw a given plot with updated axes */
  {"add_cal",           ts_add_cal,           1},

  /* Redraw a given plot with updated axes */
  {"replace_cal",       ts_replace_cal,       1},

  /* Redraw a given plot with updated axes */
  {"add_catalog",       ts_add_catalog,       1},

  /* Redraw a given plot with updated axes */
  {"replace_catalog",   ts_replace_catalog,   1},

  /* Redraw a given plot with updated axes */
  {"open_image",        ts_open_image,        1},

  /* Redraw a given plot with updated axes */
  {"change_el",         ts_change_el,         1},

  /* Redraw a given plot with updated axes */
  {"change_maglim",     ts_change_maglim,     1},

  /* Redraw a given plot with updated axes */
  {"change_sunDist",    ts_change_sunDist,    1},

  /* Redraw a given plot with updated axes */
  {"change_moonDist",   ts_change_moonDist,   1},

  /* Redraw a given plot with updated axes */
  {"change_fluxlim",    ts_change_fluxlim,    1},

  /* Redraw a given plot with updated axes */
  {"change_latitude",   ts_change_latitude,   1},

  /* Redraw a given plot with updated axes */
  {"change_longitude",  ts_change_longitude,  1},

  /* Redraw a given plot with updated axes */
  {"change_site",       ts_change_site,       3},

  /* Redraw a given plot with updated axes */
  {"mark",              ts_identify,          3},

  /* Redraw a given plot with updated axes */
  {"add_survey",        ts_add_survey,        3},

  {"replace_survey",    ts_replace_survey,    3},

  {"change_lst",        ts_change_lst,        1},

  {"clear_lst",         ts_clear_lst,         0},

  {"change_mjd",        ts_change_mjd,        1},

  {"clear_mjd",         ts_clear_mjd,         0},

  {"track",             ts_track,             0},

  {"setrange",          ts_setrange,          4},

  {"setrange_full",     ts_setrange_full,     0},

  {"search_ned",        ts_search_ned,        4},

  {"listNedObjects",    ts_listNedObjects,    2},
  
  {"maskNedObjects",    ts_maskNedObjects,    2},

  {"maskNedCatalogs",   ts_maskNedCatalogs,   1},

  {"clearNedMasks",     ts_clearNedMasks,     0},

  {"listNedCatalogs",   ts_listNedCatalogs,   1},

  {"setNedObjectSelection", ts_setNedObjectSelection,  1},

  {"setNedCatalogSelection",ts_setNedCatalogSelection, 1},
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
int TclStarPlot_Init(Tcl_Interp *interp)
{
  try {
    TclStarPlot* ts=0;   /* The resource object of the tcl_db() function */
    
    // Check arguments.

    if(!interp) {
      fprintf(stderr, "TclStarPlot_Init: NULL interpreter.\n");
      return TCL_ERROR;
    };
    
    // Allocate a resource object for the service_TclStarPlot()
    // command.

    ts = new_TclStarPlot(interp);
    if(!ts)
      return TCL_ERROR;

    createMsgqFileHandler(ts);

    // Create the TCL command that will control the monitor viewer.

    Tcl_CreateCommand(interp, "starplot", service_TclStarPlot, (ClientData) ts,
		      delete_TclStarPlot);

    return TCL_OK;

  } catch(gcp::util::Exception& err) {
    std::cout << err.what() << std::endl;
    return TCL_ERROR;
  }
}

/*.......................................................................
 * Create the resource object of the Tcl monitor viewer interface.
 *
 * Input:
 *  interp  Tcl_Interp *  The Tcl interpretter.
 * Output:
 *  return  TclStarPlot *  The resource object, or NULL on error.
 */
static TclStarPlot *new_TclStarPlot(Tcl_Interp *interp)
{
  TclStarPlot *ts;  /* The object to be returned */
  int i;
  
  // Check arguments.

  if(!interp) {
    fprintf(stderr, "new_TclStarPlot: NULL interp argument.\n");
    return NULL;
  };
  
  // Allocate the container.

  ts = (TclStarPlot *) malloc(sizeof(TclStarPlot));

  if(!ts) {
    fprintf(stderr, "new_TclStarPlot: Insufficient memory.\n");
    return NULL;
  };
  
  // Before attempting any operation that might fail, initialize the
  // container at least up to the point at which it can safely be
  // passed to del_TclStarPlot().

  ts->interp   = interp;
  ts->tmc_mem  = NULL;
  ts->hash_mem = NULL;
  ts->services = NULL;

  ts->input        = NULL;
  ts->output       = NULL;
  ts->plotter      = 0;
  ts->timer        = 0;
  ts->msgq         = 0;
#ifdef USE_TCLFILE
  ts->msgq_tclfile = 0;
#else
  ts->msgq_fd_registered = 0;
#endif
  ts->msgq_fd_event_mask = 0;
  
  Tcl_DStringInit(&ts->stderr_output);
  
  // Create a free-list of TmCallback containers.

  ts->tmc_mem = new_FreeList("new_TclStarPlot", sizeof(TmCallback), 20);
  if(!ts->tmc_mem)
    return del_TclStarPlot(ts);
  
  // Create memory for allocating hash tables.

  ts->hash_mem = new_HashMemory(10, 20);
  if(!ts->hash_mem)
    return del_TclStarPlot(ts);
  
  // Create and populate the symbol table of services.

  ts->services = new_HashTable(ts->hash_mem, 53, IGNORE_CASE, NULL, 0);
  if(!ts->services)
    return del_TclStarPlot(ts);

  for(i=0; i < (int)(sizeof(service_fns)/sizeof(service_fns[0])); i++) {
    ServiceFn *srv = service_fns + i;
    if(new_HashSymbol(ts->services, srv->name, srv->narg,
		      (void (*)(void)) srv->fn, NULL, 0) == NULL)
      return del_TclStarPlot(ts);
  };
  
  // Create the monitor viewer resource object.

  ts->plotter = new gcp::util::StarPlot();
  if(!ts->plotter)
    return del_TclStarPlot(ts);

  // Create a periodic timer

  ts->timer = new gcp::util::PeriodicTimer();
  if(!ts->timer)
    return del_TclStarPlot(ts);
  
  ts->msgq = new gcp::util::PipeQ<TclStarPlotMsg>();
  if(!ts->msgq)
    return del_TclStarPlot(ts);
  
  // Create unassigned input and output streams.

  ts->input = new_InputStream();
  if(!ts->input)
    return del_TclStarPlot(ts);

  ts->output = new_OutputStream();
  if(!ts->output)
    return del_TclStarPlot(ts);

  ts->timer->spawn();

  return ts;
}

/*.......................................................................
 * Delete the resource object of the Tcl monitor viewer interface.
 *
 * Input:
 *  tm     TclStarPlot *  The object to be deleted.
 * Output:
 *  return TclStarPlot *  The deleted object (Always NULL).
 */
static TclStarPlot *del_TclStarPlot(TclStarPlot *ts)
{
  if(ts) {
    
    // Delete the hash-table of services.

    ts->services = del_HashTable(ts->services);
    
    // Having deleted all its client hash-tables, delete the
    // hash-table memory allocator.

    ts->hash_mem = del_HashMemory(ts->hash_mem, 1);
    
    // Delete the monitor viewer resource object.

    if(ts->plotter) {
      delete ts->plotter;
      ts->plotter = 0;
    }

    if(ts->timer) {
      delete ts->timer;
      ts->timer = 0;
    }

    if(ts->msgq) {
      delete ts->msgq;
      ts->msgq = 0;
    }
    
    // Delete the callback containers. This is left until after
    // del_MonitorViewer(), so that all of the callback containers
    // will have been returned to the free-list.

    ts->tmc_mem = del_FreeList("del_TclStarPlot", ts->tmc_mem, 1);
    
    // Delete the streams.

    ts->input = del_InputStream(ts->input);
    ts->output = del_OutputStream(ts->output);
    
    // Delete the container.

    free(ts);
  };

  return NULL;
}

/*.......................................................................
 * This is a wrapper around del_TclStarPlot() that can be called with a
 * ClientData version of the (TclStarPlot *) pointer.
 *
 * Input:
 *  context    ClientData  A TclStarPlot pointer cast to ClientData.
 */
static void delete_TclStarPlot(ClientData context)
{
  del_TclStarPlot((TclStarPlot *)context);
}

/*.......................................................................
 * This is the service function of the Tcl monitor command. It dispatches
 * the appropriate function for the service that is named by the
 * first argument.
 *
 * Input:
 *  context   ClientData    The TclStarPlot resource object cast to
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
static int service_TclStarPlot(ClientData context, Tcl_Interp *interp,
			       int argc, char *argv[])
#else
     static int service_TclStarPlot(ClientData context, Tcl_Interp *interp,
				    int argc, const char *argv[])
#endif
{
  TclStarPlot* ts = (TclStarPlot *) context;
  Symbol *sym;    /* The symbol table entry of a given service */
  int status=0;     /* The return status of the service function */
  LOG_DISPATCHER(*old_dispatcher);/* The previous lprintf() dispatch function */
  void *old_context;              /* The data attached to old_dispatcher */
  
  // Check arguments.

  if(!ts || !interp || !argv) {
    fprintf(stderr, "service_TclStarPlot: NULL arguments.\n");
    return TCL_ERROR;
  };

  if(argc < 2) {
    Tcl_AppendResult(interp, "monitor service name missing.", NULL);
    return TCL_ERROR;
  };
  
  // Lookup the named service.

  sym = find_HashSymbol(ts->services, (char* )argv[1]);
  if(!sym) {
    Tcl_AppendResult(interp, "starplot service '", argv[1], "' unknown.", NULL);
    return TCL_ERROR;
  };
  
  // Check the number of argument matches that required by the
  // service.

  if(argc-2 != sym->code) {
    Tcl_AppendResult(interp, "Wrong number of arguments to the 'starplot ",
		     argv[1], "' command.", NULL);
    return TCL_ERROR;
  };
  
  // Arrange for error messages to be collected in ts->stderr_output.

  divert_lprintf(stderr, ts_log_dispatcher, ts, &old_dispatcher, &old_context);
  
  // Pass the rest of the arguments to the service function.

  try {
    status = ((SERVICE_FN(*))sym->fn)(interp, ts, argc-2, (char**)(argv+2));
  } catch(gcp::util::Exception& err) {
    lprintf(stderr, "%s", err.what());
  }
  
  // Revert to the previous dispatch function for stderr, if any, or
  // to sending stderr to the controlling terminal otherwise.

  divert_lprintf(stderr, old_dispatcher, old_context, NULL, NULL);
  
  // If any error messages were collected from lprintf(), assign them
  // to the interpretter's result object, discard the message object,
  // and return the Tcl error code.

  if(*Tcl_DStringValue(&ts->stderr_output)) {
    Tcl_DStringResult(interp, &ts->stderr_output);
    return TCL_ERROR;
  };

  return status;
}

/*.......................................................................
 * This is an lprintf() callback function for stderr. It creates
 * ts->stderr_output for the first line of an error message, then
 * appends that line and subsequent lines to it.
 */
static LOG_DISPATCHER(ts_log_dispatcher)
{
  TclStarPlot *ts = (TclStarPlot *) context;
  
  // Append the message to ts->stderr_output.

  Tcl_DStringAppend(&ts->stderr_output, message, -1);
  
  // Append a newline to separate it from subsequent messages.

  Tcl_DStringAppend(&ts->stderr_output, "\n", -1);

  return 0;
}

/*.......................................................................
 * Redraw a given plot with updated axes.
 */
static SERVICE_FN(ts_track)
{
  // Clear any fixed date and enable a timer on the expiry of which we
  // will redraw the star plot

  ts->plotter->clearDisplayLst();
  enableRedrawTimer(ts, true);

  return TCL_OK;
}

/*.......................................................................
 * Redraw a given plot with updated axes.
 */
static SERVICE_FN(ts_redraw)
{
  ts->plotter->redraw();
  return TCL_OK;
}

/**.......................................................................
 * Add catalog
 */
static SERVICE_FN(ts_add_catalog)
{
  ts->plotter->readCatalog(argv[0]);
  ts->plotter->redraw();
  return TCL_OK;
}

/**.......................................................................
 * Replace catalog
 */
static SERVICE_FN(ts_replace_catalog)
{
  ts->plotter->initCatalog();
  ts->plotter->readCatalog(argv[0]);
  ts->plotter->redraw();
  return TCL_OK;
}

/**.......................................................................
 * Add cal
 */
static SERVICE_FN(ts_add_cal)
{
  ts->plotter->readCal(argv[0]);
  ts->plotter->redraw();
  return TCL_OK;
}

/**.......................................................................
 * Replace cal
 */
static SERVICE_FN(ts_replace_cal)
{
  ts->plotter->initCatalog();
  ts->plotter->readCal(argv[0]);
  ts->plotter->redraw();
  return TCL_OK;
}

/**.......................................................................
 * Change the marked distance from the sun
 */
static SERVICE_FN(ts_change_sunDist)
{
  double dist;

 if(Tcl_GetDouble(interp, argv[0], &dist) == TCL_ERROR)
    ThrowError("Error in Tcl_GetDouble");

  gcp::util::Angle radius;
  radius.setDegrees(dist);

  ts->plotter->setSunDist(radius);
  ts->plotter->redraw();

  return TCL_OK;
}

/**.......................................................................
 * Change the marked distance from the moon
 */
static SERVICE_FN(ts_change_moonDist)
{
  double dist;

  if(Tcl_GetDouble(interp, argv[0], &dist) == TCL_ERROR)
    ThrowError("Error in Tcl_GetDouble");

  gcp::util::Angle radius;
  radius.setDegrees(dist);

  ts->plotter->setMoonDist(radius);
  ts->plotter->redraw();

  return TCL_OK;
}

/**.......................................................................
 * Change the magnitude limit
 */
static SERVICE_FN(ts_change_maglim)
{
  double mag;

  if(Tcl_GetDouble(interp, argv[0], &mag) == TCL_ERROR)
    ThrowError("Error in Tcl_GetDouble");

  ts->plotter->setMagLim(mag);
  ts->plotter->redraw();

  return TCL_OK;
}

/**.......................................................................
 * Change the latitude
 */
static SERVICE_FN(ts_change_latitude)
{
  gcp::util::Angle lat;

  gcp::util::String latStr(argv[0]);
  latStr.strip(" ");

  lat.setDegrees(latStr.str());
  ts->plotter->setSiteName("Unknown");
  ts->plotter->setLatitude(lat);
  ts->plotter->redraw();

  return TCL_OK;
}

/**.......................................................................
 * Change the longitude
 */
static SERVICE_FN(ts_change_longitude)
{
  gcp::util::Angle lng;

  gcp::util::String lngStr(argv[0]);
  lngStr.strip(" ");

  lng.setDegrees(lngStr.str());
  ts->plotter->setSiteName("Unknown");
  ts->plotter->setLongitude(lng);
  ts->plotter->redraw();

  return TCL_OK;
}

/**.......................................................................
 * Change the EL limit
 */
static SERVICE_FN(ts_change_el)
{
  gcp::util::Angle el;
  el.setDegrees(argv[0]);
  ts->plotter->setElevationLimit(el);
  ts->plotter->redraw();

  return TCL_OK;
}

/**.......................................................................
 * Clear the catalog
 */
static SERVICE_FN(ts_change_site)
{
  gcp::util::Angle lat, lng;

  ts->plotter->setSite(argv[0], lat, lng);
  ts->plotter->redraw();

  if(!Tcl_SetVar(ts->interp, argv[1], (char*)lat.strDegrees().c_str(), 0))
    return TCL_ERROR;

  if(!Tcl_SetVar(ts->interp, argv[2], (char*)lng.strDegrees().c_str(), 0))
    return TCL_ERROR;

  return TCL_OK;
}

/*.......................................................................
 * Add a new image display to the viewer.
 */
static SERVICE_FN(ts_open_image)
{
  ts->plotter->openDevice(argv[0]);
  ts->plotter->redraw();

  return TCL_OK;
}

/**.......................................................................
 * Identify a source
 */
static SERVICE_FN(ts_identify)
{
  double x, y;
  
  if(Tcl_GetDouble(interp, argv[1], &x) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &y) == TCL_ERROR)
    ThrowError("Error in Tcl_GetDouble");
    
  ts->plotter->clearMarks();

  gcp::util::StarPlot::Object obj = ts->plotter->mark(x, y);

  ts->plotter->redraw();
  
  std::ostringstream os;
  os << "Source is: " << std::endl << obj;

  if(!Tcl_SetVar(ts->interp, argv[0], (char*)os.str().c_str(), 0))
    return TCL_ERROR;

  return TCL_OK;
}

/**.......................................................................
 * Add a survey
 */
static SERVICE_FN(ts_add_survey)
{
  gcp::util::Flux min, max;
  
  double fmin, fmax;
  
  if(Tcl_GetDouble(interp, argv[1], &fmin) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &fmax) == TCL_ERROR)
    ThrowError("Error in Tcl_GetDouble");

  min.setJy(fmin);
  max.setJy(fmax);

  ts->plotter->readPtSrc(argv[0], min, max);
  ts->plotter->redraw();

  return TCL_OK;
}

/**.......................................................................
 * Replace a survey
 */
static SERVICE_FN(ts_replace_survey)
{
  gcp::util::Flux min, max;
  
  double fmin, fmax;
  
  if(Tcl_GetDouble(interp, argv[1], &fmin) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &fmax) == TCL_ERROR)
    ThrowError("Error in Tcl_GetDouble");

  min.setJy(fmin);
  max.setJy(fmax);
  
  ts->plotter->initCatalog();
  ts->plotter->readPtSrc(argv[0], min, max);
  ts->plotter->redraw();

  return TCL_OK;
}

/**.......................................................................
 * Change the MJD
 */
static SERVICE_FN(ts_change_mjd)
{
  gcp::util::TimeVal mjd;

  double mjdVal;
  if(Tcl_GetDouble(interp, argv[0], &mjdVal) == TCL_ERROR)
    ThrowError("Error converting MJD: " << argv[0]);

  mjd.setMjd(mjdVal);

  // Disable any automatic update that may previously have been enabled

  enableRedrawTimer(ts, false);

  ts->plotter->setDisplayMjd(mjd);

  return TCL_OK;
}

/**.......................................................................
 * Clear the MJD
 */
static SERVICE_FN(ts_clear_mjd)
{
  ts->plotter->clearDisplayMjd();

  return TCL_OK;
}

/**.......................................................................
 * Change the LST
 */
static SERVICE_FN(ts_change_lst)
{
  gcp::util::HourAngle lst;
  lst.setHours(argv[0]);

  // Disable any automatic update that may previously have been enabled

  enableRedrawTimer(ts, false);

  ts->plotter->setDisplayLst(lst);

  return TCL_OK;
}

/**.......................................................................
 * Clear the LST
 */
static SERVICE_FN(ts_clear_lst)
{
  ts->plotter->clearDisplayLst();

  return TCL_OK;
}

/**.......................................................................
 * Change the flux limit
 */
static SERVICE_FN(ts_change_fluxlim)
{
  double dflux;

  if(Tcl_GetDouble(interp, argv[0], &dflux) == TCL_ERROR)
    ThrowError("Error in Tcl_GetDouble");

  gcp::util::Flux flux;
  flux.setJy(dflux);
  
  ts->plotter->setFluxLim(flux);
  ts->plotter->redraw();

  return TCL_OK;
}

/**.......................................................................
 * Change the displayed range
 */
static SERVICE_FN(ts_setrange)
{
  double xa, xb, ya, yb;
  
  if(Tcl_GetDouble(interp, argv[0], &xa) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[1], &ya) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &xb) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &yb) == TCL_ERROR)
    ThrowError("Error in Tcl_GetDouble");
    
  ts->plotter->setRange(xa, xb, ya, yb);

  return TCL_OK;
}

/**.......................................................................
 * Change the displayed range
 */
static SERVICE_FN(ts_setrange_full)
{
  ts->plotter->setRange();
  return TCL_OK;
}

/**.......................................................................
 * Search the NED database
 */
static SERVICE_FN(ts_search_ned)
{
  // Now read the boundaries

  double xa, xb, ya, yb;
  
  if(Tcl_GetDouble(interp, argv[0], &xa) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[1], &ya) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[2], &xb) == TCL_ERROR ||
     Tcl_GetDouble(interp, argv[3], &yb) == TCL_ERROR)
    ThrowError("Error in Tcl_GetDouble");
    
  ts->plotter->searchNed(xa, xb, ya, yb);

  return TCL_OK;
}

/**.......................................................................
 * Return a vector of objects requested by name
 */
std::vector<std::string> getNedSources(TclStarPlot* ts, std::string id)
{
  gcp::util::NedReader* reader = (gcp::util::NedReader*)ts->plotter->ptSrcReader("ned");

  std::vector<std::string> objList;

  if(strcmp(id.c_str(), "classifiedExtragalactic")==0) {
    objList = reader->listObjects(gcp::util::NedReader::CLASSIFIED_EXTRAGALACTIC);
  } else if(strcmp(id.c_str(), "unclassifiedExtragalactic")==0) {
    objList = reader->listObjects(gcp::util::NedReader::UNCLASSIFIED_EXTRAGALACTIC);
  } else if(strcmp(id.c_str(), "galactic")==0) {
    objList = reader->listObjects(gcp::util::NedReader::GALAXY_COMPONENT);
  } else if(strcmp(id.c_str(), "all")==0) {
    objList = reader->listObjects(gcp::util::NedReader::TYPE_ALL);
  }

  return objList;
}


/**.......................................................................
 * Return a list of classified extra-galactic NED objects
 */
static SERVICE_FN(ts_listNedObjects)
{
  std::vector<std::string> objList = getNedSources(ts, argv[0]);

  std::ostringstream os;

  for(unsigned i=0; i < objList.size(); i++)
    os << "{" << objList[i] << "} ";

  if(!Tcl_SetVar(ts->interp, argv[1], (char*)os.str().c_str(), 0))
    return TCL_ERROR;

  return TCL_OK;
}

/**.......................................................................
 * Return a list of classified extra-galactic NED objects
 */
static SERVICE_FN(ts_listNedCatalogs)
{
  gcp::util::NedReader* reader = (gcp::util::NedReader*)ts->plotter->ptSrcReader("ned");

  std::vector<std::string> catList;
  catList = reader->listCatalogs();

  std::ostringstream os;

  for(unsigned i=0; i < catList.size(); i++)
    os << "{" << catList[i] << "} ";

  if(!Tcl_SetVar(ts->interp, argv[0], (char*)os.str().c_str(), 0))
    return TCL_ERROR;

  return TCL_OK;
}

/**.......................................................................
 * Mask objects
 */
static SERVICE_FN(ts_clearNedMasks)
{
  gcp::util::NedReader* reader = (gcp::util::NedReader*)ts->plotter->ptSrcReader("ned");

  reader->clearObjectIncludeMask();
  reader->clearObjectExcludeMask();
  reader->clearCatalogMask();

  return TCL_OK;
}

/**.......................................................................
 * Mask catalogs
 */
static SERVICE_FN(ts_maskNedCatalogs)
{
  // Get the objects the user requested

  gcp::util::NedReader* reader = (gcp::util::NedReader*)ts->plotter->ptSrcReader("ned");
  std::vector<std::string> catList = reader->listCatalogs();

  std::ostringstream os;
  unsigned mask=0x0;

  for(unsigned i=0; i < catList.size(); i++) {

    os.str("");

    // Build the variable name we want to check

    os << argv[0] << ".";

    for(unsigned iChar=0; iChar < catList[i].size(); iChar++)
      os << (unsigned char)tolower(catList[i].at(iChar));

    const char* val=0;
    if((val=Tcl_GetVar(ts->interp, (char*)os.str().c_str(), TCL_GLOBAL_ONLY)) != 0) {

      if(strcmp(val, "selected")==0) {
	reader->maskCatalog(catList[i], mask);
      }

    }
  }

  reader->addToCatalogMask(mask);

  return TCL_OK;
}

/**.......................................................................
 * Mask objects
 */
static SERVICE_FN(ts_maskNedObjects)
{
  // Get the objects the user requested

  gcp::util::NedReader* reader = (gcp::util::NedReader*)ts->plotter->ptSrcReader("ned");
  std::vector<std::string> objList = getNedSources(ts, argv[1]);

  std::ostringstream os;
  unsigned incMask=0x0, excMask=0x0;

  for(unsigned i=0; i < objList.size(); i++) {

    os.str("");

    // Build the variable name we want to check

    os << argv[0] << ".";

    for(unsigned iChar=0; iChar < objList[i].size(); iChar++)
      os << (unsigned char)tolower(objList[i].at(iChar));

    const char* val=0;
    if((val=Tcl_GetVar(ts->interp, (char*)os.str().c_str(), TCL_GLOBAL_ONLY)) != 0) {

      if(strcmp(val, "exclude")==0) {
	reader->maskObject(objList[i], excMask);
      }

      if(strcmp(val, "include")==0) {
	reader->maskObject(objList[i], incMask);
      }

    }
  }

  reader->addToObjectIncludeMask(incMask);
  reader->addToObjectExcludeMask(excMask);

  return TCL_OK;
}

void maskObjects(TclStarPlot* ts, char* prefix, unsigned& incMask, unsigned& excMask)
{
  gcp::util::NedReader* reader = (gcp::util::NedReader*)ts->plotter->ptSrcReader("ned");
  std::vector<std::string> objList = reader->listObjects(gcp::util::NedReader::CLASSIFIED_EXTRAGALACTIC);

  std::ostringstream os;

  incMask=0x0, excMask=0x0;

  for(unsigned i=0; i < objList.size(); i++) {

    os.str("");

    os << prefix << ".name.";

    for(unsigned iChar=0; iChar < objList[i].size(); iChar++)
      os << (unsigned char)tolower(objList[i].at(iChar));

    const char* val=0;
    if((val=Tcl_GetVar(ts->interp, (char*)os.str().c_str(), TCL_GLOBAL_ONLY)) != 0) {

      if(strcmp(val, "exclude")==0) {
	reader->maskObject(objList[i], excMask);
      }

      if(strcmp(val, "include")==0) {
	reader->maskObject(objList[i], incMask);
      }

    }
  }
}

static SERVICE_FN(ts_setNedObjectSelection)
{
  gcp::util::NedReader* reader = (gcp::util::NedReader*)ts->plotter->ptSrcReader("ned");

  if(strcmp(argv[0], "any")==0) {
    reader->setObjectSelection(gcp::util::NedReader::ANY);
  } else if(strcmp(argv[0], "all")==0) {
    reader->setObjectSelection(gcp::util::NedReader::ALL);
  }

  return TCL_OK;
}

static SERVICE_FN(ts_setNedCatalogSelection)
{
  gcp::util::NedReader* reader = (gcp::util::NedReader*)ts->plotter->ptSrcReader("ned");

  if(strcmp(argv[0], "any")==0) {
    reader->setCatalogSelection(gcp::util::NedReader::ANY);
  } else if(strcmp(argv[0], "all")==0) {
    reader->setCatalogSelection(gcp::util::NedReader::ALL);
  } else if(strcmp(argv[0], "not")==0) {
    reader->setCatalogSelection(gcp::util::NedReader::NOT);
  } else if(strcmp(argv[0], "non")==0) {
    reader->setCatalogSelection(gcp::util::NedReader::NON);
  }

  return TCL_OK;
}

void enableRedrawTimer(TclStarPlot* ts, bool enable)
{
  if(enable) {
    ts->timer->addHandler(updateStarplot, (void*)ts);
    ts->timer->enableTimer(true, 60);
  } else {
    ts->timer->enableTimer(false);
    ts->timer->removeHandler(updateStarplot);
  }
}

static PERIODIC_TIMER_HANDLER(updateStarplot)
{
  TclStarPlot* ts = (TclStarPlot*) args;
  TclStarPlotMsg msg;
  msg.type_ = TclStarPlotMsg::MSG_REDRAW;
  ts->msgq->sendMsg(&msg);
}

/**.......................................................................
 * Create the file handler to service messages received on our message
 * queue
 */
static void createMsgqFileHandler(TclStarPlot* ts) 
{
  ts->msgq_fd_event_mask = TCL_READABLE;

#ifdef USE_TCLFILE
  if((ts->msgq_tclfile = Tcl_GetFile((ClientData) ts->msgq_->fd(), TCL_UNIX_FD)) != NULL) {
    Tcl_CreateFileHandler(ts->msgq_tclfile, ts->msgq_fd_event_mask, ts_msgq_file_handler,
			  (ClientData) tm);
  }
#else
  Tcl_CreateFileHandler(ts->msgq->fd(), ts->msgq_fd_event_mask, ts_msgq_file_handler,
			(ClientData) ts);
  ts->msgq_fd_registered = 1;
#endif
}

/*.......................................................................
 * The following function is called by the TCL event loop whenever the
 * message queue becomes readable
 */
static void ts_msgq_file_handler(ClientData context, int mask)
{
  TclStarPlot* ts = (TclStarPlot*) context;
  TclStarPlotMsg msg;

  if(mask | TCL_READABLE) {
    ts->msgq->readMsg(&msg);

    switch (msg.type_) {
    case TclStarPlotMsg::MSG_REDRAW:
      ts->plotter->redraw();
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
