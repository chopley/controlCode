#define __FILEPATH__ "util/common/Test/tRegExpParser.cc"

#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/RegExpParser.h"
#include "gcp/util/common/RegAxisRange.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "reg",         "",  "s", "Register name to match"},
  { "regexp",      "",  "s", "Regexcp string"},
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel(Program::getiParameter("debuglevel"));
  
  RegExpParser parser(Program::getParameter("regexp"));

  cout << parser.matches(Program::getParameter("reg")) << endl;

  return 0;
}
