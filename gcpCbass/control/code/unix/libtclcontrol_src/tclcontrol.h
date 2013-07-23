#ifndef tclcontrol_h
#define tclcontrol_h

#include <tcl.h>

/*
 * The following function is the package initialization function
 * of the Tcl control-interface. It creates a Tcl command called
 * "control".
 */
int Tclcontrol_Init(Tcl_Interp *interp);

#endif
