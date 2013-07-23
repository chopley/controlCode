/*
 * $Id: netscape_remote.c,v 1.1.1.1 2009/07/06 23:57:08 eml Exp $
 *
 * netscape_remote - a Tcl/Tk extension to talk the remote command protocol
 *		     that Netscape uses
 *
 * This extension speaks the remote protocol that is used by the Netscape
 * web browser.  This lets us control netscape remotely without having to
 * start up a new netscape process (despite what the people at Netscape
 * say, starting up a whole new copy of Netscape takes too long for my
 * tastes).
 *
 * We also cache the window id used for Netscape so we don't have to call
 * XQueryTree for every command.
 *
 * Documentation on the protocol netscape uses can be found at the following
 * URL: http://home.netscape.com/newsref/std/x-remote-proto.html
 *
 * By Ken Hornstein <kenh@cmf.nrl.navy.mil>
 *
 */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <tk.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xmu/WinUtil.h>   /* XmuClientWindow() */

#include "netscape_remote.h"


/**
 * Does the current version of tcl use (const char *) declarations or
 * vanilla (char *) 
 */
#if TCL_MAJOR_VERSION < 8 || (TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION < 4) 
#define USE_CHAR_DECL 
#endif

/*
 * Names of some of the Netscape internal properties.
 */

#define MOZILLA_VERSION_PROP	"_MOZILLA_VERSION"
#define MOZILLA_LOCK_PROP	"_MOZILLA_LOCK"
#define MOZILLA_COMMAND_PROP	"_MOZILLA_COMMAND"
#define MOZILLA_RESPONSE_PROP	"_MOZILLA_RESPONSE"
#define MOZILLA_URL_PROP	"_MOZILLA_URL"

/*
 * This is a structure that contains all the info about pending things
 * happening on Netscape windows.  We use this to communicate between
 * NetscapeEventProc and what's happening now
 */

typedef struct PendingCommand {
	int state;		/* Type of sub-event we're waiting for */
	Window win;		/* Window we're waiting for */
	Atom atom;		/* Atom for PropertyChange/Delete */
	int response;		/* Did we get a response? */
} PendingCmd;

/*
 * This structure is a list of all of the X atoms that Netscape uses.
 * We cache these on a per-display basis.
 */

typedef struct _NetscapeAtoms {
	Atom XA_MOZILLA_VERSION;
	Atom XA_MOZILLA_LOCK;
	Atom XA_MOZILLA_COMMAND;
	Atom XA_MOZILLA_RESPONSE;
	Atom XA_MOZILLA_URL;
} NetscapeAtoms;

/*
 * This is our widget record structure.  It contains all of the information
 * needed by netscape-remote on a per-interpreter basis.
 */

typedef struct _NetscapeRecord {
	Tk_Window main_w;	/* Our main window */
	Tcl_HashTable cache;	/* Our hash table for window Id caching */
	Tcl_HashTable atoms;	/* Our hash table containing needed Atoms */
} NetscapeRecord;

#define PENDING_OK 1
#define PENDING_TIMEOUT 2
#define PENDING_DESTROY 3

/*
 * Prototypes for internal functions
 */

#ifdef USE_CHAR_DECL
static int Netscape_Remote_Cmd(ClientData, Tcl_Interp *, int, char *[]);
#else
static int Netscape_Remote_Cmd(ClientData, Tcl_Interp *, int, const char *[]);
#endif

static Window GetWindow(Tcl_Interp *, Tk_Window, Tcl_HashTable *);
static NetscapeAtoms *GetAtoms(Tk_Window, Tcl_HashTable *);
static int ListWindows(Tcl_Interp *, Tcl_HashTable *, Tk_Window);
static int CheckForNetscape(Display *, Window, NetscapeAtoms *);
static int SendCommand(Tcl_Interp *, Tk_Window, Window, char *, PendingCmd *,
		       int, NetscapeAtoms *);
static int NetscapeEventHandler(ClientData, XEvent *);
static int GetLock(Tcl_Interp *, Display *, Window, PendingCmd *, int,
		   NetscapeAtoms *);
static Tk_RestrictAction NetscapeRestrict(ClientData, XEvent *);
static void LockTimeout(ClientData);
static void ReleaseLock(Display *, Window, NetscapeAtoms *atoms);

