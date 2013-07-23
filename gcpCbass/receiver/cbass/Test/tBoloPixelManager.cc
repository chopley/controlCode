#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"

#include "gcp/receiver/specific/BoloPixelManager.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::receiver;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  {"host", "localhost", "s", "HW host"},
  {"port", "5207",      "i", "HW port"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  BoloPixelManager man(Program::getParameter("host"), Program::getiParameter("port"));

  std::vector<BoloPixel> pixels = man.getPixels();

  for(unsigned i=0; i < pixels.size(); i++)
    COUT(pixels[i]);

  return 0;
}
