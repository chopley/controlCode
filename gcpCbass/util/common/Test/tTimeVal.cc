#define __FILEPATH__ "util/common/Test/tTimeVal.cc"

#include <iostream>
#include <sstream>
#include <cmath>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h" 
#include "gcp/util/common/TimeVal.h"

#include <iomanip>

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

void printDate();

KeyTabEntry Program::keywords[] = {
  { "mjd",     "MJD0",                        "d", "MJD"},
  {END_OF_KEYWORDS}
};

int Program::main()
{
  TimeVal timeVal;

  timeVal.setMjd(Program::getdParameter("mjd"));
  COUT("Time = " << timeVal << " (" << std::setprecision(12) << timeVal.getMjd() << ")");
  COUT(timeVal.getMjdId(1000000000));

  timeVal.setToCurrentTime();
  COUT("Time = " << timeVal << " (" << std::setprecision(12) << timeVal.getMjd() << ")");

#if 0
  unsigned long days,secs,nsecs;

  days  = timeVal.getMjdDays();
  secs  = timeVal.getMjdSeconds();
  nsecs = timeVal.getMjdNanoSeconds();

  cout << "Days: " << days << " Sec: " << secs << " Nsec: " << nsecs << endl;
  cout << "Id is: " << timeVal.getMjdId() << endl;

  std::cout << timeVal << std::endl;

  timeVal.setMjd(days, secs, nsecs);

  days  = timeVal.getMjdDays();
  secs  = timeVal.getMjdSeconds();
  nsecs = timeVal.getMjdNanoSeconds();

  cout << "Days: " << days << " Sec: " << secs << " Nsec: " << nsecs << endl;
  cout << "Id is: " << timeVal.getMjdId() << endl;

  unsigned long msecs = secs * 1000 + nsecs / 1e6;

  TimeVal nTimeVal;

  nTimeVal.setMjd(days, msecs);

  days  = nTimeVal.getMjdDays();
  secs  = nTimeVal.getMjdSeconds();
  nsecs = nTimeVal.getMjdNanoSeconds();

  double dDays, dMjd = nTimeVal.getTimeInMjdDays();
  cout << "nId is: " << nTimeVal.getMjdId() << endl;

  unsigned utc[2];

  utc[1] = static_cast<unsigned int>(modf(dMjd, &dDays) * 86400 * 1000.0);  
  
  utc[0] = static_cast<unsigned int>(dDays); // Days



  cout << "Mjd Days: " << utc[0] << " MillSeconds: " << utc[1] << endl;

  timeVal.setMjd(utc[0], utc[1]);

  days  = timeVal.getMjdDays();
  secs  = timeVal.getMjdSeconds();
  nsecs = timeVal.getMjdNanoSeconds();

  cout << "Days: " << days << " Sec: " << secs << " Nsec: " << nsecs << endl;
  cout << "Id is: " << timeVal.getMjdId() << endl;

  timeVal.setMjd(53088, 52466013);

  days  = timeVal.getMjdDays();
  secs  = timeVal.getMjdSeconds();
  nsecs = timeVal.getMjdNanoSeconds();

  cout << "Days: " << days << " Sec: " << secs << " Nsec: " << nsecs << endl;
  cout << "Id is: " << timeVal.getMjdId() << endl;

  printDate();
#endif
  

  COUT("date string is: " << timeVal.dateString());

  return 0;
}

/**.......................................................................
 * Return a human-readable string representation of the UTC time
 * managed by this object
 */
void printDate()
{
  TimeVal timeVal;
  timeVal.setToCurrentTime();
  time_t time;
  time = (time_t)timeVal.getTimeInSeconds();

  // Get a string representation of the date up to integral seconds

  std::string timeString(ctime(&time));

  // Erase any newlines that ctime() may have appended

  std::string::size_type idx;
  idx = timeString.find('\n');
  if(idx != std::string::npos)
    timeString.erase(idx);

  // Construct a string representing the fractional seconds

  ostringstream fs, tt;
  fs << timeVal.getFractionalTimeInSeconds();
  string fsStr = fs.str();

  // Now cat the two together

  tt << timeString << " " << fsStr.substr(2, fsStr.length());

  cout << timeVal.getFractionalTimeInSeconds();
  cout << tt.str() << endl;
}