#ifdef USE_CHAR_DECL
static int Netscape_Info_Cmd(ClientData, Tcl_Interp *, int, char*[]);
#else
static int Netscape_Info_Cmd(ClientData, Tcl_Interp *, int, const char*[]);
#endif

#define DEFAULT_TIMEOUT 10000

/*
 * Our package init routine.  Set things up for our new interpreter commands.
 */

int
Netscape_remote_Init(Tcl_Interp *interp)
{
	NetscapeRecord *widgetrec;

	/*
	 * Before we do anything else, make sure that Tcl and Tk are
	 * loaded.
	 */

	if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL) {
		return TCL_ERROR;
	}

	if (Tcl_PkgRequire(interp, "Tk", TK_VERSION, 0) == NULL) {
		return TCL_ERROR;
	}

	/*
	 * Allocate space for our widget record
	 */

	widgetrec = (NetscapeRecord *) malloc(sizeof(*widgetrec));

	if (widgetrec == NULL) {
		Tcl_AppendResult(interp, "Cannot allocate memory for "
				 "widget record", (char *) NULL);
		return TCL_ERROR;
	}

	if ((widgetrec->main_w = Tk_MainWindow(interp)) == NULL) {
		Tcl_AppendResult(interp, "No main window associated with ",
				 "this interpreter!", (char *) NULL);
		return TCL_ERROR;
	}

	/*
	 * Create hash tables for later use.
	 */

	Tcl_InitHashTable(&widgetrec->cache, TCL_ONE_WORD_KEYS);
	Tcl_InitHashTable(&widgetrec->atoms, TCL_ONE_WORD_KEYS);

	/*
	 * Create our "send-netscape" and "info-netscape" interpreter
	 * commands
	 */
	
	Tcl_CreateCommand(interp, "send-netscape", Netscape_Remote_Cmd,
			  (ClientData) widgetrec, (void (*)(void*)) NULL);
	Tcl_CreateCommand(interp, "info-netscape", Netscape_Info_Cmd,
			  (ClientData) widgetrec, (void (*)(void*)) NULL);
	
	/*
	 * Tell the Tcl package interface that we exist.
	 */

	if (Tcl_PkgProvide(interp, "netscape_remote", "1.3") != TCL_OK)
		return TCL_ERROR;

	return TCL_OK;
}

/*
 * This is the Tcl glue routine to the routines that do the real work
 * (in this case, SendCommand)
 */
#ifdef USE_CHAR_DECL
static int
Netscape_Remote_Cmd(ClientData clientData, Tcl_Interp *interp, int argc,
	char *argv[])
#else
static int
Netscape_Remote_Cmd(ClientData clientData, Tcl_Interp *interp, int argc,
	const char *argv[])
