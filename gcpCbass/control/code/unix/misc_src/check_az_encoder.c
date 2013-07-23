#include <stdlib.h>
#include <math.h>

/*
 * Note that full documentation of the streaming interface used in this
 * demo can be found in include/monitor_stream.h.
 */
#include "monitor_stream.h"
#include "const.h"
#include "lprintf.h"
#include "cpgplot.h"

/*
 * Record the number of counts per turn of the azimuth motor encoder.
 */
#define MOTOR_COUNTS_PER_TURN  524288.0   /* 2^19 */

/*
 * The azimuth motors rotate 16 times per revolution of the azimuth
 * axis.
 */
#if 0
#define MOTOR_GEAR_RATIO 19
#endif

/*
 * Enumerate the registers that we are interested in.
 */
typedef enum {
  FRAME_FEATURES,    /* The bit-mask of frame feature markers */
  FRAME_UTC,         /* The timestamp of the archive frame */
  PMAC_BUSY,         /* The pmac data-invalid flag */
  PMAC_AZ_POS,       /* The position at the azimuth axis encoder */
  PMAC_AZ_MOTOR_ENC, /* The position of the azimuth motor encoder */
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
  {FRAME_FEATURES,    "frame",   "features"},
  {FRAME_UTC,         "frame",   "utc"},
  {PMAC_BUSY,         "pmac",    "pmac_write"},
  {PMAC_AZ_POS,       "pmac",    "az_pos"},
  {PMAC_AZ_MOTOR_ENC, "pmac",    "az_motor_enc"},
};

/*
 * Data extracted from a single frame is recorded in a container of
 * the following type. This is a single node of a linked list of
 * nodes.
 */
typedef struct EncoderSample EncoderSample;
struct EncoderSample {
  EncoderSample *next; /* The next encoder sample */
  double utc;          /* The timestamp of the sample */
  double axis;         /* The readout of the azimuth axis encoder */
  double motor[2];     /* The readout of the azimuth motor encoders */
  double diff[2];      /* The difference between the sky positions computed */
                       /*  from the axis and motor encoders (arcsec). */
};

typedef struct {
  FreeList *memory;      /* A free-list of EncoderSample nodes */
  EncoderSample *head;   /* The head of a list of EncoderSample nodes */
  EncoderSample *tail;   /* The tail of a list of EncoderSample nodes */
  int nsample;           /* The number of nodes in the list */
  int pgplot_id;         /* The device id returned by pgopen() */
  double dmin[2],dmax[2];/* The range of the azimimuth differences per motor */
  double utmin,utmax;    /* The range of sampled times */
} EncoderData;

static EncoderData *new_EncoderData(char *directory, char *calfile,
				    double start_mjd, double end_mjd);
static EncoderData *del_EncoderData(EncoderData *data);
static int read_EncoderData(MonitorStream *ms, char *calfile,
			    EncoderData *data);
static int add_EncoderSample(EncoderData *data, double utc, double axis,
			     double motors[2]);
static void compare_encoders(EncoderData *data, double motor_gear_ratio);
static int plot_samples(EncoderData *data);

/*.......................................................................
 * This is a program written to determine to what degree the azimuth
 * motor and axis encoders agree.
 *
 * Usage: check_encoder archive_directory cal_file [start_mjd] [end_mjd].
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
  char *usage="Usage: check_encoder archive_directory cal_file [start_mjd [end_mjd]]\n";
  char *directory;    /* The directory in which to look for files */
  char *calfile;      /* The name of the calibration file */
  double start_mjd=0; /* The start UTC as a Modified Julian Date */
  double end_mjd=0;   /* The end UTC as a Modified Julian Date */
  EncoderData *data;  /* The resource object of the program */
  char buffer[120];   /* The buffer into which user responses are to be read */
  int waserr = 0;     /* The error status */
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
 * Read the archive data.
 */
  data = new_EncoderData(directory, calfile, start_mjd, end_mjd);
  if(!data)
    return 1;
