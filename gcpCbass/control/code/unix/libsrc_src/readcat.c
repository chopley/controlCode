#include <pthread.h>
#include <math.h>

#include "source.h"
#include "astrom.h"
#include "const.h"
#include "input.h"
#include "lprintf.h"

int main(int argc, char *argv[])
{
  SourceCatalog *sc;   /* The source catalog */
  Site site;           /* The location of the observer */
  OutputStream *output;/* The stream connected to stdout */
  Source *src;         /* A source-catalog entry */
  SourceId id;         /* The identification of a source */
  Date date;           /* The current date and time (Gregorian UTC) */
  double utc;          /* The current date and time (MJD UTC) */
  SourceInfo info;     /* Contemporary information about a source */
  int i;
/*
 * Open an output stream and connect it to stdout.
 */
  output = new_OutputStream();
  if(!output || open_StdoutStream(output))
    return 1;
/*
 * Create an empty source catalog.
 */
  sc = new_SourceCatalog();
  if(!sc)
    return 1;
/*
 * Initialize the source catalog with its contents.
 */
  if(read_SourceCatalog(sc, "~mcs/SZA/ephem", "source.cat"))
    return 1;
/*
 * Set the site location.
 */
#if 0
  if(set_Site(&site, -(118+(17+36/60.0)/60.0) * dtor,
		 -90 * dtor, 1216.0))
    return 1;
#elif 1
  if(set_Site(&site, -(118+(17+36/60.0)/60.0) * dtor,
		 (37+(13+53.8/60.0)/60.0) * dtor, 1216.0))
    return 1;
#elif 0
  if(set_Site(&site, -(118+(16+56/60.0)/60.0) * dtor,
		 (37+(14+2/60.0)/60.0) * dtor, 1222.0))
    return 1;
#endif
/*
 * Install the latest ut1-utc ephemeris.
 */
  if(sc_set_ut1utc_ephemeris(sc, "~mcs/SZA/ephem", "ut1utc.ephem"))
    return 1;
/*
 * Get the current time.
 */
  if(current_date(&date) || (utc=date_to_mjd_utc(&date)) == -1)
    return 1;
/*
 * Lookup each command line argument as a source name.
 */
  for(i=1; i<argc; i++) {
    char *name = argv[i];
#if 1
    int min;
#endif
/*
 * Lookup the source.
 */
    src = find_SourceByName(sc, name);
    if(!src || get_SourceId(src, 1, &id)) {
      lprintf(stderr, "Source \"%s\" is not in the catalog.\n", name);
      continue;
    };
/*
 * Display a header.
 */
    if(output_printf(output, "Source: %s (%s):\n", name, id.name) < 0)
      return 1;
/*
 * Get contemporary information about the source.
 */
#if 1
    for(min=0; min<15000; min+=10) {
      utc = 51070 + min/1440.0;
#endif
    if(source_info(sc, &site, src, utc, 30.0 * dtor, SIO_ALL, &info))
      continue;
/*
 * Display its topocentric horizon coordinates.
 */
    if(write_OutputStream(output, " AZ: ") ||
       output_sexagesimal(output, "", 13, 0, 3, info.coord.az * rtod) ||
       write_OutputStream(output, "  EL: ") ||
       output_sexagesimal(output, "", 13, 0, 3, info.coord.el * rtod) ||
       write_OutputStream(output, "  PA: ") ||
       output_sexagesimal(output, "", 13, 0, 3, info.coord.pa * rtod) ||
       write_OutputStream(output, "\n"))
      return 1;
/*
 * Display its equatorial apparent geocentric coordinates.
 */
#if 1
    if(write_OutputStream(output, " RA: ") ||
       output_sexagesimal(output, "", 13, 0, 3, info.ra * rtoh) ||
       write_OutputStream(output, " DEC: ") ||
       output_sexagesimal(output, "", 13, 0, 3, info.dec * rtod) ||
       write_OutputStream(output, "\n"))
      return 1;
#endif
/*
 * Display the visibility of the source.
 */
    switch(info.sched) {
    case SRC_NEVER_RISES:
      if(write_OutputStream(output, " Never rises.\n"))
	return 1;
      break;
    case SRC_NEVER_SETS:
      if(write_OutputStream(output, " Never sets.\n"))
	return 1;
      break;
    case SRC_ALTERNATES:
      if(write_OutputStream(output, " Rises at ") ||
	 output_double(output, "", 0, 0, 'f', floor(info.rise)) ||
	 write_OutputStream(output, "/") ||
	 output_sexagesimal(output, "0", 8, 0, 0, fmod(info.rise,1.0)*24.0) ||
	 write_OutputStream(output, " Sets at ") ||
	 output_double(output, "", 0, 0, 'f', floor(info.set)) ||
	 write_OutputStream(output, "/") ||
	 output_sexagesimal(output, "0", 8, 0, 0, fmod(info.set,1.0)*24.0) ||
	 write_OutputStream(output, " UTC\n"))
	return 1;
      break;
    case SRC_IRREGULAR:
    default:
      if(write_OutputStream(output, " Rise and set times indeterminate.\n"))
	return 1;
      break;
    };
/*
 * Display the current date and time.
 */
    if(write_OutputStream(output, " UTC:     ") ||
       output_double(output, "", 0, 0, 'f', floor(utc)) ||
       write_OutputStream(output, "/") ||
       output_sexagesimal(output, "0", 8, 0, 0, fmod(utc,1.0)*24.0) ||
       write_OutputStream(output, "\n"))
      return 1;
#if 1
    };
#endif
  };
  return 0;
}
