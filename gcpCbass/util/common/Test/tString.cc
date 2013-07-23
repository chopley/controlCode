#define __FILEPATH__ "util/common/Test/tString.cc"

#include <iostream>
#include <iomanip>

#include <cmath>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/String.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { END_OF_KEYWORDS}
};

int Program::main()
{
  String str("        2007  9 21  54364       0.1613      0.2231     -0.17537              ");

  try {
    cout << str.findNextStringSeparatedByChars(" ", true);
    cout << str.findNextStringSeparatedByChars(" ", true);
    cout << str.findNextStringSeparatedByChars(" ", true);
    cout << str.findNextStringSeparatedByChars(" ", true);
  } catch(Exception& err) {
    COUT(err.what());
    return 1;
  } catch(...) {
    COUT("Caught an unknown error");
  }

#if 0
  String str("Circuit Breaker: On 1)...pxe       : On 2)...Outlet 2  : Off 3)...Outlet 3  : On 4)...Outlet 4  : Off DS-RPC>");

  cout << str.findNextInstanceOf(": ", " ") << endl;
  cout << str.findNextInstanceOf(": ", " ") << endl;
  cout << str.findNextInstanceOf(": ", " ") << endl;
  cout << str.findNextInstanceOf(": ", " ") << endl;
  cout << str.findNextInstanceOf(": ", " ") << endl;

  std::string str2 = "this,is,a,test";
  str = str2;

  cout << str.findNextInstanceOf(",", false, ",", true) << endl;
  cout << str.findNextInstanceOf(",", true,  ",", true) << endl;
  cout << str.findNextInstanceOf(",", true,  ",", true) << endl;
  cout << str.findNextInstanceOf(",", true,  ",", false) << endl;

  str2 = "|2||3||r|";
  str = str2;

  cout << str.findNextInstanceOf("|", true, "|", true) << endl;
  cout << str.findNextInstanceOf("|", true, "|", true) << endl;
  cout << str.findNextInstanceOf("|", true, "|", true) << endl;
  cout << str.findNextInstanceOf("|", true, "|", true) << endl;

  str.resetToBeginning();
  cout << str.findNextInstanceOf("|", true, "||", true) << endl;
  cout << str.findNextInstanceOf("||", true, "||", true) << endl;
  cout << str.findNextInstanceOf("||", true, "|", true) << endl;

  str2 = "this\nis\na\ntest\n";
  str = str2;

  cout << str.findNextInstanceOf("", false, "\n", true, true) << endl;
  cout << str.findNextInstanceOf("", false, "\n", true, true) << endl;
  cout << str.findNextInstanceOf("", false, "\n", true, true) << endl;
  cout << str.findNextInstanceOf("", false, "\n", true, true) << endl;

  str2 = "2004-Jan-01 00:00 2453005.50000000";
  str = str2;
  cout << str.findNextInstanceOf("", false, " ", true, true) << endl;
#endif
  return 0;
}
