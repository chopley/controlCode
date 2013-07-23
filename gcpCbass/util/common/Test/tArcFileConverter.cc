#include <iostream>
#include <iomanip>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/ArcFileConverter.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;
using namespace gcp::program;


KeyTabEntry Program::keywords[] = {
  { "datfile",    "", "s", "Data file to convert"},
  { "dirfile",    "", "s", "Dirfile to create"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

void Program::initializeUsage() {}

int Program::main()
{
  ArcFileConverter converter(Program::getParameter("datfile"),
			     Program::getParameter("dirfile"));

  converter.convert();

  return 0;
}

