/*--------------------------------------------------------------------
 * «pointing»
 * Analysis of SZA optical pointing data.
 *--------------------------------------------------------------------*/

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cpgplot.h"
#include "date.h"
#include "monitor_stream.h"
#include "color_tab.h"

/* External functions */

float gasdev(long *idum);
int get_command(FILE *fp, char *prompt, int max, int *narg, char *args[]);
int check_file(char *path);
int check_directory(char *path);

/* Constants */

#define PI     3.1415926535897932384626433832795028841971693993751
#define TWOPI  (2.0*PI)
#define HALFPI (PI/2.0)
#define RPDEG  (PI/180.0)
#define RPARCS (PI/(180.0*3600.0))
#define RPHR   (PI/12.0)
#define RPSEC  (RPHR/3600.0)
#define SRC_SIZE     20 /* Maximum chars in source name, including null */
#define MAX_POINTS 2000 /* Maximum number of points to analyse */
#define MAX_STARS   300 /* Maximum number of different stars */
#define MIN_ELEV   10.0 /* Minimum elevation for plots (degrees) */
#define N_PARS       12 /* Number of adjustable model parameters */
#define COLOR_FLAG   15 /* color index used for flagged points */
#define TITLE     "SZA Optical Pointing"   /* Title for plots */

#define PLOT_DEVICE "/xs"
#define PROGNAME  "pointing"

#define HELPFILE  "http://find.uchicago.edu/~dasi/software/pointing.html"

/* Data storage for points ("point" = observation of a star) */

struct point {
  double mjd;      /* epoch of measurement (UTC MJD) */
  double lst;      /* LST of measurement */
  double az;       /* topo azimuth of measurement */
  double el;       /* elevation of measurement */
  double deck;     /* deck-angle offset of measurement */
  double mount_az; /* position of measurement in mount coordinates */
  double mount_el;
  double mount_dk;
  double azerr[4]; /* [0] = corrections applied online */
  double elerr[4]; /* [1] = corrections from current model */
                   /* [2] = residuals online minus model */
                   /* [3] = offsets applied online */
  double tilts[2]; /* Tilt meter readings (x, y) */
  double encrot[2]; /* EL encoder rotations meters */
  double elcorr;   /* Difference between el axis encoder and el motor encoder */
  float w_air_temp;     /* Weather data */
  float w_humidity;
  float w_wind_speed;
  float w_wind_dir;
  float w_pressure;
  int index;       /* index number of star observed */
  int flag;        /* 0 => ok, 1 => flagged as bad */
  unsigned int deck_mode; /* 0 => tracking (t), 1 => fixed (f) */
  int color;       /* color index for plotting */
};

static struct point points[MAX_POINTS];  /* Point data */
static int n_pts;                        /* Number of points found */

/* Data storage for stars */

static char *star_name[MAX_STARS];       /* Star name */
static int star_count[MAX_STARS];        /* Number of points on this star */
static int n_stars;                      /* Number of stars found */

/* Models for the azimuth/elevation dependence of tilt */

static double xtm[4]={0.0, 0.0, 0.0, 0.0};
static double ytm[4]={0.0, 0.0, 0.0, 0.0};

/* Other global data */

static double lat;                    /* Observatory latitude */
static double min_mjd, max_mjd;       /* UTC range spanned by points */
static double online_model[N_PARS];   /* Parameters of the last online model */
static int tilt_on;                   /* != 0: include tilt meters in model */

static double pfac0=0.0, pfac1=0.0;   /* factors applied to el enc rot
					 corrections */

static long seed=-57;                 /* Seed for random number generator */

/* Routines defined later in this file */

static int parse_timerange(char *arg1, char *arg2, double *start, double *end);
static double range(double x);
static double sky_rms(int n, struct point points[], double *biggest);
static int plot_symbol(int index);
static int do_read(char *directory, char *calfile,
				 double start_mjd, double end_mjd,
				 int print, int mark, int check);
static int read_archive(MonitorStream *ms, char *calfile,
			int print, int mark, int check);
static void clip(int n, struct point points[], double threshold, int *count);
static void colorize(int n, struct point points[]);
static void sel_deck(double dk, int n, struct point points[], int *count);
static double gridls(int n, struct point points[], int npar, double pars[],
		   double dpars[]);
static void mean_and_rms(int n, struct point points[],
			 double *mean_az, double *rms_az,
			 double *mean_az_sky, double *rms_az_sky,
			 double *mean_el, double *rms_el);
static void new_model(int n, struct point points[], double model[],
		      double *var);
static void plot_errors(int n, double el_limit, struct point points[],
			double start, double stop, int which, double model[],
			char *device);
static void mod2dplot(double model[], double lat, char *device);
static void print_residuals(int n, struct point points[]);
static void tplot(int n, double el_limit, struct point points[],
		  double start, double stop, double model[], char *device,
		  int ns, char *name[], int sidereal);
static void wxplot(int n, struct point points[],
		  double start, double stop, char *device);
static void encplot(int n, double el_limit, struct point points[],
		  double start, double stop, double model[], char *device);
static void rotplot(int n, double el_limit, struct point points[],
		  double start, double stop, char *device);
static void xyplot(int n, struct point points[], double limit,
		  double start, double stop, char *device);
static void plot_polar(int n, double lat, double el_limit,
		       struct point points[], int ns, char *name[],
		       int count[], double start, double stop,
		       char *device);
static void plot_tilt(int type, int axis, int n, double el_limit,
		      struct point points[],
		      double start, double stop, char *device);
static void fit_tilt(int type, int n, struct point points[]);
static void pointing_model(double az, double el, double lat, double deck,
			   unsigned int deck_mode, 
			   double xtilt, double ytilt,
			   double model[],
			   double *new_az, double *new_el);
static void report_model(int n, struct point points[], double model[]);
static void unflag(int n, struct point points[], int *nflag);
static void pseudodata(int n, struct point points[], double rms);
static double elcorr(double pmac_el_pos, double pmac_motor);
static void enccor(int n, struct point points[]);
static void init_colors(void);
static void tilt_model(int n, struct point points[], double pars[], double *var);
static double tgridls(int n, struct point points[], double pars[], double dpars[]);
static void interact(int n, struct point points[], int ix, int iy, int which,
		     double xmin, double xmax, double ymin, double ymax,
		     double start, double stop);
static void annot_top(char *title1, char *title2, double start, double stop);
static void do_rotcor(int n, struct point points[], double fac0, double fac1);

/*--------------------------------------------------------------------*/

/* Enumeration constants identify the commands */

typedef enum {
  COM_APPEND,
  COM_CALFILE,
  COM_CHECK,
  COM_CLIP,
  COM_DIRECTORY,
  COM_ENCCOR,
  COM_EXIT,
  COM_FIT,
  COM_HELP,
  COM_MARK,
  COM_MODEL,
  COM_PLOT,
  COM_PSEUDODATA,
  COM_QUIT,
  COM_READ,
  COM_RESET,
  COM_ROTCOR,
  COM_SELECT,
  COM_SET,
  COM_TFIT,
  COM_TILT,
  COM_TILTMX,
  COM_TILTMY,
  COM_UNFLAG,
  COM_UTRANGE,
  COM_XYLIMIT,
  NUM_COMMANDS
} CommandIndex;

static struct {
  CommandIndex index;  /* indentifier */
  char *name;          /* command name (lower case) */
  int args;            /* number of required arguments */
  int optargs;         /* number of optional arguments */
  char *usage;         /* list of argument names */
} command_list[NUM_COMMANDS] = {
  {COM_APPEND,     "append",    0, 2, "[start_utc] [end_utc]"},
  {COM_CALFILE,    "calfile",   1, 0, "cal_file"},
  {COM_CHECK,      "check",     1, 0, "0|1"},
  {COM_CLIP,       "clip",      0, 1, "[level]"},
  {COM_DIRECTORY,  "directory", 1, 0, "archive_directory"},
  {COM_ENCCOR,     "enccor",    0, 0, ""},
  {COM_EXIT,       "exit",      0, 0, ""},
  {COM_FIT,        "fit",       0, 1, "parameters" },
  {COM_HELP,       "help",      0, 0, ""},
  {COM_MARK,       "mark",      1, 0, "number"},
  {COM_MODEL,      "model",     0, N_PARS, "[par0] [par1] ..."},
  {COM_PLOT,       "plot",      1, 1, "what [device]"},
  {COM_PSEUDODATA, "pseudodata",0, 1, "rms"},
  {COM_QUIT,       "quit",      0, 0, ""},
  {COM_READ,       "read",      0, 2, "[start_utc] [end_utc]"},
  {COM_RESET,      "reset",     0, 0, ""},
  {COM_ROTCOR,     "rotcor",    2, 0, "factor1 factor2"},
  {COM_SELECT,     "select",    1, 0, "dk"},
  {COM_SET,        "set",       2, 0, "parameter value"},
  {COM_TFIT,       "tfit",      1, 0, "type"},
  {COM_TILT,       "tilt",      1, 0, "0|1"},
  {COM_TILTMX,     "tiltmx",    0, 4, "p1 p2 p3 p4"},
  {COM_TILTMY,     "tiltmy",    0, 4, "p1 p2 p3 p4"},
  {COM_UNFLAG,     "unflag",    0, 0, ""},
  {COM_UTRANGE,    "utrange",   2, 0, "start_utc end_utc"},
  {COM_XYLIMIT,    "xylimit",   1, 0, "range"}
};

static CommandIndex which_command(FILE *fp, char *prompt, int *narg, char *args[]);
static CommandIndex which_command(FILE *fp, char *prompt, int *narg, char *args[])
{
  int i, j;
  char com[21];

  if (get_command(fp, prompt, 20, narg, args))
    return COM_QUIT;
  if (*narg < 1)
    return NUM_COMMANDS;  
  strncpy(com, args[0], sizeof(com)-1);
  for (j=0; j < strlen(com); j++)
    com[j] = tolower(com[j]);
  for (i = 0; i < NUM_COMMANDS; i++) {
    if (strcmp(com, command_list[i].name) == 0) {
      if (*narg-1 < command_list[i].args) {
	fprintf(stderr, "Too few arguments: %s %s\n",
		command_list[i].name, command_list[i].usage);
	return NUM_COMMANDS;
      } else if (*narg-1 > command_list[i].args + command_list[i].optargs) {
	fprintf(stderr, "Too many arguments: %s %s\n",
		command_list[i].name, command_list[i].usage);
	return NUM_COMMANDS;
      } else 
	return command_list[i].index;
    }
  }
  fprintf(stderr, "Unrecognized command: \"%s\". Commands are:\n", com);
  for (i=0; i < NUM_COMMANDS; i++) {
    fprintf(stderr, "%s%c%c", command_list[i].name,
	    i < NUM_COMMANDS-1 ? ',' : '.',
	    i%10 == 9 ? '\n' : ' ');
  }
  fprintf(stderr, "\n");
  return NUM_COMMANDS;
}  

