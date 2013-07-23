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

#define THEORY 1

/*
 * The angle between the face of the deck and the line connecting the
 * elevation axis to the top of the ball-screw.
 */
#if THEORY
#define EXTRA_ANGLE (7.393 * dtor)         /* Radians */
#else
#define EXTRA_ANGLE 0.128593
#endif
/*
 * Record the number of counts per turn of the elevation motor encoder.
 */
#define MOTOR_COUNTS_PER_TURN  524288.0    /* 2^19 */

/*
 * Record the angle between the horizontal and the line that joins
 * the elevation axis bearing to the elevation fork bearing.
 */
#if THEORY
#define AXIS_TO_FORK_SLOPE (13.543 * dtor)  /* Radians */
#else
#define AXIS_TO_FORK_SLOPE 0.234291
#endif

/*
 * Record the length of the line that joins
 * the elevation axis bearing to the elevation fork bearing.
 */
#if THEORY
#define AXIS_TO_FORK 2.8496                 /* Meters */
#else
#define AXIS_TO_FORK 2.87755
#endif

/*
 * Record the length of the line that joins the elevation axis bearing
 * to the point at which the ball screw attaches to the deck.
 */
#if THEORY
#define AXIS_TO_SCREW 2.5937                /* Meters */
#else
#define AXIS_TO_SCREW 2.61444
#endif

/*
 * Record the distance between turns on the ball screw.
 */
#if THEORY
#define BALL_SCREW_PITCH 0.01676            /* Meters */
#else
#define BALL_SCREW_PITCH 0.0169176
#endif

/*
 * Encapsulate the parameters of the geometrical model used to translate
 * ball-screw turns to arcseconds on the sky.
 */
typedef struct {
  double extra_angle;        /* The angle between the face of the deck and */
                             /*  the line connecting the elevation axis to */
                             /*  the top of the ball-screw. */
  double axis_to_fork_slope; /* The angle between the horizontal and */
                             /*  the line that joins the elevation axis */
                             /*  bearing to the elevation fork bearing */
                             /*  (radians). */
  double axis_to_fork;       /* The length of the line that joins the */
                             /*  elevation axis bearing to the elevation */
                             /*  fork bearing (meters). */
  double axis_to_screw;      /* The length of the line that joins the */
                             /*  elevation axis bearing  to the point at */
                             /*  which the ball screw attaches to the */
                             /*  deck (meters). */
  double ball_screw_pitch;   /* The distance between turns on the ball */
                             /*  screw (meters). */
  double rms;                /* The rms of the encoder differences that */
                             /*  result from this model. */
} TelescopeGeometry;

/*
 * Enumerate the registers that we are interested in.
 */
typedef enum {
  FRAME_FEATURES,    /* The bit-mask of frame feature markers */
  PMAC_BUSY,         /* The pmac data-invalid flag */
  PMAC_EL_POS,       /* The position at the elevation axis encoder */
  PMAC_EL_MOTOR_ENC, /* The position of the elevation motor encoder */
  PMAC_DK,           /* The position of the deck axis encoder */
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
  {PMAC_BUSY,         "pmac",    "pmac_write"},
  {PMAC_EL_POS,       "pmac",    "el_pos"},
  {PMAC_EL_MOTOR_ENC, "pmac",    "el_motor_enc"},
  {PMAC_DK,           "pmac",    "dk"},
};

/*
 * Data extracted from a single frame is recorded in a container of
 * the following type. This is a single node of a linked list of
 * nodes.
 */
typedef struct EncoderSample EncoderSample;
struct EncoderSample {
  EncoderSample *next; /* The next encoder sample */
  double axis;         /* The readout of the elevation axis encoder */
  double motor;        /* The readout of the elevation motor encoder */
  double dk;           /* The readout of the deck axis encoder */
  double diff;         /* The difference between the sky positions computed */
                       /*  from the axis and motor encoders (arcsec). */
};

typedef struct {
  FreeList *memory;      /* A free-list of EncoderSample nodes */
  EncoderSample *head;   /* The head of a list of EncoderSample nodes */
  EncoderSample *tail;   /* The tail of a list of EncoderSample nodes */
  int nsample;           /* The number of nodes in the list */
  int pgplot_id;         /* The device id returned by pgopen() */
  double extra_angle;    /* The angle between the face of the deck and */
                         /*  the line connecting the top of the ball-screw */
                         /*  to the elevation axis. */
  double dmin,dmax;      /* The range of the elevatin differences */
} EncoderData;

