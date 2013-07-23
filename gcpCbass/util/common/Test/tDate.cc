#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Date.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/IoLock.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "date",     "01-jan-2009:00:01:02",   "s", "dd-mmm-yy:hhy:mm:ss"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  Date date;
  TimeVal tVal;
  tVal.setToCurrentTime();
  
  //date.setToDateAndTime(Program::getParameter("date"));

  date.setMjd(tVal.getMjd());

  COUT(date.mjdToHorizonsCal());

  COUT(date.mjdToCal(5.4848001658368041e4));

  COUT("Day in week: " << date.dayInWeek(date.mjd()));

  COUT("Day = " << Date::days[date.dayInWeek(date.mjd())] << std::endl);

  COUT("Month[0] =  " << Date::months[0]);
  COUT("Day[0]   =  " << Date::days[0]);

  return 0;
}