/*--------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
  char *usage="Usage: " PROGNAME " directory cal_file\nSee: " HELPFILE "\n";
  char directory[FILENAME_MAX];  /* The directory in which to look for files */
  char calfile[FILENAME_MAX];    /* The name of the calibration file */
  double start_mjd=0; /* The start UTC as a Modified Julian Date */
  double end_mjd=0;   /* The end UTC as a Modified Julian Date */
  double model[N_PARS];  /* Parameters of the current model */
  double dmodel[N_PARS]; /* Parameter step sizes for model fitting */
  double rms, var, newvar;
  int i, count, iter, index;
  int narg, error;
  char *args[20];
  double utrange[2] = {0.0, 0.0};
  int enccor_done = 0;
  double xylimit = 26.0;
  double threshold;
  char *sza_dir;
  char *arc_dir;
  int mark = 0;
  int check = 0;

  printf("Program " PROGNAME  " " __DATE__ "\n");
  sza_dir = getenv("SZA_DIR");
  arc_dir = getenv("SZA_ARC_DIR");
  if (sza_dir == NULL)
    sza_dir = "/home/goblin/mcs/SZA";
  if (arc_dir == NULL)
    arc_dir = "/scr2/mcs/arc";

  /* Get the command-line arguments */

  if (argc < 1 || argc > 3) {
    fputs(usage, stderr);
    return 1;
  }
  if (argc > 2) {
    strncpy(calfile, argv[2], FILENAME_MAX);
  } else {
    strncpy(calfile, sza_dir, FILENAME_MAX);
    strncat(calfile, "/conf/cal", FILENAME_MAX - strlen("/conf/cal"));
  }
  if (argc > 1) {
    strncpy(directory, argv[1], FILENAME_MAX);
  } else {
    strncpy(directory, arc_dir, FILENAME_MAX);
  }
  if (check_file(calfile)) {
    fprintf(stderr, "File not found: %s\n", calfile);
    return EXIT_FAILURE;
  }
  if (check_directory(directory)) {
    fprintf(stderr, "Directory not found: %s\n", directory);
    return EXIT_FAILURE;
  }
  printf("Archive directory: %s\n", directory);
  printf("Cal file: %s\n", calfile);

  /* Execute interactive commands */

  n_pts = 0;
  n_stars = 0;
  min_mjd = 1e38;
  max_mjd = -1e38;
  tilt_on = 0;
  for (i=0; i < N_PARS; i++)
    model[i] = 0.0;

  for (;;) {
    index = which_command(stdin, PROGNAME "> ", &narg, args);

    switch (index) {

    case COM_CALFILE: /* CALFILE command */
      if (check_file(args[1]))
	fprintf(stderr, "File not found: %s\n", args[1]);
      else
	strncpy(calfile, args[1], FILENAME_MAX);
      printf("Cal file: %s\n", calfile);
      break;
      
    case COM_DIRECTORY: /* DIRECTORY */
      if (check_directory(args[1]))
	fprintf(stderr, "Directory not found: %s\n", args[1]);
      else
	strncpy(directory, args[1], FILENAME_MAX);
      printf("Archive directory: %s\n", directory);
      break;

    case COM_READ: /* READ command */
    case COM_APPEND: /* APPEND command */
      if (parse_timerange(narg > 1 ? args[1] : NULL,
			  narg > 2 ? args[2] : NULL,
			  &start_mjd, &end_mjd)) {
	fputs("Usage: read [start_utc] [end_utc]\n"
	      "UTC syntax: 12-may-1998:12:34:50\n", stderr);
      } else {
#ifdef DEBUG
	printf("Searching for MJD range %.6f to %.6f\n", start_mjd, end_mjd);
#endif
	/* If appending, firts undo rotcor corrections */
	
	if (index == COM_APPEND && (pfac0  != 0.0 || pfac1 != 0.0)) {
	  do_rotcor(n_pts, points, 0.0, 0.0);
	}

         /* Read data from archive files. The number of star-points found is
	  * «n_pts» and the data are in the array «points». The actual range
	  * of UTC spanned by the points is «min_mjd» to «max_mjd». */
	
	/* Initialize data structures */
	if (index != COM_APPEND) {
	  n_pts = 0;
	  n_stars = 0;
	  min_mjd = 1e38;
	  max_mjd = -1e38;
	}
      
	error = 0;
	if (do_read(directory, calfile, start_mjd, end_mjd,
		    0, mark, check))
	  error = 1;
	if (n_pts < 1) {
	  fputs("No star-points found!\n", stderr);
	  error = 1;
	}
	
	/* Use the online model as a starting model */
	
	for (i=0; i < N_PARS; i++) {
	  model[i] = online_model[i];
	}
	tilt_on = 0;
	enccor_done = 0;
	pfac0 = pfac1 = 0.0; /* el enc rot corrections not applied */
	if (!error) new_model(n_pts, points, model, &var);
	if (!error) report_model(n_pts, points, model);
      
	/* Colorize the points */
	
	colorize(n_pts, points);
      }
      break;

    case COM_PLOT: /* PLOT command */
	if (narg < 2 || strcmp("points", args[1]) == 0) {
	  /* Plot the positions of the stars found, in a zenithal equal area
	     projection. */
	  plot_polar(n_pts, lat, MIN_ELEV*RPDEG, points, n_stars,
		     star_name, star_count, min_mjd, max_mjd,
		     narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("corr", args[1]) == 0) {
	  /* Plot the online corrections versus AZ and EL
	     (residuals with no model) */
	  plot_errors(n_pts, MIN_ELEV*RPDEG, points,
		      min_mjd, max_mjd, 0, model,
		      narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("model", args[1]) == 0) {
	  /* Plot the current model */
	  plot_errors(n_pts, MIN_ELEV*RPDEG, points,
		      min_mjd, max_mjd, 1, model,
		      narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("res", args[1]) == 0) {
	  /* Plot the residuals versus AZ and EL */
	  plot_errors(n_pts, MIN_ELEV*RPDEG, points,
		      min_mjd, max_mjd, 2, model,
		      narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("off", args[1]) == 0) {
	  /* Plot the online encoder offsets versus AZ and EL */
	  plot_errors(n_pts, MIN_ELEV*RPDEG, points,
		      min_mjd, max_mjd, 3, model,
		      narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("mod2d", args[1]) == 0) {
	  /* Plot the az and el cor versus az/el as map */
	  mod2dplot(model, lat, narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("tilt", args[1]) == 0) {
	  /* Plot the tilt-meter readings (raw) */
	  plot_tilt(0, 0, n_pts, MIN_ELEV*RPDEG, points, min_mjd, max_mjd,
		    narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("ctilt", args[1]) == 0) {
	  /* Plot the tilt-meter readings (corrected) */
	  plot_tilt(1, 0, n_pts, MIN_ELEV*RPDEG, points, min_mjd, max_mjd,
		    narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("ctt", args[1]) == 0) {
	  /* Plot the tilt-meter readings (corrected) versus UTC */
	  plot_tilt(1, 1, n_pts, MIN_ELEV*RPDEG, points, min_mjd, max_mjd,
		    narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("weather", args[1]) == 0) {
	  /* Plot weather data */
	  wxplot(n_pts, points, min_mjd, max_mjd, 
		 narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("xy", args[1]) == 0) {
	  xyplot(n_pts, points, xylimit, min_mjd, max_mjd, 
		 narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("enc", args[1]) == 0) {
	  double start = min_mjd;
	  double stop = max_mjd;
	  if (utrange[0] != 0.0) start = utrange[0];
	  if (utrange[1] != 0.0) stop = utrange[1];
	  encplot(n_pts, MIN_ELEV*RPDEG, points, start, stop, model,
		  narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("rot", args[1]) == 0) {
	  rotplot(n_pts, MIN_ELEV*RPDEG, points, min_mjd, max_mjd,
		  narg > 2 ? args[2] : PLOT_DEVICE);
	} else if (strcmp("ut", args[1]) == 0) {
	  double start = min_mjd;
	  double stop = max_mjd;
	  if (utrange[0] != 0.0) start = utrange[0];
	  if (utrange[1] != 0.0) stop = utrange[1];
	  tplot(n_pts, MIN_ELEV*RPDEG, points, start, stop, model,
		narg > 2 ? args[2] : PLOT_DEVICE, n_stars, star_name, 0);
	} else if (strcmp("lst", args[1]) == 0) {
	  double start = min_mjd;
	  double stop = max_mjd;
	  if (utrange[0] != 0.0) start = utrange[0];
	  if (utrange[1] != 0.0) stop = utrange[1];
	  tplot(n_pts, MIN_ELEV*RPDEG, points, start, stop, model,
		narg > 2 ? args[2] : PLOT_DEVICE, n_stars, star_name, 1);
	} else {
	  fputs("Usage: plot points|ut|lst|corr|model|res|off|tilt|ctilt|ctt|weather|xy|enc [device]\n", stderr);
	}
	break;

    case COM_TFIT: /* TFIT command */
      fit_tilt(atoi(args[1]), n_pts, points);
      break;

    case COM_TILT: /* TILT command */
      tilt_on = atoi(args[1]);
      printf("Tilt meters are now %s\n", tilt_on ? "enabled" : "disabled");
      new_model(n_pts, points, model, &var);
      report_model(n_pts, points, model);
      break;

    case COM_TILTMX: /* TILTMX command: set parameters for x tilt model */
      if (narg == 5) {
	xtm[0] = atof(args[1]);
	xtm[1] = atof(args[2]);
	xtm[2] = atof(args[3]);
	xtm[3] = atof(args[4]);
      }
      if (narg == 1 || narg == 5) {
	printf("X tilt parameters: %.1f %.1f %.1f %.1f\n",
	       xtm[0], xtm[1], xtm[2], xtm[3]);
      } else {
	fputs("Usage: tiltmx p1 p2 p3 p4\n", stderr);
      }
      break;

    case COM_TILTMY: /* TILTMY command: set parameters for y tilt model */
      if (narg == 5) {
	ytm[0] = atof(args[1]);
	ytm[1] = atof(args[2]);
	ytm[2] = atof(args[3]);
	ytm[3] = atof(args[4]);
      }
      if (narg == 1 || narg == 5) {
	printf("Y tilt parameters: %.1f %.1f %.1f %.1f\n",
	       ytm[0], ytm[1], ytm[2], ytm[3]);
      } else {
	fputs("Usage: tiltmy p1 p2 p3 p4\n", stderr);
      }
      break;

    case COM_ENCCOR: /* ENCCOR command */
      if (enccor_done) {
	fputs("Encoder corrections have already been applied\n", stderr);
      } else {
	fputs("Correcting EL offsets using motor encoder\n", stderr);
	enccor(n_pts, points);
	enccor_done = 1;
      }
      break;

    case COM_ROTCOR: /* ROTCOR command */
      do_rotcor(n_pts, points, atof(args[1]), atof(args[2]));
      break;

    case COM_UTRANGE: /* UTRANGE command */
      error = 0;
      if (narg == 3) {
	char tmp[128];
	if (date_to_mjd(args[1], &utrange[0])) {
	  error = 1; fputs("\n", stderr);
	}
	if (strchr(args[2], '-') == 0) {
	  mjd_to_date(utrange[0], tmp, sizeof(tmp));
	  *strchr(tmp, ' ') = '\0';
	  strcat(tmp, ":");
	} else {
	  *tmp = '\0';
	}
	strncat(tmp, args[2], sizeof(tmp)-strlen(tmp)-1);
	if (date_to_mjd(tmp, &utrange[1])) {
	  error = 1; fputs("\n", stderr);
	}	    
      } else {
	error = 1;
      }
      if (error || utrange[0] < 50000. || utrange[0] >= utrange[1]) {
	fputs("Usage: utrange start-utc end-utc\n", stderr);
	utrange[0] = utrange[1] = 0.0;
      }
      break;

    case COM_XYLIMIT: /* XYLIMIT command */
      if (narg !=2 || (xylimit = atof(args[1])) <= 0) {
	fputs("Usage: xylimit arcsec\n", stderr);
	xylimit = 36.0;
      }
      break;

    case COM_FIT: /* FIT command: fit parameters */
      if (n_pts > 0) {
	double junk;
	static char digits[] = "0123456789ABCDEF";
	rms = sky_rms(n_pts, points, &junk);
	dmodel[0] = 0.02*rms;
	dmodel[1] = 0.02*rms;
	dmodel[2] = 0.02*rms;
	dmodel[3] = 0.02*rms;
	dmodel[4] = 0.02*rms;
	dmodel[5] = 0.1*RPDEG;
	dmodel[6] = 0.02*rms;
	dmodel[7] = 0.02*rms;
	dmodel[8] = 0.02*rms;
	dmodel[9] = 0.1*RPDEG;
	dmodel[10] = 0.02*rms;
	dmodel[11] = 0.02*rms;
	/* Any parameters fixed? */
	if (narg > 1) {
	  for (i=0; i < N_PARS; i++) {
	    if (strchr(args[1], digits[i]) == NULL) dmodel[i] = 0.0;
	  }
	}
	if (!tilt_on) {
	  dmodel[10] = dmodel[11] = 0.0;
	}
	printf("Fitting parameters");
	for (i=0; i < N_PARS; i++) {
	  if (dmodel[i] != 0.0) printf(" %c", digits[i]);
	}
	printf("\n");
	rms = sky_rms(n_pts, points, &junk);
	var = rms*rms;
	iter = 0;
	for (i=0; i<500; i++) {
	  newvar = gridls(n_pts, points, N_PARS, model, dmodel);
	  iter++;
	  if (newvar/var > 0.995) 
	    break;
	  var = newvar;
	}
	printf("Did %d iterations of gridls\n", iter);
	new_model(n_pts, points, model, &var);
	report_model(n_pts, points, model);
      }
      break;

    case COM_CLIP: /* CLIP command: clip "bad" points */
      if (narg < 2)
	threshold = 2.0;
      else
	threshold = atof(args[1]);
      if (narg > 2 || threshold < 1.0) {
	fputs("Usage: clip [threshold > 1.0]\n", stderr);
      } else {
	rms = sqrt(var);
	clip(n_pts, points, threshold*rms, &count);
	printf("%d points flagged\n", count);
	if (count > 0)
	  print_residuals(n_pts, points);
      }
      break;

    case COM_SELECT: /* SELECT command */
      if (narg < 2)
	fputs("Usage: select dk-offset\n", stderr);
      else {
	double dk = atof(args[1]);
	sel_deck(dk, n_pts, points, &count);
	printf("Selecting deck offset %.1f: %d points flagged\n",
	       dk, count);
	if (count > 0)
	  print_residuals(n_pts, points);
      }
      break;

    case COM_UNFLAG: /* UNFLAG command: restore all flagged points */
      unflag(n_pts, points, &count);
      printf("%d points unflagged\n", count);
      if (count > 0)
	print_residuals(n_pts, points);
      break;

    case COM_SET: /* SET command */
      if (narg < 3) {
	fputs("Usage: set param val\n", stderr);
      } else {
	int param = (int) atof(args[1]);
	double val = atof(args[2]);
	if (param >=0 && param < N_PARS)
	  model[param] = val*RPDEG;
	new_model(n_pts, points, model, &var);
	report_model(n_pts, points, model);
      }
      break;

    case COM_MODEL: /* MODEL command */
      if (narg == 1) {
	printf("model");
	for (i=0; i < N_PARS; i++)
	  printf(" %.2f", model[i] / ((i==5 || i==9) ? RPDEG : RPARCS));
	printf("\n");	
      } else {
	for (i=0; i<narg-1; i++)
	  if (i-1 < N_PARS) {
	    model[i] = atof(args[i+1]) * ((i==5 || i==9) ? RPDEG : RPARCS);
	  } else {
	    fputs("Usage: model par0 par1...\n", stderr);
	  }
	new_model(n_pts, points, model, &var);
	report_model(n_pts, points, model);
      }
      break;

    case COM_RESET: /* RESET command: use the online model as a
		       starting model */
      for (i=0; i < N_PARS; i++) {
	model[i] = online_model[i];
      }
      new_model(n_pts, points, model, &var);
      report_model(n_pts, points, model);
      break;

    case COM_PSEUDODATA: /* PSEUDODATA command: reeplace measurements
			    by current model, with optional noise */
      if (narg <= 2) {
	double prms = 0.0;
	if (narg > 1) prms = atof(args[1])*RPARCS;
	printf("Replacing observations with model\n");
	pseudodata(n_pts, points, prms);
      } else {
	fputs("Usage: pseudodata [rms]\n", stderr);
      }
      break;

    case COM_MARK: /* MARK command */
      if (narg == 2) {
	mark = atoi(args[1]);
	printf("Now looking for mark %d\n", mark);
      } else {
	fputs("Usage: mark number\n", stderr);
      }
      break;

    case COM_CHECK: /* CHECK command */
      if (narg == 2) {
	check = atoi(args[1]);
	printf("Checking of online calculations %s\n",
	       check ? "enabled" : "disabled");
      } else {
	fputs("Usage: check 0|1\n", stderr);
      }
      break;

    case COM_HELP: /* HELP command */
      if (system("netscape -remote \'openURL(" HELPFILE ", new-window)\' >/dev/null 2>&1"))
	fputs("Help failed; check that Netscape is running\n", stderr);
      break;

    case COM_EXIT: /* EXIT command */
    case COM_QUIT: /* QUIT command */
      return 0;
      break;

    default:
      break;
    }
  }

  return 0;
}

/*--------------------------------------------------------------------*/

/* Zenithal equal-area projection: convert ZA to radius in projection.
 * The radius in the projection is 2*sin(ZA/2). 
 *
 * Input: ZA (rad)
 * Returns: radius
 */

static double zeap(double theta)
{
  return 2.0*sin(theta/2.0);
}

/* Zenithal equal-area projection: the radius in the projection is
 * 2*sin(ZA/2). 
 *
 * Input: AZ, EL
 * Output: X, Y
 */

static void project_zea(double az, double el, double *x, double *y)
{
  double radius = zeap(PI/2.0 - el);
  *x = -radius*sin(az);
  *y = radius*cos(az);
}

/*---------------------------------------------------------------------*/

/* Convert (HA, Dec) to (AZ,EL) for a given latitude.
 * All angles are in radians.
 *
 * Input: ha (hour angle), dec (declination), lat (latitude).
 * Output: az (azimuth), el (elevation).
 */

static void hadec_to_azel(double ha, double dec, double lat, 
			  double *az, double *el)
{
  double cos_dec = cos(dec);
  double sin_dec = sin(dec);
  double cos_lat = cos(lat);
  double sin_lat = sin(lat);
  double cos_ha = cos(ha);
  double sin_ha = sin(ha);
  double xp = -cos_dec*sin_ha;
  double yp = sin_dec*cos_lat - cos_dec*cos_ha*sin_lat;
  double zp = sin_dec*sin_lat + cos_dec*cos_ha*cos_lat;
  *az = atan2(xp, yp);
  *el = asin(zp);
}


/*--------------------------------------------------------------------*/

/* Plot color maps of AZ and EL errors versus AZ/EL */

static void mod2dplot(double model[], double lat, char *device)
{
#define NAZ 360
#define NEL 80
  
  float aze_map[NAZ*NEL], ele_map[NAZ*NEL];
  float trans[6] = {0,0,0,0,0,0};

  double daz,del;
  double az,el,new_az,new_el;

  double azmax,azmin;
  double elmax,elmin;

  int i,j,ind;

  Cmap *cmap=NULL;
  float contra = -1, bright = 0.5;

  daz = 360/NAZ;
  del = 80/NEL;

  for(i=0; i<NAZ; i++)
  {
    for(j=0; j<NEL; j++)
    {
      ind= i + j*NAZ;
      az=RPDEG*(i+0.5)*daz;
      el=RPDEG*(j+0.5)*del;
      pointing_model(az, el, lat, 0, 1, 0, 0, model, &new_az, &new_el);
      aze_map[ind]=range(new_az-az)/RPARCS;
      ele_map[ind]=range(new_el-el)/RPARCS;
      /* printf("%d %d %f %f %f %f\n",i,j,az/RPDEG,el/RPDEG,
	 aze_map[ind],ele_map[ind]); */
    }
  }

  azmin = azmax = aze_map[0];
  elmin = elmax = ele_map[0];
  for(i=0; i<NAZ; i++)
  {
    for(j=0; j<NEL; j++)
    {
      ind = i + j*NAZ;
      azmin = aze_map[ind] < azmin ? aze_map[ind] : azmin;
      azmax = aze_map[ind] > azmax ? aze_map[ind] : azmax;
      elmin = ele_map[ind] < elmin ? ele_map[ind] : elmin;
      elmax = ele_map[ind] > elmax ? ele_map[ind] : elmax;
    }
  }

  fprintf(stdout,"azmin = %f azmax = %f elmin = %f elmax = %f\n",
	  azmin,azmax,elmin,elmax);
  
  if (cpgopen(device) < 1) return;
  /* Set the transformation matrix for the data array */
  trans[0] = -daz/2;
  trans[1] = daz;
  trans[3] = -del/2;
  trans[5] = del;

  /* Setup a color map */
  for(i=0;i < n_std_cmap;i++)
  {
    if(strcmp(std_cmaps[i].name,"rainbow")==0)
      cmap = &std_cmaps[i];
  }
  cpgctab(cmap->l,cmap->r,cmap->g,cmap->b,cmap->n,contra,bright);

  cpgpage();
  cpgvstd();

  cpgsvp(0.1,0.9,0.1,0.4);
  cpgwnad(0,360,0,80);
  cpgimag(aze_map, NAZ, NEL, 1, NAZ, 1, NEL, azmin, azmax, trans);
  cpgbox("bcnst",0,0,"bcnst",0,0);
  cpglab("AZ (\\(0718))", "EL (\\(0718))", "\\gDAZ");
  cpgwedg("RI",2,4,azmin,azmax,"Arcsec");

  cpgsvp(0.1,0.9,0.5,0.9);
  cpgwnad(0,360,0,80);
  cpgimag(ele_map, NAZ, NEL, 1, NAZ, 1, NEL, elmin, elmax, trans);
  cpgbox("bcnst",0,0,"bcnst",0,0);
  cpglab("AZ (\\(0718))", "EL (\\(0718))", "\\gDEL");
  cpgwedg("RI",2,4,elmin,elmax,"Arcsec");

  cpgend();
}

/*--------------------------------------------------------------------*/
/* Plot AZ and EL errors versus AZ and EL */
/* Input:
 *   which  = 0 for online corrections, 1 for model, 2 for residuals,
 *            3 for online encoder offsets */

static void plot_errors(int n, double el_limit,
			struct point points[],
			double start, double stop,
			int which, double model[],
			char *device)
{
  int i;
  double min_az = 0.0;
  double max_az = 360.0;
  double min_el = el_limit/RPDEG;
  double max_el = 90.0;
  double min_deck = -180.0;
  double max_deck = 180.0;
  float x0, x1, x2, x3, y0, y1, y2, y3, y4;
  char caption[128];
  char *label[] = {"TOTAL ONLINE CORRECTIONS",
		   "MODEL",
		   "ONLINE CORRECTIONS minus MODEL",
                   "ONLINE ENCODER OFFSETS"};

  double min_az_res, max_az_res, min_el_res, max_el_res, delta;
  double min_az_sky, max_az_sky;
  double azerr;
  double rms, mean_az, rms_az, mean_az_sky, rms_az_sky, mean_el, rms_el, maxerr;
  char value[32];
  int len = sizeof(value);
  float x = 0.5, y = 0.5;
  char ch;
  int cursor;

  /* Anything to plot? */

  if (n_pts < 1)
    return;

  azerr = points[0].azerr[which];
  min_az_res = max_az_res = azerr;
  azerr *= cos(points[0].el);
  min_az_sky = max_az_sky = azerr;
  min_el_res = max_el_res = points[0].elerr[which];
  for (i=1; i<n; i++) {
    azerr = points[i].azerr[which];
    if (azerr < min_az_res) min_az_res = azerr;
    if (azerr > max_az_res) max_az_res = azerr;
    azerr *= cos(points[i].el);
    if (azerr < min_az_sky) min_az_sky = azerr;
    if (azerr > max_az_sky) max_az_sky = azerr;
    if (points[i].elerr[which] < min_el_res) min_el_res = points[i].elerr[which];
    if (points[i].elerr[which] > max_el_res) max_el_res = points[i].elerr[which];
  }
  if (fabs(max_az_res) < 0.01*RPDEG && fabs(min_az_res) < 0.01*RPDEG) {
    min_az_res = -0.011*RPDEG;
    max_az_res =  0.011*RPDEG;
  } else {
    delta = 0.1*(max_az_res - min_az_res);
    if (delta < 0.1*RPDEG) delta = 0.01*RPDEG;
    min_az_res -= delta;
    max_az_res += delta;
  }
  if (fabs(max_az_sky) < 0.01*RPDEG && fabs(min_az_sky) < 0.01*RPDEG) {
    min_az_sky = -0.011*RPDEG;
    max_az_sky =  0.011*RPDEG;
  } else {
    delta = 0.1*(max_az_sky - min_az_sky);
    if (delta < 0.1*RPDEG) delta = 0.01*RPDEG;
    min_az_sky -= delta;
    max_az_sky += delta;
  }
  if (fabs(max_el_res) < 0.01*RPDEG && fabs(min_el_res) < 0.01*RPDEG) {
    min_el_res = -0.011*RPDEG;
    max_el_res =  0.011*RPDEG;
  } else {
    delta = 0.1*(max_el_res - min_el_res);
    if (delta < 0.1*RPDEG) delta = 0.01*RPDEG;
    min_el_res -= delta;
    max_el_res += delta;
  }

  /* Open plot device */ 

#ifdef DEBUG
  printf("Plotting %s...\n", label[which]);
#endif
  if (cpgopen(device) < 1) return;
  cpgask(0);
  cpgqinf("CURSOR", value, &len);
  cursor = strcmp(value, "YES") == 0;
  if (cursor)
    printf("Select a frame to expand, or type X to continue\n");
  init_colors();
  cpgpage();

  while (1) {
    cpgbbuf();
    cpgeras();
    cpgsave();

    /* Set the standard viewport, and divide it into parts */

    cpgvstd();
    cpgqvp(0, &x0, &x3, &y0, &y4);
    x1 = (2.0*x0 + x3)/3.0;
    x2 = (x0 + 2.0*x3)/3.0;
    y1 = (3.0*y0 + y4)/4.0;
    y2 = (y0 + y4)/2.0;
    y3 = (y0 + 3.0*y4)/4.0;

    /* Annotation */

    annot_top(TITLE, label[which], start, stop);
    cpgsch(0.6);

    /* AZ_RES versus AZ */
    
    cpgsvp(x0, x1, y3, y4);
    cpgswin(min_az, max_az, min_az_res/RPARCS, max_az_res/RPARCS);
    cpgsci(5);
    cpgmove(min_az, 0.0);
    cpgdraw(max_az, 0.0);
    cpgsci(1);
    cpgbox("BCST", 0.0, 0, "BCNST", 0.0, 0);
    cpglab(" ", "AZ error", " ");
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      azerr = points[i].azerr[which];
      cpgpt1(points[i].az/RPDEG, azerr/RPARCS,
	     plot_symbol(points[i].index));
    }

    /* AZ_SKY versus AZ */
    
    cpgsvp(x0, x1, y2, y3);
    cpgswin(min_az, max_az, min_az_sky/RPARCS, max_az_sky/RPARCS);
    cpgsci(5);
    cpgmove(min_az, 0.0);
    cpgdraw(max_az, 0.0);
    cpgsci(1);
    cpgbox("BCST", 0.0, 0, "BCNST", 0.0, 0);
    cpglab(" ", "AZ error \\x cos(EL)", " ");
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      azerr = points[i].azerr[which] * cos(points[i].el);
      cpgpt1(points[i].az/RPDEG, azerr/RPARCS,
	     plot_symbol(points[i].index));
    }
    
    /* EL_RES versus AZ */
    
    cpgsvp(x0, x1, y1, y2);
    cpgswin(min_az, max_az, min_el_res/RPARCS, max_el_res/RPARCS);
    cpgsci(5);
    cpgmove(min_az, 0.0);
    cpgdraw(max_az, 0.0);
    cpgsci(1);
    cpgbox("BCSTN", 0.0, 0, "BCNST", 0.0, 0);
    cpglab("Azimuth", "EL error", " ");
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1(points[i].az/RPDEG, points[i].elerr[which]/RPARCS,
	     plot_symbol(points[i].index));
    }
    
    /* Write model parameters */
    
    cpgsave();
    cpgsci(4);
    cpgsch(0.5);
    sprintf(caption,
	    "Model [0-3]: FL=%.4f AT1=%.4f AT2=%.4f ET=%.4f",
	    model[0]/RPDEG, model[1]/RPDEG, model[2]/RPDEG, model[3]/RPDEG);
    cpgmtxt("B", 6.5, 0.0, 0.0, caption);
    sprintf(caption,
	    "Model [4-7]: CO=%.4f at %.2f AZ0=%.4f EL0=%.4f",
	    model[4]/RPDEG, model[5]/RPDEG, model[6]/RPDEG, model[7]/RPDEG);
    cpgmtxt("B", 8.0, 0.0, 0.0, caption);
    sprintf(caption,
	    "Model [8-9]: CO2=%.4f at %.2f",
	    model[8]/RPDEG, model[9]/RPDEG);
    cpgmtxt("B", 9.5, 0.0, 0.0, caption);
    mean_and_rms(n, points, &mean_az, &rms_az, &mean_az_sky, &rms_az_sky,
		 &mean_el, &rms_el);
    rms = sky_rms(n, points, &maxerr);
    sprintf(caption, "rms: AZ = %.1f, AZcos(EL) = %.1f, EL=%.1f, sky = %.1f arcsec",
	    rms_az/RPARCS, rms_az_sky/RPARCS, rms_el/RPARCS, rms/RPARCS);
    cpgmtxt("B", 11.0, 0.0, 0.0, caption);
    cpgunsa();

    /* AZ_RES versus EL */
    
    cpgsvp(x1, x2, y3, y4);
    cpgswin(min_el, max_el, min_az_res/RPARCS, max_az_res/RPARCS);
    cpgsci(5);
    cpgmove(min_el, 0.0);
    cpgdraw(max_el, 0.0);
    cpgsci(1);
    cpgbox("BCST", 0.0, 0, "BCST", 0.0, 0);
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      azerr = points[i].azerr[which];
      cpgpt1(points[i].el/RPDEG, azerr/RPARCS,
	     plot_symbol(points[i].index));
    }

    /* AZ_SKY versus EL */
    
    cpgsvp(x1, x2, y2, y3);
    cpgswin(min_el, max_el, min_az_sky/RPARCS, max_az_sky/RPARCS);
    cpgsci(5);
    cpgmove(min_el, 0.0);
    cpgdraw(max_el, 0.0);
    cpgsci(1);
    cpgbox("BCST", 0.0, 0, "BCST", 0.0, 0);
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      azerr = points[i].azerr[which] * cos(points[i].el);
      cpgpt1(points[i].el/RPDEG, azerr/RPARCS,
	     plot_symbol(points[i].index));
    }
    
    /* EL_RES versus EL */
    
    cpgsvp(x1, x2, y1, y2);
    cpgswin(min_el, max_el, min_el_res/RPARCS, max_el_res/RPARCS);
    cpgsci(5);
    cpgmove(min_el, 0.0);
    cpgdraw(max_el, 0.0);
    cpgsci(1);
    cpglab("Elevation", " ", " ");
    cpgbox("BCSTN", 0.0, 0, "BCST", 0.0, 0);
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1(points[i].el/RPDEG, points[i].elerr[which]/RPARCS,
	     plot_symbol(points[i].index));
    }
    
    /* AZ_RES versus DECK */
    
    cpgsvp(x2, x3, y3, y4);
    cpgswin(min_deck, max_deck, min_az_res/RPARCS, max_az_res/RPARCS);
    cpgsci(5);
    cpgmove(min_deck, 0.0);
    cpgdraw(max_deck, 0.0);
    cpgsci(1);
    cpgbox("BCST", 0.0, 0, "BCST", 0.0, 0);
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      azerr = points[i].azerr[which];
      cpgpt1(points[i].mount_dk/RPDEG, azerr/RPARCS,
	     plot_symbol(points[i].index));
    }
    
    /* AZ_SKY versus DECK */
    
    cpgsvp(x2, x3, y2, y3);
    cpgswin(min_deck, max_deck, min_az_sky/RPARCS, max_az_sky/RPARCS);
    cpgsci(5);
    cpgmove(min_deck, 0.0);
    cpgdraw(max_deck, 0.0);
    cpgsci(1);
    cpgbox("BCST", 0.0, 0, "BCST", 0.0, 0);
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      azerr = points[i].azerr[which] * cos(points[i].el);
      cpgpt1(points[i].mount_dk/RPDEG, azerr/RPARCS,
	     plot_symbol(points[i].index));
    }
    
    /* EL_RES versus DECK */
    
    cpgsvp(x2, x3, y1, y2);
    cpgswin(min_deck, max_deck, min_el_res/RPARCS, max_el_res/RPARCS);
    cpgsci(5);
    cpgmove(min_deck, 0.0);
    cpgdraw(max_deck, 0.0);
    cpgsci(1);
    cpglab("Deck angle", " ", " ");
    cpgbox("BCSTN", 0.0, 0, "BCST", 0.0, 0);
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1(points[i].mount_dk/RPDEG, points[i].elerr[which]/RPARCS,
	     plot_symbol(points[i].index));
    }
    
    cpgunsa();
    cpgebuf();

    /* Editing */
    
    if (!cursor) {
      cpgclos();
      return;
    }
    cpgsvp(x0, x3, y0, y4);
    cpgswin(0.0, 3.0, 0.0, 4.0);
    cpgband(0, 0, 0.5, 0.5, &x, &y, &ch);
    if (ch == 'h' || ch == 'H' || ch == '?') {
      printf("H or ?:           print this message\n"
	     "X (right button): exit from plot res\n"
	     "Any other key:    expand frame\n");
    } else if (ch == 'x' || ch == 'X') {
      cpgclos();
      return;
    } else if (0.0 < x && x < 3.0 && 1.0 < y && y < 4.0) {
      double xmin[] = {min_az, min_el, min_deck};
      double xmax[] = {max_az, max_el,  max_deck};
      double ymin[] = {min_el_res/RPARCS, min_az_sky/RPARCS, min_az_res/RPARCS};
      double ymax[] = {max_el_res/RPARCS, max_az_sky/RPARCS, max_az_res/RPARCS};
 
      int ix = x;
      int iy = y-1;
      int i = ix + 3*iy;
      if (i >= 0 && i < 9)
	interact(n, points, ix, iy, which, xmin[ix], xmax[ix], ymin[iy],
		 ymax[iy], start, stop);
    }
  }
}

/*--------------------------------------------------------------------*/

/* Plot star positions in zenithal equal-area projection. */
 
static void plot_polar(int n, double lat, double el_limit,
		       struct point points[],
		       int ns, char *name[],
		       int count[], double start, double stop,
		       char *device)
{
  int i, j;
  char label[128], date_string[128];
  double x, y;
  double za_limit;
  double radius, ypos;
  double dec, halim;
  double cos_zalim, cos_lat, sin_lat, cos_halim;
  double ha, aaz, ael;
  int vis;

  /* Anything to plot? */

  if (n_pts < 1)
    return;

  /* The graph occupies a circle with radius specified by the
     zenith-angle limit */

  za_limit = PI/2.0 - el_limit;
  radius = zeap(za_limit);

  /* Open plot device */ 

  if (cpgopen(device) < 1) return;
  init_colors();
  cpgpage();
  cpgbbuf();
  
  /* Define viewport and shade the circle of the map */

  cpgsave();
  cpgsvp(0.0, 0.8, 0.0, 1.0);
  cpgwnad(-1.1*radius, 1.1*radius, -1.1*radius, 1.1*radius);
  cpgsci(7);
  cpgsfs(1);
  cpgscr(7, 0.85, 0.85, 0.85);
  cpgcirc(0.0, 0.0, radius);
  cpgunsa();
 
  /* Dec grid and labels */
 
  cpgsave();
  cpgsci(15);
  cpgsch(0.7);
  cos_zalim = cos(za_limit);
  cos_lat = cos(lat);
  sin_lat = sin(lat);
  for (i=-80; i <= 80; i += 10) {
    dec = i*RPDEG;
    /* Find the HA limit corresponding to the ZA limit */
    cos_halim = (cos_zalim - sin_lat*sin(dec))/(cos_lat*cos(dec));
    if (cos_halim > 1.0) 
      /* this dec not visible */
      continue;
    if (cos_halim < -1.0)
      /* circumpolar */
      halim = PI;
    else
      halim = acos(cos_halim);
    /* Draw the line of constant declination */
    ypos = 0.0;
    for (j=0; j <= 100; j++) {
      ha = -halim + j*(2.0*halim/100);
      hadec_to_azel(ha, dec, lat, &aaz, &ael);
      project_zea(aaz, ael, &x, &y);
      if (j == 0)
	cpgmove(x,y);
      else
	cpgdraw(x,y);
      if (j == 50) ypos = y;
    }
    sprintf(label, "\\gd=%d", i);
    cpgptxt(0.0, ypos + 0.02*radius, 0.0, 0.5, label);
  }
  cpgunsa();

  /* HA grid */
  
  cpgsave();
  cpgsci(15);
  cpgsch(0.7);
  for (i=-12; i <= 11; i += 1) {
    ha = i*RPHR;
    /* Is the north pole visible? */
    hadec_to_azel(0.0, PI/2.0, lat, &aaz, &ael);
    if (ael > el_limit) {
      vis = 1;
      project_zea(aaz, ael, &x, &y);
      cpgmove(x, y);
    } else {
      vis = 0;
    }
    /* Draw the line of constant HA */
    for (j=0; j <= 200; j++) {
      dec = RPDEG*(90.0 - j*(180.0/200.0));
      hadec_to_azel(ha, dec, lat, &aaz, &ael);
      project_zea(aaz, ael, &x, &y);
      if (ael > el_limit) {
	if (vis) 
	  cpgdraw(x,y);
	else
	  cpgmove(x, y);
	vis = 1;
      } else {
	vis = 0;
      }
    }
  }
  cpgunsa();

  /* AZ/EL grid */
 
  cpgsave();
  cpgsci(0);
  cpgsfs(2);
  for (i=10; i <= za_limit/RPDEG; i += 10) {
    cpgcirc(0.0, 0.0, zeap(i*RPDEG));
  }
  for (i=0; i < 180; i +=30) {
    project_zea(i*RPDEG, el_limit, &x, &y);
    cpgmove(-x, -y);
    cpgdraw(x, y);
  }
  cpgunsa();

  /* AZ/EL labels */
 
  cpgsave();
  cpgsci(4);
  cpgsch(0.7);
  for (i=10; i < za_limit/RPDEG; i += 10){
    x = zeap(i*RPDEG);
    y = 0.02*radius;
    sprintf(label, "%d", 90 - i);
    cpgptxt(x, y, 0.0, 0.5, label);
  }
  for (i=0; i < 360; i +=30) {
    x = -1.02*radius*sin(i*RPDEG);
    y = 1.02*radius*cos(i*RPDEG);
    sprintf(label, "%s%d", i==0 ? "AZ = " : "", i);
    cpgptxt(x, y, i, 0.5, label);
  }
  cpgsch(1.0);
  cpgpt1(0.0, 1.08*radius,  'N');
  cpgpt1(-1.08*radius, 0.0,  'E');
  cpgpt1(0.0, -1.08*radius, 'S');
  cpgpt1(1.08*radius, 0.0, 'W');
 
  cpgunsa();
 
  /* Plot points (one for each star measurement) */

  cpgsave();
  cpgsch(0.7);
  cpgsci(2);
  for (i=0; i < n; i++) {
    project_zea(points[i].az, points[i].el, &x, &y);
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(x, y, plot_symbol(points[i].index));
  }
  cpgunsa();

  /* Add star legend, showing plot symbol and star name */

  cpgsave();
  cpgsci(4);
  cpgsch(0.5);
  for (i=0; i < ns; i++) {
    if (count[i] > 0) {
      sprintf(label, "%c", plot_symbol(i));
      cpgmtxt("RV", 2.0, 0.85 - i/72.0, 0.0, label);
      sprintf(label, "%s (%d)", name[i], count[i]);
      cpgmtxt("RV", 4.0, 0.85 - i/72.0, 0.0, label);
    }
  }
  cpgunsa();

  /* Add additional annotation */

  cpgsave();
  cpgsci(1);
  cpgsch(1.0);
  cpgslw(2);
  cpgsvp(0.0, 1.0, 0.0, 1.0);
  cpgmtxt("T", -1.5, 0.0, 0.0, TITLE);
  cpgsch(0.8);

  mjd_to_date(start, date_string, sizeof(date_string));
  sprintf(label, "UTC: %s", date_string);
  cpgmtxt("T", -2.0, 1.0, 1.0, label);
  mjd_to_date(stop, date_string, sizeof(date_string));
  sprintf(label, "to %s", date_string);
  cpgmtxt("T", -3.5, 1.0, 1.0, label);

  sprintf(label, "Latitude %.1f°", lat/RPDEG);
  cpgmtxt("T", -4.0, 0.0, 0.0, label);
  sprintf(label, "Min elevation %.1f°", el_limit/RPDEG);
  cpgmtxt("T", -5.5, 0.0, 0.0, label);
  sprintf(label, "%d points", n);
  cpgmtxt("T", -7.0, 0.0, 0.0, label);
  cpgunsa();

  /* End of plot */

  cpgclos();
}

/*--------------------------------------------------------------------*/

/* Put an angle in radians into range (-PI,PI). */

static double range(double x)
{
  x = fmod(x, 2.0*PI);
  if (x > PI)   x -= 2.0*PI;
  if (x <= -PI) x += 2.0*PI;
  return x;
}

/*--------------------------------------------------------------------*/

/* Choose a PGPLOT graph marker for a given star index number.
 * This routine is needed because there can be more index numbers
 * than available markers, and not all the PG markers are appropriate.
 * The routine uses markers 2-18, followed by upper=case A-Z,
 * followed by lower-case a-z, for a total of 69 symbols. The sequence
 * is repeated for index number > 68.
 *
 * Input: index (index number, >= 0)
 * Returns: PGPLOT marker number.
 */

static int plot_symbol(int index)
{
  int symbol = (index % 69);
  if (symbol < 0)
    symbol = 2;
  else if (symbol < 17)
    symbol += 2;
  else if (symbol < 43)
    symbol += 'A' - 17;
  else 
    symbol += 'a' - 43;
  return symbol;
}

/*--------------------------------------------------------------------*/

/* In this routine we calculate the corrected az and el given a
 * specified topocentric az and el and a pointing model.
 */


/* GIVEN: topocentric (AZ, EL) «az, el», apply the following corrections
 * to determine mount (AZ, EL) «new_az, new_el»:
 *
 * (1) Flexure (1 parameter)
 * (2) Tilt of azimuth axis (2 parameters)
 * (3) Tilt of elevation axis (1 parameter)
 * (4) Collimation (2 parameters + 2 parameters)
 * (5) Encoder zero-points (2 parameters)
 * (6) Tilt-meter zero-points (2 parameters)
 *
 * Total: N_PARS=10  parameters «model[0..11]».
 *
 * We need to know the observatory latitude «lat» and the current
 * deck angle offset «deck»; if tilt-meters are enabled (global
 * parameter «tilt_on»), we also use the two readings «xtilt» and
 * «ytilt» (arcsec).
 */

static void pointing_model(double az, double el,
			   double lat, double deck, unsigned int deck_mode,
			   double xtilt, double ytilt,
			   double model[], 
			   double *new_az, double *new_el)
{
  double dec;     /* The instrumental apparent declination (radians) */
  double ha;      /* The instrumental apparent hour angle (radians) */
  double pa;
  double sin_dec; /* sin(dec) */
  double cos_dec; /* cos(dec) */
  double sin_ha;  /* sin(ha) */
  double cos_ha;  /* cos(ha) */

  double sin_lat = sin(lat);
  double cos_lat = cos(lat);

  double sin_az = sin(az);
  double cos_az = cos(az);
  double cos_el = cos(el);
  double sin_el;

  double ha_tilt, lat_tilt;

  /* (1) Adjust the elevation to account for telescope flexure.
   * Uses:
   *  model[0] = the constant of flexure (radians per cosine of
   *             elevation). */

  el -= model[0]*cos_el;
  sin_el = sin(el);
  cos_el = cos(el);

  /* (2) Correct the tilt of the azimuth axis. This is equivalent to
   * an error in the hour angle and latitude of the site.
   * Uses:
   *  model[1] = The HA tilt of the azimuth axis.
   *  model[2] = The Latitude tilt of the azimuth axis. */

  /* Compute the instrumental apparent declination of the source. */

  sin_dec = sin_lat*sin_el + cos_lat*cos_el*cos_az;
  dec = asin(sin_dec);
  cos_dec = cos(dec);

  /* Compute the instrumental apparent hour angle of the source. */

  {
    double cos_dec_sin_ha = -sin_az * cos_el;
    double cos_dec_cos_ha = sin_el*cos_lat - sin_lat*cos_el*cos_az;
    if (cos_dec_sin_ha != 0.0 || cos_dec_cos_ha != 0.0)
      ha = atan2(cos_dec_sin_ha, cos_dec_cos_ha);
    else
      ha = 0.0;
  };

  ha_tilt = 0.0;
  lat_tilt = 0.0;

#if 0
  /* Use the tilt meters to estimate a contribution to the tilt
     of the az-axis, if requested. Uses model[10] = xtilt meter zero,
     model[11] = y tilt meter zero. */

  if (tilt_on) {
    xtilt = xtilt*RPARCS - model[10];
    ytilt = ytilt*RPARCS - model[11];
    lat_tilt = (xtilt*cos_az + ytilt*sin_az);
    ha_tilt  = (xtilt*sin_az - ytilt*cos_az)/cos(lat);
  }
#endif

  /* Modify the declination and hour angle to account for the azimuth tilt. */

  ha += model[1] + ha_tilt;
  sin_ha = sin(ha);
  cos_ha = cos(ha);
  lat += model[2] + lat_tilt;
  sin_lat = sin(lat);
  cos_lat = cos(lat);

  /* Compute the modified azimuth and elevation.
   * (Note: if the new EL is 90, the new AZ doesn't matter, so we 
   * leave it unchanged.) */
  
  sin_el = sin_dec*sin_lat + cos_dec*cos_lat*cos_ha; 
  el = asin(sin_el);
  cos_el = cos(el);
  {
    double cos_el_sin_az = -sin_ha*cos_dec;
    double cos_el_cos_az = sin_dec*cos_lat - cos_dec*sin_lat*cos_ha;
    if (cos_el_sin_az != 0.0 || cos_el_cos_az != 0.0) {
      az = atan2(cos_el_sin_az, cos_el_cos_az);
      sin_az = sin(az);
      cos_az = cos(az);
    }
  };

#if 1

  /* Correction for tilt meters */

  if (tilt_on) {
    xtilt = xtilt*RPARCS - model[10];
    ytilt = ytilt*RPARCS - model[11];
    el -= xtilt;
    az -= ytilt*sin(el)/cos(el);
    cos_el = cos(el);
    sin_el = sin(el);
    cos_az = cos(az);
    sin_az = sin(az);
  }
#endif

  /* Compute the modified parallactic angle */

  {
    double cos_el_sin_pa = sin_ha*cos_lat;
    double cos_el_cos_pa = sin_lat*cos_dec - sin_dec*cos_lat*cos_ha;
    if (cos_el_cos_pa==0.0 && cos_el_sin_pa==0.0) {
      pa = 0.0;
    } else {
      pa = atan2(cos_el_sin_pa, cos_el_cos_pa);
    }
  };

  /* (3) Correct the misalignment of the elevation axis.
   * Uses:
   *  model[3] = The tilt of the elevation axis. */

  sin_el /= cos(model[3]);
  el = asin(sin_el);
  cos_el = cos(el);
  az -= (cos_el == 0.0) ? 0.0 :
                          asin(tan(model[3])*sin_el/cos_el);
  sin_az = sin(az);
  cos_az = cos(az);

  /* Correct the parallactic angle */

  if (cos_el != 0.0)
    pa -= asin(sin(model[3])/cos_el);

  /* (4) Correct for collimation error.
   * Uses:
   *  model[4] = magnitude of first harmonic collimation error
   *  model[5] = dec angle at which collimation error is directed
   *             radially outwards.
   *  model[8] = magnitude of second harmonic collimation error
   *  model[9] = dec angle at which collimation error is directed
   *             radially outwards.
   */

  {
    double cos_coll = cos(model[4]);
    double sin_coll = sin(model[4]);
    double sin_deck, cos_deck;
    double sin2deck, cos2deck;
    double t;

    /* The apparent parallactic angle being tracked */

    if (deck_mode == 0) {
      deck += pa;
    }

    /* Compute the deck position angle at which the collimation tilt points
     * radially outward from the axis of the deck.  */

    deck += model[5];
    sin_deck = sin(deck);
    cos_deck = cos(deck);
    sin2deck = sin(2.0*deck + model[9]);
    cos2deck = cos(2.0*deck + model[9]);

    /* Correct the azimuth. */
  
    if (cos_el != 0.0) {
      az -= asin(sin_coll*sin_deck/cos_el); /* Note change */
      az -= model[8]*sin2deck/cos_el;   /* Approximate */
    };

    /* Correct the elevation. */
  
    t = (cos_coll*cos_coll) + (cos_deck*cos_deck)*(sin_coll*sin_coll);
    sin_el = (cos_coll*sin_el - sin_coll*cos_deck*sqrt(t - sin_el*sin_el))
             / t;
    el = asin(sin_el);
    el -= model[8]*cos2deck; /* Approximate */
  }

  /* (5) Add encoder zero-points. */

  *new_az = fmod(az + model[6], TWOPI);
  if (*new_az < 0.0) *new_az += TWOPI;
  *new_el = el + model[7];
}

/*--------------------------------------------------------------------*/

/* Compute mean and rms of az and el residuals */

static void mean_and_rms(int n, struct point points[],
			 double *mean_az, double *rms_az,
			 double *mean_az_sky, double *rms_az_sky,
			 double *mean_el, double *rms_el)
{
  int j, count = 0;
  double sum_az = 0.0, sum_el = 0.0, sum_azsq=0.0, sum_elsq = 0.0, dev;
  double sum_azsky = 0.0, sum_azskysq = 0.0;
  for (j=0; j<n; j++) {
    sum_az    += points[j].azerr[2];
    sum_azsky += points[j].azerr[2]*cos(points[j].el);
    sum_el    += points[j].elerr[2];
  }
  *mean_az     = sum_az/n;
  *mean_az_sky = sum_azsky/n;
  *mean_el     = sum_el/n;
  for (j=0; j<n; j++) {
    if (points[j].flag == 0) {
      dev = points[j].azerr[2] - (*mean_az);
      sum_azsq += dev*dev;
      dev = points[j].azerr[2]*cos(points[j].el) - (*mean_az_sky);
      sum_azskysq += dev*dev;
      dev = points[j].elerr[2] - (*mean_el);
      sum_elsq += dev*dev;
      count ++;
    }
  }
  *rms_az     = (count > 0) ? sqrt(sum_azsq/count) : 0.0;
  *rms_az_sky = (count > 0) ? sqrt(sum_azskysq/count) : 0.0;
  *rms_el     = (count > 0) ? sqrt(sum_elsq/count) : 0.0;
}

/* Compute rms deviation on sky given AZ and EL errors */

static double sky_rms(int n, struct point points[], double *biggest)
{
  int j, count=0;
  double sum = 0.0, maxerr = 0.0, t1, t2, errsq;
  for (j=0; j<n; j++) {
    if (points[j].flag == 0) {
      t1 = points[j].elerr[2];
      t2 = points[j].azerr[2]*cos(points[j].el);
      errsq = t1*t1 + t2*t2;
      if (errsq > maxerr)
	maxerr = errsq;
      sum += errsq;
      count++;
    }
  }
  *biggest = sqrt(maxerr);
  return (count > 0) ? sqrt(sum/count) : 0.0;
}

/*--------------------------------------------------------------------*/

/* Colorize points based on day */

static void colorize(int n, struct point points[])
{
  int j;
  int day, day0, color;
  static int colors[] = {2, 3, 4, 6, 8, 12, 13};
  day0 = (int) points[0].mjd;
  color = 0;
  for (j=0; j<n; j++) {
    day = (int) points[j].mjd;
    if (day != day0) {
      color = (color+1) % 7;
      day0 = day;
    }
    points[j].color = colors[color];
  }
}

/*--------------------------------------------------------------------*/

/* Flag points with deviations larger than specified */

static void clip(int n, struct point points[], 
		 double threshold, int *nflag)
{
  int j;
  double sum, t, thresh2 = threshold*threshold;
  *nflag = 0;
  for (j=0; j<n; j++) {
    if (points[j].flag == 0) {
      t = points[j].elerr[2];
      sum = t*t;
      t = points[j].azerr[2]*cos(points[j].el);
      sum += t*t;
      if (sum > thresh2) {
	points[j].flag = 1;
	*nflag += 1;
      }
    }
  }
}

/* Select points at canonical deck angle */

static void sel_deck(double dk, int n, struct point points[], int *nflag)
{
  int j;
  *nflag = 0;
  for (j=0; j<n; j++) {
    if (points[j].flag == 0) {
      if (fabs(points[j].deck - dk*RPDEG) > 1.0*RPDEG) {
	points[j].flag = 1;
	*nflag += 1;
      }
    }
  }
}

/* Unflag all points */

static void unflag(int n, struct point points[], int *nflag)
{
  int j;
  *nflag = 0;
  for (j=0; j<n; j++) {
    if (points[j].flag != 0) {
      points[j].flag = 0;
      *nflag += 1;
    }
  }
}

/*--------------------------------------------------------------------*/

/* Grid search in parameter space */

static double gridls(int n, struct point points[],
	    int npar, double pars[], double dpars[])
{
  int j, neval;
  double var0, var1, var2, delta;

  /* Loop through parameters */

  for (j=0; j < npar; j++){
    neval = 0;
    if (dpars[j] == 0.0)
      continue;
    /* Evaluate starting var */
    new_model(n, points, pars, &var1);
    neval++;
    /* Increment parameter and re-evaluate */
    delta = dpars[j];
    pars[j] += delta;
    new_model(n, points, pars, &var2);
    neval++;
    if (var1 < var2) {
      /* increasing rms: reverse direction */
      var0 = var2;
      delta = -delta;
      pars[j] += 2.0*delta;
      new_model(n, points, pars, &var2);
      neval++;
    }
    while (var2 < var1) {
      var0 = var1;
      var1 = var2;
      pars[j] += delta;
      new_model(n, points, pars, &var2);
      neval++;
    }
    /* Found local minimum: go back */
    pars[j] -= delta;

    /* Adjust step size depending on how many steps we took 
       in this direction */
    if (neval > 5)
      /* increase step size for next call */
      dpars[j] *= neval/5.0;
    else if (neval <5)
      /* decrease step size for next call */
      dpars[j] *= 0.5;
  }
  new_model(n, points, pars, &var0);
  return var0;
}

/*--------------------------------------------------------------------*/

/* Replace the observations with the model (i.e., make observations
 * agree with current model. Used for testing fit procedures. */

static void pseudodata(int n, struct point points[], double rms)
{
  int i;
  for (i=0; i<n_pts; i++) {
    points[i].azerr[0] = points[i].azerr[1];
    points[i].elerr[0] = points[i].elerr[1];
    if (rms > 0.0) {
      points[i].azerr[0] += rms*gasdev(&seed);
      points[i].elerr[0] += rms*gasdev(&seed);
    }
    points[i].azerr[2] = points[i].azerr[0] - points[i].azerr[1];
    points[i].elerr[2] = points[i].elerr[0] - points[i].elerr[1];
  }
}

/*--------------------------------------------------------------------*/

/* Evaluate the contribution of the model. Given the pointing model
 * parameters in «model», this routine computes the model contributions
 * and the residuals in the «points» structure, and returns the
 * sky-variance of the residuals in «var». */

static void new_model(int n, struct point points[],
		      double model[], double *var)
{
  int i;
  double new_az, new_el, rms, junk;

  for (i=0; i<n_pts; i++) {
    pointing_model(points[i].az, points[i].el, lat, points[i].deck,
		   points[i].deck_mode, 
		   points[i].tilts[0], points[i].tilts[1],
		   model, &new_az, &new_el);
    points[i].azerr[1] = range(new_az - points[i].az);
    points[i].elerr[1] = range(new_el - points[i].el);
    points[i].azerr[2] = points[i].azerr[0] - points[i].azerr[1];
    points[i].elerr[2] = points[i].elerr[0] - points[i].elerr[1];
  }
  rms = sky_rms(n_pts, points, &junk);
  *var = rms*rms;
}

/*--------------------------------------------------------------------*/

/* Get pointing data from archive files.
 * Input:
 *  char *directory;    The directory in which to look for files
 *  char *calfile;      The name of the calibration file
 *  double start_mjd;   The start UTC as a Modified Julian Date
 *  double end_mjd;     The end UTC as a Modified Julian Date
 */

/* Enumeration constants identify the registers of interest */

typedef enum {
  FRAME_UTC,         /* The UTC of the end of the frame */
  SRCNAM,            /* The source name */
  FEATURES,          /*  Register containing on-star bit */
  LST,
  SITE,
  EQUAT_GEOC,
  EQUAT_OFF,
  HORIZ_OFF,
  TILTS,
  FLEXURE,
  COLLIMATION,
  REFRACTION,
  HORIZ_GEOC,
  HORIZ_TOPO,
  HORIZ_MOUNT,
  ENCODER_OFF,
  AXIS,
  DECK_MODE,
  X_TILT,
  Y_TILT,
  PMAC_EL_POS,
  PMAC_EL_MOTOR_ENC,
  PMAC_EL_ENC_ROT,
  W_AIR_TEMP,
  W_HUMIDITY,
  W_WIND_SPEED,
  W_WIND_DIR,
  W_PRESSURE,
/* The number of registers specified above - this must be last */
  NUM_MY_REGS
} MyRegIndex;

/* Associate the registers that we enumerated above with their
 * official register-map names. The name of a register consists of the
 * name of the VME or software board that it resides on, plus the name
 * assigned to a particular address range on that board. */

MonitorSelection my_sel[NUM_MY_REGS] = {
  {FRAME_UTC,        "frame",   "utc"},
  {SRCNAM,           "tracker", "source"},
  {FEATURES,         "frame",   "features"},
  {LST,              "tracker", "lst"},
  {SITE,             "tracker", "site"},
  {EQUAT_GEOC,       "tracker", "equat_geoc"},
  {EQUAT_OFF,        "tracker", "equat_off"},
  {HORIZ_OFF,        "tracker", "horiz_off"},
  {TILTS,            "tracker", "tilts"},
  {FLEXURE,          "tracker", "flexure"},
  {COLLIMATION,      "tracker", "collimation"},
  {REFRACTION,       "tracker", "refraction"},
  {HORIZ_GEOC,       "tracker", "horiz_geoc"},
  {HORIZ_TOPO,       "tracker", "horiz_topo"},
  {HORIZ_MOUNT,      "tracker", "horiz_mount"},
  {ENCODER_OFF,      "tracker", "encoder_off"},
  {AXIS,             "tracker", "axis"},
  {DECK_MODE,        "tracker", "deck_mode"},
  {X_TILT,           "pmac",    "x_tilt"},
  {Y_TILT,           "pmac",    "y_tilt"},
  {PMAC_EL_POS,      "pmac",    "el_pos"},
  {PMAC_EL_MOTOR_ENC,"pmac",    "el_motor_enc"},
  {PMAC_EL_ENC_ROT,  "pmac",    "el_enc_rot"},
  {W_AIR_TEMP,       "weather", "air_temp"},
  {W_HUMIDITY,       "weather", "humidity"},
  {W_WIND_SPEED,     "weather", "wind_speed"},
  {W_WIND_DIR,       "weather", "wind_dir"},
  {W_PRESSURE,       "weather", "pressure"}
};

static int do_read(char* directory, char *calfile,
		   double start_mjd, double end_mjd,
		   int print, int mark, int check)
{
  int status;
  MonitorStream *ms;  /* A stream for retrieving archive data */

  /* Open the archive stream and return its handle. */

  ms = new_FileMonitorStream(directory, start_mjd, end_mjd);
  if (!ms)
    return 1;

  /* Read data from the stream */

  status = read_archive(ms, calfile, print, mark, check);

  /* Get rid of the stream */

  (void) del_MonitorStream(ms);

  return status;
}


static int read_archive(MonitorStream *ms, char *calfile,
		   int print, int mark, int check)
{
  int count = 0;      /* The number of frames read so far */
  char saved_source[SRC_SIZE]; /* Current source name */
  int current_index = 0;  /* Source number */
  char *p;
  int i;
#ifdef DEBUG
  double new_az, new_el;
#endif
  FILE *logfile;
  char *filename = "pointing-logfile.txt";
  char date[128];
  double saved_day = 0.0;
  unsigned int mask = (1<<mark);

  /* Structure to hold model details */

  struct source_model { 
    double lst;
    double site[3];       /* lat, long, altitude */
    double equat_geoc[3]; /* ra, dec, distance */
    double equat_off[2];
    double horiz_off[3];
    double tilts[3];
    double flexure;
    double collimation[2];
    double refraction[2];
    double hgeoc[3];
    double htopo[3];
    double hmount[3];
    double encoff[3];
    unsigned int axis;
    unsigned int deck_mode;
    double tiltm[2];
  } src;
 
  /* The following array will contain register-map specific information
   * about the selected registers. */

  RegMapReg my_regs[NUM_MY_REGS];

  strncpy(saved_source, "@@@@@", SRC_SIZE);

  logfile = fopen(filename, "w");
  if (logfile == NULL) 
    fprintf(stderr, "Unable to open logfile %s\n", filename);
  printf("     MJD        UTC       AZ       EL       Deck    AZ err   EL err  Star\n");
  if (logfile != NULL) {
    if (mark == 20) {
      fprintf(logfile, "     MJD        UTC       AZ        x        y\n");
    } else {
      fprintf(logfile, "     MJD        UTC       AZ       EL       Deck    AZ err   EL err  Star\n");
    }
  }

  /* Look up and select the registers that are to be retrieved, then
   * load calibration parameters from the specified calibration file. */

  if(ms_select_regs(ms, 1, 0, my_sel, NUM_MY_REGS, my_regs) != MS_SEND_DONE ||
     ms_load_cal_file(ms, "", calfile)) {
    fputs("ms_select_regs call failed\n", stderr);
    return 1;
  }

  /* Read records from the monitor stream until the end of the stream
   * is reached. */

  while(1) {

    /* Read the next frame of registers. */
    switch(ms_read_frame(ms, 1)) {

    case MS_READ_AGAIN:  /* We should never get this when the dowait */
                         /*  argument or ms_read_frame() is 1, so treat */
                         /*  this as an error. */
    case MS_READ_BREAK:  /* A stream is usually composed of many files, */
                         /*  spliced together behind the scenes. The */
                         /*  MS_READ_BREAK code is returned each time */
                         /*  that the end of a file is reached. It is */
                         /*  usually ignored - but see MS_READ_REGMAP below. */
      break;
    case MS_READ_ENDED:  /* We have reached the end of the requested time */
                         /*  range. */
      printf("End of stream reached.\n");
      printf("Total of %d points on %d stars\n", n_pts, n_stars);
      if (logfile != NULL) {
	fprintf(logfile, "Total of %d points on %d stars\n", n_pts, n_stars);
	(void) fclose(logfile);
      }
#ifdef DEBUG
      for (i=0; i < n_stars; i++) {
	printf("Source number %d = %s (%d points)\n",
	       i, star_name[i], star_count[i]);
      }
#endif
      return 0;
      break;
    case MS_READ_DONE:   /* A new frame of register values has been read */
/*
 * Process this frame: extract selected registers.
 */
      {
	double utc[2];   /* The value of frame.utc[0..1] (MJD days,hours) */
	char source[SRC_SIZE]; /* source name */
	double mjd;      /* modified Julian date */
	unsigned int features;
	double pmac_el_pos;
	double pmac_motor;
	double pmac_el_enc_rot[2];

	if(ms_get_double(ms, my_regs + FRAME_UTC, 0, 2, utc) ||
	   ms_get_string(ms, my_regs + SRCNAM, SRC_SIZE, source) ||
	   ms_get_uint(ms, my_regs + FEATURES, 0, 1, &features) )
	  return 1;
	count++;
#if 0
	mjd = utc[0] + utc[1]/86400000.0;
	printf("%6d %12.6f %d\n", count, mjd, features);
#endif

	/* LSB of "features" identifies a pointing click; these are the
	   frames we are interested in. */

	if (features & mask) {

	  /* (min_mjd, max_mjd) will contain the time range spanned by
	     the pointing frames */

	  mjd = utc[0] + utc[1]/86400000.0;  /* utc[0]=MJD day, utc[1]=ms */
	  if (mjd < min_mjd) min_mjd = mjd;
	  if (mjd > max_mjd) max_mjd = mjd;

	  /* Check for a new source */

	  if (strcmp(source, saved_source) != 0) {
	    strncpy(saved_source, source, SRC_SIZE);
	    /* See if we have observed this source before */
	    current_index = -1;
	    for (i=0; i < n_stars; i++) {
	      if (strcmp(source, star_name[i]) == 0) {
		current_index = i;
		break;
	      }
	    }
	    if (current_index == -1) {
	      /* Create an entry in the table for the new source */
	      n_stars += 1;
	      if (n_stars > MAX_STARS) {
		printf("Maximum number of stars exceeded\n");
		return 1;
	      }
	      current_index = n_stars - 1;
	      p = malloc(SRC_SIZE);
	      strncpy(p, source, SRC_SIZE);
	      star_name[current_index] = p;
	      star_count[current_index] = 0;
	    }
	  }
#ifdef DEBUG
	  printf("%3.3d: UTC=%.6f features=%d star=%s index=%d\n",
		 count, mjd, features, source, current_index);
#endif
	  /* Get the model from the monitor stream */

	  if(ms_get_double(ms, my_regs + LST,  0, 1, &src.lst) ||
	     ms_get_double(ms, my_regs + SITE, 0, 3, src.site) ||
	     ms_get_double(ms, my_regs + EQUAT_GEOC, 0, 3, src.equat_geoc) ||
	     ms_get_double(ms, my_regs + EQUAT_OFF,  0, 2, src.equat_off) ||
	     ms_get_double(ms, my_regs + HORIZ_OFF, 0, 3, src.horiz_off) ||
	     ms_get_double(ms, my_regs + TILTS, 0, 3, src.tilts) ||
	     ms_get_double(ms, my_regs + FLEXURE, 0, 1, &src.flexure) ||
	     ms_get_double(ms, my_regs + COLLIMATION, 0, 2, src.collimation) ||
	     ms_get_double(ms, my_regs + REFRACTION, 0, 2, src.refraction) ||
	     ms_get_double(ms, my_regs + HORIZ_GEOC, 0, 3, src.hgeoc)||
	     ms_get_double(ms, my_regs + HORIZ_TOPO, 0, 3, src.htopo)||
	     ms_get_double(ms, my_regs + HORIZ_MOUNT, 0, 3, src.hmount) ||
	     ms_get_double(ms, my_regs + ENCODER_OFF, 0, 3, src.encoff) ||
	     ms_get_double(ms, my_regs + X_TILT, 0, 1, &src.tiltm[0]) ||
	     ms_get_double(ms, my_regs + Y_TILT, 0, 1, &src.tiltm[1]) ||
	     ms_get_uint(ms, my_regs + AXIS, 0, 1, &src.axis) ||
	     ms_get_uint(ms, my_regs + DECK_MODE, 0, 1, &src.deck_mode) ||
	     ms_get_double(ms, my_regs + PMAC_EL_POS, 0, 1, &pmac_el_pos) ||
	     ms_get_double(ms, my_regs + PMAC_EL_ENC_ROT, 0, 2, pmac_el_enc_rot) ||
	     ms_get_double(ms, my_regs + PMAC_EL_MOTOR_ENC, 0, 1, &pmac_motor)
	     ) {
	    printf("Error extracting data from monitor stream\n");
	    return 1;
	  }

	  /* Check for optical axis */

	  if (src.axis) {
	    printf("WARNING: not using optical axis, axis=%u\n", src.axis);
	    if (logfile) 
	      fprintf(logfile, "WARNING: not using optical axis, axis=%u\n", src.axis);
	  }

	  /* Record date if changed */

	  if (utc[0] != saved_day) {
	    saved_day = utc[0];
	    mjd_to_date(mjd, date, sizeof(date));
	    date[11] = '\0';
	    printf("%s\n", date);
	    if (logfile)
	      fprintf(logfile, "%s\n", date);
	  }

	  /* Check for change in online model */

	  if ((n_pts > 0) &&
	      ((online_model[0] != src.flexure*RPDEG) ||
	       (online_model[1] != src.tilts[0]*RPDEG) ||
	       (online_model[2] != src.tilts[1]*RPDEG) ||
	       (online_model[3] != src.tilts[2]*RPDEG) ||
	       (online_model[4] != src.collimation[0]*RPDEG) ||
	       (online_model[5] != src.collimation[1]*RPDEG) ||
	       (online_model[6] != src.encoff[0]*RPDEG) ||
	       (online_model[7] != src.encoff[1]*RPDEG) ||
	       (lat != src.site[0]*RPDEG))) {
	    printf("Online pointing model parameters changed\n");
	    if (logfile)
	      fprintf(logfile, "Online pointing model parameters changed\n");
	  }

	  /* Save this point */

	  points[n_pts].mjd = mjd;
	  points[n_pts].lst = src.lst*RPHR;;
	  points[n_pts].az = src.htopo[0]*RPDEG;
	  points[n_pts].el = src.htopo[1]*RPDEG;
	  points[n_pts].deck = range(src.horiz_off[2]*RPDEG);
	  points[n_pts].mount_az = src.hmount[0]*RPDEG;
	  points[n_pts].mount_el = src.hmount[1]*RPDEG;
	  points[n_pts].mount_dk = src.hmount[2]*RPDEG;
	  points[n_pts].deck_mode = src.deck_mode;
	  points[n_pts].azerr[0] = range((src.hmount[0] + src.encoff[0] - src.htopo[0])*RPDEG);
	  points[n_pts].elerr[0] = range((src.hmount[1] + src.encoff[1] - src.htopo[1])*RPDEG);
	  points[n_pts].azerr[1] = 0.0;
	  points[n_pts].elerr[1] = 0.0;
	  points[n_pts].azerr[2] = points[n_pts].azerr[0];
	  points[n_pts].elerr[2] = points[n_pts].elerr[0];
	  points[n_pts].azerr[3] = src.horiz_off[0]*RPDEG;
	  points[n_pts].elerr[3] = src.horiz_off[1]*RPDEG;
	  points[n_pts].index = current_index; /* star number */
	  points[n_pts].flag = 0; /* not flagged */
	  points[n_pts].tilts[0] = src.tiltm[0];
	  points[n_pts].tilts[1] = src.tiltm[1];
	  points[n_pts].encrot[0] = pmac_el_enc_rot[0]*RPARCS;
	  points[n_pts].encrot[1] = pmac_el_enc_rot[1]*RPARCS;
#if 0
	  fputs("Doing elcorr\n", stderr);
#endif
	  points[n_pts].elcorr = elcorr(pmac_el_pos, pmac_motor);

	  points[n_pts].color = 2;

	  /* Record this point in the logfile */

	  mjd_to_date(mjd, date, sizeof(date));
	  printf("%13.6f %s %8.3f %8.3f %8.3f%s %8.2f %8.2f %s\n",
		 mjd, date+12,
		 points[n_pts].az/RPDEG, points[n_pts].el/RPDEG,
		 points[n_pts].deck/RPDEG,
		 points[n_pts].deck_mode ? "f":"t",
		 points[n_pts].azerr[3]/RPARCS, points[n_pts].elerr[3]/RPARCS,
		 source);
	  if (logfile != NULL) {
	    if (mark == 20) {
	      fprintf(logfile, "%13.6f %s %8.3f %8.3f %8.3f\n",
		      mjd, date+12,
		      points[n_pts].mount_az/RPDEG, 
		      points[n_pts].tilts[0],
		      points[n_pts].tilts[1]);
	    } else {
	      fprintf(logfile, "%13.6f %s %8.3f %8.3f %8.3f%s %8.2f %8.2f %s\n",
		    mjd, date+12,
		    points[n_pts].az/RPDEG, points[n_pts].el/RPDEG,
		    points[n_pts].deck/RPDEG,
		    points[n_pts].deck_mode ? "f":"t",
		    points[n_pts].azerr[3]/RPARCS, points[n_pts].elerr[3]/RPARCS,
		    source);
	    }
	  }

	  /* Get weather */

	  if(ms_get_float(ms, my_regs + W_AIR_TEMP,  0, 1,
			   &points[n_pts].w_air_temp) ||
	     ms_get_float(ms, my_regs + W_HUMIDITY,  0, 1,
			   &points[n_pts].w_humidity) ||
	     ms_get_float(ms, my_regs + W_WIND_SPEED,  0, 1,
			   &points[n_pts].w_wind_speed) ||
	     ms_get_float(ms, my_regs + W_WIND_DIR,  0, 1,
			   &points[n_pts].w_wind_dir) ||
	     ms_get_float(ms, my_regs + W_PRESSURE,  0, 1,
			   &points[n_pts].w_pressure) ) {
	    fputs("Error reading weather data\n", stderr);
	  }

	  /* Save online model */

	  online_model[0] = src.flexure*RPDEG;
	  online_model[1] = src.tilts[0]*RPDEG;
	  online_model[2] = src.tilts[1]*RPDEG;
	  online_model[3] = src.tilts[2]*RPDEG;
	  online_model[4] = src.collimation[0]*RPDEG;
	  online_model[5] = src.collimation[1]*RPDEG;
	  online_model[6] = src.encoff[0]*RPDEG;
	  online_model[7] = src.encoff[1]*RPDEG;
	  online_model[8] = 0.0;
	  online_model[9] = 0.0;
	  online_model[10] = 0.0;
	  online_model[11] = 0.0;
	  lat = src.site[0]*RPDEG;

	  if (check) {
	    double new_az, new_el;
	    /* Check the calculation */

	    pointing_model(points[n_pts].az, points[n_pts].el,
			   lat, points[n_pts].deck, points[n_pts].deck_mode,
			   points[n_pts].tilts[0], points[n_pts].tilts[1],
			   online_model,
			   &new_az, &new_el);
	    printf(" (topo)   %.4f %.4f (mount) %.4f %.4f (offsets)   %.4f %.4f\n"
		   " (online) %.4f %.4f (comp)  %.4f %.4f (diff, deg) %.4f %.4f\n",
		   src.htopo[0], src.htopo[1],
		   src.hmount[0], src.hmount[1],
		   src.horiz_off[0], src.horiz_off[1],
		   src.hmount[0] + src.encoff[0],
		   src.hmount[1] + src.encoff[1],
		   new_az/RPDEG, new_el/RPDEG,
		   (src.hmount[0] + src.encoff[0] - new_az/RPDEG),
		   (src.hmount[1] + src.encoff[1] - new_el/RPDEG));
	  }

	  n_pts += 1;
	  if (n_pts >= MAX_POINTS) {
	    printf ("Maximum number of points exceeded\n");
	    return 1;
	  }
	  star_count[current_index] += 1;
	  
	}
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
      if(ms_select_regs(ms, 1, 0, my_sel, NUM_MY_REGS, my_regs) != MS_SEND_DONE ||
	 ms_load_cal_file(ms, "", calfile))
	return 1;
      break;
    };
  };
}

/*--------------------------------------------------------------------*/


static void print_residuals(int n, struct point points[])
{
  double mean_az, mean_el, rms_az, rms_el, mean_az_sky, rms_az_sky;
  double rms, maxerr;

  mean_and_rms(n, points, &mean_az, &rms_az, &mean_az_sky, &rms_az_sky,
	       &mean_el, &rms_el);
  rms = sky_rms(n_pts, points, &maxerr);
  printf("rms residual: AZ = %.1f, AZcos(EL) = %.1f, EL=%.1f, sky = %.1f arcsec\n",
	    rms_az/RPARCS, rms_az_sky/RPARCS, rms_el/RPARCS, rms/RPARCS);
  printf("max residual: sky = %.1f arcsec\n", maxerr/RPARCS);
}

static void report_model(int n, struct point points[], double model[])
{
  printf("Model:\n");
  printf("  [0]   Flexure parameter      = %8.5f° = %6.2f arcsec\n",
	 model[0]/RPDEG, model[0]/RPARCS);
  printf("  [1]   AZ axis tilt in HA     = %8.5f° = %6.2f arcsec\n",
	 model[1]/RPDEG, model[1]/RPARCS);
  printf("  [2]   AZ axis tilt in Lat    = %8.5f° = %6.2f arcsec\n",
	 model[2]/RPDEG, model[2]/RPARCS);
  printf("  [3]   EL axis tilt           = %8.5f° = %6.2f arcsec\n",
	 model[3]/RPDEG, model[3]/RPARCS);
  printf("  [4]   Collimation error      = %8.5f° = %6.2f arcsec\n",
	 model[4]/RPDEG, model[4]/RPARCS);
  printf("  [5]   PA offset of coll. err = %8.2f°\n", model[5]/RPDEG);
  printf("  [6]   AZ encoder offset      = %9.5f°\n", model[6]/RPDEG);
  printf("  [7]   EL encoder offset      = %9.5f°\n", model[7]/RPDEG);
  printf("  [8]   Collimation 2nd harm   = %8.5f°\n", model[8]/RPDEG);
  printf("  [9]   PA offset of 2nd harm  = %8.2f°\n", model[9]/RPDEG);
  if (tilt_on) {
    printf("  [A]   x tilt zero            = %8.5f° = %6.2f arcsec\n",
	   model[10]/RPDEG, model[10]/RPARCS);
    printf("  [B]   y tilt zero            = %8.5f° = %6.2f arcsec\n",
	   model[11]/RPDEG, model[11]/RPARCS);
  } else {
    printf("  Tilt meters disabled\n");
  }

  print_residuals(n, points);

  printf("collimate optical, %.5f, %.2f\n",
	 model[4]/RPDEG, model[5]/RPDEG);
  printf("tilts     %.5f, %.5f, %.5f\n",
	 model[1]/RPDEG, model[2]/RPDEG, model[3]/RPDEG);
  printf("flexure   %.5f\n",
	 model[0]/RPDEG);
  printf("encoder_zero_points %.5f, %.5f, 0.0\n",
	 model[6]/RPDEG, model[7]/RPDEG);
}

/*--------------------------------------------------------------------*/

/* Plot Tilt-Meter readings versus AZ and EL or UTC */

static void plot_tilt(int type, int axis, int n, double el_limit,
			struct point points[],
			double start, double stop, char *device)
{
  int i;
  double min_az = 0.0;
  double max_az = 360.0;
  double min_el = el_limit/RPDEG;
  double max_el = 90.0;
  double min_deck = 0.0;
  double max_deck = 360.0;
  float x0, x1, x2, x3, y0, y1, y2, y3;
  char *label[2] = {"TILT METER READINGS",
		    "TILT METER RESIDUALS"};
  char caption[128];
  float xmeter[MAX_POINTS];
  float ymeter[MAX_POINTS];
  double az, el;
  int time0 = start;
  double t0, t1;
  double x;

  /* Color indices for unflagged and flagged points */

  float min_x, max_x, min_y, max_y, delta;
  float xf[361], yf[361];
  double xrms, yrms;

  /* Anything to plot? */

  if (n_pts < 1)
    return;

  /* Undo possible wraps. This requires that the initial
     guess of the zero point is close to correct. */

  /* CLP 020601 - DASI tilt meter doesn't wrap
  min_x = xtm[0];
  min_y = ytm[0];
  for (i=0; i<n; i++) {
    if (points[i].tilts[0] - min_x > 327.68)
      points[i].tilts[0] -= 655.36;
    else if (points[i].tilts[0] - min_x < -327.68)
      points[i].tilts[0] += 655.36;
    if (points[i].tilts[1] - min_y > 327.68)
      points[i].tilts[1] -= 655.36;
    else if (points[i].tilts[1] - min_y < -327.68)
      points[i].tilts[1] += 655.36;
  }
  */

  /* Correct readings if requested */

  if (type == 1) {
    double xsumsq = 0.0;
    double ysumsq = 0.0;
    for (i=0; i<n; i++) {
      az = points[i].mount_az;
      el = points[i].mount_el;
      xmeter[i] = points[i].tilts[0] -
	(xtm[0] + xtm[1]*sin(az) + xtm[2]*cos(az) + xtm[3]*cos(el));
      ymeter[i] = points[i].tilts[1] -
	(ytm[0] + ytm[1]*sin(az) + ytm[2]*cos(az) + ytm[3]*cos(el));
      xsumsq += xmeter[i]*xmeter[i];
      ysumsq += ymeter[i]*ymeter[i];
    }
    xrms = sqrt(xsumsq/n);
    yrms = sqrt(ysumsq/n);
  } else {
    for (i=0; i<n; i++) {
      xmeter[i] = points[i].tilts[0];
      ymeter[i] = points[i].tilts[1];
    }
    xrms = yrms = 0.0;
  }

  /* Find max and min for plot */

  min_x = max_x = xmeter[0];
  min_y = max_y = ymeter[0];
  for (i=1; i<n; i++) {
    if (xmeter[i] < min_x) min_x = xmeter[i];
    if (xmeter[i] > max_x) max_x = xmeter[i];
    if (ymeter[i] < min_y) min_y = ymeter[i];
    if (ymeter[i] > max_y) max_y = ymeter[i];
  }
  delta = 0.1*(max_x - min_x);
  if (delta < 0.1) delta = 0.1*RPDEG;
  min_x -= delta;
  max_x += delta;
  delta = 0.1*(max_y - min_y);
  if (delta < 0.1) delta = 0.1*RPDEG;
  min_y -= delta;
  max_y += delta;

  t0 = (start-time0)*86400.0 - 600.0;
  t1 = (stop-time0)*86400.0 + 600.0;

  /* Open plot device */ 

#ifdef DEBUG
  printf("Plotting %s\n", label[type]);
#endif
  if (cpgopen(device) < 1) return;
  init_colors();
  cpgpage();
  cpgbbuf();

  /* Set the standard viewport, and divide it into six parts */

  cpgvstd();
  cpgqvp(0, &x0, &x3, &y0, &y3);
  x1 = (2.0*x0 + x3)/3.0;
  x2 = (x0 + 2.0*x3)/3.0;
  y1 = (2.0*y0 + y3)/3.0;
  y2 = (y0 + 2.0*y3)/3.0;

  /* Annotation */

  annot_top(TITLE, label[type], start, stop);
  cpgsch(0.6);

  if (axis) {
    x1 = (3.0*x0 + 2.0*x3)/5.0;

    /* 1. X versus UTC */

    cpgsci(1);
    cpgsvp(x0, x1, y2, y3);
    cpgswin(t0, t1, min_x, max_x);
    cpgtbox("BCSTZH", 0.0, 0, "BCNST", 0.0, 0);
    cpglab(" ", "X tilt (arcsec)", " ");
    cpgsci(2);
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1((points[i].mjd-time0)*86400.0, xmeter[i],
	     plot_symbol(points[i].index));
    }

    /* 2. Y versus UTC */

    cpgsci(1);
    cpgsvp(x0, x1, y1, y2);
    cpgswin(t0, t1, min_y, max_y);
    cpgtbox("BCSTNZHO", 0.0, 0, "BCNST", 0.0, 0);
    cpglab("UTC", "Y tilt (arcsec)", " ");
    cpgsci(2);
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1((points[i].mjd-time0)*86400.0, ymeter[i],
	     plot_symbol(points[i].index));
    }
    
  } else {

    /* AZ and EL */

    /* 1. X versus AZ */

    cpgsci(1);
    cpgsvp(x0, x1, y2, y3);
    cpgswin(min_az, max_az, min_x, max_x);
    cpgbox("BCST", 0.0, 0, "BCNST", 0.0, 0);
    cpglab(" ", "X tilt (arcsec)", " ");
    if (type == 0) {
      for(i=0; i<361; i++) {
	xf[i] = i;
	az = i*RPDEG;
	yf[i] = xtm[0] + xtm[1]*sin(az) + xtm[2]*cos(az);
      }
      cpgsci(5);
      cpgline(361, xf, yf);
    }
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      x = fmod(points[i].mount_az, TWOPI);
      if (x < 0.0) x+= TWOPI;
      cpgpt1(x/RPDEG, xmeter[i],
	     plot_symbol(points[i].index));
    }

    /* 2. Y versus AZ */

    cpgsci(1);
    cpgsvp(x0, x1, y1, y2);
    cpgswin(min_az, max_az, min_y, max_y);
    cpgbox("BCSTN", 0.0, 0, "BCNST", 0.0, 0);
    cpglab("Mount Azimuth", "Y tilt (arcsec)", " ");
    if (type == 0) {
      for(i=0; i<361; i++) {
	xf[i] = i;
	az = i*RPDEG;
	yf[i] = ytm[0] + ytm[1]*sin(az) + ytm[2]*cos(az);
      }
      cpgsci(5);
      cpgline(361, xf, yf);
    }
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      x = fmod(points[i].mount_az, TWOPI);
      if (x < 0.0) x+= TWOPI;
      cpgpt1(x/RPDEG, ymeter[i],
	     plot_symbol(points[i].index));
    }
    
    /* Write model parameters */
    
    cpgsave();
    cpgsch(0.7);
    cpgsci(4);
    if (type == 1)
      cpgmtxt("B", 5.0, 0.0, 0.0, "Model subtracted from readings:");
    else
      cpgmtxt("B", 5.0, 0.0, 0.0, "Model:");
    sprintf(caption,
	    "X = %.1f %+.1f sin(az) %+.1f cos(az) %+.1f cos(el) = %.1f %+.1f sin(az%+.1f) %+.1f cos(el)",
	    xtm[0], xtm[1], xtm[2], xtm[3],
	    xtm[0], sqrt(xtm[1]*xtm[1]+xtm[2]*xtm[2]), 
	    atan2(xtm[2], xtm[1])/RPDEG, xtm[3]);
    cpgmtxt("B", 6.5, 0.0, 0.0, caption);
    sprintf(caption, 
	    "Y = %.1f %+.1f sin(az) %+.1f cos(az) %+.1f cos(el) = %.1f %+.1f sin(az%+.1f) %+.1f cos(el)",
	    ytm[0], ytm[1], ytm[2], ytm[3],
	    ytm[0], sqrt(ytm[1]*ytm[1]+ytm[2]*ytm[2]),
	    atan2(ytm[2], ytm[1])/RPDEG, ytm[3]);
    cpgmtxt("B", 8.0, 0.0, 0.0, caption);
    if (type == 1) {
      sprintf(caption, "RMS residuals: X=%.1f, Y=%.1f", xrms, yrms);
      cpgmtxt("B", 9.5, 0.0, 0.0, caption);
    }
    cpgunsa();

    /* 3. X versus EL */

    cpgsci(1);
    cpgsvp(x1, x2, y2, y3);
    cpgswin(min_el, max_el, min_x, max_x);
    cpgbox("BCST", 0.0, 0, "BCST", 0.0, 0);
    if (type == 0) {
      for(i=0; i<91; i++) {
	xf[i] = i;
	el = i*RPDEG;
	yf[i] = xtm[0] + xtm[3]*cos(el);
      }
      cpgsci(5);
      cpgline(91, xf, yf);
    }
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1(points[i].mount_el/RPDEG, xmeter[i],
	     plot_symbol(points[i].index));
    }
    
    /* 4. Y versus EL */
    
    cpgsci(1);
    cpgsvp(x1, x2, y1, y2);
    cpgswin(min_el, max_el, min_y, max_y);
    cpglab("Mount Elevation", " ", " ");
    cpgbox("BCSTN", 0.0, 0, "BCST", 0.0, 0);
    if (type == 0) {
      for(i=0; i<91; i++) {
	xf[i] = i;
	el = i*RPDEG;
	yf[i] = ytm[0] + ytm[3]*cos(el);
      }
      cpgsci(5);
      cpgline(91, xf, yf);
    }
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1(points[i].mount_el/RPDEG, ymeter[i],
	     plot_symbol(points[i].index));
    }

    /* 5. X versus DECK */

    cpgsci(1);
    cpgsvp(x2, x3, y2, y3);
    cpgswin(min_deck, max_deck, min_x, max_x);
    cpgbox("BCST", 0.0, 0, "BCST", 0.0, 0);
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      x = fmod(points[i].mount_dk, TWOPI);
      if (x < 0.0) x+= TWOPI;
      cpgpt1(x/RPDEG, xmeter[i],
	     plot_symbol(points[i].index));
    }

    /* 6. Y versus DECK */
    
    cpgsci(1);
    cpgsvp(x2, x3, y1, y2);
    cpgswin(min_deck, max_deck, min_y, max_y);
    cpglab("Deck Angle", " ", " ");
    cpgbox("BCSTN", 0.0, 0, "BCST", 0.0, 0);
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      x = fmod(points[i].mount_dk, TWOPI);
      if (x < 0.0) x+= TWOPI;
      cpgpt1(x/RPDEG, ymeter[i],
	     plot_symbol(points[i].index));
    }
  }

  cpgebuf();
  cpgclos();
}

