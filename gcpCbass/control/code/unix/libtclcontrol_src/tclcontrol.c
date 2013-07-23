#include <iostream>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/socket.h>

#include <tcl.h>
#include <math.h>

#include "tcpip.h"
#include "lprintf.h"
#include "control.h"
#include "netbuf.h"
#include "netobj.h"
#include "tclcontrol.h"

#include "gcp/util/common/ArrayDataFrameManager.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogMsgHandler.h"
#include "gcp/util/common/PagerMonitor.h"
#include "gcp/util/common/PeriodicTimer.h"
#include "gcp/util/common/PointingTelescopes.h"
#include "gcp/util/common/SshTunnel.h"
#include "gcp/util/common/SpecificName.h"

#include "gcp/grabber/common/Channel.h"

#include <sstream>

using namespace gcp::control;

/*
 * Does the current version of tcl use file handles or fds?
 */
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

typedef enum {
  TC_HOST,              // Set or query the server host address 

  TC_SEND,              // A command string to be sent to the szacontrol 

  TC_LOG_ARRAY,         // Register an array in which to record log messages. 

  TC_REPLY_ARRAY,       // Register an array in which to record reply messages. 

  TC_PAGECOND_ARRAY,    // Register an array in which to record pager conditions

  TC_CMD_TIMEOUT_ARRAY, // Register an array in which to record
			// command timeout messages
  TC_DATA_TIMEOUT_ARRAY,// Register an array in which to record data
			// timeout messages
  TC_SCHED_VAR,         // Register a variable in which to record scheduler 
                        // status messages. 
  TC_ARCHIVER_VAR,      // Register a variable in which to record archiver 
                        // status messages. 
  TC_VIEWERNAME_VAR,    // Register a variable in which to record the
		        // vewer name status messages.
  TC_EXPNAME_VAR,       // Register a variable in which to record the
		        // experiment name status messages.
  TC_PAGER_VAR,         // Register a variable in which to record the
			// paging status status messages.
  TC_PAGER_REG_VAR,     // Register a variable in which to record
			// which register activated the pager 

  TC_GRABBER_VAR,       // Register a variable in which to record the
  		        // grabber parameters' status
  TC_ANT_VAR,           // Register a variable in which to record the
                        // default antenna selection status messages.
  TC_HOST_CALLBACK,     // Register a callback to changes in connection state 

  TC_STATUS,            // Notify the C layer that the control
			// connection has been lost (or established)

  TC_LISTEN             // Initiate the Tcl event loop 

} TcCommandType;

/*
 * Enumerate the provided command names.
 */
typedef struct {
  TcCommandType type; /* The enumerated version of the command */
  char *name;         /* The TCL symbol used to select the command */
} TcCommand;

static TcCommand tc_commands[] = {
  {TC_HOST,               "host"},
  {TC_SEND,               "send"},
  {TC_LOG_ARRAY,          "log_array"},
  {TC_REPLY_ARRAY,        "reply_array"},
  {TC_PAGECOND_ARRAY,     "pageCond_array"},
  {TC_CMD_TIMEOUT_ARRAY,  "cmdTimeout_array"},
  {TC_DATA_TIMEOUT_ARRAY, "dataTimeout_array"},
  {TC_SCHED_VAR,          "sched_var"},
  {TC_ARCHIVER_VAR,       "archiver_var"},
  {TC_EXPNAME_VAR,        "expname_var"},
  {TC_VIEWERNAME_VAR,     "viewername_var"},
  {TC_PAGER_VAR,          "pager_var"},
  {TC_PAGER_REG_VAR,      "pagerReg_var"},
  {TC_ANT_VAR,            "ant_var"},
  {TC_GRABBER_VAR,        "grabber_var"},
  {TC_HOST_CALLBACK,      "host_callback"},
  {TC_STATUS,             "status"},
  {TC_LISTEN,             "listen"},
};

/*
 * The following structure is used by tc_event_loop().
 */
typedef struct {
  int active;              /* True if the event loop is running */
  Tcl_Channel channel;     /* The stdin channel */
  Tcl_DString command;     /* The one or more lines of the latest command */
} TcEventLoop;

namespace gcp {
  namespace tcl {
    class Retry {
    public:
      std::string* host_;
      std::string* gateway_;
      unsigned timeout_;
      bool enabled_;
      gcp::util::Mutex* guard_;
      
      Retry() 
	{
	  enabled_ = false;
	  host_ = 0;
	  gateway_ = 0;
	  guard_ = 0;
	  
	  host_    = new std::string();
	  gateway_ = new std::string();
	  guard_   = new gcp::util::Mutex();
	}
      
      ~Retry() 
	{
	  if(host_)
	    delete host_;
	  if(gateway_)
	    delete gateway_;
	  if(guard_)
	    delete guard_;
	}
      
      void setTo(std::string host, std::string gateway, 
		 unsigned timeout, bool enabled) 
      {
	guard_->lock();
	
	*host_    = host;
	*gateway_ = gateway;
	timeout_ = timeout;
	enabled_ = enabled;
	
	guard_->unlock();
      }
      
      void copyArgs(std::string& host, std::string& gateway, unsigned& timeout)
      {
	guard_->lock();
	
	host    = *host_;
	gateway = *gateway_;
	timeout = timeout_;
	
	guard_->unlock();
      }
      
      bool enabled() 
      {
	bool retval=false;
	
	guard_->lock();
	retval = enabled_;
	guard_->unlock();
	
	return retval;
      }
    };
    
  };
};

/*
 * Objects of the following type record the resources of each package instance.
 */
struct TclControl {
  Tcl_Interp *interp;      // The Tcl interpreter 
  Tcl_HashTable commands;  // The symbol table of package commands 
  char *host;              // The address of the remote server 
  int port;                // The TCP/IP port of the remote server 
  int fd;                  // The socket file descriptor. -1 if not connected 
  NetSendStr *nss;         // The output iterator 
  NetReadStr *nrs;         // The input iterator 
  char *log_array;         // The name of the Tcl array in which to 
                           //  record log messages. 
  char *reply_array;       // The name of the Tcl array in which to 
                           //  record reply messages. 
  char *pageCond_array;    // The name of the Tcl array in which to 
                           //  record pager messages. 
  char *cmdTimeout_array;  // The name of the Tcl array in which to 
                           //  record command timeout pager messages. 
  char *dataTimeout_array; // The name of the Tcl array in which to 
                           //  record data timeout pager messages. 
  char *sched_var;         // The name of the Tcl variable in which to 
                           //  record scheduler status messages. 
  char *archiver_var;      // The name of the Tcl variable in which to 
                           //  record archiver status messages. 
  char *expname_var;       // The name of the Tcl variable in which
			   // record the experiment name
  char *viewername_var;    // The name of the Tcl variable in which to
			   // record the viewer name
  char *pager_var;         // The name of the Tcl variable in which to 
                           //  record pager status messages. 
  char *pagerReg_var;      // The name of the Tcl variable in which to 
                           //  record which register activated the pager 
  char *ant_var;           // The name of the Tcl variable in which to 
                           //  record changes in the default antenna selection 
  struct {
    char* channel_var;
    char* redraw_var;
    char* reconf_var;
    char* flatfield_var;
    char* aspect_var;
    char* fov_var;
    char* collimation_var;
    char* combine_var;
    char* ptel_var;
    char* ximdir_var;
    char* yimdir_var;
    char* dkRotSense_var;
    char* xpeak_var;
    char* ypeak_var;
    char* ixmin_var;
    char* ixmax_var;
    char* iymin_var;
    char* iymax_var;
    char* inc_var;
    char* rem_var;
  } grabber;

  char *host_callback;     /* The command that is called when host changes */
#ifdef USE_TCLFILE
  Tcl_File tclfile;        /* A Tcl wrapper around fd */
#else
  int fd_registered;       /* True if a file handler has been created for fd */
#endif
  TcEventLoop loop;        /* The context of tc_event_loop() */

  gcp::util::LogMsgHandler* logMsgHandler_;

  // A struct that we will use to manage pager registers

  struct TclPagerManager {

    TclPagerManager(TclControl* tc);
    ~TclPagerManager();

    void add(std::string regName,
	     double min, double max, bool delta, bool outOfRange, unsigned nFrame);

    void remove(std::string regName);
    void clear();
    void update();

    TclControl* tc_;
    gcp::util::PagerMonitor* pm_;
    gcp::util::ArrayDataFrameManager* fm_;    
  };

  TclPagerManager* pagerManager_;

  gcp::util::SshTunnel* controlTunnel_;
  gcp::util::PeriodicTimer* connectTimer_;
  gcp::tcl::Retry* retry_;
};

static PERIODIC_TIMER_HANDLER(retryConnection);
static void enableRetryTimer(TclControl* tc, bool enable);

static TclControl *new_TclControl(Tcl_Interp *interp, int port);
static TclControl *del_TclControl(TclControl *tc);

static void delete_TclControl(ClientData context);

#ifdef USE_CHAR_DECL
static int service_TclControl(ClientData context, Tcl_Interp *interp,
			      int argc, char *argv[]);
#else
static int service_TclControl(ClientData context, Tcl_Interp *interp,
			      int argc, const char *argv[]);
#endif

static int tc_host(TclControl *tc, Tcl_Interp *interp, int argc, char *argv[]);
static int tc_status(TclControl *tc, Tcl_Interp *interp, int argc, char *argv[]);
static int tc_send(TclControl *tc, Tcl_Interp *interp, int argc, char *argv[]);
static int tc_event_loop(TclControl *tc, Tcl_Interp *interp,
			 int argc, char *argv[]);

static int tc_log_array(TclControl *tc, Tcl_Interp *interp,
			int argc, char *argv[]);
static int tc_reply_array(TclControl *tc, Tcl_Interp *interp,
			  int argc, char *argv[]);