static EncoderData *new_EncoderData(char *directory, char *calfile,
				    double start_mjd, double end_mjd);
static EncoderData *del_EncoderData(EncoderData *data);
static int read_EncoderData(MonitorStream *ms, char *calfile,
			    EncoderData *data);
static int add_EncoderSample(EncoderData *data, double axis, double motor,
			     double dk);
static void compare_encoders(EncoderData *data, TelescopeGeometry *tg);
static int plot_samples(EncoderData *data);

/*.......................................................................
 * This is a program written to determine to what degree the elevation
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
    TelescopeGeometry best; /* The best fit geometric parameters */
    TelescopeGeometry next; /* The next set of parameters to try */
    const int count = 10;
    int first = 1;
    int j,k,l,m;
    lprintf(stdout, "A total of %d samples were read.\n", data->nsample);
/*
 * Allow the user to request ranges of gear ratios to be plotted.
 */
    for(j=-count; j<count; j++) {
      next.extra_angle = EXTRA_ANGLE * (1.0 + j/(double)count/250.0);
      for(k=-count; k<count; k++) {
	next.axis_to_fork_slope = AXIS_TO_FORK_SLOPE *
	  (1.0 + k/(double)count/250.0);
	for(l=-count; l<count; l++) {
	  next.axis_to_fork = AXIS_TO_FORK  *
	    (1.0 + l/(double)count/500.0);
	  for(m=-count; m<count; m++) {
	    next.axis_to_screw = AXIS_TO_SCREW *
	      (1.0 + m/(double)count/500.0);
#if 0
	    for(n=-count; n<count; n++) {
	      next.ball_screw_pitch = BALL_SCREW_PITCH *
		(1.0 + n/(double)count/500.0);
#else
	      next.ball_screw_pitch = BALL_SCREW_PITCH;
#endif
	      compare_encoders(data, &next);
	      if(first || next.rms < best.rms) {
		best = next;
		first = 0;
		printf("rms=%g (%g %g %g %g %g)\n", next.rms,
		       next.extra_angle, next.axis_to_fork_slope,
		       next.axis_to_fork, next.axis_to_screw,
		       next.ball_screw_pitch);
		cpgpage();
		plot_samples(data);
	      };
#if 0
	    };
#endif
	  };
	};
      };
    };
/*
 * Plot the data to a postscript file.
 */
    data->pgplot_id = cpgopen("/ps");
    if(data->pgplot_id > 0) {
      compare_encoders(data, &best);
      cpgpage();
      plot_samples(data);
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
  data->extra_angle = 0.0;
  data->dmin = data->dmax = 0.0;
  
/*
 * Allocate a freelist for the allocation of EncoderSample nodes.
 */
  data->memory = new_FreeList("new_EncoderData", sizeof(EncoderSample),
			      1000);
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
  double axis;     /* The readout of the elevation axis encoder */
  double motor;    /* The readout of the elevation motor encoder */
  double dk;       /* The readout of the deck axis encoder */
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
	 ms_get_int(ms, my_regs + PMAC_BUSY, 0, 1, &busy) ||
	 ms_get_double(ms, my_regs + PMAC_EL_POS, 0, 1, &axis) ||
	 ms_get_double(ms, my_regs + PMAC_EL_MOTOR_ENC, 0, 1, &motor) ||
	 ms_get_double(ms, my_regs + PMAC_DK, 0, 1, &dk))
	return 1;