/*--------------------------------------------------------------------*/

/* Plot as function of time (UTC or LST) */

static void tplot(int n, double el_limit, struct point points[],
		  double start, double stop, double model[], 
		  char *device, int nstars, char *name[], int sidereal)
{
  int i;
  double min_az = 0.0;
  double max_az = 360.0;
  double min_el = el_limit/RPDEG;
  double max_el = 90.0;
  double min_deck = -180.0;
  double max_deck = 180.0;
  float x0, x1, x2, x3, y0, y1, y2, y3;
  char *label = "ONLINE CORRECTIONS minus MODEL";
  int time0 = start;
  double t0, t1;
  char caption[128];
  double rms, mean_az, rms_az, mean_el, rms_el, mean_az_sky, rms_az_sky;

  double min_az_res, max_az_res, min_el_res, max_el_res, delta;
  double min_az_sky, max_az_sky;
  double min_lst, max_lst;
  double azerr;
  double time;
  double junk;
  int cursor;
  char ch;
  char value[32];
  int len = sizeof(value);
  float x, y;

  /* Anything to plot? */

  if (n_pts < 1)
    return;

  azerr = points[0].azerr[2];
  min_az_res = max_az_res = azerr;
  azerr *=  cos(points[0].el);
  min_az_sky = max_az_sky = azerr;
  min_el_res = max_el_res = points[0].elerr[2];
  min_lst = max_lst = points[0].lst;
  for (i=1; i<n; i++) {
    azerr = points[i].azerr[2];
    if (azerr < min_az_res) min_az_res = azerr;
    if (azerr > max_az_res) max_az_res = azerr;
    azerr *= cos(points[i].el);
    if (azerr < min_az_sky) min_az_sky = azerr;
    if (azerr > max_az_sky) max_az_sky = azerr;
    if (points[i].elerr[2] < min_el_res) min_el_res = points[i].elerr[2];
    if (points[i].elerr[2] > max_el_res) max_el_res = points[i].elerr[2];
    if (points[i].lst < min_lst) min_lst = points[i].lst;
    if (points[i].lst > max_lst) max_lst = points[i].lst;
  }
  if (fabs(max_az_res) < 0.01*RPDEG && fabs(min_az_res) < 0.01*RPDEG) {
    min_az_res = -0.011*RPDEG;
    max_az_res =  0.011*RPDEG;
  } else {
    delta = 0.1*(max_az_res - min_az_res);
    if (delta < 0.1*RPDEG) delta = 0.01*RPDEG;
    min_az_res -= delta;
    max_az_res += delta;
  }
  if (fabs(max_az_sky) < 0.01*RPDEG && fabs(min_az_sky) < 0.01*RPDEG) {
    min_az_sky = -0.011*RPDEG;
    max_az_sky =  0.011*RPDEG;
  } else {
    delta = 0.1*(max_az_sky - min_az_sky);
    if (delta < 0.1*RPDEG) delta = 0.01*RPDEG;
    min_az_sky -= delta;
    max_az_sky += delta;
  }
  if (fabs(max_el_res) < 0.01*RPDEG && fabs(min_el_res) < 0.01*RPDEG) {
    min_el_res = -0.011*RPDEG;
    max_el_res =  0.011*RPDEG;
  } else {
    delta = 0.1*(max_el_res - min_el_res);
    if (delta < 0.1*RPDEG) delta = 0.01*RPDEG;
    min_el_res -= delta;
    max_el_res += delta;
  }

  if (sidereal) {
    /* Caution: this does not deal with the wrap required when
       the observations span LST 0. */
    min_lst /= RPHR;
    max_lst /= RPHR;
    t0 = floor(min_lst)*3600.0;
    t1 = ceil(max_lst)*3600.0;
  } else {
    t0 = (start-time0)*86400.0 - 600.0;
    t1 = (stop-time0)*86400.0 + 600.0;
  }

  /* Open plot device */ 

#ifdef DEBUG
  printf("Plotting %s...\n", label);
#endif
  if (cpgopen(device) < 1) return;
  cpgqinf("CURSOR", value, &len);
  cursor = strcmp(value, "YES") == 0;
  if (cursor)
    printf("Select a frame to expand, or type X to continue\n");
  init_colors();
  cpgpage();

  while (1) {
    cpgbbuf();
    cpgeras();
    cpgsch(1.0);

    /* Set the standard viewport, and divide it into six parts */

    cpgvstd();
    cpgqvp(0, &x0, &x3, &y0, &y3);
    x1 = (3.0*x0 + 2.0*x3)/5.0;
    x2 = (2.0*x0 + 3.0*x3)/5.0;
    y1 = (2.0*y0 + y3)/3.0;
    y2 = (y0 + 2.0*y3)/3.0;

    /* Annotation */

    annot_top(TITLE, label, start, stop);
    cpgsch(0.6);
    
    /* 1. AZ versus Time */
    
    cpgsci(1);
    cpgsvp(x0, x1, y2, y3);
    cpgswin(t0, t1, min_az, max_az);
    cpgtbox("BCSTZH", 0.0, 0, "BCNST", 0.0, 0);
    cpglab(" ", "AZ", " ");
    for (i=0; i<n; i++) {
      time = sidereal ? points[i].lst/RPSEC : (points[i].mjd-time0)*86400.0;
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1(time, points[i].az/RPDEG,
	     plot_symbol(points[i].index));
    }

    /* 2. EL versus Time */

    cpgsci(1);
    cpgsvp(x0, x1, y1, y2);
    cpgswin(t0, t1, min_el, max_el);
    cpgtbox("BCSTZH", 0.0, 0, "BCNST", 0.0, 0);
    cpglab(" ", "EL", " ");
    for (i=0; i<n; i++) {
      time = sidereal ? points[i].lst/RPSEC : (points[i].mjd-time0)*86400.0;
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1(time, points[i].el/RPDEG, 
	     plot_symbol(points[i].index));
    }
    
    /* 3. DECK versus Time */
    
    cpgsci(1);
    cpgsvp(x0, x1, y0, y1);
    cpgswin(t0, t1, min_deck, max_deck);
    cpgtbox("BCSTNZHO", 0.0, 0, "BCNST", 0.0, 0);
    cpglab(sidereal ? "LST" : "UTC", "Deck Angle", " ");
    for (i=0; i<n; i++) {
      time = sidereal ? points[i].lst/RPSEC : (points[i].mjd-time0)*86400.0;
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1(time, points[i].mount_dk/RPDEG, 
	     plot_symbol(points[i].index));
    }
    
    /* Add star legend, showing plot symbol and star name */

    cpgsave();
    cpgsci(4);
    cpgsch(0.5);
    for (i=0; i < nstars; i++) {
      /* printf("%d %s\n", i, name[i]); */
      sprintf(caption, "%c", plot_symbol(i));
      cpgmtxt("RV", 2.0, 3.0 - i/18.0, 0.0, caption);
      cpgmtxt("RV", 4.0, 3.0 - i/18.0, 0.0, name[i]);
    }
    cpgunsa();
    
    /* 4. AZ error versus Time */
    
    cpgsvp(x2, x3, (y0 + 3.0*y3)/4.0, y3);
    cpgswin(t0, t1, min_az_res/RPARCS, max_az_res/RPARCS);
    cpgsci(5);
    cpgmove(t0, 0.0);
    cpgdraw(t1, 0.0);
    cpgsci(1);
    cpgtbox("BCSTZH", 0.0, 0, "BCNST", 0.0, 0);
    cpglab(" ", "AZ error (arcsec)", " ");
    for (i=0; i<n; i++) {
      time = sidereal ? points[i].lst/RPSEC : (points[i].mjd-time0)*86400.0;
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      azerr = points[i].azerr[2];
      cpgpt1(time, azerr/RPARCS, 
	     plot_symbol(points[i].index));
    }

    /* 5. AZ error times cos(el) versus Time */
    
    cpgsvp(x2, x3, (y0+y3)/2.0, (y0 + 3.0*y3)/4.0);
    cpgswin(t0, t1, min_az_sky/RPARCS, max_az_sky/RPARCS);
    cpgsci(5);
    cpgmove(t0, 0.0);
    cpgdraw(t1, 0.0);
    cpgsci(1);
    cpgtbox("BCSTZH", 0.0, 0, "BCNST", 0.0, 0);
    cpglab(" ", "AZ error \\x cos(EL)", " ");
    for (i=0; i<n; i++) {
      time = sidereal ? points[i].lst/RPSEC : (points[i].mjd-time0)*86400.0;
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      azerr = points[i].azerr[2]*cos(points[i].el);
      cpgpt1(time, azerr/RPARCS, 
	     plot_symbol(points[i].index));
    }
    
    /* 6. EL error versus Time */
    
    cpgsvp(x2, x3, (3.0*y0+y3)/4.0, (y0+y3)/2.0);
    cpgswin(t0, t1, min_el_res/RPARCS, max_el_res/RPARCS);
    cpgsci(5);
    cpgmove(t0, 0.0);
    cpgdraw(t1, 0.0);
    cpgsci(1);
    cpgtbox("BCSTNZHO", 0.0, 0, "BCNST", 0.0, 0);
    cpglab(sidereal ? "LST" : "UTC", "EL error (arcsec)", " ");
    for (i=0; i<n; i++) {
      time = sidereal ? points[i].lst/RPSEC : (points[i].mjd-time0)*86400.0;
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1(time, points[i].elerr[2]/RPARCS, 
	     plot_symbol(points[i].index));
    }
    
    /* Write model parameters */
    
    cpgsave();
    cpgsci(4);
    cpgsch(0.5);
    sprintf(caption,
	    "Model [0-3]: FL=%.4f AT1=%.4f AT2=%.4f ET=%.4f",
	    model[0]/RPDEG, model[1]/RPDEG, model[2]/RPDEG, model[3]/RPDEG);
    cpgmtxt("B", 6.5, 0.0, 0.0, caption);
    sprintf(caption,
	    "Model [4-7]: CO=%.4f at %.2f AZ0=%.4f EL0=%.4f",
	    model[4]/RPDEG, model[5]/RPDEG, model[6]/RPDEG, model[7]/RPDEG);
    cpgmtxt("B", 8.0, 0.0, 0.0, caption);
    sprintf(caption,
	    "Model [8-9]: CO2=%.4f at %.2f",
	    model[8]/RPDEG, model[9]/RPDEG);
    cpgmtxt("B", 9.5, 0.0, 0.0, caption);
    mean_and_rms(n, points, &mean_az, &rms_az, &mean_az_sky, &rms_az_sky,
		 &mean_el, &rms_el);
    rms = sky_rms(n, points, &junk);
    sprintf(caption, "rms: AZ = %.1f, AZcos(EL) = %.1f, EL=%.1f, sky = %.1f arcsec",
	    rms_az/RPARCS, rms_az_sky/RPARCS, rms_el/RPARCS, rms/RPARCS);
    cpgmtxt("B", 11.0, 0.0, 0.0, caption);
    cpgunsa();

    cpgebuf();
  
    /* Editing */
    
    if (!cursor) {
      cpgclos();
      return;
    }
    cpgsvp(x0, x3, y0, y3);
    cpgswin(x0, x3, y0, y3);
    cpgband(0, 0, 0.5, 0.5, &x, &y, &ch);
    if (ch == 'h' || ch == 'H' || ch == '?') {
      printf("H or ?:           print this message\n"
	     "X (right button): exit from plot res\n"
	     "Any other key:    expand frame\n");
    } else if (ch == 'x' || ch == 'X') {
      cpgclos();
      return;
    } else if (x0 < x && x < x1 && y2 < y && y < y3) {
      /* AZ versus time */
      interact(n, points, sidereal ? 4 : 3, 3, 2, t0, t1,
	       min_az, max_az, start, stop);
    } else if (x0 < x && x < x1 && y1 < y && y < y2) {
      /* EL versus time */
      interact(n, points, sidereal ? 4 : 3, 4, 2, t0, t1,
	       min_el, max_el, start, stop);
    } else if (x0 < x && x < x1 && y0 < y && y < y1) {
      /* DK versus time */
      interact(n, points, sidereal ? 4 : 3, 5, 2, t0, t1,
	       min_deck, max_deck, start, stop);
    } else if (x2 < x && x < x3 && (y0+3.0*y3)/4.0 < y && y < y3) {
      /* AZ error versus time */
      interact(n, points, sidereal ? 4 : 3, 2, 2, t0, t1,
	       min_az_res/RPARCS, max_az_res/RPARCS, start, stop);
    } else if (x2 < x && x < x3 && (y0+y3)/2.0 < y && y < (y0+3.0*y3)/4.0) {
      /* AZ error * cos(EL) versus time */
      interact(n, points, sidereal ? 4 : 3, 1, 2, t0, t1,
	       min_az_sky/RPARCS, max_az_sky/RPARCS, start, stop);
    } else if (x2 < x && x < x3 && (3.0*y0+y3)/4.0 < y && y < (y0+y3)/2.0) {
      /* EL error  versus time */
      interact(n, points, sidereal ? 4 : 3, 0, 2, t0, t1, 
	       min_el_res/RPARCS, max_el_res/RPARCS, start, stop);
    }
  }
}
/*--------------------------------------------------------------------*/

