#include <stdlib.h>

/*
 * Note that full documentation of the streaming interface used in this
 * demo can be found in include/monitor_stream.h.
 */
#include "monitor_stream.h"

/*
 * Enumerate the registers that we are interested in.
 */
typedef enum {
  FRAME_UTC,         /* The UTC of the end of the frame */
  CORR0_VIS,         /* The visibilities of band 0 */
  WEATHER_AIR_TEMP,  /* The ambient temperature */
  TRACKER_SOURCE,    /* The name of the current source */
/* The number of registers specified above - this must be last */
  NUM_MY_REGS
} MyRegIndex;

/*
 * Associate the registers that we enumerated above with their
 * official register-map names. The name of a register consists of the
 * name of the VME or software board that it resides on, plus the name
 * assigned to a particular address range on that board.
 */
MonitorSelection my_sel[NUM_MY_REGS] = {
  {FRAME_UTC,        "frame",   "utc"},
  {CORR0_VIS,        "corr0",   "vis"},
  {WEATHER_AIR_TEMP, "weather", "air_temp"},
  {TRACKER_SOURCE,   "tracker", "source"},
};

/*.......................................................................
 * This is a test program that selects and displays a few sample registers
 * over a given range of times.
 *
 * Usage: read_archive directory cal_file [start_mjd] [end_mjd].
 *
 * The values of start_mjd and end_mjd set the range of times over which
 * data are to be retrieved. They are UTCs expressed as Modified Julian
 * Dates.
 *
 * If start_mjd is omitted or specified as 0, the start time will default
 * to the earliest time available. Similarly end_mjd defaults to the
 * latest time available.
 */
int main(int argc, char *argv[])
{
  char *usage="Usage: read_archive directory cal_file [start_mjd [end_mjd]]\n";
  char *directory;    /* The directory in which to look for files */
  char *calfile;      /* The name of the calibration file */
  double start_mjd=0; /* The start UTC as a Modified Julian Date */
  double end_mjd=0;   /* The end UTC as a Modified Julian Date */
  MonitorStream *ms;  /* A stream for retrieving archive data */
  int count = 0;      /* The number of frames read so far */
/*
 * The following array will contain register-map specific information
 * about the selected registers.
 */
  RegMapReg my_regs[NUM_MY_REGS];
/*
 * Get the command-line arguments - note the lack of break statements
 * at the end of each of the initial clauses.
 */
  switch(argc) {
  case 5:
    end_mjd = atof(argv[4]);
  case 4:
    start_mjd = atof(argv[3]);
  case 3:
    calfile = argv[2];
    directory = argv[1];
    break;
  default:
    fputs(usage, stderr);
    return 1;
  };
/*
 * Open the archive stream and return its handle.
 */
  ms = new_FileMonitorStream(directory, start_mjd, end_mjd);
  if(!ms)
    return 1;
/*
 * Look up and select the registers that are to be retrieved, then
 * load calibration parameters from the specified calibration file.
 */
  if(ms_select_regs(ms, 1, 1, my_sel, NUM_MY_REGS, my_regs) != MS_SEND_DONE ||
     ms_load_cal_file(ms, "", calfile))
    return 1;
/*
 * Read records from the monitor stream until the end of the stream
 * is reached.
 */
  while(1) {
/*
 * Read the next frame of registers.
 */
    switch(ms_read_frame(ms, 1)) {
    case MS_READ_AGAIN:  /* We should never get this when the dowait */
                         /*  argument or ms_read_frame() is 1, so treat */
                         /*  this as an error. */
      return 1;
      break;
    case MS_READ_BREAK:  /* A stream is usually composed of many files, */
                         /*  spliced together behind the scenes. The */
                         /*  MS_READ_BREAK code is returned each time */
                         /*  that the end of a file is reached. It is */
                         /*  usually ignored - but see MS_READ_REGMAP below. */
      break;
    case MS_READ_ENDED:  /* We have reached the end of the requested time */
                         /*  range. */
      printf("End of stream reached.\n");
      return 0;
      break;
    case MS_READ_DONE:   /* A new frame of register values has been read */
/*
 * Display a few register values every 100 frames.
 */
      if(count++ % 100 == 0) {
	double utc[2];   /* The value of frame.utc[0..1] (MJD days,hours) */
        float vis[156];  /* The 78 complex visibilities re,im,re,im... */
	float air_temp;  /* The recorded air temperature */
/*
 * Extract the new values of the selected register into local variables.
 */
	if(ms_get_double(ms, my_regs + FRAME_UTC, 0, 2, utc) ||
	   ms_get_float(ms, my_regs + CORR0_VIS, 0, 156 , vis) ||
	   ms_get_float(ms, my_regs + WEATHER_AIR_TEMP, 0, 1, &air_temp))
	  return 1;
/*
 * Display the newly read values.
 */
	printf("%3.3d: UTC=%.6f temp=%.3f C\n", count,
	       utc[0] + utc[1]/86400000.0,  /* utc[0]=MJD day, utc[1]=ms */
	       air_temp);
      };
      break;
    case MS_READ_REGMAP:  /* An incompatible register map was encountered */
                          /*  at the start of a new file. The old register */
                          /*  map has now been deleted and a new one has */
                          /*  has been allocated in its place. Note that */
                          /*  where needed, the MS_READ_BREAK return code */
                          /*  can be interpretted as prior warning that */
                          /*  this might happen on the following read. */
/*
 * Look up and re-select the desired registers from the new register map,
 * and load its calibration parameters from the specified calibration file.
 */
      if(ms_select_regs(ms, 1, 1, my_sel, NUM_MY_REGS, my_regs)!=MS_SEND_DONE ||
	 ms_load_cal_file(ms, "", calfile))
	return 1;
      break;
    };
  };
}

