/*
 * The following is a test program that sends fake register frames
 * to the control program to test the host archiving and monitoring
 * facilities.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <math.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "netbuf.h"
#include "tcpip.h"
#include "genericregs.h"
#include "scanner.h"
#include "const.h"
#include "rtcnetcoms.h"
#include "astrom.h"

#include <string>

#define DEBUG

#ifdef _GPP // If compiling with the C++ compiler
using namespace gcp::control;
#endif

typedef struct Supplier Supplier;

struct Supplier {
  SzaRegMap *regmap;      /* The SZA register map */
  char *control_host;     /* The IP address of the control host */
  int archiver_fd;        /* The connection to the control program */
                          /* archiver port (-1 when not connected). */
  int control_fd;         /* The connection to the control program */
                          /* controller port (-1 when not connected). */
  NetReadStr *nrs;        /* The TCP/IP network input stream for the scanner */
  NetSendStr *nss;        /* The TCP/IP network output stream for the scanner */
  unsigned *fb;           /* The output frame buffer */
  unsigned narchive;      /* The number of archived registers */
  struct timeval interval;/* The interval between frames */
  unsigned nsnap_slot;    /* The archive slot of the frame.nsnap register */
  unsigned record_slot;   /* The archive slot of the frame.record register */
  unsigned utc_slot;      /* The archive slot of the frame.utc register */
  unsigned source_slot;   /* The first archive slot of the source register */
};

static Supplier *new_Supplier(char *host);
static Supplier *del_Supplier(Supplier *sup);
static int send_frames(Supplier *sup);
static int connect_control(Supplier *sup);
static int connect_archiver(Supplier *sup);
static void disconnect_control(Supplier *sup);
static void disconnect_archiver(Supplier *sup);

static jmp_buf interrupt_env;
static void interrupt_handler(int sig);

#if 0
#define INTERVAL 0   /* The time between samples (milliseconds) */
#else
#define INTERVAL 1000
#endif

int main(int argc, char *argv[])
{
  static Supplier *sup;
/*
 * Check arguments.
 */
  if(argc != 2) {
    fprintf(stderr, "Usage: %s control_host\n", argv[0]);
    return 1;
  };
/*
 * Allocate the resources of the program.
 */
  sup = new_Supplier(argv[1]);
  if(!sup)
    return 1;
/*
 * When an attempt is made to write to a socket who's connection
 * has been broken, a sigpipe signal is generated. By default this
 * kills the process. Arrange to ignore this signal. nss_send_msg
 * will still detect the error via the return value of write().
 */
  signal(SIGPIPE, SIG_IGN);
/*
 * Trap interrupts.
 */
  signal(SIGINT, interrupt_handler);
  if(setjmp(interrupt_env) == 0) {
/*
 * Serve one of more clients with fake register-frames.
 */
    while(1) {
      if(connect_control(sup) || connect_archiver(sup)) {
	sleep(2);
      } else {
	printf("Connected to control program.\n");
	send_frames(sup);
      };
      disconnect_control(sup);
      disconnect_archiver(sup);
    };
  };
/*
 * Clean up.
 */
  sup = del_Supplier(sup);
  return 0;
}

/*.......................................................................
 * Create the resource object of the test scanner.
 *
 * Input:
 *  host             char *   The IP address of the computer on which
 *                            the control program is running.
 * Output:
 *  return       Supplier *   The new object, or NULL on error.
 */