static int tc_pageCond_array(TclControl *tc, Tcl_Interp *interp,
			  int argc, char *argv[]);
static int tc_cmdTimeout_array(TclControl *tc, Tcl_Interp *interp,
			       int argc, char *argv[]);
static int tc_dataTimeout_array(TclControl *tc, Tcl_Interp *interp,
				int argc, char *argv[]);
static int tc_sched_var(TclControl *tc, Tcl_Interp *interp,
			int argc, char *argv[]);
static int tc_archiver_var(TclControl *tc, Tcl_Interp *interp,
			   int argc, char *argv[]);
static int tc_expname_var(TclControl *tc, Tcl_Interp *interp,
			  int argc, char *argv[]);
static int tc_viewername_var(TclControl *tc, Tcl_Interp *interp,
			     int argc, char *argv[]);
static int tc_pager_var(TclControl *tc, Tcl_Interp *interp,
			int argc, char *argv[]);
static int tc_pagerReg_var(TclControl *tc, Tcl_Interp *interp,
                           int argc, char *argv[]);
static int tc_ant_var(TclControl *tc, Tcl_Interp *interp,
		      int argc, char *argv[]);
static int tc_grabber_var(TclControl *tc, Tcl_Interp *interp,
			  int argc, char *argv[]);
static int tc_host_callback(TclControl *tc, Tcl_Interp *interp,
			    int argc, char *argv[]);

static int is_empty_string(char *s);

static int tc_connect(TclControl *tc, Tcl_Interp *interp, 
		      char *host, char* gateway, unsigned timeout);
static int tc_disconnect(TclControl *tc, Tcl_Interp *interp);

static int tc_check_connection(TclControl *tc, char *caller,
			       Tcl_Interp *interp);
static void tc_server_event(ClientData client_data, int mask);

static void tc_read_stdin(ClientData client_data, int mask);
static void tc_prep_stdin(TclControl *tc);
static void updateGrabberTclVariables(TclControl* tc, Tcl_Interp* interp, 
				      CcNetMsg* netmsg);

/*.......................................................................
 * This is the TCL control client package-initialization function.
 *
 * Input:
 *  interp     Tcl_Interp *  The TCL interpreter.
 * Output:
 *  return            int    TCL_OK    - Success.
 *                           TCL_ERROR - Failure.           
 */
int Tclcontrol_Init(Tcl_Interp *interp)
{
  TclControl *tc;   /* The resource object of the package */
  /*
   * Check arguments.
   */
  if(!interp) {
    fprintf(stderr, "Tclcontrol_Init: NULL interpreter.\n");
    return TCL_ERROR;
  };
  /*
   * Allocate the resource object of the package.
   */
  tc = new_TclControl(interp, CP_CONTROL_PORT);
  if(!tc)
    return TCL_ERROR;

  /*
   * Create the Tcl command that users use to interact with the package.
   */
  Tcl_CreateCommand(interp, "control", service_TclControl, (ClientData) tc,
		    delete_TclControl);

  return TCL_OK;
}

/*.......................................................................
 * Allocate the resources of a TclControl package.
 *
 * Input:
 *  interp      Tcl_Interp *  The Tcl interpreter associated with the
 *                            database.
 *  port               int    The TCP/IP port number used by the package
 *                            database exploder.
 * Output:
 *  return       TclControl *  The new resource object, or NULL on error.
 */
static TclControl *new_TclControl(Tcl_Interp *interp, int port)
{
  TclControl *tc;    /* The resource object to be returned */
  int i;
  /*
   * Allocate the container.
   */
  tc = (TclControl *) malloc(sizeof(TclControl));
  if(!tc) {
    fprintf(stderr, "new_TclControl: Insufficient memory for container.\n");
    return NULL;
  };

  /*
   * Before attempting any operation that might fail, initialize the
   * container at least up to the point at which it can safely be passed
   * to del_TclControl().
   */
  tc->interp = interp;
  Tcl_InitHashTable(&tc->commands, TCL_STRING_KEYS);
  tc->host = NULL;
  tc->port = port;
  tc->fd = -1;
  tc->nss = NULL;
  tc->nrs = NULL;
  tc->log_array = NULL;
  tc->reply_array = NULL;

  tc->pageCond_array = NULL;
  tc->cmdTimeout_array = NULL;
  tc->dataTimeout_array = NULL;

  tc->sched_var      = NULL;
  tc->archiver_var   = NULL;
  tc->pager_var      = NULL;
  tc->pagerReg_var   = NULL;
  tc->expname_var    = NULL;
  tc->viewername_var = NULL;
  tc->ant_var        = NULL;

  tc->grabber.channel_var     = NULL;
  tc->grabber.redraw_var      = NULL;
  tc->grabber.reconf_var      = NULL;
  tc->grabber.combine_var     = NULL;
  tc->grabber.ptel_var        = NULL;
  tc->grabber.fov_var         = NULL;
  tc->grabber.flatfield_var   = NULL;
  tc->grabber.aspect_var      = NULL;
  tc->grabber.collimation_var = NULL;
  tc->grabber.ximdir_var      = NULL;
  tc->grabber.yimdir_var      = NULL;
  tc->grabber.dkRotSense_var  = NULL;
  tc->grabber.xpeak_var       = NULL;
  tc->grabber.ypeak_var       = NULL;
  tc->grabber.ixmin_var       = NULL;
  tc->grabber.ixmax_var       = NULL;
  tc->grabber.iymin_var       = NULL;
  tc->grabber.iymax_var       = NULL;
  tc->grabber.inc_var         = NULL;
  tc->grabber.rem_var         = NULL;

  tc->host_callback = NULL;

#ifdef USE_TCLFILE
  tc->tclfile = NULL;
#else
  tc->fd_registered = 0;
#endif

  tc->loop.active    = 0;
  tc->loop.channel   = NULL;
  tc->logMsgHandler_ = 0;
  tc->pagerManager_  = 0;
  tc->controlTunnel_ = 0;
  tc->connectTimer_  = 0;
  tc->retry_         = 0;

  Tcl_DStringInit(&tc->loop.command);

  /*
   * Create the network input stream.
   */
  tc->nrs = new_NetReadStr(-1, NET_PREFIX_LEN +
			   net_max_obj_size(&cc_msg_table));
  if(!tc->nrs)
    return del_TclControl(tc);
  /*
   * Create the network output stream.
   */
  tc->nss = new_NetSendStr(-1, NET_PREFIX_LEN +
			   net_max_obj_size(&cc_cmd_table));
  if(!tc->nss)
    return del_TclControl(tc);
  /*
   * Arrange a hash table that hashes command names to enumerators.
   */
  for(i=0; i <(int)(sizeof(tc_commands)/sizeof(tc_commands[0])); i++) {
    int wasnew;
    Tcl_HashEntry *hash = Tcl_CreateHashEntry(&tc->commands,
					      tc_commands[i].name, &wasnew);
    Tcl_SetHashValue(hash, (ClientData) &tc_commands[i]);
  };

  tc->logMsgHandler_ = new gcp::util::LogMsgHandler();

  if(!tc->logMsgHandler_)
    return del_TclControl(tc);

  tc->pagerManager_ = new TclControl::TclPagerManager(tc);

  if(!tc->pagerManager_)
    return del_TclControl(tc);

  tc->retry_ = new gcp::tcl::Retry();

  if(!tc->retry_)
    return del_TclControl(tc);

  tc->connectTimer_ = new gcp::util::PeriodicTimer();

  if(!tc->connectTimer_)
    return del_TclControl(tc);

  tc->connectTimer_->spawn();

  return tc;
}

/*.......................................................................
 * Delete the resource object of a Tcl exploder database.
 *
 * Input:
 *  tc     TclControl *   The object to be deleted.
 * Output:
 *  return TclControl *   The deleted object (always NULL).
 */
static TclControl *del_TclControl(TclControl *tc)
{
  if(tc) {
    Tcl_DeleteHashTable(&tc->commands);
    tc_disconnect(tc, tc->interp);
    tc->nrs = del_NetReadStr(tc->nrs);
    tc->nss = del_NetSendStr(tc->nss);

    if(tc->log_array)
      free(tc->log_array);

    if(tc->reply_array)
      free(tc->reply_array);

    if(tc->pageCond_array)
      free(tc->pageCond_array);

    if(tc->cmdTimeout_array)
      free(tc->cmdTimeout_array);

    if(tc->dataTimeout_array)
      free(tc->dataTimeout_array);

    if(tc->sched_var)
      free(tc->sched_var);

    if(tc->archiver_var)
      free(tc->archiver_var);

    if(tc->expname_var)
      free(tc->expname_var);

    if(tc->pager_var)
      free(tc->pager_var);

    if(tc->pagerReg_var)
      free(tc->pagerReg_var);

    if(tc->ant_var)
      free(tc->ant_var);

    if(tc->host_callback)
      free(tc->host_callback);

    if(tc->host)
      free(tc->host);

    if(tc->logMsgHandler_) {
      delete tc->logMsgHandler_;
      tc->logMsgHandler_ = 0;
    }

    if(tc->pagerManager_) {
      delete tc->pagerManager_;
      tc->pagerManager_ = 0;
    }

    if(tc->controlTunnel_) {
      delete tc->controlTunnel_;
      tc->controlTunnel_ = 0;
    }

    if(tc->connectTimer_) {
      delete tc->connectTimer_;
      tc->connectTimer_ = 0;
    }

    if(tc->retry_) {
      delete tc->retry_;
      tc->retry_ = 0;
    }

    free(tc);
  };

  return NULL;
}

/*.......................................................................
 * A Tcl_DeleteProc() wrapper around del_TclControl().
 *
 * Input:
 *  context  ClientData   The TclControl object registered to the
 *                        command by Tcl_NetObjInit().
 */
