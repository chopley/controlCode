#define __FILEPATH__ "util/common/Test/tRegDescription.cc"

#include <iostream>

#include "gcp/program/common/Program.h"

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

  CoordRange range;
  RegDescription desc(true);
  desc.setTo("corr", "band0", "usbAvg", REG_PLAIN, REG_INT_PLAIN, range);

  for(desc.begin(); !desc.isEnd(); ++desc)
    cout << desc.currentSlot() << endl;

  RegDescription descNew(desc);

  for(descNew.begin(); !descNew.isEnd(); ++descNew)
    cout << descNew.currentSlot() << endl;

  std::vector<RegDescription> regs;

  cout << regs.size() << endl;
  regs.push_back(desc);
  cout << regs.size() << endl;

  return 0;
}