static Supplier *new_Supplier(char *host)
{
  Supplier *sup;  /* The object to be returned */
  RegMapReg reg;  /* The details of a register map entry */
/*
 * Check arguments.
 */
  if(!host) {
    fprintf(stderr, "new_Supplier: NULL host.\n");
    return NULL;
  };
/*
 * Create the container.
 */
  sup = (Supplier *) malloc(sizeof(Supplier));
  if(!sup) {
    fprintf(stderr, "new_Supplier: Insufficient memory.\n");
    return NULL;
  };
/*
 * Before attempting any operation that might fail, initialize the
 * container at least up to the point at which it can safely be
 * passed to del_Supplier().
 */
  sup->regmap = NULL;
  sup->control_host = NULL;
  sup->archiver_fd = -1;
  sup->control_fd = -1;
  sup->nrs = NULL;
  sup->nss = NULL;
  sup->fb = NULL;
  sup->narchive = 0;
  sup->interval.tv_sec  = (INTERVAL) / 1000;
  sup->interval.tv_usec = (INTERVAL % 1000) * 1000;
  sup->nsnap_slot = 0;
  sup->record_slot = 0;
  sup->utc_slot = 0;
  sup->source_slot = 0;
/*
 * Get the SZA register map.
 */
  sup->regmap = new_SzaAntRegMap();
  if(!sup->regmap)
    return del_Supplier(sup);
  sup->narchive = sup->regmap->narchive_;
/*
 * Lookup the archive slot of the snapshot coadding count register.
 */
  if(find_RegMapReg(sup->regmap, "frame", "nsnap", REG_PLAIN, 0, 1, &reg))
    return del_Supplier(sup);
  sup->nsnap_slot = reg.slot;
/*
 * Lookup the archive slot of the record number register.
 */
  if(find_RegMapReg(sup->regmap, "frame", "record", REG_PLAIN, 0, 1, &reg))
    return del_Supplier(sup);
  sup->record_slot = reg.slot;
/*
 * Lookup the archive slot of the utc register.
 */
  if(find_RegMapReg(sup->regmap, "frame", "utc", REG_PLAIN, 0, 2, &reg))
    return del_Supplier(sup);
  sup->utc_slot = reg.slot;
/*
 * Lookup the first archive slot of the source name register.
 */
  if(find_RegMapReg(sup->regmap, "tracker", "source", REG_PLAIN, 0, 3, &reg))
    return del_Supplier(sup);
  sup->source_slot = reg.slot;
/*
 * Make a copy of the host name.
 */
  sup->control_host = (char *) malloc(strlen(host) + 1);
  if(!sup->control_host) {
    fprintf(stderr, "new_Supplier: Insufficient memory.\n");
    return del_Supplier(sup);
  };
  strcpy(sup->control_host, host);
/*
 * Create a TCP/IP network input stream. The longest message
 * will be the array of register blocks passed as a 4-byte address
 * plus 2-byte block dimension per register.
 */
  sup->nrs = new_NetReadStr(-1, SCAN_MAX_CMD_SIZE);
  if(!sup->nrs)
    return del_Supplier(sup);
/*
 * Create a TCP/IP network output stream. Note that the buffer
 * will be assigned below.
 */
  sup->nss = new_NetSendStr(-1, 0);
  if(!sup->nss)
    return del_Supplier(sup);
/*
 * Allocate the frame-buffer.
 */
  sup->fb = (unsigned *) malloc(SCAN_BUFF_SIZE(sup->narchive));
  if(!sup->fb) {
    fprintf(stderr, "new_Supplier: Insufficient memory.\n");
    return del_Supplier(sup);
  };
/*
 * Install the frame buffer as the network buffer and pre-format
 * the register frame output message.
 */
  {
    NetBuf *net = sup->nss->net;
    net_set_buffer(sup->nss->net, sup->fb, SCAN_BUFF_SIZE(sup->narchive));
    if(net_start_put(net, 0) ||
       net_inc_nput(net, SCAN_BUFF_SIZE(sup->narchive) - NET_PREFIX_LEN) < 0 ||
       net_end_put(net))
      return del_Supplier(sup);
  };
  return sup;
}

/*.......................................................................
 * Delete the resource object of the program.
 *
 * Input:
 *  sup     Supplier *  The object to be deleted.
 * Output:
 *  return  Supplier *  The deleted object (always NULL).
 */
static Supplier *del_Supplier(Supplier *sup)
{
  if(sup) {
    sup->regmap = del_SzaAntRegMap(sup->regmap);
    if(sup->archiver_fd >= 0) {
      shutdown(sup->archiver_fd, 2);
      close(sup->archiver_fd);
    };
    if(sup->control_fd >= 0) {
      shutdown(sup->control_fd, 2);
      close(sup->control_fd);
    };
    sup->nrs = del_NetReadStr(sup->nrs);
    sup->nss = del_NetSendStr(sup->nss);
    if(sup->fb)
      free(sup->fb);
    free(sup);
  };
  return NULL;
}

/*.......................................................................
 * This is a signal handler for trapping interrupts It simply calls
 * longjmp to cause the main loop to abort, so that cleanup can be
 * performed before the program is terminated.
 */
static void interrupt_handler(int sig)
{
  longjmp(interrupt_env, 1);  
}

/*.......................................................................
 * Connect to the controller port of the control program.
 *
 * Input:
 *  sup    Supplier *  The task context object.
 * Output:
 *  return      int    0 - OK.
 *                     1 - Error.
 */
static int connect_control(Supplier *sup)
{
/*
 * Terminate any existing connection.
 */
  disconnect_control(sup);
  sup->control_fd = tcp_connect(sup->control_host, CP_RTC_PORT, 1);
  return sup->control_fd >= 0 ? 0 : 1;
}

/*.......................................................................
 * Connect to the archiver port of the control program.
 *
 * Input:
 *  sup    Supplier *  The task context object.
 * Output:
 *  return      int    0 - OK.
 *                     1 - Error.
 */