/* Plot errors on sky */

static void xyplot(int n, struct point points[], double limit,
		  double start, double stop, char *device)
{
  int i;
  float x0, x2, x3, y0, y1, y3;
  char *label = "ONLINE CORRECTIONS minus MODEL";
  double rms, junk;

  /* Anything to plot? */

  if (n_pts < 1)
    return;
  rms = sky_rms(n, points, &junk);

  /* Open plot device */ 

#ifdef DEBUG
  printf("Plotting %s...\n", label);
#endif
  if (cpgopen(device) < 1) return;
  init_colors();
  cpgpage();
  cpgbbuf();

  /* Set the standard viewport, and divide it into parts */

  cpgvstd();
  cpgqvp(0, &x0, &x3, &y0, &y3);
  x2 = (2.0*x0 + 3.0*x3)/5.0;
  y1 = (2.0*y0 + y3)/3.0;

  /* Annotation */

  annot_top(TITLE, label, start, stop);
  cpgsch(0.6);

  /* xyplot */

  cpgsci(1);
  cpgsvp(x0, x2, y1, y3);
  cpgwnad(-limit, limit, -limit, limit);
  cpgsci(5);
  cpgmove(-limit, 0.0);
  cpgdraw(limit, 0.0);
  cpgmove(0.0, -limit);
  cpgdraw(0.0, limit);
  cpgsfs(2);
  cpgcirc(0.0, 0.0, rms/RPARCS);
  cpgcirc(0.0, 0.0, 2.0*rms/RPARCS);
  cpgsci(1);
  cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
  cpglab("AZ err \\x cos(EL) (arcsec)", "EL err (arcsec)", " ");
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].azerr[2]/RPARCS*cos(points[i].el),
	   points[i].elerr[2]/RPARCS,
	   plot_symbol(points[i].index));
  }

  cpgebuf();
  cpgclos();
}
/*--------------------------------------------------------------------*/