/*
 * Was any data read?
 */
  if(data->nsample < 1) {
    lprintf(stderr,
	    "No suitable data was found within the specified time range.\n");
    waserr = 1;
  } else {
    int stop = 0;  /* True when the user wants to stop */
    lprintf(stdout, "A total of %d samples were read.\n", data->nsample);
/*
 * Allow the user to request ranges of gear ratios to be plotted.
 */
    while(!stop) {
      double gear_ratio;
      printf("Specify the gear ratio (18): ");
      if(fgets(buffer, sizeof(buffer), stdin) &&
	 sscanf(buffer, "%lf", &gear_ratio) >= 1) {
/*
 * Plot the data.
 */
	cpgpage();
	compare_encoders(data, gear_ratio);
	plot_samples(data);
      } else {
	stop = 1;
      };
    };
  };
/*
 * Discard the archive data.
 */
  data = del_EncoderData(data);
  return waserr;
}

/*.......................................................................
 * Create a new EncoderData object.
 *
 * Input:
 *  directory     char *  The path of the archive directory.
 *  calfile       char *  The calibration file.
 *  start_mjd   double    The start date as a Modified Julian Date,
 *                        or 0 to start with the oldest available data.
 *  end_mjd     double    The end date as a Modified Julian Date,
 *                        or 0 to read up to the newest available data.
 * Output:
 *  return EncoderData *  The new object, or NULL on error.
 */
static EncoderData *new_EncoderData(char *directory, char *calfile,
				    double start_mjd, double end_mjd)
{
  EncoderData *data;  /* The object to be returned */
  MonitorStream *ms;  /* The archive monitor stream */
  int i;
/*
 * Check the arguments.
 */
  if(!directory || !calfile) {
    lprintf(stderr, "new_EncoderData: NULL argument.\n");
    return NULL;
  };
/*
 * Allocate the container.
 */
  data = (EncoderData* )malloc(sizeof(EncoderData));
  if(!data) {
    lprintf(stderr, "new_EncoderData: Insufficient memory.\n");
    return NULL;
  };
/*
 * Before attempting any operation that might fail, initialize the
 * container at least up to the point at which it can safely be passed
 * to del_EncoderData().
 */
  data->memory = NULL;
  data->head = NULL;
  data->tail = NULL;
  data->nsample = 0;
  data->pgplot_id = 0;
  for(i=0; i<2; i++)
    data->dmin[i] = data->dmax[i] = 0.0;
  data->utmin = data->utmax = 0.0;
  
/*
 * Allocate a freelist for the allocation of EncoderSample nodes.
 */
  data->memory = new_FreeList("new_EncoderData", sizeof(EncoderSample), 1000);
  if(!data->memory)
    return del_EncoderData(data);
/*
 * Open the archive stream and return its handle.
 */
  ms = new_FileMonitorStream(directory, start_mjd, end_mjd);
  if(!ms)
    return del_EncoderData(data);
/*
 * Open the default pgplot device.
 */
  data->pgplot_id = cpgopen("");
  if(data->pgplot_id <= 0)
    return del_EncoderData(data);
/*
 * Turn off new-page prompting.
 */
  cpgask(0);
/*
 * Read selected data from the archive stream.
 */
  if(read_EncoderData(ms, calfile, data))
    data = del_EncoderData(data);
/*
 * Discard the archive stream.
 */
  ms = del_MonitorStream(ms);
/*
 * Return the loaded data for further processing.
 */
  return data;
}

/*.......................................................................
 * Delete an EncoderData object.
 *
 * Input:
 *  data   EncoderData *  The object to be deleted.
 * Output:
 *  return EncoderData *  The deleted object (always NULL).
 */
static EncoderData *del_EncoderData(EncoderData *data)
{
  if(data) {
    data->memory = del_FreeList("del_EncoderData", data->memory, 1);
    cpgend();
    free(data);
  };
  return NULL;
}

