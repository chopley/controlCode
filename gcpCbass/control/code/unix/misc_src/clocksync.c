/*-----------------------------------------------------------------------*
 * This program acts as a daemon, listening for GPS time updates from    *
 * the real-time CPU, and slowly adjusting the local clock to match      *
 * GPS.                                                                  *
 *-----------------------------------------------------------------------*/

#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>

#include "netbuf.h"
#include "tcpip.h"
#include "clocksync.h"
#include "astrom.h"
#include "const.h"

#define DEBUG

/*
 * The following variable is set to 1 in the catch_sighup signal handler.
 */
static sig_atomic_t stop_now = 0;
static void catch_sighup(int sig);

/*
 * The following container encapsulates the resources needed to
 * receive and process GPS time updates received from the real-time CPU.
 */
typedef struct {
  int port;        /* The UDP port to listen on for time updates */
  long msglen;     /* The length of a time-update message */
  NetBuf *net;     /* The network buffer to use to decompose the message */
  int ngood;       /* The number of usable updates received since the */
                   /*  last clock update. */
  int nbad;        /* The number of unusable updates received since the */
                   /*  last clock update. */
  double offsets[TIME_UPDATE_COUNT]; /* The last ngood <= TIME_UPDATE_COUNT */
                   /*  time offsets */
} ClockSynch;

static ClockSynch *new_ClockSynch(void);
static ClockSynch *del_ClockSynch(ClockSynch *cs);
static int cmp_time_offsets(const void *v1, const void *v2);
static int read_time_update(ClockSynch *cs);
static int synchronize_clock(double offset);

/*.......................................................................
 * This program orphans itself to become a daemon, then waits for double
 * precision MJD time updates on port SZA_TIME_PORT, and uses them to
 * correct the system clock.
 *
 * Input:
 *  argc     int      The number of command-line arguments (0).
 *  argv    char *[]  The command-line arguments (none).
 * Output:
 *  return   int      The exit status of the process.
 */
int main(int argc, char *argv[])
{
  ClockSynch *cs;  /* The resource object of the daemon */
/*
 * Close all I/O connections to the invoking terminal.
 */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
/*
 * Orphan the process, so that it can act as a daemon.
 */
  if(fork() != 0)
    return 0;
/*
 * Make the process its own session leader.
 */
  setsid();
/*
 * Open a connection to the system logger.
 */
  openlog("clocksync", LOG_CONS, LOG_USER);
/*
 * Arrange to shutdown when a HUP signal is received.
 */
  signal(SIGHUP, catch_sighup);
/*
 * Allocate the resources needed by the process.
 */
  cs = new_ClockSynch();
  if(!cs)
    return 1;
/*
 * Place a startup message in the log.
 */
  syslog(LOG_NOTICE, "clocksync daemon started.\n");
/*
 * Listen for time updates.
 */
  while(!stop_now)
    read_time_update(cs);
/*
 * Release all resources.
 */
  cs = del_ClockSynch(cs);
/*
 * Report program termination.
 */
  syslog(LOG_NOTICE, "Stopping after receiving SIGHUP.\n");
  closelog();
  return 0;
}

/*.......................................................................
 * Allocate and initialize the resources of the socket that is used to
 * receive time updates from the real-time cpu.
 *
 * Output:
 *  return  ClockSynch *   The new resource object, or NULL on error.
 */
static ClockSynch *new_ClockSynch(void)
{
  ClockSynch *cs;    /* The resource object to be returned */
/*
 * Allocate the container.
 */
  cs = (ClockSynch *) malloc(sizeof(ClockSynch));
  if(!cs) {
    syslog(LOG_ERR, "new_ClockSynch: Insufficient memory.\n");
    return NULL;
  };
/*
 * Before attempting any operation that might fail, initialize the
 * container at least up to the point at which it can safely be passed
 * to del_ClockSynch().
 */
  cs->port = -1;
  cs->msglen = 0;
  cs->net = NULL;
  cs->ngood = 0;
  cs->nbad = 0;
/*
 * Attempt to allocate a UDP port on which to listen for time
 * updates.
 */
  cs->port = udp_server_sock(SZA_TIME_PORT, 1);
  if(cs->port == -1)
    return del_ClockSynch(cs);
/*
 * Allocate a network buffer with enough room for one 64-bit double.
 */
  cs->msglen = NET_PREFIX_LEN + 8;
  cs->net = new_NetBuf(cs->msglen);
  if(!cs->net)
    return del_ClockSynch(cs);
  return cs;
}

/*.......................................................................
 * Delete the resources of the time synchronizer.
 *
 * Input:
 *  cs     ClockSynch *  The resource container to be deleted.
 * Output:
 *  return ClockSynch *  The deleted container (ie. NULL).
 */
static ClockSynch *del_ClockSynch(ClockSynch *cs)
{
  if(cs) {
    if(cs->port >= 0)
      close(cs->port);
    free(cs);
  };
  return NULL;
}

/*.......................................................................
 * Read a time update.
 *
 * Input:
 *  cs   ClockSynch *   The time listner resource object.
 * Output:
 *  return        int     0 - OK.
 *                        1 - Error.
 */