/* Compute the difference between the EL axis encoder reading and the 
   EL motor encoder redaing, using the mount geometry to resolve the
   ambiguity in the motor encoder reading. */

/* Parameters updated according to fit by MCS, 3/12/99 */

  /* The previous estimate; the cyclic ambiguity is resolved to minimize
     the variation from the previous estimate. */

static double prev_diff = 0.0;

static double elcorr(double pmac_el_pos, double pmac_motor)
{
  /* The number of counts per turn of the elevation motor encoder.  */
  double MOTOR_COUNTS_PER_TURN = 524288.0;   /* 2^19 */

  /* The angle between the horizontal and the line that joins
   * the elevation axis bearing to the elevation fork bearing.  */
  double AXIS_TO_FORK_SLOPE = (13.42389*RPDEG);  /* Radians */

  /* The length of the line that joins the elevation axis bearing to the
   *  elevation fork bearing.  */
  double AXIS_TO_FORK = 2.87496;                 /* Meters */

  /* The length of the line that joins the elevation axis bearing
   * to the point at which the ball screw attaches to the deck.  */
  double AXIS_TO_SCREW = 2.61209;                /* Meters */

  /* The distance between turns on the ball screw.  */
  double BALL_SCREW_PITCH = 0.0169024;            /* Meters */

  /* The angle between the deck plane and the line joining the motor pivot
   * and the el axis */
  double EXTRA_ANGLE = (7.36784*RPDEG);             /* radians */

  /* The elevation measured by the encoder on the elevation axis. */
  double elevation = pmac_el_pos*RPDEG;
  /* The angle between the line that joins the elevation axis bearing to
   * the elevation fork bearing (radians). */
  double angle = (HALFPI - elevation) + AXIS_TO_FORK_SLOPE - EXTRA_ANGLE;
  /* The current extension of the ball screw (meters). */
  double screw_length =
    sqrt(AXIS_TO_SCREW*AXIS_TO_SCREW + AXIS_TO_FORK*AXIS_TO_FORK -
	 2*AXIS_TO_SCREW*AXIS_TO_FORK*cos(angle));
  /* The number of arcseconds of elevation change per turn of
   * the ball screw. */
  double arcsec_per_turn = (1.0/RPARCS) * BALL_SCREW_PITCH * screw_length /
    (AXIS_TO_FORK * AXIS_TO_SCREW * sin(angle));
  /* The number of turns of the ball screw corresponding to
   * the above length. */
  double nturn = screw_length / BALL_SCREW_PITCH;
  /* The discrepancy between the predicted and actual readout
   * of the ball-screw encoder.  */
  double diff = arcsec_per_turn * 
    (fmod(nturn, 1.0) - pmac_motor/MOTOR_COUNTS_PER_TURN);
  /* Reduce the difference to be positive and within one turn of zero. */
  while (diff - prev_diff > arcsec_per_turn/2.0)
    diff -= arcsec_per_turn;
  while (diff - prev_diff <= -arcsec_per_turn/2.0)
    diff += arcsec_per_turn;
  prev_diff = diff;

  return -diff*RPARCS;
}

