#define __FILEPATH__ "util/common/Test/tRegDate.cc"

#include <iostream>
#include <vector>
#include <iomanip>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/RegDate.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "reg",         "",  "s", "Register to check"},
  { "dir",         "",  "s", "Cal file directory"},
  { "name",        "",  "s", "Cal file name"},
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel(Program::getiParameter("debuglevel"));
 
  COUT("sizeof(RegDate) = " << sizeof(RegDate));
  COUT("sizeof(RegDate::Data) = " << sizeof(RegDate::Data));

  RegDate date;

  cout << date << endl;

  *date.data() += 100;

  cout << date << endl;

  date.setDayNumber(40587);

  cout << date << endl;

  date.setMilliSeconds(204563);

  cout << date << endl;

  TimeVal timeVal = date.timeVal();

  cout << timeVal << endl;

  RegDate curr, last, diff;

  last.setToCurrentTime();
  curr.setToCurrentTime();

  diff = curr-last;

  COUT(curr);
  COUT(last);
  COUT(diff.timeInSeconds());
}