static int read_time_update(ClockSynch *cs)
{
  int opcode;            /* The identifier code of the message */
  double rcvd_utc;       /* The received time */
  double local_utc;      /* The time on the local clock */
  double diff_utc;       /* The new time offset */
  NetBuf *net = cs->net; /* The network buffer used to decompose the message */
  struct timeval delta;  /* The amount of time to advance the system clock */
  struct timeval tbd;    /* The outstanding time correction from the previous */
                         /*  call to adjtime() */
  long net_utc[2];       /* The received time as Modified Julian Day number */
                         /*  and seconds into day */
/*
 * Read the message.
 */
  net->nput = recvfrom(cs->port, net->buf, cs->msglen, 0, NULL,NULL);
  if(net->nput == -1) {
    if(!stop_now)
      syslog(LOG_ERR, "recvfrom: %m");
    return 1;
  } else if(net->nput != cs->msglen) {
    syslog(LOG_ERR, "Bad packet length (%ld).\n", net->nput);
    return 1;
  };
/*
 * Decompose it.
 */
  if(net_start_get(net, &opcode) ||
     net_get_long(net, 2, (long unsigned int* )net_utc) ||
     net_end_get(net)) {
    syslog(LOG_ERR, "read_time_update: Corrupt time message.\n");
    return 1;
  };
/*
 * Convert the received time into a Modified Julian date.
 */
  rcvd_utc = net_utc[0] + net_utc[1] / daysec;
/*
 * Get the time on the szacontrol clock.
 */
  local_utc = current_mjd_utc();
/*
 * If we only called adjtime() a short time ago, then it may still
 * be in the process of adjusting the clock. Find out how many
 * seconds it is still working on removing.
 */
  delta.tv_sec = 0;
  delta.tv_usec = 0;
  if(adjtime(&delta, &tbd) < 0) {
    syslog(LOG_ERR, "adjtime: %m.\n");
    return 1;
  };
/*
 * Compute the discrepancy between the local clock and the
 * received time.
 */
  diff_utc = (local_utc - rcvd_utc) * daysec + (tbd.tv_sec + tbd.tv_usec/1e6);
/*
 * Silently discard the received date if it disagrees with our clock
 * by too much.
 */
  if(fabs(diff_utc) > MAX_CLOCK_OFFSET) {
    if(++cs->nbad >= TIME_UPDATE_COUNT) {
      syslog(LOG_ERR,
        "The offset between the sun and gps clocks is too great to correct.\n");
#ifdef DEBUG
      syslog(LOG_ERR,
	     "Szacontrol time is: %lf, gps is %lf\n",local_utc,rcvd_utc);
      
#endif
      cs->ngood = cs->nbad = 0;
      return 1;
    };
    return 0;
  };
/*
 * Record the latest time offset.
 */
  cs->offsets[cs->ngood++] = diff_utc;
/*
 * When enough updates have been received, compute the median of the
 * received offsets and use it to correct the clock.
 */
  if(cs->ngood >= TIME_UPDATE_COUNT) {
    double offset;  /* The median clock offset */
/*
 * Sort the offsets into ascending order.
 */
    qsort(cs->offsets, TIME_UPDATE_COUNT, sizeof(cs->offsets[0]),
	  cmp_time_offsets);
/*
 * Find the mid point value of the array, interpolating if the
 * array size is even.
 */
    if(TIME_UPDATE_COUNT % 2) {
      offset = (cs->offsets[TIME_UPDATE_COUNT/2] + cs->offsets[TIME_UPDATE_COUNT/2+1]) / 2.0;
    } else {
      offset = cs->offsets[TIME_UPDATE_COUNT/2];
    };
    cs->ngood = cs->nbad = 0;
/*
 * Adjust the system clock to fix the discrepancy.
 */
    synchronize_clock(-offset);
  };
  return 0;
}

/*.......................................................................
 * This is a qsort comparison function used for sorting time offsets into
 * ascending order.
 *
 * Input:
 *  v1     void *   The first of the two time offsets to compare.
 *  v2     void *   The second of the two time offsets to compare.
 * Output:
 *  return   int     < 0  -  v1 < v2
 *                     0  -  v1 == v2
 *                   > 0  -  v1 > v2
 */
static int cmp_time_offsets(const void *v1, const void *v2)
{
  double dt1 = *(double *)v1;
  double dt2 = *(double *)v2;
  return dt1 < dt2 ? -1 : (dt1 == dt2 ? 0 : -1);
}

/*.......................................................................
 * When a SIGHUP signal is received, set the file-scope stop_now variable
 * to tell the program to quit.
 *
 * Input:
 *  sig   int    The signal that was received.
 */
static void catch_sighup(int sig)
{
  stop_now = 1;
}

/*.......................................................................
 * Slowly adjust the system clock to remove the specified offset.
 *
 * Input:
 *  offset    double   The current offset of the clock wrt the received
 *                     time updates (seconds).
 * Output:
 *  return       int   0 - OK.
 *                     1 - Error.
 */
static int synchronize_clock(double offset)
{
  struct timeval delta;  /* The amount of time to advance the system clock */
/*
 * Compute the correction needed to bring the system clock into
 * synchronization with the received time updates.
 */
  delta.tv_sec = offset;
  delta.tv_usec = (offset - delta.tv_sec) * 1.0e6;
/*
 * The adjtime function changes the clock rate slightly until the
 * specified adjustment has been effected.
 */
  syslog(LOG_NOTICE, "The system clock is slowly being %s by %g seconds.\n",
	 offset < 0 ? "retarded":"advanced", fabs(offset));
  if(adjtime(&delta, NULL) == -1) {
    syslog(LOG_ERR, "adjtime({%d,%d},NULL): %m.\n", delta.tv_sec,
	   delta.tv_usec);
    return 1;
  };
  return 0;
}