/*.......................................................................
 * Read selected archive registers from an archive stream into the
 * passed container object.
 *
 * Input:
 *  ms    MonitorStream *  A newly opened archive stream.
 *  calfile        char *  The calibration file.
 * Input/Output:
 *  data    EncoderData *  The container in which to record the data.
 */
static int read_EncoderData(MonitorStream *ms, char *calfile, EncoderData *data)
{
  double axis;     /* The readout of the azimuth axis encoder */
  double motors[2];/* The readout of the azimuth motor encoders */
  double utc[2];   /* The day number and time of day of each sample */
  int busy;        /* True if the pmac data isn't useable */
  int features;    /* The bit-mask of frame feature markers */
/*
 * The following array will contain register-map specific information
 * about the selected registers.
 */
  RegMapReg my_regs[NUM_MY_REGS];
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
      return 0;
      break;
    case MS_READ_DONE:   /* A new frame of register values has been read */
/*
 * Extract the new values of the selected register into local variables.
 */
      if(ms_get_int(ms, my_regs + FRAME_FEATURES, 0, 1, &features) ||
	 ms_get_double(ms, my_regs + FRAME_UTC, 0, 2, utc) ||
	 ms_get_int(ms, my_regs + PMAC_BUSY, 0, 1, &busy) ||
	 ms_get_double(ms, my_regs + PMAC_AZ_POS, 0, 1, &axis) ||
	 ms_get_double(ms, my_regs + PMAC_AZ_MOTOR_ENC, 0, 2, motors))
	return 1;
/*
 * If we are currently tracking, record the new data.
 */
      if(!busy && features & 1) {
	if(add_EncoderSample(data, utc[0]+utc[1]/daysec/1000.0, axis, motors))
	  return 1;
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
  return 0;
}

/*.......................................................................
 * Add a newly read sample to the extracted selection of archive registers.
 *
 * Input:
 *  data    EncoderData *   The sample will be added to the sample list
 *                          in *data.
 *  utc          double     The UTC of the sample, expressed as a Modified
 *                          Julian Date.
 *  axis         double     The readout of the azimuth axis encoder.
 *  motors       double[2]  The array of the two azimuth motor readouts.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static int add_EncoderSample(EncoderData *data, double utc, double axis,
			     double motors[2])
{
  int i;
/*
 * Allocate a new list node for the sample.
 */
  EncoderSample *sample = (EncoderSample* )new_FreeListNode("add_EncoderSample", data->memory);
  if(!sample)
    return 1;
/*
 * Copy the new data into the node.
 */
  sample->next = NULL;
  sample->utc = utc;
  sample->axis = axis;
  for(i=0; i<2; i++) {
    sample->motor[i] = motors[i];
    sample->diff[i] = 0;
  };
/*
 * Append the new sample to the list.
 */
  if(data->tail)
    data->tail->next = sample;
  else
    data->head = sample;
  data->tail = sample;
  data->nsample++;
/*
 * Keep a record of the range of times seen.
 */
  if(data->utmin == 0.0)
    data->utmin = utc;
  if(utc > data->utmax)
    data->utmax = utc;
  return 0;
}

/*.......................................................................
 * Compare the azimuth readouts that are implied by the azimuth motor
 * and azimuth axis encoders.
 *
 * Input:
 *  data        EncoderData *  The object that contains the data. The difference
 *                             between the encoders will be recorded per sample
 *                             in the list of samples. The range of differences
 *                             will be recorded in data->dmin and data->dmax.
 */
