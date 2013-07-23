#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<ctype.h>
#include<string.h>

#define NEXT(a) ((a)->next)

// #define DEBUG 

typedef struct range_node Range_node, *Range;
typedef struct source_node Source_node, *Source;

typedef struct {
  int year;
  int month;
  int day;
} Date;

struct source_node {
  char name[100];
  Range times;
  Range last;
  Source next;
};

struct range_node {
  char start[100];
  char stop[100];
  Range next;
};
/*
 * Enumerate valid months and their non-leap year number of days.
 */
struct Month {
  char *name;
  int imonth;
  int nday;
};
static struct Month months[12] = {
  {"Jan", 1, 31},
  {"Feb", 2, 28},
  {"Mar", 3, 31},
  {"Apr", 4, 30},
  {"May", 5, 31},
  {"Jun", 6, 30},
  {"Jul", 7, 31},
  {"Aug", 8, 31},
  {"Sep", 9, 30},
  {"Oct", 10,31},
  {"Nov", 11,30},
  {"Dec", 12,31},
};

static int compdate(Date date1, Date date2, Date *result);
static int parsedate(char *fdate, Date *date);
static int getsdate(char *sdate, Date *date);

Range installSource(Source* list, char* name, char* starttime, char* stoptime);

Range new_Range(char* name);
Range append_Range(Range* list, char* starttime, char* stoptime);

Source new_Source(char* name);
Source append_Source(Source *list, char* name);
Source find_Source(Source *list, char* name);

int emptySourceList(Source list);
int emptyRangeList(Range list);

void free_Source(Source *list);
void free_Range(Range *list);
extern int coprocess(char *exe, char **argv, FILE **rfp, FILE **wfp);