static void delete_TclControl(ClientData context)
{
  TclControl *tc = (TclControl *) context;
  tc = del_TclControl(tc);
}

/*.......................................................................
 * This function is called whenever the Tcl command registered by
 * ControlInit() is invoked.
 *
 * Input:
 *  context  ClientData     The TclControl resource object registered to
 *                          the TCL command by Tclcontrol_Init().
 *  interp   Tcl_Interp *   The TCL interpreter instance.
 *  argc            int     The number of arguments in argv[].
 *  argv           char *[] TCL arguments including the name of the
 *                          calling TCL command, followed by one of:
 *
 *                           host <address>
 *                             Set or query the remote server host.
 *                             If <address> is omitted then
 *                             the current host address will be
 *                             returned, or "" if no server is
 *                             currently connected. If <address>
 *                             is given then any existing connection
 *                             will first be terminated, then if
 *                             <address>!="" then a new connection
 *                             will be attempted to the specified
 *                             address.
 *
 *                           send <command>
 *                             Send a string command to the SZA
 *                             control program.
 *
 *                           log_array <array>
 *                             Register the global or fully qualified
 *                             name of the TCL array in which to
 *                             record log messages from the control
 *                             program. The log text will be stored in
 *                             array(text). The message status will be
 *                             stored in array(is_error_msg).  To
 *                             receive notification of updates the
 *                             caller should put a trace on
 *                             array(text).
 *                             Specify "" to unregister the callback.
 *
 *                           reply_array <array>
 *                             Register the global or fully qualified
 *                             name of the TCL array in which to
 *                             record command replies from the control
 *                             program. The text of the reply will be
 *                             stored in array(text). The message
 *                             status will be stored in
 *                             array(is_error_msg).  To receive
 *                             notification of updates the caller
 *                             should put a trace on array(text).
 *                             Specify "" to unregister the array.
 *
 *                           sched_var <var>
 *                             Register the global or fully qualified
 *                             name of the TCL variable in which to
 *                             record status messages received from
 *                             the scheduler. To receive notification
 *                             of updates the caller should put a trace on
 *                             var.
 *                             Specify "" to unregister the variable.
 *
 *                           archiver_var <var>
 *                             Register the global or fully qualified
 *                             name of the TCL variable in which to
 *                             record status messages received from
 *                             the archiver. To receive notification
 *                             of updates the caller should put a trace on
 *                             var.
 *                             Specify "" to unregister the variable.
 *
 *                           pager_var <var>
 *                             Register the global or fully qualified
 *                             name of the TCL variable in which to
 *                             record status messages received about
 *                             the pager. To receive notification
 *                             of updates the caller should put a trace on
 *                             var.
 *                             Specify "" to unregister the variable.
 *
 *                           ant_var <var>
 *                             Register the global or fully qualified
 *                             name of the TCL variable in which to
 *                             record status messages received about
 *                             the default antenna selection. To receive 
 *                             notification of updates the caller should put a 
 *                             trace on var.
 *                             Specify "" to unregister the variable.
 *
 *                           host_callback <command>
 *                             Register a Tcl command to be
 *                             invoked whenever a connection to the
 *                             server is made or lost. Use the host
 *                             command to actually get the status.
 *                             Specify "" to remove the callback.
 * Output:
 *  return          int      TCL_OK or TCL_ERROR.
 */
#ifdef USE_CHAR_DECL
static int service_TclControl(ClientData context, Tcl_Interp *interp,
			      int argc, char *argv[])
#else
     static int service_TclControl(ClientData context, Tcl_Interp *interp,
				   int argc, const char *argv[])
