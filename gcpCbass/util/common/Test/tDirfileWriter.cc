#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/DirfileWriter.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "antenna",     "0",                        "i", "Antenna number"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  DirfileWriter writer;

  float fVal=1.0;

  writer.addFloatRegister("fVal", (unsigned char*)&fVal);

  writer.openArcfile("arcTest");
  writer.writeIntegration();
  writer.writeIntegration();

  return 0;
}
