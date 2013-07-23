#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/antenna/control/specific/Scan.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;
using namespace gcp::antenna::control;

KeyTabEntry Program::keywords[] = {
  { "antenna",     "0",                        "i", "Antenna number"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  ScanCacheOffset off1, off2;

  off1.irep = 1;
  off1.az   = 1.234;
  off1.el   = 4.321;

  off2.irep = 2;
  off2.az   = 0.0;
  off2.el   = 0.0;

  off2 = off1;

  COUT(off2.irep);
  COUT(off2.az);
  COUT(off2.el);

  Scan scan1, scan2;

  scan1.current_.irep = 12;
  scan1.current_.az = 1.2345;
  scan1.current_.el = 2.3456;

  scan2 = scan1;

  COUT(scan2.rep());

  return 0;
}