int main(int argc, char *argv[])
{
  FILE *pipe;
  char command[300];
  char buf[200], tmpbuf[200];
  Source list=NULL,tmp,tmpsource;
  Source currsource=NULL;
  Range currrange=NULL,tmp2;
  char *time=NULL,*date=NULL,*sdate=NULL,lastsource[100],source[100],
    timestring[100], starttime[100], stoptime[100];
  FILE *rfp, *wfp;
  int nsource = 0;
  char *junk[]={NULL},*localhost=NULL;
  int dofull=0,pending=0;
  bool startLine=false;
  int i;
  int islinux=1;
  Date datetmp = {0}; /* A container for reading new dates. */
  Date current = {0}; /* The highest of the current date stamp from the 
			       file name or from a Date line in the logfile */
  static char srccheck[]="track";
  static char schstartcheck[]=" Starting schedule:";
  static char schstopcheck[]=" Exiting schedule:";
  char *startcheck=NULL, *stopcheck=NULL;
  int sort=1;
  int counter=0;
  bool doSrc=true;

  typedef enum { /* Enumerate the different hosts we know about */
    HOST_SPT,
    HOST_CROSS,
    HOST_NONLOCAL
  } Host;
  Host host=HOST_NONLOCAL;
  bool test=false;

  lastsource[0] = '\0';
  datetmp.year = current.year = 0;
  datetmp.month = current.month = 0;
  datetmp.day = current.day = 0;

  if(argc < 4) {
    fprintf(stderr,"Usage: src|sch list|sort filename1 filename2 ... (regexp)\n");
    fprintf(stderr,"Note: If you use regexp characters, you must enclose the\n      expression in single quotes (\')\n");
    return 0;
  }
  /*
   * Check what host we're on.
   */
  if((localhost=getenv("HOST"))==NULL) 
    host = HOST_NONLOCAL;
  else if(strstr(localhost,"spt")!=NULL) 
    host = HOST_SPT;
  else if(strstr(localhost,"cross")!=NULL) 
    host = HOST_CROSS;
  /*
   * Now check whether we are searching for sources or schedules.
   */
  if(strcmp(argv[1], "src")==0) {
    startcheck = srccheck;
    doSrc = true;
  }
  else if(strcmp(argv[1], "sch")==0) {
    startcheck = schstartcheck;
    stopcheck  = schstopcheck;
    doSrc = false;
  }
  /*
   * A kludge to look in the testcrate directory
   */
  else if(strcmp(argv[1], "srctest")==0) {
    startcheck = srccheck;
    test = true;
  }
  else if(strcmp(argv[1], "schtest")==0) {
    startcheck = schstartcheck;
    stopcheck  = schstopcheck;
    test = true;
  }
  else {
    fprintf(stderr,"Unrecognized parameter: %s\n",argv[1]);
    return 1;
  }
  /*
   * Now check what output was requested.
   */
  if(strcmp(argv[2],"sort")==0) 
    sort = 1;
  else if(strcmp(argv[2],"list")==0) 
    sort = 0;
  else {
    fprintf(stderr,"Unrecognized output format: %s\n",argv[2]);
    return 1;
  }
  /*
   * Main loop -- for each argument, grep out relevant lines
   */
  for(i=3;i < argc;i++) {

    if(host != HOST_NONLOCAL) {
      if(host==HOST_SPT || host==HOST_CROSS)
	strcpy(command,"egrep --with-filename --binary-files=text \"[0-9] track|Starting schedule:|Exiting schedule:|Date:\" ");
      else
	strcpy(command,"egrep --binary-files=text \"[0-9] track|Starting schedule:|Exiting schedule:|Date:\" ");
    }
    else
      strcpy(command,"egrep --with-filename --binary-files=text \"[0-9] track|Starting schedule:|Exiting schedule:|Date:\" ");

    /*
     * Do we need to parse a regexp string?
     */
    if(strchr(argv[i],'*') || strchr(argv[i],'['))
      dofull = 1;
    else
      dofull = 0;
    /*
     * First argument should be the directory in which the logfiles are to 
     * be found.
     */
    switch (host) {
    case HOST_SPT:
    case HOST_CROSS:
      strcat(command, "/data/sptdaq/log/");
      break;
    default: /* Default to parker */
      strcat(command, "/data/sptdaq/log/");
      break;
    };
    /*
     * Next, tack on the name of the file(s) to search.
     */
    strcat(command,argv[i]);
    if(host==HOST_NONLOCAL)
      strcat(command,"\'");
    /*
     * Now execute the command.
     */    

    pipe = popen(command,"r");
    /*
     * If this was a single file, read the date stamp out of it directly 
     * from the file name.
     */
    if(!dofull) {
      /*
       * Read the day stamp out of the filename -- this will be the first 
       * string of every line.
       */

      date = strtok(argv[i], "_");
      while(!isdigit((int)(*date))) ++date;

      parsedate(date, &current);
    }
    /*
     * Now loop over all matching lines.  These can be either track lines, or
     * Date lines.
     */
    while(fgets(buf, sizeof(buf),pipe) != NULL) {
      strcpy(tmpbuf, buf);
      /*
       * If this is not a single file, we need to read the date from
       * the file name at the head of the line.
       *
       *
       * Read the day stamp out of the filename -- this will be the first 
       * string of every line.  
       */
      /*
       * Ignore emacs droppings
       */
      if(strstr(buf, ".log~"))
	 continue;

      if(!islinux) 
	dofull = strstr(buf, ".log:")!=NULL;
      
      if(dofull) {
	date = strstr(buf, ".log:");
	date -= 15;



	date = strtok(date, "_");
	
	parsedate(date, &datetmp);

	strtok(NULL, ":");
      }
      /*
       * And read the timestamp from this line.
       */

      time = strtok(dofull ? NULL : buf, " ");
      /*
       * If this file was created after the time format was changed,
       * read the next line.  
       */
      if(strstr(time,":")==NULL)

	time = strtok(NULL," ");
      /*
       * And if this was a Date line, read the new date.
       */
      if(strstr(tmpbuf,"Date")) {

	strtok(strstr(buf,"Date")," ");

	sdate = strtok(NULL," ");
	getsdate(sdate, &current);
      }
      /*
       * Store the most recent of the file date stamp and the date stamp 
       * from the last Date line.
       */
      compdate(current, datetmp, &current);
      /*
       * And construct the date string out of the date and time.
       */
      sprintf(timestring, "%02d-%s-%04d:%s", 
	      current.day, months[current.month].name, current.year, time);
      /*
       * Furthermore, if the line contained a src or sch specifier, extract 
       * the name.
       */
      if(strstr(tmpbuf,startcheck) || (stopcheck && strstr(tmpbuf,stopcheck))) {

#if 0
	if(strstr(tmpbuf, "blank_sky.sch") != 0)
	  fprintf(stdout, "Hello\n");
#endif

	// If this was a start line

	if(strstr(tmpbuf,startcheck)) {

	  // Now extract the source/sch name

	  strtok(strstr(tmpbuf,startcheck)," ");

	  if(startcheck==schstartcheck) 
	    strtok(NULL," ");

	  strcpy(source,strtok(NULL,"\n"));

	  //------------------------------------------------------------
	  // List the source
	  //------------------------------------------------------------

	  /*
	   * If the new source doesn't match the last, we encountered two 
	   * start lines with no intervening exit lines.
	   */
	  if(!sort) {
	    if(startcheck==schstartcheck) {

	      /*
	       * If we are waiting for an exit line from the previous
	       * schedule, but have instead encountered a new start line,
	       * terminate the old listing without an end time (regardless
	       * of whether the two schedules match.
	       */
	      if(pending)
		fprintf(stdout, "                     %s\n", lastsource);
	      /*
	       * And start the new listing.
	       */
	      fprintf(stdout,"%s, ",timestring);
	      
	      if(startcheck==schstartcheck) 
		pending = 1;
	    }
	    
	    // Else this is a source listing -- don't check for exit
	    // lines.

	    else
	      fprintf(stdout,"%s %s\n",timestring,source);
	  }
	  
	  //------------------------------------------------------------
	  // Now add the source to the list of known sources
	  //------------------------------------------------------------

	  if(doSrc) {
	    // Case 1: this is the first source encountered: install
	    // it as the current source and keep the start time

	    if(lastsource[0] == '\0') {
	      strcpy(lastsource, source);
	      strcpy(starttime, timestring);
	    }
	    
	    // Case 2: this is not the first source encountered, and it
	    // is the same as the last source. Do nothing -- we will
	    // elide consecutive source listings into a single range
	    
	    // Case 3: this is not the first source encountered, and
	    // it is different from the last source.  Close off the
	    // range with the stop time, and add it to the list.  Install it as the
	    // current source, and store the new start time.

	    else if(strcmp(lastsource, source) != 0) {

	      strcpy(stoptime, timestring);
	      installSource(&list, lastsource, starttime, stoptime);

	      strcpy(lastsource, source);
	      strcpy(starttime, timestring);

	    }

	  } else {
	    // Case 1: this is the first schedule encountered: install
	    // it as the current source and keep the start time

	    if(lastsource[0] == '\0') {
	      strcpy(lastsource, source);
	      strcpy(starttime, timestring);
	    }

	    // Case 2: This is not the first schedule encountered, and
	    // we are done with the previous schedule listing: Install
	    // it as the current source, and store the start time.

	    else if(!pending) {
	      strcpy(lastsource, source);
	      strcpy(starttime, timestring);
	    }

	    // Case 2: This is not the first schedule encountered, and
	    // we are waiting for an exit line from the previous
	    // schedule listing: Close off the range with no time
	    // stamp, and add it to the list.  Install it as the
	    // current source and store the new start time.

	    else {
	      installSource(&list, lastsource, starttime, 0);

	      strcpy(lastsource, source);
	      strcpy(starttime, timestring);
	    }
	  }

	  // Set the pending flag to true

	  pending = true;
	}
	/*
	 * Else if this was an exit line for a schedule
	 */
	else {

	  if(startcheck==schstartcheck) {
	    
	    strtok(strstr(tmpbuf,stopcheck)," ");
	    
	    strtok(NULL," ");
	    
	    strcpy(source,strtok(NULL,"\n"));
	  }

	  if(!sort) {
	    /*
	     * If the schedule exiting is not the same one that started last,
	     * then a new schedule was started with no record of it in the log
	     * file
	     */
	    if(strcmp(lastsource, source) != 0) {
	      /*
	       * If we were waiting for an exit line for the previous schedule,
	       * and we instead encountered an exit line for a different 
	       * schedule, terminate the current schedule listing with no end 
	       * time.
	       */
	      if(pending)
		fprintf(stdout, "                     %s\n", lastsource);
	      /*
	       * And write an entry for the schedule we just encountered, with
	       * no start time.
	       */
	      fprintf(stdout, "                    , %s %s\n", timestring, 
		      source);
	    }
	    else if(pending)
	      fprintf(stdout, "%s %s\n", timestring, lastsource);
	    pending = 0;
	  }

	  //------------------------------------------------------------
	  // Store the schedule
	  //------------------------------------------------------------

	  // Case 1: we are waiting for an exit line, and the schedule
	  // matches the current one: close off the range, and add it
	  // to the list. 

	  if(pending) {

	    if(strcmp(lastsource, source)==0) {

	      strcpy(stoptime, timestring);
	      installSource(&list, lastsource, starttime, stoptime);

	    } else {

	      // Case 2: we are waiting for an exit line, and the schedule
	      // doesn't match the current one: close off the current
	      // range with no end time, and add it to the list.  Add a
	      // range for the new schedule with no start time.

	      installSource(&list, lastsource, starttime, 0);

	      strcpy(stoptime, timestring);
	      installSource(&list, source, 0, stoptime);
	    }
	  } else {

	    // Case 3: we are not waiting for an exit line.  Add a range
	    // for the new schedule with no start time.

	    strcpy(stoptime, timestring);
	    installSource(&list, source, 0, stoptime);
	  }

	  // And set the pending flag to false

	  pending = false;
	}
      }

      /*
       * If not, skip the rest of the loop.
       */
      else
	continue;
    }

    pclose(pipe);
  }

  // If we were waiting for an exit line when the read ended, close
  // out the terminal range with no stop time

  if(!doSrc && pending) {
    installSource(&list, lastsource, starttime, 0);

    // And if not sorting, print the final entry

    if(!sort)
      fprintf(stdout, "                     %s\n", lastsource);
  }

  /*
   * If sorted output was requested, pipe the list to sort.
   */
  if(sort) {
    if(coprocess("sort", junk, &rfp, &wfp))
      exit(1);

    for(tmp=list;tmp != NULL;tmp = tmp->next)  
      fprintf(wfp, "%s\n", tmp->name);
    fclose(wfp);
    
    while(fgets(source, sizeof(source), rfp)) {
      char *cptr = strchr(source, '\n');
      if(cptr) *cptr = '\0';
      for(tmp=list;tmp != NULL;tmp = tmp->next)
	
	if(strcmp(tmp->name,source)==0) {
	  printf("\n%s:\n\n",source);
	  for(tmp2=tmp->times;tmp2 != NULL;tmp2=tmp2->next) {
	    printf("\t%s, ",tmp2->start);
	    if(tmp2->stop[0] == '\0')
	      printf("%s","          \n");
	    else
	      printf("%s\n",tmp2->stop);
	  }
	}
    }
  }
  fprintf(stdout,"\n");
  free_Source(&list);

  return 1;
}

