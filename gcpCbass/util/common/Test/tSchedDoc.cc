#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/SchedDoc.h"
#include "gcp/util/common/Exception.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "schDir",      "~/gcp/control/sch",  "s", "the directory to search for schedule files"},
  { "outDir",      ".",                  "s", "the output directory"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  SchedDoc schedDoc;

  schedDoc.documentSchedules(Program::getParameter("schDir"), Program::getParameter("outDir"));

  return 0;
}
