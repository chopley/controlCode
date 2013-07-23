#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>

#define stor M_PI/(12*3600)
#define rtos 12*3600/M_PI
#define astor 1.0/206265
#define rtoas 206265
#define rtod 180.0/M_PI
#define daysec 86400

#define NLINE 100

/*
 * The following define the default location of the site.
 */
//static char lat_site[] = "37:13:50.9";/* N latitude */
static char lat_site[] = "-89:00:00";/* N latitude */
/* 
 * This was the position when I originally wrote the program -- this
 * seems to have changed.  Odd -- 25 Jan 2001 
 */
//static char lng_site[] = "118:17:43.8"; /* W longitude */
static char lng_site[] = "0:17:43.8"; /* W longitude */

static int ranum(char *ra, double *fra);
static int decnum(char *dec, double *fdec);
static int rastring(double fra, char *string);
static int decstring(double fdec, char *string);
static double ranrm(double angle);
static double gmst(double mjd);
static int apparent_position(char *ra, char *dec, char *az_s, char *el_s, time_t clock);
static int parse_date(char *dstring, struct tm *gmt);
/*.......................................................................
 * Given a SZA source catalog, print a list of source az and elevations.
 */
int main(int argc, char *argv[]) {
  FILE *fp=NULL;
  char line[NLINE+1];
  char *srcname=NULL,*ra=NULL,*dec=NULL,*mag=NULL;
  char az_s[13],el_s[13];
  double az_f,el_f;
  double elmin=0.0,elmax=90.0; /* min/max elevation (defaults to these values
				  if none are specified by the user */
  double azmin=0.0,azmax=360.0; /* min/max azimuth (defaults to these values
				  if none are specified by the user */
  int dofloat=0;
  time_t clock, clock_tmp;
  struct tm *gmt;
  char *datestring=NULL;
  /*
   * Get the current clock time
   */
  clock = time(NULL);
  gmt = gmtime(&clock);
  /*
   * Show usage if fewer arguments than expected are given.
   */
  if(argc < 2) {
    fprintf(stderr,"Usage: source_catalog (\"str\"|\"float\") (elmin) (elmax) (azmin) (azmax) (date: \"mmm dd hh:mm:ss yyyy\").\n");
    return 0;
  }

  /* 
   * Get command-line arguments here.
   */
  switch (argc) {
  case 8:
    datestring = argv[7];
    if(parse_date(datestring, gmt))
      return 1;
    /*
     * And update clock too -- this is seconds since ??
     */
    clock = mktime(gmt);
  case 7:
    azmax = atof(argv[6]);
    while(azmax < 0)
      azmax += 360;
  case 6:
    azmin = atof(argv[5]);
    while(azmin < 0)
      azmin += 360;
  case 5:
    elmax = atof(argv[4]);
  case 4:
    elmin = atof(argv[3]);
  case 3:
    if(strcmp(argv[2],"float")==0)
      dofloat = 1;
  case 2:
    if((fp=fopen(argv[1],"r"))==NULL) {
      fprintf(stderr,"Unable to open file: %s\n",argv[1]);
      return 1;
    }
    break;
  }
  /*
   * print the date and location
   */
  fprintf(stdout,"Source locations are for longitude: %sW, latitude: %s\nlocal time: %s\n",lng_site, lat_site, ctime(&clock));
  /*
   * Read lines until we encounter "J2000" or "B1950"
   */
  fprintf(stdout,"%-13s %13s %13s %13s %13s %13s\n","Name","RA","DEC","AZ","EL","Mag");
  
  while(fgets(line, NLINE, fp) != NULL) {
    if(line[0] == 'J' || line[0] == 'B') {
#ifdef DEBUG
      fprintf(stdout,"%s\n",line);
#endif

#ifdef DEBUG
      if(strstr(line,"ra5dec-55")!=NULL)
	printf("test\n");
#endif

      strtok(line," \t");
      srcname = strtok(NULL," \t");
      ra = strtok(NULL," \t\n");
      dec = strtok(NULL," \t\n");
      strtok(NULL,"#");
      mag = strtok(NULL," \t\n");
      apparent_position(ra,dec,az_s,el_s,clock);

      if(decnum(az_s, &az_f))
	return 1;
      
      az_f *= rtod;
      
      if(decnum(el_s, &el_f))
	return 1;
      
      el_f *= rtod;
      /*
       * Only print this source if the elevation is within the specified
       * range.
       */
      if(el_f >= elmin && el_f <= elmax && az_f >= azmin && az_f <= azmax) {
	if(!dofloat)
	  fprintf(stdout,"%-13s %13s %13s %13s %13s %13s\n",srcname,ra,dec,
		  az_s,el_s,mag ? mag : " ");
	else
	  fprintf(stdout,"%-13s %13s %13s %13.3f %13.3f %13s\n",srcname,ra,
		  dec,az_f,el_f,mag ? mag : "??");
      }
    }
  }
  if(fp)
    fclose(fp);
  return 0;    
}
/*.......................................................................
 * Given a string ra and dec, return character strings containing az and
 * el
 */
