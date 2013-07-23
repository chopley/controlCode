#include <iostream>
#include <cerrno>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/FdSet.h"

#include "gcp/util/common/IersCommunicator.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "host", "maia.usno.navy.mil",  "s", "host"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int runStandAlone(std::string host);
int runInOwnThread(std::string host);
int runInExtThread(std::string host);

int Program::main()
{
  runStandAlone(Program::getParameter("host"));
  //  runInOwnThread(Program::getParameter("host"));
  return 0;
}

static IERS_HANDLER(iersHandler)
{
  COUT("Handler got called");
}

int runStandAlone(std::string host)
{
  IersCommunicator comm(".", "test.dat", 5);
  comm.addHandler(iersHandler);
  comm.spawn();
  comm.blockForever();
}

int runInOwnThread(std::string host)
{
  bool stop   = false;
  int  nready = 0;

  FdSet fdSet;
  fdSet.clear();

  IersCommunicator comm(host, ".");

#if 1
  TimeOut timeOut;
  timeOut.setIntervalInSeconds(1);
  timeOut.activate(true);

  comm.spawn();

  nready=select(fdSet.size(), fdSet.readFdSet(), NULL, NULL, timeOut.tVal());

  if(nready == 0) {
    comm.getIersBulletin();
  }

  sleep(10);
#else
  COUT("Ephem file exists: "              << comm.ephemExists());
  COUT("Ephem file is out of date: "      << comm.ephemFileIsOutOfDate());
  COUT("Ephem file is about to run out: " << comm.ephemIsAboutToRunOut());
#endif
}

int runInExtThread(std::string host)
{
#if 0
  bool stop   = false;
  int  nready = 0;

  FdSet fdSet;
  fdSet.clear();

  IersCommunicator comm(&fdSet, host, ".");
  
  // Initiate a comms sequence to this antenna's power strip

  comm.initiateGetIersBulletinCommSequence();

  // Register this communicator's fd to be watched for readability
  
  fdSet.registerReadFd(comm.getFtpCommFd());

  // Our select loop will check the msgq file descriptor for
  // readability, except when a round of communciations with antenna
  // power strips has been initiated.  No further messages are
  // processed until the comms either finish, or timeout.

  std::ostringstream os;


  while(!stop) {

    nready=select(fdSet.size(), fdSet.readFdSet(), NULL, NULL, NULL);

    // If this communicator's read fd was set, read the next line
    // from the strip
    
    if(fdSet.isSetInRead(comm.getFtpCommFd())) {
      comm.processClientMessage();
    }

    if(comm.getFtpDataFd() > 0 && fdSet.isSetInRead(comm.getFtpDataFd())) {
      comm.processIersBulletin();
    }

    if(nready < 0) {
      COUT(strerror(errno));
      stop = true;
    }
  }

#endif
  return 0;
}
