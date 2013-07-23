#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/SshTunnel.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "gateway", "(none)",  "s", "The gateway computer, if any"},
  { "host",    "",        "s", "The host computer"},
  { "port",    "",        "i", "The port to forward"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  if(Program::isDefault("gateway")) {
    SshTunnel tunnel(Program::getParameter("host"), 
		     Program::getiParameter("port")); 

    sleep(1000);

  } else {
    SshTunnel tunnel(Program::getParameter("gateway"), 
		     Program::getParameter("host"), 
		     Program::getiParameter("port"));

    sleep(1000);

  }


  return 0;
}