static int apparent_position(char *ra_s, char *dec_s, char *az_s, char *el_s, time_t clock)
{
  double lst,lng,ra,dec,gmtr;
  double ha,az,el,lat;
  double cos_el_sin_az,cos_el_cos_az,sin_el;
  struct tm *gmt=NULL;
  double mjd,mjd0 = 40587; /* Mjd at Jan 1 1970 (0 UT ?)  -- this is the 
				 zero point of the unix system clock */
  double gmstr;
  /*
   * Initialize the latitude, longitude and lst of the site.
   */
  if(decnum(lat_site, &lat))
    return 1;
  
  if(decnum(lng_site, &lng))
    return 1;
  /*
   * Get the UT corresponding to this clock time.
   */
  gmt = gmtime(&clock);
  /*
   * Convert gmt to radians.
   */
  gmtr = ((gmt->tm_hour*60 + gmt->tm_min)*60 + gmt->tm_sec)*stor;
  
  mjd = mjd0 + (double)(clock)/daysec; /* Compute the (integer) Modified 
					  Julian date. */
  /* 
   * Convert to GMST (in seconds) based on explanatory suppl. (2.24-1)
   */
  gmstr = gmst(mjd);
  /*
   * Subtract the west longitude to get lst.
   */
  lst = gmstr-lng;
  
  while (lst < 0)
    lst += 2*M_PI;
  
  if(ranum(ra_s, &ra))
    return 1;
  
  if(decnum(dec_s, &dec))
    return 1;
  
  ha = lst-ra;
  cos_el_cos_az = sin(dec) * cos(lat) - cos(dec) * sin(lat) * cos(ha);
  cos_el_sin_az = -cos(dec) * sin(ha);
  sin_el = sin(dec) * sin(lat) + cos(dec) * cos(lat) * cos(ha);
  el = asin(sin_el);
  /*
   * And compute Azimuth.
   */
  az = atan2(cos_el_sin_az, cos_el_cos_az);
  /*
   * Constrain the azimuth to lie between 0 and 360.
   */
  while(az < 0)
    az += 2*M_PI;
  
  if(decstring(az, az_s))
    return 1;
  if(decstring(el, el_s))
    return 1;

  return 0;
}
/*.......................................................................
 * Given a string of the form (-)hh:mm:ss, return the ra in radians.
 */
static int ranum(char *ra, double *dra)
{
  int  rahr,ramin,sign;
  float rasec;

  if(ra==NULL) {
    fprintf(stderr,"ranum: Received NULL string\n");
    return 1;
  }

  sign = (*ra=='-' ? -1 : 1);

  if(strstr(ra,":") != NULL) {
    sscanf(ra,"%d:%d:%f",&rahr,&ramin,&rasec);
    rahr = abs(rahr);
    *dra = (double)(sign*((rahr*60 + ramin)*60 + rasec)*stor);
  }
  else 
    *dra = atof(ra)*3600*stor;

  return 0;
}
/*.......................................................................
 * Given a string of the form (-)dd:mm:ss, return the dec in radians.
 * 
 * Will also accept a string of the form -48.455 now
 */
static int decnum(char *dec, double *fdec)
{
  int  decdeg,decmin,sign;
  float decsec;
  /* 
   * Check the sign of the passed dec -- if we just check the sign of decdeg
   * after we read it out of the string, we will not get the sign right
   * for strings like -00:22:45, since decdeg will be 0.
   */
  if(dec==NULL) {
    fprintf(stderr,"decnum: Received NULL string\n");
    return 1;
  }
  sign = (*dec=='-' ? -1 : 1);

  if(strstr(dec,":") != NULL) {
    sscanf(dec,"%d:%d:%f",&decdeg,&decmin,&decsec);
    decdeg = abs(decdeg);
    *fdec = (double)(sign*((decdeg*60 + decmin)*60 + decsec)*astor);
  }
  /*
   * Else just degrees and a fractional part were passed
   */
  else 
    *fdec = atof(dec)*3600*astor;

  return 0;
}
/*.......................................................................
 * Given a dec in radians, convert to the string equivalent.
 */