/* Plot EL encoder errors */

static void encplot(int n, double el_limit, struct point points[],
		  double start, double stop, double model[], 
		  char *device)
{
  int i;
  double min_el = el_limit/RPDEG;
  double max_el = 90.0;
  double min_deck = -180.0;
  double max_deck = 180.0;
  float x0, x1, x2, x3, y0, y1, y2, y3;
  char *label = "EL encoder corrections";
  int time0 = start;
  double t0, t1;
  char caption[128];
  double rms, junk, mean;

  double min_elerr, max_elerr, delta;

  /* Anything to plot? */

  if (n_pts < 1)
    return;

  /* Find mean */

  mean = 0.0;
  for (i=0; i<n; i++) {
    mean += points[i].elcorr;
  }
  mean /= n;

  min_elerr = max_elerr = points[0].elcorr;
  min_elerr = max_elerr = points[0].elerr[2];
  for (i=1; i<n; i++) {
    if (points[i].elcorr - mean < min_elerr)
      min_elerr = points[i].elcorr - mean;
    if (points[i].elcorr - mean > max_elerr)
      max_elerr = points[i].elcorr -mean;
    if (points[i].elerr[2] < min_elerr) min_elerr = points[i].elerr[2];
    if (points[i].elerr[2] > max_elerr) max_elerr = points[i].elerr[2];
  }
  delta = 0.1*(max_elerr - min_elerr);
  if (delta < 0.1*RPDEG) delta = 0.01*RPDEG;
  min_elerr -= delta;
  max_elerr += delta;

  /* Open plot device */ 

#ifdef DEBUG
  printf("Plotting %s...\n", label);
#endif
  if (cpgopen(device) < 1) return;
  init_colors();
  cpgpage();
  cpgbbuf();

  /* Set the standard viewport, and divide it into six parts */

  cpgvstd();
  cpgqvp(0, &x0, &x3, &y0, &y3);
  x1 = (2.0*x0 + x3)/3.0;
  x2 = (x0 + 2.0*x3)/3.0;
  y1 = (2.0*y0 + y3)/3.0;
  y2 = (y0 + 2.0*y3)/3.0;

  /* Annotation */

  annot_top(TITLE, label, start, stop);
  cpgsch(0.6);

  t0 = (start-time0)*86400.0 - 600.0;
  t1 = (stop-time0)*86400.0 + 600.0;

  /* 1. EL error versus EL */

  cpgsci(1);
  cpgsvp(x0, x1, y2, y3);
  cpgswin(min_el, max_el, min_elerr/RPDEG, max_elerr/RPDEG);
  cpgtbox("BCST", 0.0, 0, "BCNST", 0.0, 0);
  cpglab(" ", "EL error", " ");
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].el/RPDEG, points[i].elerr[2]/RPDEG,
	   plot_symbol(points[i].index));
  }

  /* 2. Encoder difference versus EL */

  cpgsci(1);
  cpgsvp(x0, x1, y1, y2);
  cpgswin(min_el, max_el, (mean+min_elerr)/RPDEG, (mean+max_elerr)/RPDEG);
  cpgtbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
  cpglab("Elevation", "Encoder difference", " ");
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].el/RPDEG, points[i].elcorr/RPDEG,
	   plot_symbol(points[i].index));
  }

  /* Write model parameters */
  cpgsci(4);
  rms = sky_rms(n, points, &junk);
  sprintf(caption, "rms residual on sky = %.4f° = %.1f arcsec",
	  rms/RPDEG, rms/RPARCS);
  cpgmtxt("B", 5.0, 0.0, 0.0, caption);
  sprintf(caption,
	  "Model [0-3]: FL=%.4f AT1=%.4f AT2=%.4f ET=%.4f",
	  model[0]/RPDEG, model[1]/RPDEG, model[2]/RPDEG, model[3]/RPDEG);
  cpgmtxt("B", 6.5, 0.0, 0.0, caption);
  sprintf(caption,
	  "Model [4-7]: CO=%.4f at %.2f AZ0=%.4f EL0=%.4f",
	  model[4]/RPDEG, model[5]/RPDEG, model[6]/RPDEG, model[7]/RPDEG);
  cpgmtxt("B", 8.0, 0.0, 0.0, caption);
  sprintf(caption,
	  "Model [8-9]: CO2=%.4f at %.2f",
	  model[8]/RPDEG, model[9]/RPDEG);
  cpgmtxt("B", 9.5, 0.0, 0.0, caption);

  /* 3. EL error versus Deck */

  cpgsci(1);
  cpgsvp(x1, x2, y2, y3);
  cpgswin(min_deck, max_deck, min_elerr/RPDEG, max_elerr/RPDEG);
  cpgtbox("BCST", 0.0, 0, "BCST", 0.0, 0);
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].mount_dk/RPDEG, points[i].elerr[2]/RPDEG,
	   plot_symbol(points[i].index));
  }

  /* 4. Encoder difference versus Deck */

  cpgsci(1);
  cpgsvp(x1, x2, y1, y2);
  cpgswin(min_deck, max_deck, (mean+min_elerr)/RPDEG, (mean+max_elerr)/RPDEG);
  cpgtbox("BCNST", 0.0, 0, "BCST", 0.0, 0);
  cpglab("Deck angle", " ", " ");
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].mount_dk/RPDEG, points[i].elcorr/RPDEG,
	   plot_symbol(points[i].index));
  }

  /* 5. EL error versus Time */

  cpgsci(1);
  cpgsvp(x2, x3, y2, y3);
  cpgswin(t0, t1, min_elerr/RPDEG, max_elerr/RPDEG);
  cpgtbox("BCSTZHO", 0.0, 0, "BST", 0.0, 0);
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1((points[i].mjd-time0)*86400.0, points[i].elerr[2]/RPDEG, 
	   plot_symbol(points[i].index));
  }

  /* Add arcsec axis */
  cpgsci(1);
  cpgswin(t0, t1, min_elerr/RPARCS, max_elerr/RPARCS);
  cpgbox(" ", 0.0, 0, "CMST", 0.0, 0);
  cpgmtxt("R", 2.2, 0.5, 0.5, "arcsec");

  /* 6. EL encoder Difference versus Time */

  cpgsci(1);
  cpgsvp(x2, x3, y1, y2);
  cpgswin(t0, t1, (mean+min_elerr)/RPDEG, (mean+max_elerr)/RPDEG);
  cpgtbox("BCSTNZHO", 0.0, 0, "BST", 0.0, 0);
  cpglab("UTC", " ", " ");
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1((points[i].mjd-time0)*86400.0, points[i].elcorr/RPDEG, 
	   plot_symbol(points[i].index));
  }

  /* Add arcsec axis */
  cpgsci(1);
  cpgswin(t0, t1, min_elerr/RPARCS, max_elerr/RPARCS);
  cpgbox(" ", 0.0, 0, "CMST", 0.0, 0);
  cpgmtxt("R", 2.2, 0.5, 0.5, "arcsec");


  cpgebuf();
  cpgclos();
}