Range installSource(Source* list, char* name, char* starttime, char* stoptime)
{
  Source src = find_Source(list, name);
  src->last = append_Range(&src->times, starttime, stoptime);

  return src->last;
}

Range new_Range(char* starttime, char* stoptime)
{
  Range newRange = (Range) malloc(sizeof(Range_node));
  
  if(newRange == NULL)
    return 0;

  NEXT(newRange) = NULL;

  sprintf(newRange->start, "%20s", "");
  sprintf(newRange->stop, "%20s", "");

  if(starttime)
    strcpy(newRange->start, starttime);

  if(stoptime)
    strcpy(newRange->stop, stoptime);

  return newRange;
}

Range append_Range(Range* list, char* starttime, char* stoptime)
{
  Range newRange = new_Range(starttime, stoptime);
  Range tmp;
  
  if(newRange == NULL)
    return 0;
  
  if(emptyRangeList(*list))
    *list = newRange;

  else {
    for(tmp = *list; NEXT(tmp) != NULL;tmp = NEXT(tmp));
    NEXT(tmp) = newRange;
  }

  return newRange;
}

Source new_Source(char* name)
{
  Source src = (Source) malloc(sizeof(Source_node));
  
  if(src == NULL)
    return 0;

  src->times = 0x0;
  src->last  = 0x0;

  NEXT(src) = NULL;

  strcpy(src->name, name);

  return src;
}

