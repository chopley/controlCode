#include <iostream>
#include <string>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/FunctionName.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "index", "0",  "i", "Index"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  FunctionName func(__PRETTY_FUNCTION__);
  COUT(func.noArgs());
  COUT(func.prettyFunction());
  return 0;
}