static int decstring(double dec, char *string)
{
  /* Assumes dec is in radians */

  int deg,min;
  int sec;
  int sdec;
  int remain;
  int isneg = 0;

  if(string==NULL) {
    fprintf(stderr,"decstring: Received NULL argument\n");
    return 1;
  }

  sdec = dec*rtoas;          /* dec in arcseconds */

  if(sdec < 0) {
    sdec *= -1;
    dec *= -1;
    isneg = 1;
  }

  remain = (dec*rtoas-sdec)*100;
  
  deg = sdec/3600;
  min = (sdec%3600)/60;
  sec = sdec-60*(deg*60 + min);

  sprintf(string, "%c%02d:%02d:%02d.%02d",(isneg ? '-' : ' '),deg,min,sec,
	  remain);

  return 0;
}
/*.......................................................................
 * Given an RA in radians, convert to the string equivalent.
 */
static int rastring(double fra, char *string)
{
  /* Assumes ra is in radians */

  int ra;
  int hr,min;
  int sec;
  int remain;

/* Convert to seconds */

  if(string==NULL) {
    fprintf(stderr,"rastring: Received NULL argument\n");
    return 1;
  }

  while(fra > 2*M_PI)
    fra -= 2*M_PI;

  ra = fra*rtos;
  remain = (fra*rtos - ra)*100;

  hr = ra/3600;
  min = (ra%3600)/60;
  sec = ra-60*(hr*60 + min);

  sprintf(string, "%02d:%02d:%02d.%02d",hr,min,sec,remain);

  return 0;
}
/*.......................................................................
 * Given the mjd, return the gmst.
 */
static double gmst(double mjd)
{
  double tu;
  /* 
   * Julian centuries from fundamental epoch J2000 to this UT
   */
  tu = (mjd - 51544.5 ) / 36525.0;
  
  /* GMST at this UT */
  return ranrm(fmod(mjd, 1.0)*2*M_PI + 
	       (24110.54841 + 
		(8640184.812866 +
		 (0.093104 - 6.2e-6*tu)*tu)*tu)*stor);
}
/*.......................................................................
 * Normalize angle into 0-2*pi range.
 */
static double ranrm(double angle)
{
   double w;
 
   w = fmod(angle, 2*M_PI);
   return (w >= 0.0) ? w : w + 2*M_PI;
}



/*.......................................................................
 * Parse an alternate date of the form:
 * 
 * Thu Jan 25 18:45:22 2001
 */
static int parse_date(char *dstring, struct tm *gmt)
{
  static char *wdays[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
  static char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  int nday=7,nmonth=12;
  static int nmdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  char *day=NULL,*month=NULL, *hr=NULL, *min=NULL, *sec=NULL, *year=NULL;
  int year_day=0,iyear,iday,imonth;
  /*
   * Get the day
   */
  day = strtok(dstring," \t\n");

  for(iday=0;iday < nday;iday++)
    if(strncmp(wdays[iday],day,3)==0)
      break;
  if(iday==nday) {
    fprintf(stderr,"Invalid weekday: %s\n",day);
    return NULL;
  }
  gmt->tm_wday = iday+1;
  /*
   * Get the month
   */
  month = strtok(NULL," \t\n");
  for(imonth=0;imonth < nmonth;imonth++) {
    if(strncmp(months[imonth],month,3)==0)
      break;
  }
  if(imonth==nmonth) {
    fprintf(stderr,"Invalid month: %s\n",month);
    return 1;
  }
  gmt->tm_mon = imonth;
  /*
   * Get the day of the month
   */
  day = strtok(NULL," \t\n");

  gmt->tm_mday = atoi(day);
  /*
   * Now get the time
   */
  gmt->tm_hour = atoi(strtok(NULL,":"));
  gmt->tm_min = atoi(strtok(NULL,":"));
  gmt->tm_sec = atoi(strtok(NULL," \t"));
  /*
   * Now the year, since 1900
   */
  gmt->tm_year = iyear = atoi(strtok(NULL," \t\n"))-1900;
  /*
   * And get the day in the year
   */
  for(imonth=0;imonth < gmt->tm_mon;imonth++)
    year_day += imonth==1 ? (iyear%4==0 ? (iyear%100==0 ? 28 : 29) : 28) : nmdays[imonth];
  gmt->tm_yday = year_day+gmt->tm_mday-1;

  gmt->tm_isdst = 1;
  return 0;
}