#endif
{
  TclControl *tc = (TclControl *) context;
  Tcl_HashEntry *hash;  /* A hash-table entry */
  TcCommand *cmd;       /* The selected command */
  int i;
  /*
   * We must have at least one command argument.
   */
  if(argc < 2) {
    Tcl_SetResult(interp, "Wrong number of arguments.", TCL_STATIC);
    return TCL_ERROR;
  };
  /*
   * Lookup the command specified by the first argument.
   */
  hash = Tcl_FindHashEntry(&tc->commands, argv[1]);
  if(!hash) {
    Tcl_AppendResult(interp, argv[0], ": Unknown command name: \"",
		     argv[1], "\"\nShould be one of:", NULL);
    for(i=0; i < (int)(sizeof(tc_commands)/sizeof(tc_commands[0])); i++)
      Tcl_AppendResult(interp, " ", tc_commands[i].name, NULL);
    return TCL_ERROR;
  };
  
  // Hand interpretation of the command to a suitable function.

  cmd = (TcCommand* )Tcl_GetHashValue(hash);

  switch(cmd->type) {
  case TC_HOST:
    return tc_host(         tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_SEND:
    return tc_send(         tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_LOG_ARRAY:
    return tc_log_array(    tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_REPLY_ARRAY:
    return tc_reply_array(  tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_PAGECOND_ARRAY:
    return tc_pageCond_array(tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_CMD_TIMEOUT_ARRAY:
    return tc_cmdTimeout_array(tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_DATA_TIMEOUT_ARRAY:
    return tc_dataTimeout_array(tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_SCHED_VAR:
    return tc_sched_var(    tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_ARCHIVER_VAR:
    return tc_archiver_var( tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_VIEWERNAME_VAR:
    return tc_viewername_var(  tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_EXPNAME_VAR:
    return tc_expname_var(  tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_PAGER_VAR:
    return tc_pager_var(    tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_PAGER_REG_VAR:
    return tc_pagerReg_var( tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_ANT_VAR:
    return tc_ant_var(      tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_GRABBER_VAR:
    return tc_grabber_var(  tc, interp, argc - 2, (char**)(argv + 2));
    break;
  case TC_HOST_CALLBACK:
    return tc_host_callback(tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_STATUS:
    return tc_status(tc, interp, argc - 2, (char** )(argv + 2));
    break;
  case TC_LISTEN:
    return tc_event_loop(   tc, interp, argc - 2, (char** )(argv + 2));
    break;
  default:
    Tcl_AppendResult(interp, argv[0], ": Missing case for command: ", 
		     argv[1], NULL);
						   return TCL_ERROR;
						   break;
  };
  return TCL_OK;
}

/*.......................................................................
 * Set or query the current control-program host address.
 *
 * Input:
 *  tc       TclControl *  The package-instance resource object.
 *  interp   Tcl_Interp *  The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[] (0 or 1).
 *  argv          char **  [0]  - If this argument is omitted, the
 *                                current host address will be
 *                                returned. This will be "" if no
 *                                connection currently exists.
 *                                Otherwise, any existing connection
 *                                will be terminated, then if this
 *                                argument is not an empty string, a
 *                                new connection to the given host
 *                                address will be attemped.
 * Output:
 *  return         int     TCL_OK    - Success.
 *                         TCL_ERROR - Error. Message recorded in
 *                                     interp->result[].
 */
static int tc_host(TclControl *tc, Tcl_Interp *interp, int argc, char *argv[])
{
  // Wrong number of arguments?

  if(argc != 0 && argc != 4) {
    Tcl_AppendResult(interp, "Wrong number of arguments to the host command.",
		     NULL);
    return TCL_ERROR;
  };
  
  // If no address argument was provided, return the current host
  // name.

  if(argc == 0) {
    Tcl_AppendResult(interp, tc->host ? tc->host : "", NULL);
    return TCL_OK;
  };
  
  // If a non-empty host name was provided, attempt to connect to the
  // exploder there. Otherwise disconnect.
  
  if(is_empty_string(argv[0])) {

    return tc_disconnect(tc, interp);

  } else {

    // Get arguments

    char* host = argv[0];

    int retry;
    if(Tcl_GetInt(interp, argv[1], &retry)==TCL_ERROR)
      return TCL_ERROR;

    char* gateway = argv[2];

    int timeout;
    if(Tcl_GetInt(interp, argv[3], &timeout)==TCL_ERROR)
      return TCL_ERROR;

    // Set information in the Retry object regardless of whether or
    // not auto-reconnect is enabled

    tc->retry_->setTo(host, gateway, timeout, retry); 

    // Attempt to connect

    int retVal = tc_connect(tc, interp, host, gateway, timeout);

    // If we failed, set the periodic timer to fire, and a handler to be called

    if(retVal == TCL_ERROR) 
      enableRetryTimer(tc, true);
    
    return retVal;
  }
}

/**.......................................................................
 * Activate or deactivate the retry timer
 */
void enableRetryTimer(TclControl* tc, bool enable)
{
  // If auto connect is not enabled, do nothing

  if(!tc->retry_->enabled()) 
    return;

  // Else enable or disable the timer

  if(enable) {
    tc->connectTimer_->addHandler(retryConnection, (void*)tc);
    tc->connectTimer_->enableTimer(true, 10);
  } else {
    tc->connectTimer_->enableTimer(false);
    tc->connectTimer_->removeHandler(retryConnection);
  }
}

/**.......................................................................
 * If requested to reconnect when the connection is lost, we set a
 * periodic timer.  On expiry of this timer, this function is called
 * to retry the connection.  If the connection succeeds, the timer is
 * disabled, else we keep trying.
 */
PERIODIC_TIMER_HANDLER(retryConnection) 
{
  TclControl* tc = (TclControl*)args;
  std::string host, gateway;
  unsigned timeout;

  tc->retry_->copyArgs(host, gateway, timeout);

  // Attempt to connect

  int retVal = tc_connect(tc, tc->interp, (char*)host.c_str(), (char*)gateway.c_str(), timeout);

  //  If the connection succeeded, disable the timer

  if(retVal == TCL_OK) 
    enableRetryTimer(tc, false);
}

/*.......................................................................
 * Implement the TCL send command.
 *
 * Input:
 *  tc      TclControl *   The package-instance resource object.
 *  interp  Tcl_Interp *   The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[].
 *  argv          char **  [0]  - The command string to be sent to the
 *                                control program.
 * Output:
 *  return         int    TCL_OK    - Success.
 *                        TCL_ERROR - Error. Message recorded in
 *                                    interp->result[].
 */
static int tc_send(TclControl *tc, Tcl_Interp *interp, int argc, char *argv[])
{
  char *command;   /* The command string to be sent */
  CcNetCmd netcmd; /* The encapsulated command, ready to be sent */
  int state;       /* The return status of nss_send_msg() */
  /*
   * Get the output network buffer.
   */
  NetBuf *net = tc->nss->net;
  /*
   * Complain if there is no connection to the remote server.
   */
  if(tc_check_connection(tc, "send", interp) == TCL_ERROR)
    return TCL_ERROR;
  /*
   * Get the command string that is to be sent.
   */
  if(argc < 1) {
    Tcl_AppendResult(interp, "send: Missing command string.",NULL);
    return TCL_ERROR;
  };
  command = argv[0];
  if(strlen(command) > CC_CMD_MAX) {
    Tcl_AppendResult(interp, "send: Command string too long.",NULL);
    return TCL_ERROR;
  };
  /*
   * Prepare the message to be sent.
   */
  strcpy(netcmd.input.cmd, command);
  /*
   * Pack the message into the output network buffer.
   */
  if(net_start_put(net, CC_INPUT_CMD) ||
     obj_to_net(&cc_cmd_table, net, CC_INPUT_CMD, &netcmd) ||
     net_end_put(net)) {
    Tcl_AppendResult(interp, "send: Error packing output message.", NULL);
    return TCL_ERROR;
  };
  /*
   * Send the object to the control program.
   */
  do {
    state = nss_send_msg(tc->nss);

    if(state == NetSendStr::NET_SEND_ERROR || 
       state == NetSendStr::NET_SEND_CLOSED) {
      Tcl_AppendResult(interp, "Unable to send output message.", NULL);
      /*
       * Disconnect from the server after communication errors.
       */
      tc_disconnect(tc, interp);
      return TCL_ERROR;
    };
  } while(state != NetSendStr::NET_SEND_DONE);
  return TCL_OK;
}

/*.......................................................................
 * After a call to this function, whenever log messages are received
 * from the control program their text and error status flag will be
 * recorded in two elements of the specified Tcl array.
 *
 * Input:
 *  tc      TclControl *   The package-instance resource object.
 *  interp  Tcl_Interp *   The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[].
 *  argv          char **  [0] - The global or fully qualified name of
 *                               the array in which to record
 *                               subsequent log messages. The text of
 *                               these messages will be recorded in
 *                               array(text) and the boolean error
 *                               status of the message will be stored
 *                               in array(is_error_msg). Of these
 *                               elements the (text) element is
 *                               updated last, so to be informed of
 *                               updates, the caller should place a
 *                               trace on array(text).
 * Output:
 *  return         int     TCL_OK    - Success.
 *                         TCL_ERROR - Error. Message recorded in
 *                                     interp->result[].
 */
static int tc_log_array(TclControl *tc, Tcl_Interp *interp,
			int argc, char *argv[])
{
  char *name;          /* The name of the TCL array */
  /*
   * Wrong number of arguments?
   */
  if(argc != 1) {
    Tcl_AppendResult(interp,
		     "Wrong number of arguments to the log_array command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];
  /*
   * If an array has previously been registered, remove it
   * first.
   */
  if(tc->log_array) {
    free(tc->log_array);
    tc->log_array = NULL;
  };
  /*
   * If a non-empty array name has been specified, record a copy of it.
   */
  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
		       "log_array: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    tc->log_array = strcpy(tmp, name);
  };
  return TCL_OK;
}

/*.......................................................................
 * After a call to this function, whenever textual replies to commands
 * are received from the control program their text and error status
 * flag will be recorded in two elements of the specified Tcl array.
 *
 * Input:
 *  tc      TclControl *   The package-instance resource object.
 *  interp  Tcl_Interp *   The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[].
 *  argv          char **  [0] - The global or fully qualified name of
 *                               the array in which to record
 *                               subsequent reply messages. The text
 *                               of these messages will be recorded in
 *                               array(text) and the boolean error
 *                               status of the message will be stored
 *                               in array(is_error_msg). Of these
 *                               elements the (text) element is
 *                               updated last, so to be informed of
 *                               updates, the caller should place a
 *                               trace on array(text).
 * Output:
 *  return         int     TCL_OK    - Success.
 *                         TCL_ERROR - Error. Message recorded in
 *                                     interp->result[].
 */
static int tc_reply_array(TclControl *tc, Tcl_Interp *interp,
			  int argc, char *argv[])
{
  char *name;          /* The name of the TCL array */
  /*
   * Wrong number of arguments?
   */
  if(argc != 1) {
    Tcl_AppendResult(interp,
		     "Wrong number of arguments to the reply_array command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];
  /*
   * If an array has previously been registered, remove it
   * first.
   */
  if(tc->reply_array) {
    free(tc->reply_array);
    tc->reply_array = NULL;
  };
  /*
   * If a non-empty array name has been specified, record a copy of it.
   */
  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
		       "reply_array: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    tc->reply_array = strcpy(tmp, name);
  };
  return TCL_OK;
}

/*.......................................................................
 * After a call to this function, whenever textual replies to commands
 * are received from the control program their text and error status
 * flag will be recorded in two elements of the specified Tcl array.
 *
 * Input:
 *  tc      TclControl *   The package-instance resource object.
 *  interp  Tcl_Interp *   The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[].
 *  argv          char **  [0] - The global or fully qualified name of
 *                               the array in which to record
 *                               subsequent pager messages. The text
 *                               of these messages will be recorded in
 *                               array(text) and the boolean error
 *                               status of the message will be stored
 *                               in array(is_error_msg). Of these
 *                               elements the (text) element is
 *                               updated last, so to be informed of
 *                               updates, the caller should place a
 *                               trace on array(text).
 * Output:
 *  return         int     TCL_OK    - Success.
 *                         TCL_ERROR - Error. Message recorded in
 *                                     interp->result[].
 */
static int tc_pageCond_array(TclControl *tc, Tcl_Interp *interp,
			int argc, char *argv[])
{
  char *name;          /* The name of the TCL array */

/*
 * Wrong number of arguments?
 */
  if(argc != 1) {
    Tcl_AppendResult(interp,
	   "Wrong number of arguments to the pagecond_var command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];
/*
 * If an array has previously been registered, remove it
 * first.
 */

  if(tc->pageCond_array) {
    free(tc->pageCond_array);
    tc->pageCond_array = NULL;
  };

/*
 * If a non-empty array name has been specified, record a copy of it.
 */
  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
	       "pageCond_array: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    tc->pageCond_array = strcpy(tmp, name);
  };

  return TCL_OK;
}


/**.......................................................................
 * After a call to this function, whenever textual replies to commands
 * are received from the control program their text and error status
 * flag will be recorded in two elements of the specified Tcl array.
 */
static int tc_cmdTimeout_array(TclControl *tc, Tcl_Interp *interp,
			       int argc, char *argv[])
{
  char *name;          /* The name of the TCL array */
  
  // Wrong number of arguments?

  if(argc != 1) {
    Tcl_AppendResult(interp,
	   "Wrong number of arguments to the pagecond_var command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];
  
  // If an array has previously been registered, remove it first.

  if(tc->cmdTimeout_array) {
    free(tc->cmdTimeout_array);
    tc->cmdTimeout_array = NULL;
  };
  
  // If a non-empty array name has been specified, record a copy of
  // it.

  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
	       "cmdTimeout_array: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    tc->cmdTimeout_array = strcpy(tmp, name);
  };
  return TCL_OK;
}

/**.......................................................................
 * After a call to this function, whenever textual replies to commands
 * are received from the control program their text and error status
 * flag will be recorded in two elements of the specified Tcl array.
 */
static int tc_dataTimeout_array(TclControl *tc, Tcl_Interp *interp,
			       int argc, char *argv[])
{
  char *name;          /* The name of the TCL array */
  
  // Wrong number of arguments?

  if(argc != 1) {
    Tcl_AppendResult(interp,
	   "Wrong number of arguments to the pagecond_var command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];
  
  // If an array has previously been registered, remove it first.

  if(tc->dataTimeout_array) {
    free(tc->dataTimeout_array);
    tc->dataTimeout_array = NULL;
  };
  
  // If a non-empty array name has been specified, record a copy of
  // it.

  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
	       "dataTimeout_array: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    tc->dataTimeout_array = strcpy(tmp, name);
  };
  return TCL_OK;
}

/*.......................................................................
 * After a call to this function, whenever status messages
 * are received from the scheduler their contents will be recorded in
 * the specified variable.
 *
 * Input:
 *  tc      TclControl *   The package-instance resource object.
 *  interp  Tcl_Interp *   The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[].
 *  argv          char **  [0] - The global or fully qualified name of
 *                               the Tcl variable in which to record
 *                               subsequent sched messages.
 * Output:
 *  return         int     TCL_OK    - Success.
 *                         TCL_ERROR - Error. Message recorded in
 *                                     interp->result[].
 */
static int tc_sched_var(TclControl *tc, Tcl_Interp *interp,
			int argc, char *argv[])
{
  char *name;          /* The name of the TCL array */
  /*
   * Wrong number of arguments?
   */
  if(argc != 1) {
    Tcl_AppendResult(interp,
		     "Wrong number of arguments to the sched_array command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];
  /*
   * If an array has previously been registered, remove it
   * first.
   */
  if(tc->sched_var) {
    free(tc->sched_var);
    tc->sched_var = NULL;
  };
  /*
   * If a non-empty array name has been specified, record a copy of it.
   */
  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
		       "sched_var: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    tc->sched_var = strcpy(tmp, name);
  };
  return TCL_OK;
}

/*.......................................................................
 * After a call to this function, whenever status messages
 * are received from the archiver their contents will be recorded in
 * the specified variable.
 *
 * Input:
 *  tc      TclControl *   The package-instance resource object.
 *  interp  Tcl_Interp *   The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[].
 *  argv          char **  [0] - The global or fully qualified name of
 *                               the Tcl variable in which to record
 *                               subsequent archiver messages.
 * Output:
 *  return         int     TCL_OK    - Success.
 *                         TCL_ERROR - Error. Message recorded in
 *                                     interp->result[].
 */
static int tc_archiver_var(TclControl *tc, Tcl_Interp *interp,
			   int argc, char *argv[])
{
  char *name;          /* The name of the TCL array */
  /*
   * Wrong number of arguments?
   */
  if(argc != 1) {
    Tcl_AppendResult(interp,
		     "Wrong number of arguments to the archiver_array command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];
  /*
   * If an array has previously been registered, remove it
   * first.
   */
  if(tc->archiver_var) {
    free(tc->archiver_var);
    tc->archiver_var = NULL;
  };
  /*
   * If a non-empty array name has been specified, record a copy of it.
   */
  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
		       "archiver_var: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    tc->archiver_var = strcpy(tmp, name);
  };
  return TCL_OK;
}
/*.......................................................................
 * After a call to this function, whenever status messages
 * are received from the pager their contents will be recorded in
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
static int tc_pager_var(TclControl *tc, Tcl_Interp *interp,
			int argc, char *argv[])
{
  char *name;          // The name of the TCL array 
  
  // Wrong number of arguments?

  if(argc != 1) {
    Tcl_AppendResult(interp,
		     "Wrong number of arguments to the pager_array command.", 
		     NULL);
    return TCL_ERROR;
  };

  name = argv[0];
  
  // If an array has previously been registered, remove it first.

  if(tc->pager_var) {
    free(tc->pager_var);
    tc->pager_var = NULL;
  };
  
  // If a non-empty array name has been specified, record a copy of
  // it.

  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
		       "pager_var: Insufficient memory for new callback.", 
		       NULL);
      return TCL_ERROR;
    };
    tc->pager_var = strcpy(tmp, name);
  };
  return TCL_OK;
}

static int tc_pagerReg_var(TclControl *tc, Tcl_Interp *interp,
                           int argc, char *argv[])
{
  char *name;          // The name of the TCL array

  // Wrong number of arguments?

  if(argc != 1) {
    Tcl_AppendResult(interp,
                     "Wrong number of arguments to the tc_pagerReg_var command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];

  // If an array has previously been registered, remove it first.

  if(tc->pagerReg_var) {
    free(tc->pagerReg_var);
    tc->pagerReg_var = NULL;
  };

  // If a non-empty array name has been specified, record a copy of
  // it.

  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
                       "pagerReg_var: Insufficient memory for new callback.", 
		       NULL);
      return TCL_ERROR;
   };

    tc->pagerReg_var = strcpy(tmp, name);
  };

  return TCL_OK;
}

/*.......................................................................
 * After a call to this function, whenever status messages
 * are received from the pager their contents will be recorded in
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
static int tc_expname_var(TclControl *tc, Tcl_Interp *interp,
			  int argc, char *argv[])
{
  char *name;          /* The name of the TCL array */
  /*
   * Wrong number of arguments?
   */
  if(argc != 1) {
    Tcl_AppendResult(interp,
		     "Wrong number of arguments to the expname_array command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];
  /*
   * If an array has previously been registered, remove it
   * first.
   */
  if(tc->expname_var) {
    free(tc->expname_var);
    tc->expname_var = NULL;
  };
  /*
   * If a non-empty array name has been specified, record a copy of it.
   */
  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
		       "expname_var: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    tc->expname_var = strcpy(tmp, name);
  };

  // Now set it

  if(Tcl_SetVar(interp, tc->expname_var, (char*)gcp::util::SpecificName::experimentName().c_str(),
		TCL_GLOBAL_ONLY) == NULL)
    return TCL_ERROR;

  return TCL_OK;
}

/*.......................................................................
 * After a call to this function, whenever status messages
 * are received from the pager their contents will be recorded in
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
static int tc_viewername_var(TclControl *tc, Tcl_Interp *interp,
			     int argc, char *argv[])
{
  char *name;          /* The name of the TCL array */
  /*
   * Wrong number of arguments?
   */
  if(argc != 1) {
    Tcl_AppendResult(interp,
		     "Wrong number of arguments to the viewername_array command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];
  /*
   * If an array has previously been registered, remove it
   * first.
   */
  if(tc->viewername_var) {
    free(tc->viewername_var);
    tc->viewername_var = NULL;
  };
  /*
   * If a non-empty array name has been specified, record a copy of it.
   */
  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
		       "viewername_var: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    tc->viewername_var = strcpy(tmp, name);
  };

  // Now set it

  if(Tcl_SetVar(interp, tc->viewername_var, 
		(char*)gcp::util::SpecificName::viewerName().c_str(),
		TCL_GLOBAL_ONLY) == NULL)
    return TCL_ERROR;

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
static int tc_grabber_var(TclControl *tc, Tcl_Interp *interp,
			  int argc, char *argv[])
{
  char *param;         /* The name of the TCL array */
  char *name;          /* The name of the TCL array */
  char** var=NULL;

  /*
   * Wrong number of arguments?
   */
  if(argc != 2) {
    Tcl_AppendResult(interp,
		     "Wrong number of arguments to the pager_array command.", NULL);
    return TCL_ERROR;
  };

  param = argv[0];
  name  = argv[1];

  if(strcmp(param, "channel")==0) 
    var = &tc->grabber.channel_var;
  else if(strcmp(param, "redraw")==0) 
    var = &tc->grabber.redraw_var;
  else if(strcmp(param, "reconf")==0) 
    var = &tc->grabber.reconf_var;
  else if(strcmp(param, "combine")==0) 
    var = &tc->grabber.combine_var;
  else if(strcmp(param, "ptel")==0) 
    var = &tc->grabber.ptel_var;
  else if(strcmp(param, "flatfield")==0) 
    var = &tc->grabber.flatfield_var;
  else if(strcmp(param, "aspect")==0) 
    var = &tc->grabber.aspect_var;
  else if(strcmp(param, "fov")==0) 
    var = &tc->grabber.fov_var;
  else if(strcmp(param, "collimation")==0) 
    var = &tc->grabber.collimation_var;
  else if(strcmp(param, "ximdir")==0) 
    var = &tc->grabber.ximdir_var;
  else if(strcmp(param, "yimdir")==0) 
    var = &tc->grabber.yimdir_var;
  else if(strcmp(param, "dkRotSense")==0) 
    var = &tc->grabber.dkRotSense_var;
  else if(strcmp(param, "xpeak")==0) 
    var = &tc->grabber.xpeak_var;
  else if(strcmp(param, "ypeak")==0) 
    var = &tc->grabber.ypeak_var;
  else if(strcmp(param, "ixmin")==0)
    var = &tc->grabber.ixmin_var;
  else if(strcmp(param, "ixmax")==0)
    var = &tc->grabber.ixmax_var;
  else if(strcmp(param, "iymin")==0)
    var = &tc->grabber.iymin_var;
  else if(strcmp(param, "iymax")==0)
    var = &tc->grabber.iymax_var;
  else if(strcmp(param, "inc")==0)
    var = &tc->grabber.inc_var;
  else if(strcmp(param, "rem")==0)
    var = &tc->grabber.rem_var;
  else {
    Tcl_AppendResult(interp,
		     "Unrecognized parameter", NULL);
    return TCL_ERROR;
  }
  
  /*
   * If an array has previously been registered, remove it
   * first.
   */
  if(*var) {
    free(*var);
    *var = NULL;
  };

  /*
   * If a non-empty array name has been specified, record a copy of it.
   */
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
 * After a call to this function, whenever status messages
 * are received from the ant their contents will be recorded in
 * the specified variable.
 *
 * Input:
 *  tc      TclControl *   The package-instance resource object.
 *  interp  Tcl_Interp *   The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[].
 *  argv          char **  [0] - The global or fully qualified name of
 *                               the Tcl variable in which to record
 *                               subsequent ant messages.
 * Output:
 *  return         int     TCL_OK    - Success.
 *                         TCL_ERROR - Error. Message recorded in
 *                                     interp->result[].
 */
static int tc_ant_var(TclControl *tc, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  char *name;          /* The name of the TCL array */
  /*
   * Wrong number of arguments?
   */
  if(argc != 1) {
    Tcl_AppendResult(interp,
		     "Wrong number of arguments to the ant_array command.", NULL);
    return TCL_ERROR;
  };
  name = argv[0];
  /*
   * If an array has previously been registered, remove it
   * first.
   */
  if(tc->ant_var) {
    free(tc->ant_var);
    tc->ant_var = NULL;
  };
  /*
   * If a non-empty array name has been specified, record a copy of it.
   */
  if(!is_empty_string(name)) {
    char *tmp = (char *) malloc(strlen(name) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
		       "ant_var: Insufficient memory for new callback.", NULL);
      return TCL_ERROR;
    };
    tc->ant_var = strcpy(tmp, name);
  };
  return TCL_OK;
}

/*.......................................................................
 * Change the TCL callback command that is invoked whenever a control
 * program connection is established or lost.
 *
 * Input:
 *  tc      TclControl *   The package-instance resource object.
 *  interp  Tcl_Interp *   The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[].
 *  argv          char **  [0] - The TCL callback command script,
 *                               or "" to simply remove the current
 *                               callback.
 * Output:
 *  return         int     TCL_OK    - Success.
 *                         TCL_ERROR - Error. Message recorded in
 *                                     interp->result[].
 */
static int tc_host_callback(TclControl *tc, Tcl_Interp *interp,
			    int argc, char *argv[])
{
  char *cmd;          /* The TCL callback command */
  /*
   * Wrong number of arguments?
   */
  if(argc != 1) {
    Tcl_AppendResult(interp,
		     "Wrong number of arguments to the host_callback command.",
		     NULL);
    return TCL_ERROR;
  };
  cmd = argv[0];
  /*
   * If an existing callback exists, remove it first.
   */
  if(tc->host_callback) {
    free(tc->host_callback);
    tc->host_callback = NULL;
  };
  /*
   * If a non-empty callback has been specified, record a copy of it.
   */
  if(!is_empty_string(cmd)) {
    char *tmp = (char *) malloc(strlen(cmd) + 1);
    if(!tmp) {
      Tcl_AppendResult(interp,
		       "host_callback: Insufficient memory for new callback.",
		       NULL);
      return TCL_ERROR;
    };

    tc->host_callback = strcpy(tmp, cmd);

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

/*.......................................................................
 * Connect to the control program.
 *
 * Input:
 *  tc       TclControl *   The resource object registered to the
 *                          TCL package command by Tclcontrol_Init().
 *  interp   Tcl_Interp *   The TCL interpreter instance.
 *  host           char *   The address of the remote server.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Error.
 */
static int tc_connect(TclControl *tc, Tcl_Interp *interp, 
		      char *host, char* gateway, unsigned timeout)
{
  // Make sure that any control tunnel is deleted before we attempt to
  // create a new one

  if(tc->controlTunnel_) {
    delete tc->controlTunnel_;
    tc->controlTunnel_ = 0;
  }

  // Make sure that we are disconnected before making a new
  // connection.

  if(tc_disconnect(tc, interp) == TCL_ERROR)
    return TCL_ERROR;

  // Attempt to connect to the remote server host.

  if(*gateway != '\0') {

    tc->controlTunnel_ =
      new gcp::util::SshTunnel(gateway, host, CP_CONTROL_PORT, timeout);

    if(tc->controlTunnel_->succeeded()) {
      tc->fd = tcp_connect("localhost", tc->port, 1);
    } else {
      Tcl_AppendResult(interp, tc->controlTunnel_->error().c_str(), NULL);
      delete tc->controlTunnel_;
      tc->controlTunnel_ = 0;
      return TCL_ERROR;
    }

  } else {
    tc->fd = tcp_connect(host, tc->port, 1);
  }

  if(tc->fd < 0) {
    std::ostringstream os;

    os << "Couldn't connect to " << gcp::util::SpecificName::controlName() 
       << " at ";

    Tcl_AppendResult(interp, os.str().c_str(), host, NULL);

    return TCL_ERROR;
  };
  
  // Record the name of the host.

  tc->host = (char *) malloc(strlen(host) + 1);
  if(!tc->host) {
    Tcl_AppendResult(interp, "tc_connect: Insufficient memory.\n");
    tc_disconnect(tc, interp);
    return TCL_ERROR;
  };
  strcpy(tc->host, host);
  /*
   * Connect the socket to the network input and output streams.
   */
  attach_NetSendStr(tc->nss, tc->fd);
  attach_NetReadStr(tc->nrs, tc->fd);
#ifdef USE_TCLFILE
  /*
   * Create a new Tcl file handle for the connection.
   */
  tc->tclfile = Tcl_GetFile((ClientData)tc->fd, TCL_UNIX_FD);
  if(!tc->tclfile) {
    Tcl_AppendResult(interp, "tc_connect: Tcl_GetFile failed.\n");
    tc_disconnect(tc, interp);
    return TCL_ERROR;
  };
#endif
  /*
   * Register a Tcl read-event handler for the connection.
   */
#if USE_TCLFILE
  Tcl_CreateFileHandler(tc->tclfile, TCL_READABLE, tc_server_event,
			(ClientData) tc);
#else
  Tcl_CreateFileHandler(tc->fd, TCL_READABLE, tc_server_event,
			(ClientData) tc);
  tc->fd_registered = 1;
#endif

  /*
   * Inform the Tcl application that the connection status may have changed.
   */
  if(tc->host_callback) 
    return Tcl_GlobalEval(interp, tc->host_callback);

  return TCL_OK;
}

/*.......................................................................
 * Disconnect from the remote server host.
 *
 * Input:
 *  tc       TclControl *   The package-instance resource object.
 *  interp   Tcl_Interp *   The TCL interpreter instance.
 * Output:
 *  return           int    TCL_OK    - Success.
 *                          TCL_ERROR - Error.
 */
static int tc_disconnect(TclControl *tc, Tcl_Interp *interp)
{
  int was_connected = 0;  /* True if a connection has to be broken */
  /*
   * Un-register any Tcl file handle and event handler.
   */
#ifdef USE_TCLFILE
  if(tc->tclfile) {
    Tcl_DeleteFileHandler(tc->tclfile);
    Tcl_FreeFile(tc->tclfile);
    tc->tclfile = NULL;
  };
#else
  if(tc->fd_registered) {
    Tcl_DeleteFileHandler(tc->fd);
    tc->fd_registered = 0;
  };
#endif
  if(tc->host) {
    free(tc->host);
    tc->host = NULL;
  };
  /*
   * Dettach the socket from the network input and output streams.
   */
  attach_NetSendStr(tc->nss, -1);
  attach_NetReadStr(tc->nrs, -1);
  /*
   * Close the connection to the remote server host.
   */
  if(tc->fd >= 0) {
    shutdown(tc->fd, 2);
    close(tc->fd);
    tc->fd = -1;
    was_connected = 1;
  };
  /*
   * Inform the Tcl application that the connection status may have changed.
   */
  if(was_connected && tc->host_callback)
    return Tcl_GlobalEval(interp, tc->host_callback);
  return TCL_OK;
}

/*.......................................................................
 * If no control-program connection currently exists, place an error
 * message in interp->result and return TCL_ERROR. Otherwise return TCL_OK.
 *
 * Input:
 *  tc        TclControl *   The package-instance resource object.
 *  caller         char *   The name of the calling command.
 *  interp   Tcl_Interp *   The TCL interpreter instance.
 * Output:
 *  return          int     TCL_OK or TCL_ERROR.
 */
static int tc_check_connection(TclControl *tc, char *caller, Tcl_Interp *interp)
{
  if(!tc->host) {
    Tcl_AppendResult(interp, caller, ": Not connected to the control program.",
		     NULL);
    return TCL_ERROR;
  };
  return TCL_OK;
}

/*.......................................................................
 * This function is called by the Tcl event dispatcher whenever the
 * control-program connection becomes readable.
 *
 * Input:
 *  client_data   ClientData   The TclControl object cast to ClientData.
 *  mask                 int   The type of event that triggered the
 *                             callback. In our case we have only
 *                             registered read events, so this should
 *                             be TCL_READABLE.
 */
static void tc_server_event(ClientData client_data, int mask)
{
  TclControl *tc = (TclControl *) client_data;
  Tcl_Interp *interp = tc->interp;
  CcNetMsg netmsg; /* The message received from the control program */
  int state;       /* The state of the input stream */
  int id;          /* The type of message received */
  /*
   * Read the next message.
   */
  do {
    state = nrs_read_msg(tc->nrs);
    /*
     * Disconnect from the server after communication errors.
     */
    if(state == NetReadStr::NET_READ_ERROR || 
       state == NetReadStr::NET_READ_CLOSED) {
      tc_disconnect(tc, interp);
      return;
    };
  } while(state != NetReadStr::NET_READ_DONE);
  /*
   * Decode the reply.
   */
  if(net_start_get(tc->nrs->net, &id) ||
     net_to_obj(&cc_msg_table, tc->nrs->net, id, &netmsg) ||
     net_end_get(tc->nrs->net)) {
    return;
  };
  /*
   * Determine what type of message we were sent.
   */
  switch((CcNetMsgId) id) {   /* The cast allows compiler checking */
  case CC_LOG_MSG:

    {
      unsigned seq = netmsg.log.seq;

      tc->logMsgHandler_->appendWithSpace(seq, netmsg.log.text);
	
      if(netmsg.log.end) {

	if(tc->log_array) {
	  if(Tcl_SetVar2(interp, tc->log_array, "is_error_msg",
			 (char*)(netmsg.log.error ? "1" : (netmsg.log.interactive ? "2" : "0")), 
			 TCL_GLOBAL_ONLY) == NULL ||
	     Tcl_SetVar2(interp, tc->log_array, "text", 
			 (char*)tc->logMsgHandler_->getMessage(seq).c_str(),
			 TCL_GLOBAL_ONLY) == NULL)
	    return;
	};

      }
    }

    break;

  case CC_REPLY_MSG:
    if(tc->reply_array) {
      if(Tcl_SetVar2(interp, tc->reply_array, "is_error_msg",
		     (char *)(netmsg.reply.error ? "1":"0"), 
		     TCL_GLOBAL_ONLY) == NULL ||
	 Tcl_SetVar2(interp, tc->reply_array, "text", netmsg.reply.text,
		     TCL_GLOBAL_ONLY) == NULL)
	return;
    };
    break;

  case CC_PAGECOND_MSG:

    switch (netmsg.pageCond.mode) {

    case PAGECOND_ADD:
      tc->pagerManager_->add(netmsg.pageCond.text, 
			     netmsg.pageCond.min,   netmsg.pageCond.max,
			     netmsg.pageCond.isDelta, netmsg.pageCond.isOutOfRange,
			     netmsg.pageCond.nFrame);
      break;
      
    case PAGECOND_REMOVE:
      tc->pagerManager_->remove(netmsg.pageCond.text);
      break;
      
    case PAGECOND_CLEAR:
      tc->pagerManager_->clear();
      break;
        
    case PAGECOND_UPDATE:
      tc->pagerManager_->update();
      break;

    default:
      COUT("Got an unrecognized message");
      break;
    }

    break;

  case CC_CMD_TIMEOUT_MSG:

    switch(netmsg.cmdTimeout.mode) {

    case CT_CMD_ACTIVE:
      if(tc->cmdTimeout_array) {
	if(Tcl_SetVar2(tc->interp, tc->cmdTimeout_array, "active",  
		       (char*)(netmsg.cmdTimeout.active ? "1" : "0"), 
		       TCL_GLOBAL_ONLY) == NULL)
	  return;
      }
      break;
      
    case CT_CMD_TIMEOUT:
      if(tc->cmdTimeout_array) {
	std::ostringstream os;
	os << netmsg.cmdTimeout.seconds;
	if(Tcl_SetVar2(tc->interp, tc->cmdTimeout_array, "seconds",  
		       (char*)os.str().c_str(), TCL_GLOBAL_ONLY) == NULL)
	  return;
      }
      break;
      
    case CT_DATA_ACTIVE:
      if(tc->dataTimeout_array) {
	if(Tcl_SetVar2(tc->interp, tc->dataTimeout_array, "active",  
		       (char*)(netmsg.cmdTimeout.active ? "1" : "0"), 
		       TCL_GLOBAL_ONLY) == NULL)
	  return;
      }
      break;

    case CT_DATA_TIMEOUT:
      if(tc->dataTimeout_array) {
	std::ostringstream os;
	os << netmsg.cmdTimeout.seconds;
	if(Tcl_SetVar2(tc->interp, tc->dataTimeout_array, "seconds",  
		       (char*)os.str().c_str(), TCL_GLOBAL_ONLY) == NULL)
	  return;
      }
      break;
    }

    break;

  case CC_SCHED_MSG:
    if(tc->sched_var) {
      if(Tcl_SetVar(interp, tc->sched_var, netmsg.sched.text,
		    TCL_GLOBAL_ONLY) == NULL)
	return;
    };
    break;
  case CC_ARC_MSG:
    if(tc->archiver_var) {
      if(Tcl_SetVar(interp, tc->archiver_var, netmsg.arc.text,
		    TCL_GLOBAL_ONLY) == NULL)
	return;
    };
    break;

    //------------------------------------------------------------
    // A message about the state of the pager
    //------------------------------------------------------------

  case CC_PAGE_MSG:

    if(tc->pager_var) {
      if(Tcl_SetVar(interp, tc->pager_var,
		    (char* )(netmsg.page.allow ? "1" : "0"),
		    TCL_GLOBAL_ONLY) == NULL)
	return;
    };
    
    if((netmsg.page.mask & PAGE_REG) && tc->pagerReg_var) {
      if(Tcl_SetVar(interp, tc->pagerReg_var,
		    (char* )(netmsg.page.text),
		    TCL_GLOBAL_ONLY) == NULL)
	return;
    };
    
    if((netmsg.page.mask & PAGE_MSG) && tc->pagerReg_var) {
      if(Tcl_SetVar(interp, tc->pagerReg_var,
		    (char* )(netmsg.page.text),
		    TCL_GLOBAL_ONLY) == NULL)
	return;
    };
    
    break;
    
  case CC_ANT_MSG:
    if(tc->ant_var) {
      if(Tcl_SetVar(interp, tc->ant_var, netmsg.ant.text,
		    TCL_GLOBAL_ONLY) == NULL)
	return;
    };
    break;
  case CC_GRABBER_MSG:
    updateGrabberTclVariables(tc, interp, &netmsg);
    break;
  default:
    {
      std::ostringstream os;
      os <<   "An unclassifiable message from " 
	 << gcp::util::SpecificName::experimentName() << "Control has been discarded." << std::endl;
      lprintf(stderr, os.str().c_str());
    }
    break;
  };
  return;
}

/*.......................................................................
 * Implement the TCL listen command. This initiate the Tcl event loop.
 *
 * Input:
 *  tc      TclControl *   The package-instance resource object.
 *  interp  Tcl_Interp *   The Tcl interpreter associated with this
 *                         package instance.
 *  argc           int     The number of arguments in argv[] (0).
 *  argv          char **  Unused.
 * Output:
 *  return         int    TCL_OK    - Success.
 *                        TCL_ERROR - Error. Message recorded in
 *                                    interp->result[].
 */
static int tc_event_loop(TclControl *tc, Tcl_Interp *interp,
			 int argc, char *argv[])
{
  char *tcl_interactive;  /* The value of the tcl_interactive variable */
  int interactive;        /* True if tcl_interactive exists and is non-zero */
  /*
   * Wrong number of arguments?
   */
  if(argc != 0) {
    Tcl_AppendResult(interp, "listen: Unexpected arguments.", NULL);
    return TCL_ERROR;
  };
  /*
   * Is the event loop already running?
   */
  if(tc->loop.active)
    return TCL_OK;
  /*
   * Is this an interactive session?
   */
  tcl_interactive = (char* )Tcl_GetVar2(tc->interp, "tcl_interactive", NULL,
					TCL_GLOBAL_ONLY);
  interactive = tcl_interactive && tcl_interactive[0] != '0' &&
    (tc->loop.channel = Tcl_GetChannel(interp, "stdin", NULL)) != NULL;
  /*
   * If it is interactive, prepare to read commands from stdin.
   */
  if(interactive)
    tc_prep_stdin(tc);
  /*
   * Run the event loop until all event sources have been removed.
   */
  tc->loop.active = 1;
  while(Tcl_DoOneEvent(0))
    ;
  tc->loop.active = 0;
  /*
   * Discard any partially complete commands and remove the stdin
   * event handler.
   */
  if(interactive) {
    Tcl_DStringFree(&tc->loop.command);
    Tcl_DeleteChannelHandler(tc->loop.channel, tc_read_stdin, (ClientData) tc);
  };
  return TCL_OK;
}

/*.......................................................................
 * A user-input file handler for stdin.
 *
 * Input:
 *  client_data   ClientData   The TclControl object cast to ClientData.
 *  mask                 int   The type of event that triggered the
 *                             callback. In our case we have only
 *                             registered read events, so this should
 *                             be TCL_READABLE.
 */
static void tc_read_stdin(ClientData client_data, int mask)
{
  TclControl *tc = (TclControl *) client_data;
  Tcl_Interp *interp = tc->interp;
  char *cmd;    /* The currently accumulated portion of the command */
  int status;   /* The return code of the command */
  /*
   * Read the next line from stdin. On end of file or error simply
   * remove the stdin handler so that no more input is attempted.
   * If no more event sources exist, then this will cause tc_event_loop()
   * to return after the next call to Tcl_DoOneEvent().
   */
  if(Tcl_Gets(tc->loop.channel, &tc->loop.command) == -1) {
    Tcl_CreateChannelHandler(tc->loop.channel, 0, tc_read_stdin,(ClientData)tc);
    return;
  };
  /*
   * Reinstate the newline character that gets() zapped and
   * get the resulting string.
   */
  cmd = Tcl_DStringAppend(&tc->loop.command, "\n", -1);
  /*
   * If the command isn't complete yet, return to the event loop.
   */
  if(!Tcl_CommandComplete(cmd))
    return;
  /*
   * Before executing the command, disable the stdin handler, so that
   * the child command is able to use stdin for its own input.
   */
  Tcl_CreateChannelHandler(tc->loop.channel, 0, tc_read_stdin, (ClientData)tc);
  /*
   * Evaluate the user's command after entering it onto the history list.
   */
  status = Tcl_RecordAndEval(interp, cmd, TCL_EVAL_GLOBAL);
  /*
   * Discard the redundant command string.
   */
  Tcl_DStringFree(&tc->loop.command);
  /*
   * If the command returned a value, display it and discard it.
   */
  if(interp->result && interp->result[0] != '\0') {
    puts(interp->result);
    Tcl_ResetResult(interp);
  };
  /*
   * Prepare for a new command line.
   */
  tc_prep_stdin(tc);
  return;
}

/*.......................................................................
 * Prepare to receive command-line input from the user on stdin.
 *
 * Input:
 *  tc            TclControl *   The package-instance resource object.
 */
static void tc_prep_stdin(TclControl *tc)
{
  Tcl_Interp *interp = tc->interp;
  Tcl_Channel stdout_chan;
  /*
   * Clear the command-line accumulation buffer.
   */
  Tcl_DStringFree(&tc->loop.command);
  /*
   * Establish a file handler for stdin.
   */
  Tcl_CreateChannelHandler(tc->loop.channel, TCL_READABLE, tc_read_stdin,
			   (ClientData) tc);
  /*
   * Prompt for input if stdout is a normal channel.
   */
  stdout_chan = Tcl_GetChannel(interp, "stdout", NULL);
  if(stdout_chan) {
    Tcl_Write(stdout_chan, "$ ", 2);
    Tcl_Flush(stdout_chan);
  };
}

/**.......................................................................
 * Respond to a message from the control program to update grabber variables
 */
static void updateGrabberTclVariables(TclControl* tc, Tcl_Interp* interp, 
				      CcNetMsg* netmsg)
{
  CcFrameGrabberOpt params = (CcFrameGrabberOpt)netmsg->grabber.mask;
  char stringVar[10];

  // Now see if deck rotation sense was specified

  if(params & CFG_DKROTSENSE) {
    if(tc->grabber.dkRotSense_var) {
      if(Tcl_SetVar(interp, tc->grabber.dkRotSense_var, 
		    (char*)(netmsg->grabber.dkRotSense==gcp::control::CW ? 
			    "1" : "-1"),
		    TCL_GLOBAL_ONLY) == NULL) {
	COUT("Exiting with error");
	return;
      }
    }
  }

  // Now that we are supporting multiple channels, iterate over
  // channels to see which ones were specified in the mask

  for(unsigned iChan=0; iChan < gcp::grabber::Channel::nChan_; iChan++) {

    if(netmsg->grabber.channelMask & 
       gcp::grabber::Channel::intToChannel(iChan)) {

      bool redraw=false;

      // Always set the channel first, as this variable will be used
      // for subsequent configuration commands
      
      if(tc->grabber.channel_var) {
	sprintf(stringVar, "%d", iChan);
	if(Tcl_SetVar(interp, tc->grabber.channel_var, stringVar,  
		      TCL_GLOBAL_ONLY) == NULL)
	  return;
      }

      // If a channel-ptel assignment was made
      
      if(params & CFG_CHAN_ASSIGN) {
	if(tc->grabber.ptel_var) {
	  sprintf(stringVar, "%d", (unsigned)netmsg->grabber.ptelMask);
	  if(Tcl_SetVar(interp, tc->grabber.ptel_var, stringVar,
			TCL_GLOBAL_ONLY) == NULL) 
	    return;
	}
      }

      // Now see if the X-image direction was specified

      if(params & CFG_XIMDIR) {
	redraw = true;
	if(tc->grabber.ximdir_var) {
	  if(Tcl_SetVar(interp, tc->grabber.ximdir_var, 
			(char*)(netmsg->grabber.ximdir==gcp::control::UPRIGHT 
				? "1" : "-1"),
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
      }

      // Now see if the Y-image direction was specified
      
      if(params & CFG_YIMDIR) {
	redraw = true;
	if(tc->grabber.yimdir_var) {
	  if(Tcl_SetVar(interp, tc->grabber.yimdir_var, 
			(char*)(netmsg->grabber.yimdir==gcp::control::UPRIGHT 
				? "1" : "-1"),
			TCL_GLOBAL_ONLY) == NULL)
	    return;
	}
      }
      
      // Now see if number of frames to combine was specified

      if(params & CFG_COMBINE) {
	if(tc->grabber.combine_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.nCombine);
	  if(Tcl_SetVar(interp, tc->grabber.combine_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
      }

      // Now see if the flatfielding type was specified
      
      if(params & CFG_FLATFIELD) {
	if(tc->grabber.flatfield_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.flatfield);
	  if(Tcl_SetVar(interp, tc->grabber.flatfield_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL)
	    return;
	}
      }
      
      // Now see if the aspect ratio was specified

      if(params & CFG_ASPECT) {
	redraw = true;
	if(tc->grabber.aspect_var) {
	  sprintf(stringVar, "%6.3f", fabs(netmsg->grabber.aspect));
	  if(Tcl_SetVar(interp, tc->grabber.aspect_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL)
	    return;
	}
      }
      
      // Now see if the field of view was specified

      if(params & CFG_FOV) {
	redraw = true;
	if(tc->grabber.fov_var) {
	  sprintf(stringVar, "%6.3f", fabs(netmsg->grabber.fov));
	  
	  if(Tcl_SetVar(interp, tc->grabber.fov_var, stringVar,  TCL_GLOBAL_ONLY) == NULL)
	    return;
	}
      }
      
      // Now see if the collimation angle was specified

      if(params & CFG_COLLIMATION) {
	redraw = true;
	if(tc->grabber.collimation_var) {
	  sprintf(stringVar, "%6.3f", netmsg->grabber.collimation);
	  if(Tcl_SetVar(interp, tc->grabber.collimation_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL)
	    return;
	}
      }
      
      if(params & CFG_PEAK_OFFSETS) {
	redraw = true;
	if(tc->grabber.xpeak_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.ipeak);
	  if(Tcl_SetVar(interp, tc->grabber.xpeak_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL)
	    return;
	}
	
	if(tc->grabber.ypeak_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.jpeak);
	  if(Tcl_SetVar(interp, tc->grabber.ypeak_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL)
	    return;
	}
      }
      
      //------------------------------------------------------------
      // Now see if a search box was defined
      //------------------------------------------------------------

      if(params & CFG_ADD_SEARCH_BOX) {
	redraw = true;
	if(tc->grabber.ixmin_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.ixmin);
	  if(Tcl_SetVar(interp, tc->grabber.ixmin_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
	if(tc->grabber.ixmax_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.ixmax);
	  if(Tcl_SetVar(interp, tc->grabber.ixmax_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
	if(tc->grabber.iymin_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.iymin);
	  if(Tcl_SetVar(interp, tc->grabber.iymin_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
	if(tc->grabber.iymax_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.iymax);
	  if(Tcl_SetVar(interp, tc->grabber.iymax_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
	if(tc->grabber.inc_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.inc ? 1 : 0);
	  if(Tcl_SetVar(interp, tc->grabber.inc_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
      }

      //------------------------------------------------------------
      // Now see if a delete search box command was received
      //------------------------------------------------------------

      if(params & CFG_REM_SEARCH_BOX) {
	redraw = true;
	if(tc->grabber.ixmin_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.ixmin);
	  if(Tcl_SetVar(interp, tc->grabber.ixmin_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
	if(tc->grabber.iymin_var) {
	  sprintf(stringVar, "%d", netmsg->grabber.iymin);
	  if(Tcl_SetVar(interp, tc->grabber.iymin_var, stringVar,  
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
	if(tc->grabber.rem_var) {
	  if(Tcl_SetVar(interp, tc->grabber.rem_var, "1",  
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
      }

      //------------------------------------------------------------
      // Now see if a delete all search boxes command was received
      //------------------------------------------------------------

      if(params & CFG_REM_ALL_SEARCH_BOX) {
	redraw = true;
	if(tc->grabber.rem_var) {
	  if(Tcl_SetVar(interp, tc->grabber.rem_var, "0",  
			TCL_GLOBAL_ONLY) == NULL) {
	    return;
	  }
	}
      }

      // Now that we have updated the configuration parameters for
      // this channel, set the reconf variable to cause the grabber
      // layer to update the windows, if the user is currently
      // displaying this channel
      
      if(tc->grabber.reconf_var) {
	if(Tcl_SetVar(interp, tc->grabber.reconf_var, "true",
		      TCL_GLOBAL_ONLY) == NULL)
	  return;
      }

      // If we reconfigured any parameters that might affect the
      // display of the image, redraw it too

      if(tc->grabber.redraw_var && redraw) {
	if(Tcl_SetVar(interp, tc->grabber.redraw_var, "true",
		      TCL_GLOBAL_ONLY) == NULL)
	  return;
      }
    }
    
  }

}

/**.......................................................................
 * Clear the list of registers displayed in the pager window
 */
void TclControl::TclPagerManager::clear()
{
  pm_->clear();

  update();
}

/**.......................................................................
 * Add to the list of registers displayed in the pager window
 */
void TclControl::TclPagerManager::add(std::string regName, 
				      double min, double max, 
				      bool delta, bool outOfRange, unsigned nFrame)
{
  if(outOfRange)
    pm_->addOutOfRangeMonitorPoint(regName, min, max, delta, nFrame);
  else
    pm_->addInRangeMonitorPoint(regName, min, max, delta, nFrame);
  
  std::vector<std::string> regs = pm_->getList(true);
}

/**.......................................................................
 * Remove from the list of registers displayed in the pager window
 */
void TclControl::TclPagerManager::remove(std::string regName)
{
  pm_->remMonitorPoint(regName);

  update();
}

/**.......................................................................
 * Update the list of registers displayed in the pager window
 */
void TclControl::TclPagerManager::update()
{
  if(tc_->pageCond_array) {

    // First clear the registers

    if(Tcl_SetVar2(tc_->interp, tc_->pageCond_array, "add", "0", 
		   TCL_GLOBAL_ONLY) == NULL ||
       Tcl_SetVar2(tc_->interp, tc_->pageCond_array, "text", "",
		   TCL_GLOBAL_ONLY) == NULL)
      return;

    // Get the list of registers being monitored

    std::vector<std::string> regs = pm_->getList(true);

    // Now write each register in turn to the pager window

    for(unsigned iReg=0; iReg < regs.size(); iReg++) {


      if(Tcl_SetVar2(tc_->interp, tc_->pageCond_array, "add", "1", 
		     TCL_GLOBAL_ONLY) == NULL)
	return;
      

      if(Tcl_SetVar2(tc_->interp, tc_->pageCond_array, "text", 
		     (char*)regs[iReg].c_str(),
		     TCL_GLOBAL_ONLY) == NULL)
	return;
    }

  }

}

/**.......................................................................
 * Update the list of registers displayed in the pager window
 */
TclControl::TclPagerManager::TclPagerManager(TclControl* tc)
{
  tc_ = tc;

  fm_ = 0;
  fm_ = new gcp::util::ArrayDataFrameManager();

  pm_ = 0;
  pm_ = new gcp::util::PagerMonitor(fm_);
}

/**.......................................................................
 * Update the list of registers displayed in the pager window
 */
TclControl::TclPagerManager::~TclPagerManager()
{
  tc_ = 0;

  if(pm_) {
    delete pm_;
    pm_ = 0;
  }
}

/*.......................................................................
 * Respond to a status change in the control connection
 */
static int tc_status(TclControl *tc, Tcl_Interp *interp, int argc, char *argv[])
{
  // Wrong number of arguments?

  if(argc != 1) {
    Tcl_AppendResult(interp, "Wrong number of arguments to the status command.",
		     NULL);
    return TCL_ERROR;
  };
  
  int connected;
  if(Tcl_GetInt(interp, argv[0], &connected)==TCL_ERROR)
    return TCL_ERROR;

  // If we are now connected, disable the retry timer, else enable it

  enableRetryTimer(tc, !connected);

  return TCL_OK;
}
