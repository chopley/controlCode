#define __FILEPATH__ "util/common/Test/tRegCoordRange.cc"

#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/RegAxisRange.h"
#include "gcp/util/common/RegDescription.h"
#include "gcp/util/common/RegParser.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "reg",         "",  "s", "Register name to parse"},
  { "extend",      "t", "b", "Extend"},
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel(Program::getiParameter("debuglevel"));

  // Read the register specification
  
  RegParser parser;
  std::vector<RegDescription> regs = parser.inputRegs(Program::getParameter("reg"),
						      REG_INPUT_RANGE,
						      true,
						      Program::getbParameter("extend"));

  // Now test iterating over register ranges

  for(unsigned iReg=0; iReg < regs.size(); iReg++) {
    RegDescription& desc = regs[iReg];

    CoordRange range = desc.range();
    RegAxisRange regRange(desc, range);

    for(regRange.reset(); !regRange.isEnd(); ++regRange)
      cout << regRange << endl;
  }

  return 0;
}