static void compare_encoders(EncoderData *data, double motor_gear_ratio)
{
  EncoderSample *sample;   /* A data sample that is being processed */
  double anchor = 0.0;     /* The differences will be rounded to whithin */
                           /*  one turn of this value. */
  int motor;               /* The index of the motor being processed (0..1) */
/*
 * Compute the number of arcseconds moved on the sky per turn of the
 * azimuth motors.
 */
  double arcsec_per_turn = 360.0 * 3600.0 / motor_gear_ratio;
/*
 * Traverse the list of recorded samples.
 */
  for(motor=0; motor<2; motor++) {
    double dmin=0.0, dmax=0.0;  /* The range of the encoder differences */
    int n = 0;                  /* The number of samples processed */
    for(sample=data->head; sample; sample=sample->next) {
/*
 * Compute the difference between the motor and axis encoders measured
 * in arcseconds on the sky.
 */
      double diff = sample->axis * 3600.0 -
	arcsec_per_turn * sample->motor[motor]/MOTOR_COUNTS_PER_TURN;
/*
 * Use the first point to set the anchor point.
 */
      if(n==0)
	anchor = diff;
/*
 * Reduce the difference to be positive and within one turn of 'anchor'.
 */
      while(diff - anchor > arcsec_per_turn/2.0)
	diff -= arcsec_per_turn;
      while(diff - anchor <= -arcsec_per_turn/2.0)
	diff += arcsec_per_turn;
/*
 * Record the difference for later plotting.
 */
      sample->diff[motor] = diff;
/*
 * Update the range of values seen so far.
 */
      if(++n == 1)
	dmin = dmax = diff;
      else if(diff < dmin)
	dmin = diff;
      else if(diff > dmax)
	dmax = diff;
/*
 * Record the statistics for return.
 */
      data->dmin[motor] = dmin;
      data->dmax[motor] = dmax;
    };
  };
}


static int plot_samples(EncoderData *data)
{
  EncoderSample *sample;/* The sample being plotted */
  double mid;           /* The mid y-axis value */
  double y_margin;      /* The margin to leave above and below plotted points */
/*
 * Compute the modified julian day number of the first day of the observation.
 */
  double day0 = floor(data->head->utc);
/*
 * Buffer while the plot is being drawn - not only does this speed up
 * plotting, but it also leaves the plot from the previous iteration
 * visible until the new plot has been completed.
 */
  cpgbbuf();
/*
 * Use pgeras() in place of pgpage(). The latter isn't buffered.
 */
  cpgeras();
  cpgvstd();
  cpglab("", "Encoder difference (arcsec)",
	 "The difference between the azimuth axis and azimuth motor encoders");
/*
 * Plot the azimuth encoder discrepancies wrt the first motor versus azimuth.
 */
  cpgsvp(0.1,0.99, 0.5,0.9);
  y_margin = (data->dmax[0] - data->dmin[0]) * 0.07;
  mid = (data->dmax[0] + data->dmin[0]) / 2.0;
  cpgswin(data->utmin - day0, data->utmax - day0,
	  data->dmin[0] - mid - y_margin,
	  data->dmax[0] - mid + y_margin);
  cpgbox("BCST", 0.0, 0, "BCNST", 0.0, 0);
  for(sample=data->head; sample; sample=sample->next)
    cpgpt1(sample->utc - day0, sample->diff[0] - mid, 16);
/*
 * Plot the azimuth encoder discrepancies wrt the second motor versus azimuth.
 */
  cpgsvp(0.1,0.99, 0.1,0.5);
  y_margin = (data->dmax[1] - data->dmin[1]) * 0.07;
  mid = (data->dmax[1] + data->dmin[1]) / 2.0;
  cpgswin(data->utmin - day0, data->utmax - day0,
	  data->dmin[1] - mid - y_margin,
	  data->dmax[1] - mid + y_margin);
  cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
  cpgmtxt("B", 2.7, 0.5, 0.5, "Time (days)");
  for(sample=data->head; sample; sample=sample->next)
    cpgpt1(sample->utc - day0, sample->diff[1] - mid, 16);
/*
 * Reveal the plot.
 */
  cpgebuf();
  return 0;
}

