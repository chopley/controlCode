#define __FILEPATH__ "antenna/control/specific/Date.cc"

#include "gcp/util/common/Exception.h"

#include "gcp/antenna/control/specific/Date.h"

using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor function just intializes the date fields by calling
 * reset(), below.
 */
Date::Date()
{
  reset();
}

/**.......................................................................
 * Intialize the date fields to zero
 */
void Date::reset() 
{
  gcp::control::init_Date(&date_, 0, 0, 0, 0, 0, 0, 0);
}

/**.......................................................................
 * Convert from MJD to broken down calendar date
 */
void Date::convertMjdUtcToDate(double utc)
{
  if(gcp::control::mjd_utc_to_date(utc, &date_))
    throw Error("Date::convertMjdToDate: "
		"Error in gcp::control::mjd_utc_to_date().\n");
}

/**.......................................................................
 * Return the year.
 */
int Date::getYear()
{
  return date_.year;
}