Source find_Source(Source* list, char* name)
{
  Source tmp=0;

  if(emptySourceList(*list))
    return append_Source(list, name);
  else {
    for(tmp = *list; tmp != NULL;tmp = NEXT(tmp))
      if(strcmp(tmp->name, name)==0)
	return tmp;
  }

  return append_Source(list, name);
}

Source append_Source(Source *list, char* name)
{
  Source src = new_Source(name);
  Source tmp=0;

  if(emptySourceList(*list))
    *list = src;
 
  else {
    for(tmp = *list; NEXT(tmp) != NULL;tmp = NEXT(tmp));
    NEXT(tmp) = src;
  }

  return src;
}

int emptySourceList(Source list)
{
  return (list == NULL) ? 1 : 0;
}

int emptyRangeList(Range list)
{
  return (list == NULL) ? 1 : 0;
}


void free_Source(Source *list)
{
  Source temp,nxt;

  if(*list) 
    for(temp = *list;NEXT(temp) != NULL;temp = nxt) {
      nxt = NEXT(temp);
      free_Range(&temp->times);
      free(temp);
    }
}
void free_Range(Range *list)
{
  Range temp,nxt;

  if(*list) 
    for(temp = *list;NEXT(temp) != NULL;temp = nxt) {
      nxt = NEXT(temp);
      free(temp);
    }
}
/*.......................................................................
 * Compare two dates, and return the more recent of the two.
 */
