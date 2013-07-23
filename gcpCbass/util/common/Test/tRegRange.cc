#define __FILEPATH__ "util/common/Test/tRegRange.cc"

#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/RegRange.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "start",       "0", "i", "Start byte index"},
  { "stop",        "0", "i", "Stop byte index"},
  { "archive",     "t", "b", "Archived only?"},
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel(Program::getiParameter("debuglevel"));

  RegRange range(Program::getiParameter("start"),
		 Program::getiParameter("stop"),
		 Program::getbParameter("archive"));

  for(range.reset(); !range.isEnd(); ++range)
    cout << range 
	 << " " << range.currentSlot() 
	 << " " << range.currentByteOffset() << endl;

  return 0;
}