#endif
{
	NetscapeRecord *widgetrec = (NetscapeRecord *) clientData;
	Tk_Window disp_window = widgetrec->main_w;
	Tk_ErrorHandler error;
	Window w = None;
	PendingCmd pending;
	NetscapeAtoms *atoms = NULL;
	int timeout = DEFAULT_TIMEOUT;
	char *idVar = NULL;
	char *arg = NULL;
	int i;

	if (argc < 2 || argc > 10) {
		goto usage;
	}

	/*
	 * Parse our command-line arguments
	 */

	for (i = 1; i < argc; i++) {
		arg = (char* )argv[i];

		if (arg[0] == '-') {

			/*
			 * Process the -id (specify the window id) option
			 */

			if (strcmp(arg, "-id") == 0) {
				i++;
				if (i >= argc) {
					Tcl_AppendResult(interp, "\"-id\" must",
						" be followed by a window id",
						(char *) NULL);
					return TCL_ERROR;
				}
				if (Tcl_GetInt(interp, argv[i], (int *) &w)
				    != TCL_OK) {
					return TCL_ERROR;
				}
			}

			/*
			 * Process -idvar (variable for window id) option
			 */

			else if (strcmp(arg, "-idvar") == 0) {
				i++;
				if (i >= argc) {
					Tcl_AppendResult(interp, "\"-idvar\" "
							 "must be followed ",
							 "by a variable name",
							 (char *) NULL);
					return TCL_ERROR;
				}
				idVar = (char* )argv[i];
			}

			/*
			 * Process the -timeout (for various protocol timeouts)
			 * option
			 */

			else if (strcmp(arg, "-timeout") == 0) {
				i++;
				if (i >= argc) {
					Tcl_AppendResult(interp,
							 "\"-timeout\" must ",
							 "be followed by an ",
							 "integer",
							 (char *) NULL);
					return TCL_ERROR;
				}
				if (Tcl_GetInt(interp, argv[i], &timeout)
				    != TCL_OK) {
					return TCL_ERROR;
				}
				if (timeout <= 0) {
					Tcl_AppendResult(interp, "\"timeout\" "
							 "must be a positive "
							 "interger.",
							 (char *) NULL);
					return TCL_ERROR;
				}
			}

			/*
			 * Process the -displayof (alternate display location)
			 * option.
			 */
			
			else if (strcmp(arg, "-displayof") == 0) {
				i++;
				if (i >= argc) {
					Tcl_AppendResult(interp,
							 "\"-displayof\" must ",
							 "be followed by a ",
							 "window name",
							 (char *) NULL);
					return TCL_ERROR;
				}
				disp_window = Tk_NameToWindow(interp, argv[i],
							      disp_window);
				if (disp_window == NULL)
					return TCL_ERROR;
			} else {
				break;
			}
		} else {
			break;
		}
	}

	if (i != argc - 1) {
usage:
		Tcl_AppendResult(interp, "wrong # args: should be \"",
			argv[0], " ?-id id? ?-idvar idvar? ?-timeout timeout?",
			" ?-displayof window? netscapeCommand\"",
			(char *) NULL);
		return TCL_ERROR;
	}
		

	/*
	 * Figure out which window to use.  Check to see if we have
	 * a cached window - if so, use that one, rather than iterating
	 * through all of the windows on our display.
	 *
	 * We need to setup an error handler here, otherwise we will
	 * exit if the window doesn't exist.
	 */

	error = Tk_CreateErrorHandler(Tk_Display(disp_window), BadWindow,
				      X_GetProperty, -1, NULL, NULL);

	if (w != None) {
		atoms = GetAtoms(disp_window, &widgetrec->atoms);
						 
		if (! CheckForNetscape(Tk_Display(disp_window), w, atoms)) {
			Tcl_AppendResult(interp, "Invalid window Id, or "
					 "window is not a Netscape window",
					 (char *) NULL);
			Tk_DeleteErrorHandler(error);
			return TCL_ERROR;
		}
	} else {
	  // C++ won't let you pass void *, which is apparently what
	  // ClientData is #defined to be
	  Tcl_HashEntry *entry = Tcl_FindHashEntry(&widgetrec->cache, 
						   (char *) Tk_Display(disp_window)); 
	  atoms = GetAtoms(disp_window, &widgetrec->atoms);
		
		if (entry && CheckForNetscape(Tk_Display(disp_window),
					(Window) Tcl_GetHashValue(entry),
					atoms)) {
			w = (Window) Tcl_GetHashValue(entry);
		} else if (entry) {
			Tcl_DeleteHashEntry(entry);
		}
	}

	if (w == None) {
		Tcl_HashEntry *entry;
		int dummy;
		if ((w = GetWindow(interp, disp_window, &widgetrec->atoms))
								== None) {
			Tk_DeleteErrorHandler(error);
			return TCL_ERROR;
		}
		entry = Tcl_CreateHashEntry(&widgetrec->cache,
					    (char *) Tk_Display(disp_window),
					    &dummy);
		Tcl_SetHashValue(entry, (ClientData) w);
	}

	Tk_DeleteErrorHandler(error);

	if (idVar) {
		char value[256];
		sprintf(value, "0x%08x", (int) w);
		if (Tcl_SetVar(interp, idVar, value, TCL_LEAVE_ERR_MSG) ==
		    NULL) {
			return TCL_ERROR;
		}
	}

	if (!atoms)
		atoms = GetAtoms(disp_window, &widgetrec->atoms);

	return SendCommand(interp, disp_window, w, (char* )argv[i], &pending, 
			   timeout, atoms);
}