/* Apply encoder corrections */

static void enccor(int n, struct point points[])
{
  int j;
  for (j=0; j<n; j++) {
    points[j].elerr[0] -= points[j].elcorr;
    points[j].elerr[2] -= points[j].elcorr;
  }
}

/* Apply el encoder rotation corrections, removing any previous corrections */

static void do_rotcor(int n, struct point points[], double fac0, double fac1)
{
  int j;
  double c0, c1, corr;
  printf("Applying el encoder rotation corrections, with factors %.3f, %.3f\n",
	 fac0, fac1);
  c0 = fac0 - pfac0;
  c1 = fac1 - pfac1;
  if (fabs(c0) < 1e-6  && fabs(c1) < 1e-6) {
    printf("No changes required\n");
  } else {
    printf("                        Incremental correction factors %.3f, %.3f\n",
	   c0, c1);
    for (j=0; j<n; j++) {
      corr = c0*points[j].encrot[0] + c1*points[j].encrot[1];
      points[j].elerr[0] -= corr;
      points[j].elerr[2] -= corr;
    }
    pfac0 = fac0;
    pfac1 = fac1;
  }
}

/*--------------------------------------------------------------------*/

/* Fit model to Tilt-Meter readings (4 or 6 parameters, linear fit) */

static void fit_tilt(int type, int n, struct point points[])
{
  int i, j;
  double min_x, min_y, x, y, a, b, c, d;
  double az, cosaz, sinaz, cosel, denom;

  double sumx = 0.0, sumy = 0.0;
  double sumcos = 0.0, sumsin = 0.0;
  double sumxcos = 0.0, sumycos = 0.0, sumxsin = 0.0, sumysin = 0.0;
  double sumcos2 = 0.0, sumsin2 = 0.0, sumcossin = 0.0;

  if (n < 1)
    return; 

  /* Undo possible wraps. This requires that the initial
     guess of the zero point is close to correct. */

  /* CLP 020601 - DASI tilt meter doesn't wrap
  min_x = xtm[0];
  min_y = ytm[0];
  for (i=0; i<n; i++) {
    if (points[i].tilts[0] - min_x > 327.68)
      points[i].tilts[0] -= 655.36;
    else if (points[i].tilts[0] - min_x < -327.68)
      points[i].tilts[0] += 655.36;
    if (points[i].tilts[1] - min_y > 327.68)
      points[i].tilts[1] -= 655.36;
    else if (points[i].tilts[1] - min_y < -327.68)
      points[i].tilts[1] += 655.36;
  }
  */

  switch (type) {
  case 0:
    printf("Resetting tilt model parameters to zero:\n");
    xtm[0] = xtm[1] = xtm[2] = xtm[3] = 0.0;
    ytm[0] = ytm[1] = ytm[2] = ytm[4] = 0.0;
    break;

  case 1:
    printf("4-parameter AZ fit:\n");
     
    /* Type 1: fit 4-parameter model, with amplitudes constrained to be
       equal and phase difference 90 deg */

    /* Loop over data points */

    for (i=0; i<n; i++) {
      az = points[i].mount_az;
      cosaz = cos(az);
      sinaz = sin(az);
      x = points[i].tilts[0];
      y = points[i].tilts[1];
      sumx += x;
      sumy += y;
      sumcos += cosaz;
      sumsin += sinaz;
      sumxcos += x*cosaz;
      sumxsin += x*sinaz;
      sumycos += y*cosaz;
      sumysin += y*sinaz;
    }
    
    /* Normalize */
    
    sumx /= n;
    sumy /= n;
    sumcos /= n;
    sumsin /= n;
    sumxcos /= n;
    sumxsin /= n;
    sumycos /= n;
    sumysin /= n;    
    
    /* Evaluate results */
    
    denom = 1.0 - sumcos*sumcos - sumsin*sumsin;
    a = (sumx - sumcos*sumxcos - sumsin*sumxsin +
	 sumsin*sumycos - sumcos*sumysin)/denom;
    b = (-sumx*sumsin + sumxsin + sumcos*sumy - sumycos)/denom;
    c = (-sumx*sumcos + sumxcos - sumsin*sumy + sumysin)/denom;
    d = (sumy + sumcos*sumxsin - sumsin*sumxcos -
	 sumcos*sumycos - sumsin*sumysin)/denom;
    
    /* Store them in the appropriate places */
    
    xtm[0] = a;
    xtm[1] = b;
    xtm[2] = c;
    xtm[3] = 0.0;
    ytm[0] = d;
    ytm[1] = c;
    ytm[2] = -b;
    ytm[3] = 0.0;

    break;

  case 2:
    printf("6-parameter AZ fit:\n");

    /* Type 2: fit 6-parameter model, with independent parameters
       for x and y meters */

    for (j=0; j<2; j++) {

      /* Loop over data points */

      for (i=0; i<n; i++) {
	az = points[i].mount_az;
	cosaz = cos(az);
	sinaz = sin(az);
	x = points[i].tilts[j];
	sumx += x;
	sumcos += cosaz;
	sumsin += sinaz;
	sumcos2 += cosaz*cosaz;
	sumsin2 += sinaz*sinaz;
	sumcossin += cosaz*sinaz;
	sumxcos += x*cosaz;
	sumxsin += x*sinaz;
      }
    
      /* Normalize */
    
      sumx /= n;
      sumcos /= n;
      sumsin /= n;
      sumcos2 /= n;
      sumsin2 /= n;
      sumcossin /= n;
      sumxcos /= n;
      sumxsin /= n;
    
      /* Evaluate results */
    
      denom = -(sumcossin*sumcossin - 2.0*sumcos*sumsin*sumcossin
		+ sumcos2*sumsin*sumsin + sumsin2*sumcos*sumcos
		- sumcos2*sumsin2);
      a = (-sumcossin*sumcossin*sumx + sumcos2*sumsin2*sumx +
	   sumcossin*sumsin*sumxcos - sumcos*sumsin2*sumxcos +
	   sumcos*sumcossin*sumxsin - sumcos2*sumsin*sumxsin)/denom;
      b = (sumcos*sumcossin*sumx - sumcos2*sumsin*sumx - sumcossin*sumxcos +
	   sumcos*sumsin*sumxcos - sumcos*sumcos*sumxsin +
	   sumcos2*sumxsin)/denom;
      c = (sumcossin*sumsin*sumx - sumcos*sumsin2*sumx - 
	   sumsin*sumsin*sumxcos + sumsin2*sumxcos - sumcossin*sumxsin +
	   sumcos*sumsin*sumxsin)/denom;
    
      /* Store them in the appropriate places */
      
      if (j==0) {
	xtm[0] = a;
	xtm[1] = b;
	xtm[2] = c;
	xtm[3] = 0.0;
      } else {
	ytm[0] = a;
	ytm[1] = b;
	ytm[2] = c;
        ytm[3] = 0.0;
      }
    }
    break;

  case 3:
    printf("Fitting x,y = a + b cos(el):\n");

    /* Type 3: fit x,y dependence on el */

    /* Loop over data points */

    for (j=0; j<2; j++) {
      for (i=0; i<n; i++) {
	cosel = cos(points[i].mount_el);
	x = points[i].tilts[j];
	sumx += x;
	sumcos += cosel;
	sumcos2 += cosel*cosel;
	sumxcos += x*cosel;
      }
    
      /* Normalize */
    
      sumx /= n;
      sumcos /= n;
      sumcos2 /= n;
      sumxcos /= n;
      
      /* Evaluate results */
      
      denom = sumcos2 - sumcos*sumcos;
      a = (sumcos2*sumx - sumcos*sumxcos)/denom;
      b = (sumxcos - sumx*sumcos)/denom;
      
      /* Store them in the appropriate places */

      if (j==0) {
	xtm[0] = a;
	xtm[1] = 0.0;
	xtm[2] = 0.0;
	xtm[3] = b;
      } else {
	ytm[0] = a;
	ytm[1] = 0.0;
	ytm[2] = 0.0;
	ytm[3] = b;
      }
    }
    break;

  case 4:
    /* Type 4 */
    {
      int i, iter;
      double var, newvar;
      double tpars[6], dtpars[6];
      printf("Fitting x = a + b sin(az) + c cos(az) + d cos(el):\n"
	     "        y = e + c cos(az) - b sin(az) + f cos(el)\n");
      tpars[0] = xtm[0];
      tpars[1] = xtm[1];
      tpars[2] = xtm[2];
      tpars[3] = xtm[3];
      tpars[4] = ytm[0];
      tpars[5] = ytm[3];
      dtpars[0] = 0.5; /*arcsec*/
      dtpars[1] = 0.5;
      dtpars[2] = 0.5;
      dtpars[3] = 0.5;
      dtpars[4] = 0.5;
      dtpars[5] = 0.5;

      /* Iterative fit */
      iter = 0;
      var = tgridls(n, points, tpars, dtpars);
      for (i=0; i<500; i++) {
	newvar = tgridls(n, points, tpars, dtpars);
	iter++;
	if (newvar/var > 0.995)
	  break;
	var = newvar;
      }
      printf("Did %d iterations of tgridls\n", iter);

      /* Save the results */
      xtm[0] = tpars[0];
      xtm[1] = tpars[1];
      xtm[2] = tpars[2];
      xtm[3] = tpars[3];
      ytm[0] = tpars[4];
      ytm[1] = tpars[2];
      ytm[2] = -tpars[1];
      ytm[3] = tpars[5];
    }    
    break;

  default:
    fputs("Unknown option in tfit\n", stderr);
    break;
  }
    
  /* Report results */

  {
    double xsumsq = 0.0;
    double ysumsq = 0.0;
    double az, el, xmeter, ymeter;
    for (i=0; i<n; i++) {
      az = points[i].mount_az;
      el = points[i].mount_el;
      xmeter= points[i].tilts[0] -
	(xtm[0] + xtm[1]*sin(az) + xtm[2]*cos(az) + xtm[3]*cos(el));
      ymeter = points[i].tilts[1] -
	(ytm[0] + ytm[1]*sin(az) + ytm[2]*cos(az) + ytm[3]*cos(el));
      xsumsq += xmeter*xmeter;
      ysumsq += ymeter*ymeter;
    }

    printf("X = %.1f %+.1f sin(az) %+.1f cos(az) %+.1f cos(el)\n"
	   "  = %.1f %+.1f sin(az%+.1f) %+.1f cos(el)\n"
	   "  (rms %.2f arcsec)\n",
	   xtm[0], xtm[1], xtm[2], xtm[3],
	   xtm[0], sqrt(xtm[1]*xtm[1]+xtm[2]*xtm[2]), 
	   atan2(xtm[2], xtm[1])/RPDEG, xtm[3],
	   sqrt(xsumsq/n));
    printf("Y = %.1f %+.1f sin(az) %+.1f cos(az) %+.1f cos(el)\n"
	   "  = %.1f %+.1f sin(az%+.1f) %+.1f cos(el)\n"
	   "  (rms %.2f arcsec)\n",
	   ytm[0], ytm[1], ytm[2], ytm[3],
	   ytm[0], sqrt(ytm[1]*ytm[1]+ytm[2]*ytm[2]),
	   atan2(ytm[2], ytm[1])/RPDEG, ytm[3],
	   sqrt(ysumsq/n));
  }
}
/*--------------------------------------------------------------------*/

/* Plot weather as function of time */

static void wxplot(int n, struct point points[],
		  double start, double stop, char *device)
{
  int i;
  float x0, x1, x2, x3, y0, y1, y2, y3;
  char *label = "WEATHER";
  int time0 = start;
  double t0, t1;
  float ymin, ymax;
  float ylo, yhi;

  /* Anything to plot? */

  if (n_pts < 1)
    return;

#ifdef DEBUG
  printf("Plotting %s...\n", label);
#endif
  if (cpgopen(device) < 1) return;
  init_colors();
  cpgpage();
  cpgbbuf();

  /* Set the standard viewport, and divide it into six parts */

  cpgvstd();
  cpgqvp(0, &x0, &x3, &y0, &y3);
  x1 = (3.0*x0 + 2.0*x3)/5.0;
  x2 = (2.0*x0 + 3.0*x3)/5.0;
  y1 = (2.0*y0 + y3)/3.0;
  y2 = (y0 + 2.0*y3)/3.0;

  /* Annotation */

  annot_top(TITLE, label, start, stop);
  cpgsch(0.6);

  t0 = (start-time0)*86400.0 - 600.0;
  t1 = (stop-time0)*86400.0 + 600.0;

  /* 1. Air Temp  versus Time */

  ymin = ymax = points[0].w_air_temp;
  for (i=1; i<n; i++) {
    if (points[i].w_air_temp < ymin) ymin = points[i].w_air_temp;
    if (points[i].w_air_temp > ymax) ymax = points[i].w_air_temp;
  }
  cpgrnge(ymin, ymax, &ylo, &yhi);
  cpgsci(1);
  cpgsvp(x0, x1, y2, y3);
  cpgswin(t0, t1, ylo, yhi);
  cpgtbox("BCSTZH", 0.0, 0, "BCNST", 0.0, 0);
  cpglab(" ", "Air Temp (°C)", " ");
  cpgsci(2);
  for (i=0; i<n; i++) {
    cpgpt1((points[i].mjd-time0)*86400.0, points[i].w_air_temp, 17);
  }

  /* 2. Pressure versus Time */

  ymin = ymax = points[0].w_pressure;
  for (i=1; i<n; i++) {
    if (points[i].w_pressure < ymin) ymin = points[i].w_pressure;
    if (points[i].w_pressure > ymax) ymax = points[i].w_pressure;
  }
  cpgrnge(ymin, ymax, &ylo, &yhi);
  cpgsci(1);
  cpgsvp(x0, x1, y1, y2);
  cpgswin(t0, t1, ylo, yhi);
  cpgtbox("BCSTZH", 0.0, 0, "BCNST", 0.0, 0);
  cpglab(" ", "Pressure (mbar)", " ");
  cpgsci(2);
  for (i=0; i<n; i++) {
    cpgpt1((points[i].mjd-time0)*86400.0, points[i].w_pressure, 17);
  }

  /* 3. Humidity versus Time */

  ymin = ymax = points[0].w_humidity;
  for (i=1; i<n; i++) {
    if (points[i].w_humidity < ymin) ymin = points[i].w_humidity;
    if (points[i].w_humidity > ymax) ymax = points[i].w_humidity;
  }
  cpgrnge(ymin, ymax, &ylo, &yhi);
  cpgsci(1);
  cpgsvp(x0, x1, y0, y1);
  cpgswin(t0, t1, ylo, yhi);
  cpgtbox("BCSTNZHO", 0.0, 0, "BCNST", 0.0, 0);
  cpglab("UTC", "Humidity (%)", " ");
  cpgsci(2);
  for (i=0; i<n; i++) {
    cpgpt1((points[i].mjd-time0)*86400.0, points[i].w_humidity, 17);
  }

  /* 4. Wind Speed versus Time */

  ymin = ymax = points[0].w_wind_speed;
  for (i=1; i<n; i++) {
    if (points[i].w_wind_speed < ymin) ymin = points[i].w_wind_speed;
    if (points[i].w_wind_speed > ymax) ymax = points[i].w_wind_speed;
  }
  cpgrnge(ymin, ymax, &ylo, &yhi);
  cpgsci(1);
  cpgsvp(x2, x3, y2, y3);
  cpgswin(t0, t1, 0.0, yhi);
  cpgtbox("BCSTZH", 0.0, 0, "BCNST", 0.0, 0);
  cpglab(" ", "Wind Speed (m/s)", " ");
  cpgsci(2);
  for (i=0; i<n; i++) {
    cpgpt1((points[i].mjd-time0)*86400.0, points[i].w_wind_speed, 17);
  }

  /* 5. Wind Direction versus Time */

  cpgsci(1);
  cpgsvp(x2, x3, y1, y2);
  cpgswin(t0, t1, 0.0, 360.0);
  cpgtbox("BCSTNZHO", 0.0, 0, "BCNST", 0.0, 0);
  cpglab("UTC", "Wind Direction", " ");
  cpgsci(2);
  for (i=0; i<n; i++) {
    cpgpt1((points[i].mjd-time0)*86400.0, points[i].w_wind_dir, 17);
  }

  cpgebuf();
  cpgclos();
}

