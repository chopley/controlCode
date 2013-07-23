#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/SshTunnel.h"
#include "gcp/util/common/CoProc.h"
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/TimeOut.h"
#include "gcp/util/common/TipperCommunicator.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "host",  "smmtipper", "s", "FTP host"},
  { "port",  "0",         "i", "Remote FTP port"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  TcpClient client(Program::getParameter("host"), Program::getiParameter("port"));
  FdSet fdSet;

  client.connectToServer(true);

  // Register this communicator's fd to be watched for readability
  
  fdSet.registerReadFd(client.getFd());

  bool stop   = false;
  int  nready = 0;

  // Our select loop will check the msgq file descriptor for
  // readability, except when a round of communciations with antenna
  // power strips has been initiated.  No further messages are
  // processed until the comms either finish, or timeout.

  std::ostringstream os;
  TimeOut timeOut;
  timeOut.setIntervalInSeconds(5);

  timeOut.activate(true);

  //  while(!stop) {

    nready=select(fdSet.size(), fdSet.readFdSet(), fdSet.writeFdSet(),
                  NULL, timeOut.tVal());

    // If this communicator's read fd was set, read the next line
    // from the strip
    
    //    if(fdSet.isSetInRead(comm.getReadFd()))
    //      comm.processClientMessage();

    if(fdSet.isSetInRead(client.getFd())) {
      COUT("Set in Read");
      client.concatenateString(os);
      timeOut.reset();
    } else if(nready == 0) {
      COUT("Registering timeout");
      stop = true;
    }
    //  }

  COUT("os = " << os.str());

  return 0;
}
