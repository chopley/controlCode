#include <stdio.h>
#include <stdlib.h>
#include <tk.h>

#include "tclmonitor.h"
#include "tclcontrol.h"
#include "tclgrabber.h"
#include "tclstarplot.h"

#include "tkpgplot.h"
#include "panel.h"
#include "netscape_remote.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Directives.h"

static int Viewer_AppInit(Tcl_Interp *interp);

int main(int argc, char**argv)
{
  char **args=NULL;  /* The replacement argument vector for argv[] */
  int i;

  try {

  gcp::util::Debug::setLevel(gcp::util::Debug::DEBUGNONE);

/*
 * In order to prevent Tcl/Tk from interpretting command-line flags
 * we have to insert an extra argument containing "--". To do this
 * requires malloc'ing a new argument vector.
 */
  args = (char** )malloc(sizeof(char *) * (argc+1));
  if(!args) {
    fprintf(stderr, "main: Unable to allocate argument vector.\n");
    return 1;
  };

/*
 * Copy the original command-line argument vector into args[] along
 * with the new "--" argument.
 */
  args[0] = argv[0];     /* The name of this program */
  if(argc > 1) {
    args[1] = argv[1];   /* The szaviewer script */
    i = 2;
  } else {
    i = 1;
  };
  args[i] = "--";      /* The argument preservation argument */
  for( ; i<argc; i++)
    args[i+1] = argv[i];
/*
 * Start Tk.
 */
  Tk_Main(argc+1, args, Viewer_AppInit);

  } catch(gcp::util::Exception& err) {
    std::cout << err.what() << std::endl;
    free(args);
    return 1;
  }
/*
 * Discard the modified argument vector.
 */
  free(args);

  return 0;
}

/*.......................................................................
 * Viewer_AppInit is called by Tcl_Main() to initialize application-specific
 * packages.
 *
 * Input:
 *  interp   Tcl_Interp *   The TCL interpreter.
 */
static int Viewer_AppInit(Tcl_Interp *interp)
{
/*
 * Create the standard TCL package, Tk, and the viewer package.
 */
  if(Tcl_Init(interp)    == TCL_ERROR ||
     Tk_Init(interp)     == TCL_ERROR ||
     Tkpgplot_Init(interp) == TCL_ERROR ||
     TclMonitor_Init(interp) == TCL_ERROR ||
     Tclcontrol_Init(interp) == TCL_ERROR ||
     TclGrabber_Init(interp) == TCL_ERROR ||
     TclStarPlot_Init(interp) == TCL_ERROR ||
     Tkpanel_Init(interp) == TCL_ERROR ||
     Netscape_remote_Init(interp) == TCL_ERROR)
    return TCL_ERROR;

  return TCL_OK;  

}