static void init_colors(void)
{
  int error;
  cpgscr(0, 1.0, 1.0, 1.0); /* background */
  cpgscr(1, 0.0, 0.0, 0.0); /* foreground */
  cpgscrn(2, "Tomato", &error);
  cpgscrn(3, "ForestGreen", &error);
  cpgscrn(4, "Blue", &error);
  cpgscrn(6, "Gold2", &error);
  cpgscrn(8, "Turquoise", &error);
  cpgscrn(12, "Deep Pink", &error);
  cpgscrn(13, "SeaGreen", &error);
}

/* Grid search in tilt parameter space */

static double tgridls(int n, struct point points[],
	    double pars[], double dpars[])
{
  int npar = 6;
  int j, neval;
  double var0, var1, var2, delta;

  /* Loop through parameters */

  for (j=0; j < npar; j++){
    neval = 0;
    if (dpars[j] == 0.0)
      continue;
    /* Evaluate starting var */
    tilt_model(n, points, pars, &var1);
    neval++;
    /* Increment parameter and re-evaluate */
    delta = dpars[j];
    pars[j] += delta;
    tilt_model(n, points, pars, &var2);
    neval++;
    if (var1 < var2) {
      /* increasing rms: reverse direction */
      var0 = var2;
      delta = -delta;
      pars[j] += 2.0*delta;
      tilt_model(n, points, pars, &var2);
      neval++;
    }
    while (var2 < var1) {
      var0 = var1;
      var1 = var2;
      pars[j] += delta;
      tilt_model(n, points, pars, &var2);
      neval++;
    }
    /* Found local minimum: go back */
    pars[j] -= delta;

    /* Adjust step size depending on how many steps we took 
       in this direction */
    if (neval > 5)
      /* increase step size for next call */
      dpars[j] *= neval/5.0;
    else if (neval <5)
      /* decrease step size for next call */
      dpars[j] *= 0.5;
  }
  tilt_model(n, points, pars, &var0);
  return var0;
}

static void tilt_model(int n, struct point points[], double pars[], double *var)
{
  int i;
  double az, el, xmeter, ymeter;
  *var = 0.0;
  for (i=0; i< n; i++) {
      az = points[i].mount_az;
      el = points[i].mount_el;
      xmeter= points[i].tilts[0] -
	(pars[0] + pars[1]*sin(az) + pars[2]*cos(az) + pars[3]*cos(el));
      ymeter = points[i].tilts[1] -
	(pars[4] + pars[2]*sin(az) - pars[1]*cos(az) + pars[5]*cos(el));
      *var += xmeter*xmeter;
      *var += ymeter*ymeter;
  }
}

/* Interactive plot of item iy versus item ix in the current plot
   device */

static void interact(int n, struct point points[], int ix, int iy, int which,
		     double xmin, double xmax, double ymin, double ymax,
		     double start, double stop)
{
  char *xlab[] = {"Azimuth", "Elevation", "Deck angle",
		  "Time (UTC)", "Time (LST)"};
  char *ylab[] = {"EL error (arcsec)", "AZ error × cos(EL) (arcsec)", 
		  "AZ error (arcsec)",
		  "Azimuth", "Elevation", "Deck angle"};
  int multiple_symbol = 1;
  int line = 0;
  static int grid = 0;
  static int bg = 0;
  int more = 1;
  int hardcopy = 0;
  int i, pgid;
  char ch;
  float x, y, x1, y1, win[4];
  double region[4];
  float xp[MAX_POINTS], yp[MAX_POINTS];
  int time0 = start;

  /* Check arguments */

  if (ix > 4 || iy > 5) {
    fputs("Invalid option in routine interact\n", stderr);
    return;
  }
#if DEBUG
  printf("Plotting %s versus %s\n", ylab[iy], xlab[ix]);
#endif

  /* Default window for selected items */

  win[0] = xmin;
  win[1] = xmax;
  win[2] = ymin;
  win[3] = ymax;

  /* Get a copy of the selected data */

  for (i=0; i<n; i++) {
    switch (ix) {
    case 0: xp[i] = points[i].az/RPDEG; break;
    case 1: xp[i] = points[i].el/RPDEG; break;
    case 2: xp[i] = points[i].mount_dk/RPDEG; break;
    case 3: xp[i] = (points[i].mjd - time0)*86400.0; break;
    case 4: xp[i] = points[i].lst/RPSEC; break;
    }
    switch (iy) {
    case 0: yp[i] = points[i].elerr[which]/RPARCS; break;
    case 1: yp[i] = points[i].azerr[which]*cos(points[i].el)/RPARCS; break;
    case 2: yp[i] = points[i].azerr[which]/RPARCS; break;
    case 3: yp[i] = points[i].az/RPDEG; break;
    case 4: yp[i] = points[i].el/RPDEG; break;
    case 5: yp[i] = points[i].mount_dk/RPDEG; break;
    }
  }

  /* Draw the plot as many times as requested */

  while (more) {
    if (hardcopy) {
      cpgqid(&pgid);
      if (cpgopen("?") < 1)
	hardcopy = 0;
    }
    if (hardcopy) {
      init_colors();
      cpgpage();
    }
    cpgsave();
    cpgbbuf();
    cpgeras();
    cpgsch(1.0);
    cpgvstd();
    cpgsch(0.8);
    cpgswin(win[0], win[1], win[2], win[3]);
    cpgsci(5);
    if (grid) {
      if (ix == 3 || ix == 4)
	cpgtbox("GZH", 0.0, 0, "G", 0.0, 0);
      else
	cpgbox("G", 0.0, 0, "G", 0.0, 0);
    }
    cpgmove(win[0], 0.0);
    cpgdraw(win[1], 0.0);
    cpgsch(1.0);
    if (line) {
      cpgsci(1);
      cpgline(n, xp, yp);
      if (n > 1) cpgarro(xp[n-2], yp[n-2], xp[n-1], yp[n-1]);
    }
    for (i=0; i<n; i++) {
      cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
      cpgpt1(xp[i], yp[i], multiple_symbol ? plot_symbol(points[i].index) : 16);
    }
    cpgsci(1);
    cpgsch(0.8);
    cpglab(xlab[ix], ylab[iy], "");
    if (ix == 3 || ix == 4)
      cpgtbox("BCNSTZH", 0.0, 0, "BCNST", 0.0, 0);
    else
      cpgbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
    annot_top(TITLE, (char *) 0, start, stop);

    cpgebuf();
    cpgunsa();

    if (hardcopy) {
      cpgclos();
      cpgslct(pgid);
      hardcopy = 0;
    }

    /* Cursor interaction */

  cursor:
    cpgsci(1);
    cpgband(0, 0, 0.5, 0.5, &x, &y, &ch);
    switch (ch) {
    case 'h': case 'H': case '?':
      printf("H or ?:           print this message\n"
	     "S:                replot with a single symbol (toggle)\n"
	     "L:                replot with line joining points (toggle)\n"
	     "B:                black background (toggle)\n"
	     "G:                display grid (toggle)\n"
	     "F:                flag points in rectangle\n"
	     "Z:                zoom\n"
	     "U:                unzoom\n"
	     "P:                make PostScript file\n"
	     "Any other key:    return to plot\n");
      break;
    case 's': case 'S':
      multiple_symbol = !multiple_symbol;
      break;
    case 'l': case 'L':
      line = !line;
      break;
    case 'g': case 'G':
      grid = !grid;
      break;
    case 'z': case 'Z':
      fputs("Zoom: position cursor at one corner of region\n", stderr);
      cpgband(7, 0, x, y, &x, &y, &ch);
      if (ch =='d' || ch == 'D')
	break;
      fputs("Zoom: position cursor at other corner of region\n", stderr);
      cpgband(2, 0, x, y, &x1, &y1, &ch);
      if (ch =='d' || ch == 'D')
	break;
      win[0] = (x < x1) ? x : x1;
      win[1] = (x < x1) ? x1 : x;
      win[2] = (y < y1) ? y : y1;
      win[3] = (y < y1) ? y1 : y;
      break;
    case 'f': case 'F':
      fputs("Flag: position cursor at one corner of region\n", stderr);
      cpgband(7, 0, x, y, &x, &y, &ch);
      if (ch =='d' || ch == 'D')
	break;
      fputs("Flag: position cursor at other corner of region\n", stderr);
      cpgband(2, 0, x, y, &x1, &y1, &ch);
      if (ch =='d' || ch == 'D')
	break;
      region[0] = (x < x1) ? x : x1;
      region[1] = (x < x1) ? x1 : x;
      region[2] = (y < y1) ? y : y1;
      region[3] = (y < y1) ? y1 : y;
      for (i=0; i<n; i++) {
	if (xp[i] >= region[0] && xp[i] <= region[1] &&
	    yp[i] >= region[2] && yp[i] <= region[3]) {
	  points[i].flag = 1;
	}
      }
      break;
    case 'u': case 'U':
      win[0] = xmin;
      win[1] = xmax;
      win[2] = ymin;
      win[3] = ymax;
      break;
    case 'b': case 'B':
      bg = (bg+1) % 5;
      cpgscr(0, 0.25*bg, 0.25*bg, 0.25*bg);
      if (bg < 3)
	cpgscr(1, 1.0, 1.0, 1.0);
      else
	cpgscr(1, 0.0, 0.0, 0.0);
      break;
    case 'p': case 'P':
      hardcopy = 1;
      break;
    case 'i': case 'I':
      /* Find point in window nearest cursor */
      {
	int which = -1;
	double dx, dy, rsq;
	double rsqmin = 1e38;
	for (i=0; i<n; i++) {
	  dx = x - xp[i];
	  dy = y - yp[i];
	  rsq = dx*dx + dy*dy;
	  if (rsq < rsqmin) {
	    rsqmin = rsq;
	    which = i;
	  }
	}
	if (which == -1) {
	  fputs("No point found near cursor\n", stderr);
	} else {
	  printf("Point %d\n", which);
	  cpgsci(7);
	  cpgpt1(xp[which], yp[which], 25);
	}
	goto cursor;
      }
    case 'x': case 'X':
    default:
      more = 0;
      break;
    }
  }
}

/* Annotation for top of plots */

static void annot_top(char *title1, char *title2, double start, double stop)
{
  char date[32];

  cpgsave();
  cpgsch(1.2);
  cpgslw(3);
  cpgsci(4);
  if (title1)
    cpgmtxt("T", 1.8, 0.0, 0.0, title1);
  if (title2)
    cpgmtxt("T", 0.5, 0.0, 0.0, title2);
  cpgsch(0.8);
  cpgslw(2);
  if (start != 0.0) {
    mjd_to_date(start, date, sizeof(date));
    cpgmtxt("T", 1.5, 1.0, 1.0, date);
  }
  if (stop != 0.0) {
    mjd_to_date(stop, date, sizeof(date));
    cpgmtxt("T", 0.5, 1.0, 1.0, date);
  }
  cpgunsa();
}
/*--------------------------------------------------------------------*/

/* Plot EL axis rotation meters (encrot) */

static void rotplot(int n, double el_limit, struct point points[],
		  double start, double stop,
		  char *device)
{
  int i;
  double min_el = el_limit/RPDEG;
  double max_el = 90.0;
  double min_az = 0.0;
  double max_az = 360.0;
  double min_deck = -180.0;
  double max_deck = 180.0;
  float x0, x1, x2, x3, y0, y1, y2, y3;
  char *label = "EL encoder rotation";
  int time0 = start;
  double t0, t1;

  double min_encrot0, max_encrot0, min_encrot1, max_encrot1, delta;

  /* Anything to plot? */

  if (n_pts < 1)
    return;

  /* Find axis range */

  min_encrot0 = max_encrot0 = points[0].encrot[0];
  min_encrot1 = max_encrot1 = points[0].encrot[1];
  for (i=1; i<n; i++) {
    if (points[i].encrot[0] < min_encrot0)
      min_encrot0 = points[i].encrot[0];
    if (points[i].encrot[1] < min_encrot1)
      min_encrot1 = points[i].encrot[1];
    if (points[i].encrot[0] > max_encrot0)
      max_encrot0 = points[i].encrot[0];
    if (points[i].encrot[1] > max_encrot1)
      max_encrot1 = points[i].encrot[1];
  }
  delta = 0.1*(max_encrot0 - min_encrot0);
  if (delta <= 0.0) delta = 10.0*RPARCS;
  min_encrot0 -= delta;
  max_encrot0 += delta;
  delta = 0.1*(max_encrot1 - min_encrot1);
  if (delta <= 0.0) delta = 10.0*RPARCS;
  min_encrot1 -= delta;
  max_encrot1 += delta;

  /* Open plot device */ 

#ifdef DEBUG
  printf("Plotting %s...\n", label);
#endif
  if (cpgopen(device) < 1) return;
  init_colors();
  cpgpage();
  cpgbbuf();

  /* Set the standard viewport, and divide it into six parts */

  cpgvstd();
  cpgqvp(0, &x0, &x3, &y0, &y3);
  x1 = (2.0*x0 + x3)/3.0;
  x2 = (x0 + 2.0*x3)/3.0;
  y1 = (2.0*y0 + y3)/3.0;
  y2 = (y0 + 2.0*y3)/3.0;

  /* Annotation */

  annot_top(TITLE, label, start, stop);
  cpgsch(0.6);

  t0 = (start-time0)*86400.0 - 600.0;
  t1 = (stop-time0)*86400.0 + 600.0;

  /* 1. ENCROT0 versus AZ */

  cpgsci(1);
  cpgsvp(x0, x1, y2, y3);
  cpgswin(min_az, max_az, min_encrot0/RPARCS, max_encrot0/RPARCS);
  cpgtbox("BCST", 0.0, 0, "BCNST", 0.0, 0);
  cpglab(" ", "EL ENC ROT 0 (arcsec)", " ");
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].az/RPDEG, points[i].encrot[0]/RPARCS,
	   plot_symbol(points[i].index));
  }

  /* 2. ENCROT1 versus AZ */

  cpgsci(1);
  cpgsvp(x0, x1, y1, y2);
  cpgswin(min_az, max_az, min_encrot1/RPARCS, max_encrot1/RPARCS);
  cpgtbox("BCNST", 0.0, 0, "BCNST", 0.0, 0);
  cpglab("Azimuth", "EL ENC ROT 1 (arcsec)", " ");
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].az/RPDEG, points[i].encrot[1]/RPARCS,
	   plot_symbol(points[i].index));
  }

  /* 3. ENCROT0 versus EL */

  cpgsci(1);
  cpgsvp(x1, x2, y2, y3);
  cpgswin(min_el, max_el, min_encrot0/RPARCS, max_encrot0/RPARCS);
  cpgtbox("BCST", 0.0, 0, "BCST", 0.0, 0);
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].el/RPDEG, points[i].encrot[0]/RPARCS,
	   plot_symbol(points[i].index));
  }

  /* 4. ENCROT1 versus EL */

  cpgsci(1);
  cpgsvp(x1, x2, y1, y2);
  cpgswin(min_el, max_el, min_encrot1/RPARCS, max_encrot1/RPARCS);
  cpgtbox("BCNST", 0.0, 0, "BCST", 0.0, 0);
  cpglab("Elevation", " ", " ");
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].el/RPDEG, points[i].encrot[1]/RPARCS,
	   plot_symbol(points[i].index));
  }

  /* 5. ENCROT0 versus DK */

  cpgsci(1);
  cpgsvp(x2, x3, y2, y3);
  cpgswin(min_deck, max_deck, min_encrot0/RPARCS, max_encrot0/RPARCS);
  cpgtbox("BCST", 0.0, 0, "BCST", 0.0, 0);
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].mount_dk/RPDEG, points[i].encrot[0]/RPARCS,
	   plot_symbol(points[i].index));
  }

  /* 6. ENCROT1 versus DK */

  cpgsci(1);
  cpgsvp(x2, x3, y1, y2);
  cpgswin(min_deck, max_deck, min_encrot1/RPARCS, max_encrot1/RPARCS);
  cpgtbox("BCNST", 0.0, 0, "BCST", 0.0, 0);
  cpglab("Deck Angle", " ", " ");
  for (i=0; i<n; i++) {
    cpgsci(points[i].flag ? COLOR_FLAG : points[i].color);
    cpgpt1(points[i].mount_dk/RPDEG, points[i].encrot[1]/RPARCS,
	   plot_symbol(points[i].index));
  }

  cpgebuf();
  cpgclos();
}

/*--------------------------------------------------------------------*/

/* Parse a pair of date:time strings to get a UT range as two modified
 * Julian dates.
 * Input: arg1, arg2 (may be null)
 * Output: start, end
 * Returns: 0 if valid, otherwise invalid input strings
 */

static int parse_timerange(char *arg1, char *arg2,
			   double *start, double *end)
{
  char tmp[128];
  int i;

  /* arg1 is start time */

  if (!arg1) {
    *start = 0.0;
  } else if (date_to_mjd(arg1, start) || *start < 50000.) {
    return 1; /* bad start time */
  }

  /* arg2 is end time */

  if (!arg2) {
    *end = 0.0;
    return 0;
  }

  /* if arg2 doesn't contain a date part, add one from start; if
     necessary, increment the date to make the end time later than the
     start */

  if (strchr(arg2, '-') != 0) {
    if (date_to_mjd(arg2, end) || *end < 50000. || *start > *end)
      return 2; /* bad end time */
  } else {
    mjd_to_date(*start, tmp, sizeof(tmp)); /* convert to string */
    for (i=0; i<2; i++) {
      *strchr(tmp, ' ') = '\0'; /* truncate string at end of date */
      strcat(tmp, ":");
      strncat(tmp, arg2, sizeof(tmp)-strlen(tmp)-1); /* append time */
      if (date_to_mjd(tmp, end))
	return 2; 
      if (*start < *end)
	return 0;
      mjd_to_date(*start+1.0, tmp, sizeof(tmp));
    }
  }
  return 0;
}