static int compdate(Date date1, Date date2, Date *result) 
{
  int dayno, iy, i;
  double jul1, jul2;
/*
 * Check for leap years
 */
  if(date1.year % 4 == 0)
    if((date1.year % 100 != 0) || (date1.year % 400 == 0)) 
      months[1].nday = 29;
  /*
   * Now convert the first date's month and day into a day number.
   */
  dayno = 0;
  for(i=0;i < date1.month;i++)
    dayno += months[i].nday;
  dayno += date1.day;

  iy = date1.year-1;
  jul1 = iy*365 + (iy/4) - (iy/100) + (iy/400);
  jul1 += dayno;
/*
 * Check for leap years
 */
  if(date2.year % 4 == 0)
    if((date2.year % 100 != 0) || (date2.year % 400 == 0)) 
      months[1].nday = 29;
  /*
   * Now convert the first date's month and day into a day number.
   */
  dayno = 0;
  for(i=0;i < date2.month;i++)
    dayno += months[i].nday;
  dayno += date2.day;

  iy = date2.year-1;
  jul2 = iy*365 + (iy/4) - (iy/100) + (iy/400);
  jul2 += dayno;
  /*
   * Now store whichever is the larger of the two dates.
   */
  result->year = jul1 > jul2 ? date1.year : date2.year;
  result->month = jul1 > jul2 ? date1.month : date2.month;
  result->day = jul1 > jul2 ? date1.day : date2.day;

  return 0;
}
/*.......................................................................
 * Parse a date string into a date structure.
 */
static int parsedate(char *fdate, Date *date)
{
  char num[5];
  int i;

  for(i=0;i < 4;i++)
    num[i] = *fdate++;
  num[i] = '\0';
  date->year = atoi(num);

  for(i=0;i < 2;i++)
    num[i] = *fdate++;
  num[i] = '\0';
  date->month = atoi(num)-1;

  for(i=0;i < 2;i++)
    num[i] = *fdate++;
  num[i] = '\0';
  date->day = atoi(num);

  return 0;
}

/*.......................................................................
 * Parse a date string into a date structure.
 */
static int getsdate(char *sdate, Date *date)
{
  char *month=NULL;
  int day, year;
  int i;


  day = atoi(strtok(sdate,"-"));

  month = strtok(NULL,"-");

  year = atoi(strtok(NULL," "));

  for(i=0;i < 12;i++)
    if(strcasecmp(month, months[i].name)==0)
      break;
  
  date->day = day;
  date->year = year;
  date->month = i;

  return 0;
}