/*
 * This is the Tcl glue code for the "info-netscape" command
 */
#ifdef USE_CHAR_DECL
static int
Netscape_Info_Cmd(ClientData clientData, Tcl_Interp *interp, int argc,
		  char *argv[])
#else
static int
Netscape_Info_Cmd(ClientData clientData, Tcl_Interp *interp, int argc,
		  const char *argv[])
#endif
{
	NetscapeRecord *widgetrec = (NetscapeRecord *) clientData;
	Tk_Window dispwin = widgetrec->main_w;
	Window w;
	Tk_ErrorHandler error;
	Atom type;
	int format, status;
	unsigned long nitems, bytesafter;
	unsigned char *data;

	if (argc < 2) {
		Tcl_AppendResult(interp, "wrong # args, should be \"", argv[0],
				 " option ?arg arg ...?\"", (char *) NULL);
		return TCL_ERROR;
	}

	/*
	 * Kinda a heinous hack.  Check for -displayof right here.
	 */

	if (strcmp(argv[1], "-displayof") == 0) {
		if (argc < 4) {
			Tcl_AppendResult(interp, "-displayof must be followed "
					 "by a window name", (char *) NULL);
			return TCL_ERROR;
		}

#ifdef USE_CHAR_DECL
		dispwin = Tk_NameToWindow(interp, (char* )argv[2], dispwin);
#else
		dispwin = Tk_NameToWindow(interp, argv[2], dispwin);
#endif
		if (dispwin == NULL)
			return TCL_ERROR;

		argv += 2;
		argc -= 2;
	}

	/*
	 * Check which option we were given
	 */

	if (strcmp(argv[1], "list") == 0) {

		return ListWindows(interp, &widgetrec->atoms, dispwin);

	} else if (strcmp(argv[1], "version") == 0 ||
		   strcmp(argv[1], "url") == 0) {

		NetscapeAtoms *atoms;

		/*
		 * Handle the "version" or the "url" command.  This code
		 * is nearly identical, except that a different property
		 * is fetched at the last part.
		 */

		if (argc != 3) {
			Tcl_AppendResult(interp, "Wrong # args, must be: \"",
					 argv[0], " ", argv[1], " windowId\"",
					 (char *) NULL);
			return TCL_ERROR;
		}

#ifdef USE_CHAR_DECL
		if (Tcl_GetInt(interp, (char* )argv[2], (int *) &w) != TCL_OK) {
#else
		if (Tcl_GetInt(interp, argv[2], (int *) &w) != TCL_OK) {
#endif
			return TCL_ERROR;
		}

		error = Tk_CreateErrorHandler(Tk_Display(dispwin), BadWindow,
				      X_GetProperty, -1, NULL, NULL);

		atoms = GetAtoms(dispwin, &widgetrec->atoms);

		if (! CheckForNetscape(Tk_Display(dispwin), w, atoms)) {
			Tcl_AppendResult(interp, "Window is either nonexistant"
					 " or is not a valid Netscape window",
					 (char *) NULL);
			return TCL_ERROR;
		}

		status = XGetWindowProperty(Tk_Display(dispwin), w,
					    argv[1][0] == 'v' ?
					    atoms->XA_MOZILLA_VERSION :
					    atoms->XA_MOZILLA_URL,
					    0, 65536 / sizeof(long), False,
					    XA_STRING, &type, &format,
					    &nitems, &bytesafter, &data);

		Tk_DeleteErrorHandler(error);

		if (status != Success) {
			Tcl_AppendResult(interp, "Error while reading "
					 " Netscape ", argv[1], (char *) NULL);
			if (data)
				XFree(data);

			return TCL_ERROR;
		}

		Tcl_SetResult(interp, (char* )data, TCL_VOLATILE);

		XFree(data);

		return TCL_OK;

	} else {
		Tcl_AppendResult(interp, "Invalid option: \"", argv[1],
				 "\"; must be one of: list version url",
				 (char *) NULL);
		return TCL_ERROR;
	}

	return TCL_OK;
}

/*
 * Find the window to use on the remote display.  Most of this code is
 * taken from Netscape reference implementation.  We don't do any version
 * checking right now.
 */

static Window
GetWindow(Tcl_Interp *interp, Tk_Window disp, Tcl_HashTable *hash)
{
	int i;
	Window root = RootWindowOfScreen(Tk_Screen(disp));
	Window root2, parent, *kids;
	unsigned int nkids;
	Window result = None;
	NetscapeAtoms *atoms;

	atoms = GetAtoms(disp, hash);

	if (! XQueryTree(Tk_Display(disp), root, &root2, &parent, &kids,
			 &nkids)) {
		Tcl_AppendResult(interp, "XQueryTree failed", (char *) NULL);
		return None;
	}

	if (root != root2) {
		Tcl_AppendResult(interp, "Root windows didn't match!",
				 (char *) NULL);
		return None;
	}

	if (parent != None) {
		Tcl_AppendResult(interp, "We got a valid parent window, but",
				 " we shouldn't have!", (char *) NULL);
		return None;
	}

	if (! (kids && nkids)) {
		Tcl_AppendResult(interp, "No children found!", (char *) NULL);
		return None;
	}

	for (i = 0; i < (int)nkids; i++) {
		Window w = XmuClientWindow(Tk_Display(disp), kids[i]);
		if (CheckForNetscape(Tk_Display(disp), w, atoms)) {
			result = w;
			break;
		}
	}

	if (result == None) {
		Tcl_AppendResult(interp, "Couldn't find a netscape window",
				 (char *) NULL);
	}

	return result;
}

/*
 * Return all Netscape windows on a given display.
 */

static int
ListWindows(Tcl_Interp *interp, Tcl_HashTable *hash, Tk_Window win)
{
	Window root = RootWindowOfScreen(DefaultScreenOfDisplay(Tk_Display(win)));
	Window root2, parent, *kids;
	unsigned int nkids;
	int i;
	char value[256];
	Tcl_DString dstr;
	NetscapeAtoms *atoms;

	Tcl_DStringInit(&dstr);

	atoms = GetAtoms(win, hash);

	/*
	 * Much of the work is the same as in GetWindow, but we're going
	 * to return all valid Netscape windows
	 */

	if (! XQueryTree(Tk_Display(win), root, &root2, &parent, &kids,
			 &nkids)) {
		Tcl_AppendResult(interp, "XQueryTree failed", (char *) NULL);
		return TCL_ERROR;
	}

	if (root != root2) {
		Tcl_AppendResult(interp, "Root windows didn't match!",
				 (char *) NULL);
		return TCL_ERROR;
	}

	if (parent != None) {
		Tcl_AppendResult(interp, "We got a valid parent window, but",
				 " we shouldn't have!", (char *) NULL);
		return TCL_ERROR;
	}

	if (! (kids && nkids)) {
		Tcl_AppendResult(interp, "No children found!", (char *) NULL);
		return TCL_ERROR;
	}

	for (i = 0; i < (int)nkids; i++) {
		Window w = XmuClientWindow(Tk_Display(win), kids[i]);
		if (CheckForNetscape(Tk_Display(win), w, atoms)) {
			sprintf(value, "0x%08x", (int) w);
			Tcl_DStringAppendElement(&dstr, value);
		}
	}

	Tcl_DStringResult(interp, &dstr);

	return TCL_OK;
}

/*
 * Search our hash table for the Netscape atoms associate with that
 * display.
 */

static NetscapeAtoms *
GetAtoms(Tk_Window win, Tcl_HashTable *hash)
{
	Tcl_HashEntry *entry;
	int newentry;
	NetscapeAtoms *atoms;

	entry = Tcl_CreateHashEntry(hash, (char *) Tk_Display(win), &newentry);

	if (newentry) {
		atoms = (NetscapeAtoms *) malloc(sizeof(*atoms));

		/*
		 * Get the Atoms corresponding to these property names; we use
		 * them later.
		 */

		atoms->XA_MOZILLA_VERSION = Tk_InternAtom(win,
							  MOZILLA_VERSION_PROP);
		atoms->XA_MOZILLA_LOCK = Tk_InternAtom(win, MOZILLA_LOCK_PROP);
		atoms->XA_MOZILLA_COMMAND = Tk_InternAtom(win,
							  MOZILLA_COMMAND_PROP);
		atoms->XA_MOZILLA_RESPONSE = Tk_InternAtom(win,
						MOZILLA_RESPONSE_PROP);
		atoms->XA_MOZILLA_URL = Tk_InternAtom(win, MOZILLA_URL_PROP);

		Tcl_SetHashValue(entry, (ClientData) atoms);
	}

	return (NetscapeAtoms *) Tcl_GetHashValue(entry);
}

/*
 * See if the given window is a Netscape window by looking for the
 * XA_MOZILLA_VERSION property
 */

static int
CheckForNetscape(Display *d, Window w, NetscapeAtoms *atoms)
{
	Atom type;
	int format;
	unsigned long nitems, bytesafter;
	unsigned char *version = NULL;
	int status = XGetWindowProperty(d, w, atoms->XA_MOZILLA_VERSION, 0,
					65536 / sizeof(long), False,
					XA_STRING, &type, &format,
					&nitems, &bytesafter, &version);

	if (status != Success || !version) {
		if (version)
			XFree(version);
		return 0;
	}

	/*
	 * We don't do anything with the version right now
	 */

	XFree(version);

	return 1;
}

/*
 * Send a command to the Netscape window we found previously
 */

static int
SendCommand(Tcl_Interp *interp, Tk_Window dispwin, Window win, char *command,
	    PendingCmd *pending, int timeout, NetscapeAtoms *atoms)
{
	Tk_RestrictProc *prevRestrict;
	ClientData prevArgs;
	int result;
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *data;
	Tk_ErrorHandler error;
	Tcl_TimerToken token;

	/*
	 * Select for PropertyChange events on the Netscape window
	 */
	
	XSelectInput(Tk_Display(dispwin), win, (PropertyChangeMask |
		     StructureNotifyMask));

	/*
	 * Create a generic event handler to get events on that window
	 */

	pending->state = 0;
	pending->win = None;
	pending->response = 0;

	Tk_CreateGenericHandler(NetscapeEventHandler, (ClientData) pending);

	if (GetLock(interp, Tk_Display(dispwin), win, pending, timeout,
		    atoms) == 0) {
		Tk_DeleteGenericHandler(NetscapeEventHandler,
					(ClientData) pending);
		XSelectInput(Tk_Display(dispwin), win, 0);
		return TCL_ERROR;
	}

	/*
	 * We've got a successful lock, so send the command to Netscape
	 */

	XChangeProperty(Tk_Display(dispwin), win, atoms->XA_MOZILLA_COMMAND,
			XA_STRING, 8, PropModeReplace,
			(unsigned char *) command, strlen(command));

	/*
	 * Netscape should delete the property containing the command
	 * Wait for this to happen.
	 */

	prevRestrict = Tk_RestrictEvents(NetscapeRestrict,
				(ClientData) pending,
				&prevArgs);
	pending->win = win;
	pending->state = PropertyDelete;
	pending->atom = atoms->XA_MOZILLA_COMMAND;
	pending->response = 0;

	token = Tcl_CreateTimerHandler(timeout, LockTimeout,
				       (ClientData) pending);
	while (!pending->response) {
		Tcl_DoOneEvent(TCL_ALL_EVENTS);
	}
	Tcl_DeleteTimerHandler(token);
	Tk_RestrictEvents(prevRestrict, prevArgs, &prevArgs);

	if (pending->response == PENDING_TIMEOUT) {
		Tcl_AppendResult(interp, "Timeout waiting for Netscape to ",
				 "acknowledge command", (char *) NULL);
		ReleaseLock(Tk_Display(dispwin), win, atoms);
		Tk_DeleteGenericHandler(NetscapeEventHandler,
					(ClientData) pending);
		XSelectInput(Tk_Display(dispwin), win, 0);
		return TCL_ERROR;
	} else if (pending->response == PENDING_DESTROY) {
		Tcl_AppendResult(interp, "Window was destroyed while waiting "
				 "for acknowledgement", (char *) NULL);
		Tk_DeleteGenericHandler(NetscapeEventHandler,
					(ClientData) pending);
		return TCL_ERROR;
	}

	/*
	 * Wait for a response.  Netscape will write it's response code
	 * in the XA_MOZILLA_RESPONSE property -- check that for the
	 * response code
	 */

	prevRestrict = Tk_RestrictEvents(NetscapeRestrict,
				(ClientData) pending,
				&prevArgs);
	pending->win = win;
	pending->state = PropertyNewValue;
	pending->atom = atoms->XA_MOZILLA_RESPONSE;
	pending->response = 0;

	token = Tcl_CreateTimerHandler(timeout, LockTimeout,
				       (ClientData) pending);
	while (!pending->response) {
		Tcl_DoOneEvent(TCL_ALL_EVENTS);
	}
	Tcl_DeleteTimerHandler(token);
	Tk_RestrictEvents(prevRestrict, prevArgs, &prevArgs);

	if (pending->response == PENDING_TIMEOUT) {
		Tcl_AppendResult(interp, "Timeout waiting for a response from",
				 " Netscape", (char *) NULL);
		ReleaseLock(Tk_Display(dispwin), win, atoms);
		Tk_DeleteGenericHandler(NetscapeEventHandler,
					(ClientData) pending);
		XSelectInput(Tk_Display(dispwin), win, 0);
		return TCL_ERROR;
	} else if (pending->response == PENDING_DESTROY) {
		Tcl_AppendResult(interp, "Window was destroyed while waiting ",
				 "for a response", (char *) NULL);
		Tk_DeleteGenericHandler(NetscapeEventHandler,
					(ClientData) pending);
		return TCL_ERROR;
	}

	/*
	 * Get the response string from Netscape
	 */
	
	result = XGetWindowProperty(Tk_Display(dispwin), win,
				    atoms->XA_MOZILLA_RESPONSE, 0,
				    65536 / sizeof(long), True /* delete */,
				    XA_STRING, &actual_type, &actual_format,
				    &nitems, &bytes_after, &data);
	
	if (result != Success) {
		Tcl_AppendResult(interp, "Failed to read response from "
				 "Netscape", (char *) NULL);
		ReleaseLock(Tk_Display(dispwin), win, atoms);
		Tk_DeleteGenericHandler(NetscapeEventHandler,
					(ClientData) pending);
		XSelectInput(Tk_Display(dispwin), win, 0);
		return TCL_ERROR;
	}

	if (! data) {
		Tcl_AppendResult(interp, "No data returned from Netscape",
				 (char *) NULL);
		ReleaseLock(Tk_Display(dispwin), win, atoms);
		Tk_DeleteGenericHandler(NetscapeEventHandler,
					(ClientData) pending);
		XSelectInput(Tk_Display(dispwin), win, 0);
		return TCL_ERROR;
	}

	Tcl_AppendResult(interp, data, (char *) NULL);

	XFree(data);

	/*
	 * Remove the lock on Netscape.  Note that first we install an
	 * error handler for BadWindow errors.  We do this because if we
	 * send Netscape a command such as delete() or exit(), Netscape
	 * will destroy that window before we can clean up completely.
	 * The error handler prevents our Tk application from exiting.
	 */

	error = Tk_CreateErrorHandler(Tk_Display(dispwin), BadWindow,
				      X_ChangeWindowAttributes, -1, NULL,
				      NULL);

	ReleaseLock(Tk_Display(dispwin), win, atoms);

	/*
	 * Delete the generic event handler (otherwise we would be getting
	 * _all_ X events, which would be wasteful)
	 */

	Tk_DeleteGenericHandler(NetscapeEventHandler, (ClientData) pending);

	/*
	 * Don't select these events anymore.
	 */

	XSelectInput(Tk_Display(dispwin), win, 0);

	Tk_DeleteErrorHandler(error);

	return TCL_OK;
}

static int
NetscapeEventHandler(ClientData clientData, XEvent *event)
{
	PendingCmd *pending = (PendingCmd *) clientData;

	if (pending->win == None)
		return 0;

	if (event->type == PropertyNotify && event->xproperty.window ==
	    pending->win && event->xproperty.state == pending->state &&
	    event->xproperty.atom == pending->atom) {
		pending->response = PENDING_OK;
	} else if (event->type == DestroyNotify &&
		   event->xdestroywindow.window == pending->win) {
		pending->response = PENDING_DESTROY;
	}

	return 0;
}

/*
 * Participate in the Netscape locking protocol so our commands don't
 * collide
 */

static int
GetLock(Tcl_Interp *interp, Display *d, Window win, PendingCmd *pending,
	int timeout, NetscapeAtoms *atoms)
{
	char lock_data[255];
	Bool locked = False;
	Tk_RestrictProc *prevRestrict;
	ClientData prevArgs;
	Tcl_TimerToken token;

	sprintf(lock_data, "TkApp-pid%d@", (int) getpid());
	if (gethostname(lock_data + strlen(lock_data), 100) == -1) {
		Tcl_AppendResult(interp, "gethostname() returned an error",
				 (char *) NULL);
		return 0;
	}

	do {
		int result;
		Atom actual_type;
		int actual_format;
		unsigned long nitems, bytes_after;
		unsigned char *data = NULL;

		/*
		 * Grab the server so nobody else can do anything
		 */

		XGrabServer(d);

		/*
		 * See if it's locked
		 */

		result = XGetWindowProperty(d, win, atoms->XA_MOZILLA_LOCK,
					    0, (65536 / sizeof(long)),
					    False, XA_STRING,
					    &actual_type, &actual_format,
					    &nitems, &bytes_after,
					    &data);

		if (result != Success || actual_type == None) {
			/*
			 * It's not locked now, lock it!
			 */

			 XChangeProperty(d, win, atoms->XA_MOZILLA_LOCK,
					 XA_STRING,
					 8, PropModeReplace,
					 (unsigned char *) lock_data,
					 strlen(lock_data));
			locked = True;
		}

		/*
		 * Release the server grab
		 */

		XUngrabServer(d);
		XSync(d, False);

		if (! locked) {
			/*
			 * There was already a lock in place.  Wait for
			 * a PropertyDelete event.  Use a RestrictProc
			 * to make sure we're synchronous
			 */

			prevRestrict = Tk_RestrictEvents(NetscapeRestrict,
						(ClientData) pending,
						&prevArgs);
			pending->win = win;
			pending->state = PropertyDelete;
			pending->atom = atoms->XA_MOZILLA_LOCK;
			pending->response = 0;
			token = Tcl_CreateTimerHandler(timeout, LockTimeout,
						       (ClientData) pending);
			while (!pending->response) {
				Tcl_DoOneEvent(TCL_ALL_EVENTS);
			}
			Tcl_DeleteTimerHandler(token);
			Tk_RestrictEvents(prevRestrict, prevArgs, &prevArgs);

			if (pending->response == PENDING_TIMEOUT) {
				Tcl_AppendResult(interp, "Timeout waiting for "
						 "locked to be released",
						 (char *) NULL);
				if (data) {
					Tcl_AppendResult(interp, " by ",
							 data, (char *) NULL);
					XFree(data);
				}
				break;
			} else if (pending->response == PENDING_DESTROY) {
				Tcl_AppendResult(interp, "Window was destoyed "
						 "while trying to get lock",
						 (char *) NULL);
				if (data)
					XFree(data);
				break;
			}
		}

		if (data)
			XFree(data);
	} while (! locked);

	return locked == True ? 1 : 0;
}

/*
 * Unlock our lock with Netscape.  We should check for errors, but this
 * routine doesn't
 */

static void
ReleaseLock(Display *d, Window win, NetscapeAtoms *atoms)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *data;

	XGetWindowProperty(d, win, atoms->XA_MOZILLA_LOCK, 0,
			   65536 / sizeof(long), True /* delete */,
			   XA_STRING, &actual_type, &actual_format,
			   &nitems, &bytes_after, &data);
		
	if (data)
		XFree(data);
}

static Tk_RestrictAction
NetscapeRestrict(ClientData clientData, XEvent *event)
{
	PendingCmd *pending = (PendingCmd *) clientData;

	if ((event->type != PropertyNotify ||
	     event->xproperty.window != pending->win ||
	     event->xproperty.atom != pending->atom) &&
	    (event->type != DestroyNotify ||
	     event->xdestroywindow.window != pending->win)) {
		return TK_DEFER_EVENT;
	}

	return TK_PROCESS_EVENT;
}

static void
LockTimeout(ClientData clientData)
{
	PendingCmd *pending = (PendingCmd *) clientData;

	pending->response = PENDING_TIMEOUT;
}
