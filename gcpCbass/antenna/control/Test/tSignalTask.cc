#define __FILEPATH__ "antenna/control/Test/tSignalTask.cc"

// Test using SignalTask with AbsTimers.

#include <iomanip>

#include "gcp/util/common/SignalTask.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/Thread.h"

#define NSEC_PER_SEC 1000000000
#define NSEC_PER_MSEC   1000000

static unsigned long nSecInterval;

using namespace gcp::util;
using namespace carma::util;

KeyTabEntry Program::keywords[] = {
  { "interval",     "100",      "i",   "timer interval (ms)"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

static SIGNALTASK_HANDLER_FN(sigIntHandler)
{
  exit(1);
}

/**.......................................................................
 * Signal handler called by SignalTask
 */
static SIGNALTASK_HANDLER_FN(sigHandler)
{
  static TimeVal lastTime, expected(0,500000000);
  static int counter=0;
  static bool first=true;
  TimeVal currentTime, relDelta, absDelta;
  long deltaNsec;

  currentTime.setToCurrentTime();

  // Absolute time difference
  
  absDelta = currentTime - expected;

  // Relative time difference
  
  relDelta  = currentTime - lastTime;
  deltaNsec = relDelta.getNanoSeconds()-nSecInterval;

  if(!first)
    cout << std::setw(11) << std::right << absDelta.getSeconds()
	 << std::setw(11) << std::right << absDelta.getNanoSeconds()
	 << std::setw(11) << std::right << relDelta.getSeconds()
	 << std::setw(11) << std::right << deltaNsec << endl;
  else
    first = false;

  //  cout << std::setw(11) << std::right << currentTime.getSeconds() 
  //       << std::setw(11) << std::right << currentTime.getNanoSeconds()
  //       << std::setw(11) << std::right << lastTime.getSeconds() 
  //       << std::setw(11) << std::right << lastTime.getNanoSeconds()
  //       << std::setw(11) << std::right << delta.getSeconds()
  //       << std::setw(11) << std::right << delta.getNanoSeconds() << endl;

  lastTime = currentTime;

  // Calculate the next expected time 

  expected.incrementSeconds((double)(nSecInterval)/NSEC_PER_SEC);
};

static THREAD_START(startSignalTask)
{
  SignalTask** signalTask = (SignalTask**) arg;

  *signalTask = new SignalTask();

  cout << "Inside startSignalTask" << endl;

  // Block in signalTask.

  (*signalTask)->SignalTask::run();

  cout << "Leaving startSignalTask" << endl;
}

int carma::util::Program::main(void)
{
  sigset_t allSignals;
  sigfillset(&allSignals);
  pthread_sigmask(SIG_BLOCK, &allSignals, NULL);

  // Get the timer interval, in nanoseconds.

  nSecInterval = Program::getiParameter("interval") * NSEC_PER_MSEC;

  pthread_t id;

  SignalTask* signalTask = 0;

  pthread_create(&id, NULL, &startSignalTask, (void*)(&signalTask));

  sleep(2);

  cout << "Calling sendInstallTimerMsg" << endl;
  signalTask->sendInstallTimerMsg("drive", 
				  36,             // signal no
				  0, 500000000,   // Delay in sec, nsec
				  0, nSecInterval,// Interval in sec, nsec
				  &sigHandler);

  cout << "Calling sendInstallSignalMsg" << endl;
  signalTask->sendInstallSignalMsg(SIGINT, &sigIntHandler);

  sleep(2);

  cout << "Calling sendEnableTimerMsg" << endl;
  signalTask->sendEnableTimerMsg("drive", true);


  while(true);

  cout << "Exiting?" << endl;
  return 0;
}
