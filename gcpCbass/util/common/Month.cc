#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Month.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Month::Month() {}

/**.......................................................................
 * Destructor.
 */
Month::~Month() {}

std::map<unsigned, std::string> Month::monthNames_ = Month::createNameMap();
std::map<unsigned, unsigned>    Month::monthDays_  = Month::createDayMap();

/**.......................................................................
 * Return the full month name corresponding to index iMonth (1-12)
 */
std::string Month::fullMonthName(unsigned iMonth, bool capitalize)
{
  validateMonthIndex(iMonth);

  std::string name  = monthNames_[iMonth];

  if(capitalize)
    name[0] = toupper(name[0]);
  
  return name;
}

/**.......................................................................
 * Return the abreviated month name corresponding to index iMonth (1-12)
 */
std::string Month::abbreviatedMonthName(unsigned iMonth, bool capitalize)
{
  std::string name = fullMonthName(iMonth, capitalize);
  return name.substr(0, 3);
}

std::map<unsigned, std::string> Month::createNameMap()
{
  std::map<unsigned, std::string> names;

  names[1]  = "january";
  names[2]  = "february";
  names[3]  = "march";
  names[4]  = "april";
  names[5]  = "may";
  names[6]  = "june";
  names[7]  = "july";
  names[8]  = "august";
  names[9]  = "september";
  names[10] = "october";
  names[11] = "november";
  names[12] = "december";

  return names;
}

/**.......................................................................
 * Return the number of days in a month
 */
unsigned Month::daysInMonth(unsigned iMonth, unsigned iYear)
{
  validateMonthIndex(iMonth);

  bool isLeapYear = ((iYear % 4 == 0) && (iYear % 100 != 0));

  unsigned days = monthDays_[iMonth];

  if(iMonth == 2 && isLeapYear)
    days += 1;

  return days;
}

 std::map<unsigned, unsigned> Month::createDayMap()
{
  std::map<unsigned, unsigned> days;

  days[1]  = 31;
  days[2]  = 28;
  days[3]  = 31;
  days[4]  = 30;
  days[5]  = 31;
  days[6]  = 30;
  days[7]  = 31;
  days[8]  = 31;
  days[9]  = 30;
  days[10] = 31;
  days[11] = 30;
  days[12] = 31;

  return days;
}

void Month::validateMonthIndex(unsigned iMonth)
{
  if(iMonth < 1 || iMonth > 12) {
    ThrowError("Invalid month index: " << iMonth << " (must be 1-12)");
  }
}
