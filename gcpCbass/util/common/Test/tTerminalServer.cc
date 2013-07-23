#define __FILEPATH__ "util/common/Test/tTerminalServer.cc"

#include <iostream>
#include <vector>
#include <iostream>

#include "gcp/util/common/Directives.h"

#if DIR_HAVE_CARMA
#include "carma/util/Program.h"
using namespace carma::util;
#else
#include "gcp/program/common/Program.h"
using namespace gcp::program;
#endif

#include "gcp/util/common/Coordinates.h"
#include "gcp/util/common/Vector.h"
#include "gcp/util/common/Debug.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/TerminalServer.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "ip",           " ",          "s", "IP address if serial=f"},
  { "port",         "23",         "i", "Port number if serial=f"},
  { "append",       "\n",       "s", "Append string"},
  { "baud",         "9600",       "i", "Baud Rate"},
  { "tty",          "/dev/ttyS0", "s", "Serial port"},
  { "serial",       "t",          "b", "Serial connection?"},
  { "listenToStdin","t",          "b", "Listen to stdin?"},
  { "log",          "t",          "b", "Log traffic on the port/tty?"},
  { "listenPort",   "7000",       "i", "Port on which we will listen for clients"},
  { "debuglevel",   "0",          "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel((Debug::Level)Program::getiParameter("debuglevel"));
  TerminalServer* server = 0;

  if(Program::getbParameter("serial")) 
    server = new TerminalServer(Program::getiParameter("baud"),
				Program::getParameter("tty"));
  else
    server = new TerminalServer(Program::getParameter("ip"),
				Program::getiParameter("port"));

  server->listenToStdin(Program::getbParameter("listenToStdin"));

  server->append(Program::getParameter("append"));

  server->listen(Program::getiParameter("listenPort"));

  server->log(Program::getbParameter("log"));

  server->run();

  if(server != 0)
    delete server;

  return 0;
}
