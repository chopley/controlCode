#include "gcp/util/common/Connection.h"
#include "gcp/util/common/CoProc.h"
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/Port.h"
#include "gcp/util/common/String.h"
#include "gcp/util/common/TimeOut.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Connection::Connection() {}

/**.......................................................................
 * Destructor.
 */
Connection::~Connection() {}

bool Connection::isReachable(std::string host, unsigned timeOutInSeconds)
{
  std::ostringstream os;

  // On Mac OSX, ping takes -t to specify a timeout in seconds.  Under
  // linux, the option is -W

#if MAC_OSX == 0
  os << "ping -c 1 " << host;
#else
  os << "ping -c 1 " << host;
#endif

  CoProc proc(os.str());

  if(proc.stdOut()->readFd() < 0) {
    LogMessage(true, "Error in spawning ping process.");
    return false;
  }
  
  int fdStdout = proc.stdOut()->readFd();
  int fdStderr = proc.stdErr()->readFd();

  FdSet fdSet;
  fdSet.registerReadFd(fdStdout);
  fdSet.registerReadFd(fdStderr);

  Port stdoutPort(fdStdout);
  Port stderrPort(fdStderr);

  TimeOut timeOut;
  timeOut.setIntervalInSeconds(timeOutInSeconds);
  timeOut.activate(true);

  int nready = select(fdSet.size(), fdSet.readFdSet(), 0, 0, timeOut.tVal());

  if(nready < 0) {
    LogMessage(true, "Error in select");
    return false;
  } else if(nready == 0) {
    LogMessage(true, "Timed out in select");
    return false;
  } else if(fdSet.isSetInRead(fdStdout)) {
    os.str("");
    stdoutPort.concatenateString(os);
    String str(os.str());

    unsigned percLoss = 100;

    try {
      percLoss = str.findNextInstanceOf("received,", true, "%", "true").toInt();
    } catch(...) {
      percLoss = 100;
    }

    if(percLoss == 0) {
      return true;
    } else {
      return false;
    }

  } else if(fdSet.isSetInRead(fdStderr)) {
    os.str("");
    stderrPort.concatenateString(os);

    LogMessage(true, os.str());

    return false;
  } else {
    LogMessage(true, "Invalid fd was set");
    return false;
  }
}
