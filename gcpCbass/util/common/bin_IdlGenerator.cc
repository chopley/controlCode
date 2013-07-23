#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/IdlGenerator.h"
#include "gcp/util/common/IoLock.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  {    "dir",      "",     "s", "Input/output directory"},
  {   "file",      "",     "s", "Input file"},
  { "prefix",      "",     "s", "Prefix of the output cc/dlm file"},
  { "suffix", "idlcc",     "s", "Suffix of the output cc file"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  IdlGenerator idlGen(Program::getParameter("file"), Program::getParameter("dir"));

  if(Program::hasValue("prefix"))
    idlGen.setOutputPrefix(Program::getParameter("prefix"));

  if(Program::hasValue("suffix"))
    idlGen.setOutputCcSuffix(Program::getParameter("suffix"));

  idlGen.outputCcFile();
  idlGen.outputDlmFile();

  return 0;
}