/*
 * If we are currently tracking, record the new data.
 */
      if(!busy && features & 1) {
	if(add_EncoderSample(data, axis, motor, dk))
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
 *  data    EncoderData *  The sample will be added to the sample list
 *                         in *data.
 *  axis         double    The readout of the elevation axis encoder.
 *  motor        double    The readout of the elevation motor encoder.
 *  dk           double    The readout of the deck axis encoder.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static int add_EncoderSample(EncoderData *data, double axis, double motor,
			     double dk)
{
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
  sample->axis = axis;
  sample->motor = motor;
  sample->dk = dk;
  sample->diff = 0;
/*
 * Append the new sample to the list.
 */
  if(data->tail)
    data->tail->next = sample;
  else
    data->head = sample;
  data->tail = sample;
  data->nsample++;
  return 0;
}

/*.......................................................................
 * Compare the elevation readouts that are implied by the elevation motor
 * and elevation axis encoders.
 *
 * Input:
 *  data      EncoderData *  The object that contains the data. The difference
 *                           between the encoders will be recorded per sample
 *                           in the list of samples. The range of differences
 *                           will be recorded in data->dmin and data->dmax.
 *  tg  TelescopeGeometry *  The parameters of the geometrical model used
 *                           to translate ball-screw turns to arcseconds on
 *                           the sky.
 */
static void compare_encoders(EncoderData *data, TelescopeGeometry *tg)
{
  EncoderSample *sample;      /* A data sample that is being processed */
  double dmin=0.0, dmax=0.0;  /* The range of the encoder differences */
  double anchor = 0.0;        /* The differences will be rounded to whithin */
                              /*  one turn of this value. */
  double sumsq = 0.0;         /* The sum of the squares of the differences */
  int n = 0;                  /* The number of samples processed */
/*
 * Traverse the list of recorded samples.
 */
  for(sample=data->head; sample; sample=sample->next) {
/*
 * Get the elevation measured by the encoder on the elevation axis.
 */
    double elevation = sample->axis * dtor;
/*
 * Compute the angle between the line that joins the elevation axis bearing to
 * the elevation fork bearing (radians).
 */
    double angle = (halfpi - elevation) + tg->axis_to_fork_slope -
      tg->extra_angle;
/*
 * Work out the current extension of the ball screw (meters).
 */
    double screw_length =
      sqrt(tg->axis_to_screw * tg->axis_to_screw +
	   tg->axis_to_fork * tg->axis_to_fork -
	   2 * tg->axis_to_screw * tg->axis_to_fork * cos(angle));
/*
 * Work out the number of arcseconds of elevation change per turn of
 * the ball screw.
 */
    double arcsec_per_turn = rtoas * tg->ball_screw_pitch * screw_length /
             (tg->axis_to_fork * tg->axis_to_screw * sin(angle));
/*
 * Work out the number of turns of the ball screw corresponding to
 * the above length.
 */
    double nturn = screw_length / tg->ball_screw_pitch;
/*
 * Compute the discrepancy between the predicted and actual readout
 * of the ball-screw encoder.
 */
    double diff = arcsec_per_turn * 
      (fmod(nturn, 1.0) - sample->motor/MOTOR_COUNTS_PER_TURN);
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
    sample->diff = diff;
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
 * Add to the sum of the squares of the differences.
 */
    sumsq += diff * diff;
  };
/*
 * Record the statistics for return.
 */
  data->extra_angle = tg->extra_angle;
  data->dmin = dmin;
  data->dmax = dmax;
  tg->rms = n ? sqrt(sumsq/n) : 0.0;
}


static int plot_samples(EncoderData *data)
{
  EncoderSample *sample; /* The sample being plotted */
  char label[120];       /* A buffer for constructing plot labels in */
/*
 * Compute the margin to leave above and below the plotted points.
 */
  double y_margin = (data->dmax - data->dmin) * 0.07;
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
	 "The difference between the elevation bearing and elevation motor encoders");
#if 0
/*
 * Plot the elevation encoder discrepancies versus deck angle.
 */
  cpgsvp(0.09,0.53, 0.1,0.92);
  cpgswin(-360, 0.0, -y_margin, (data->dmax - data->dmin) + y_margin);
  cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
  cpgmtxt("B", 2.7, 0.5, 0.5, "Deck Angle (degrees)");
  for(sample=data->head; sample; sample=sample->next)
    cpgpt1(sample->dk, sample->diff - data->dmin, 16);
/*
 * Plot the elevation encoder discrepancies versus elevation.
 */
  cpgsvp(0.55,0.99, 0.1,0.92);
#endif
  cpgswin(40.0, 90.0, -y_margin, (data->dmax - data->dmin) + y_margin);
  cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
  cpgmtxt("B", 2.7, 0.5, 0.5, "Elevation (degrees)");
  sprintf(label, "Angle D = %g\\uo",
	  data->extra_angle * rtod);
  cpgmtxt("T", -1.5, 0.97, 1.0, label);
  for(sample=data->head; sample; sample=sample->next)
    cpgpt1(sample->axis, sample->diff - data->dmin, 16);
/*
 * Reveal the plot.
 */
  cpgebuf();
  return 0;
}

