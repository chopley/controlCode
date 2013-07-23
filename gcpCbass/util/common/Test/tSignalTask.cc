#define __FILEPATH__ "util/common/Test/tSignalTask.cc"

#include <iostream>
#include <cmath>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/AbsTimer.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/SignalTask.h"
#include "gcp/util/common/Profiler.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

SIGNALTASK_HANDLER_FN(sigIntHandler) 
{
  cout << "Inside sigIntHandler" << endl;
  exit(0);
}

SIGNALTASK_HANDLER_FN(sigSegvHandler) 
{
  cout << "Inside sigSegvHandler" << endl;
  exit(0);
}

SIGNALTASK_HANDLER_FN(sigTimerHandler) 
{
  static Profiler prof;
  static TimeVal curr;

  curr.setToCurrentTime();

  prof.stop();
  CTOUT("Timer Handler was called: " << prof.diff().getTimeInSeconds()
	<< curr.getMjdId(1000000000));
  prof.start();
}

KeyTabEntry Program::keywords[] = {
  {END_OF_KEYWORDS}
};

int Program::main()
{
  sigset_t allSignals;
  sigfillset(&allSignals);
  sigprocmask(SIG_BLOCK, &allSignals, NULL);

  SignalTask signalTask(true);

  signalTask.sendInstallSignalMsg(SIGINT, sigIntHandler);
  signalTask.sendInstallSignalMsg(SIGSEGV, sigSegvHandler);
  signalTask.sendInstallSignalMsg(SIGBUS, sigSegvHandler);

  signalTask.sendInstallTimerMsg("timer", 
    				 SIGRTMIN+1, 0, 0, 0, 10000000, sigTimerHandler);
  signalTask.sendEnableTimerMsg("timer", true);
  
  while(true);
  return 0;
}