static int connect_archiver(Supplier *sup)
{
  int opcode;                    /* Message-type opcode */
  unsigned long regmap_revision; /* Register map structure revision */
  unsigned long regmap_narchive; /* The number of archive registers */
/*
 * Terminate any existing connection.
 */
  disconnect_archiver(sup);
  sup->archiver_fd = tcp_connect(sup->control_host, CP_RTS_PORT, 1);
  if(sup->archiver_fd < 0)
    return 1;
/*
 * Attach the network I/O streams to the new client socket.
 */
  attach_NetReadStr(sup->nrs, sup->archiver_fd);
  attach_NetSendStr(sup->nss, sup->archiver_fd);
/*
 * Attempt to read the greeting message.
 */
#ifndef _GPP
  if(nrs_read_msg(sup->nrs) != NET_READ_DONE ||
#else
  if(nrs_read_msg(sup->nrs) != NetReadStr::NET_READ_DONE ||
#endif
     net_start_get(sup->nrs->net, &opcode) ||
     opcode != SCAN_GREETING ||
     net_get_long(sup->nrs->net, 1, &regmap_revision) ||
     net_get_long(sup->nrs->net, 1, &regmap_narchive) ||
     net_end_get(sup->nrs->net)) {
    fprintf(stderr, "Corrupt greeting message.\n");
    disconnect_archiver(sup);
    return 1;
  };
/*
 * Check that the register map being used by this program and the
 * remote host are in sync.
 */
  if(regmap_revision != REGMAP_REVISION ||
     regmap_narchive != sup->regmap->narchive_) {
    fprintf(stderr, "Register-map mismatch wrt host.\n");
    disconnect_archiver(sup);
    return 1;
  };
  return 0;
}

/*.......................................................................
 * Disconnect the connection to the control-program control port.
 *
 * Input:
 *  sup    Supplier *  The resource object of the program.
 */
static void disconnect_control(Supplier *sup)
{
  if(sup->control_fd >= 0) {
    shutdown(sup->control_fd, 2);
    close(sup->control_fd);
    sup->control_fd = -1;
  };
}

/*.......................................................................
 * Disconnect the connection to the control-program archiver.
 *
 * Input:
 *  sup    Supplier *  The resource object of the program.
 */
static void disconnect_archiver(Supplier *sup)
{
  if(sup->archiver_fd >= 0) {
    shutdown(sup->archiver_fd, 2);
    close(sup->archiver_fd);
    attach_NetReadStr(sup->nrs, -1);
    attach_NetSendStr(sup->nss, -1);
    sup->archiver_fd = -1;
  };
}

/*.......................................................................
 * Construct and send fake register frames to the currently connected
 * client.
 *
 * Input:
 *  sup    Supplier *  The resource object of the program.
 * Output:
 *  return      int    0 - OK.
 *                     1 - Error.
 */
static int send_frames(Supplier *sup)
{
  unsigned frame;     /* The sequential frame number */
  unsigned nslot;     /* The number of archived registers */
  unsigned *slot;     /* A pointer to the first register in the output frame */
  int i;
/*
 * Get a pointer to the start of the output register frame.
 */
  slot = sup->fb + SCAN_HEADER_DIM;
  nslot = sup->narchive;
/*
 * Keep sending frames until an error occurs.
 */
  for(frame=0; ; frame++) {
    double days;   /* The Modified Julian Day number */
/*
 * Get the current UTC as a Modified Julian Date.
 */
    double mjd = current_mjd_utc();
    if(mjd < 0)
      return 1;
/*
 * Fill the current frame with sine waves of different periods.
 */
    for(i=0; i<(int)nslot; i++) {
/*
      float w = twopi * 0.1 * (float)i / (float)nslot;
      slot[i] = 500 * (1.0 + sin(w*frame));
*/
      slot[i] = (unsigned int)(500 * (1.0 + sin(twopi/20 * frame)));
    };
/*
 * Record the current frame number and coadd-count.
 */
    slot[sup->record_slot] = frame;
    slot[sup->nsnap_slot] = 1;
/*
 * Compute and record the current UTC, split into MJD day-number and
 * milli-second components.
 */
    slot[sup->utc_slot+1] = (unsigned int)(modf(mjd, &days) * daysec * 1000.0);
    slot[sup->utc_slot] = (unsigned int)days;
/*
 * Record a dummy source name.
 */
    if(pack_int_string((char *)(frame % 10 < 5 ? "3C123":"0552+398"),
		       3, slot + sup->source_slot))
      return 1;
/*
 * Send the latest frame.
 */
#ifndef _GPP
    if(nss_send_msg(sup->nss) != NET_SEND_DONE)
#else
    if(nss_send_msg(sup->nss) != NetSendStr::NET_SEND_DONE)
#endif
      return 1;
/*
 * Wait before sending the next frame.
 */
    /*
     * Under linux, select() updates the timeout parameter to indicate
     * how much time was left.  We could call pselect() to avoid this,
     * or just reset sup->interval before every call 
     */
    sup->interval.tv_sec  = (INTERVAL) / 1000;
    sup->interval.tv_usec = (INTERVAL % 1000) * 1000;

    select(0, NULL, NULL, NULL, &sup->interval);
  };
}
