#include "gcp/util/common/CoProc.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/ModemPager.h"

#include<iostream>
#include <errno.h>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
ModemPager::ModemPager() :
  SpawnableTask<ModemPagerMsg>(true) 
{
  // Set the alive timeout to default values

  aliveTimeOut_.setSeconds(10 * 60);

  stdoutDispatcher_   = 0;
  stdoutContext_      = 0;
  stdoutStream_       = 0;

  stderrDispatcher_   = 0;
  stderrContext_      = 0;
  stderrStream_       = 0;
}

/**.......................................................................
 * Constructor.
 */
ModemPager::ModemPager(FILE* stdoutStream, LOG_DISPATCHER(stdoutDispatcher), 
		       void* stdoutContext,
		       FILE* stderrStream, LOG_DISPATCHER(stderrDispatcher), 
		       void* stderrContext) :
  SpawnableTask<ModemPagerMsg>(true) 
{
  // Set the alive timeout to default values

  aliveTimeOut_.setSeconds(10 * 60);

  stdoutDispatcher_   = stdoutDispatcher;
  stdoutContext_      = stdoutContext;
  stdoutStream_       = stdoutStream;

  stderrDispatcher_   = stderrDispatcher;
  stderrContext_      = stderrContext;
  stderrStream_       = stderrStream;
}

/**.......................................................................
 * Destructor.
 */
ModemPager::~ModemPager() {}

/**.......................................................................
 * Main Task event loop: when this is called, the task blocks forever
 * in select(), or until a stop message is received.
 */
void ModemPager::serviceMsgQ(void) 
{
  bool stop=false;
  int nready; // number of file descriptors ready for reading

  if(msgq_.fd() < 0) {
    ThrowError("Received NULL file descriptor");
  }
  
  // Loop, checking the message queue file descriptor for readability
  
  aliveTimeOut_.reset();

  // If a dispatch function was registered, divert lprintf to use it
  // now

  if(stdoutDispatcher_) { 
    divert_lprintf(stdoutStream_, stdoutDispatcher_, stdoutContext_, NULL, NULL);
  }

  if(stderrDispatcher_) { 
    divert_lprintf(stderrStream_, stderrDispatcher_, stderrContext_, NULL, NULL);
  }

  while(!stop) {

    nready = select(fdSet_.size(), fdSet_.readFdSet(), 
		    NULL, NULL, aliveTimeOut_.timeVal());

    switch (nready) {
    case 1:
      processTaskMsg(&stop);
      break;

      // If no file descriptors were ready, it is time to notify the
      // pager that we are alive

    case 0:
      executeAlive();
      aliveTimeOut_.reset();
      break;
    default:
      stop = true;
      ThrowSysError("select()");
      break;
    }
  }


}

/**.......................................................................
 * Main Task event loop: when this is called, the task blocks forever
 * in select(), or until a stop message is received.
 */
void ModemPager::processMsg(ModemPagerMsg* msg)
{
  switch (msg->type) {
  case ModemPagerMsg::ALIVE:
    executeAlive();
    break;
  case ModemPagerMsg::ACK:
    executeAcknowledge();
    break;
  case ModemPagerMsg::ERROR:
    executeError();
    break;
  case ModemPagerMsg::ENABLE:
    executeEnable(msg->body.enable);
    break;
  case ModemPagerMsg::RESET:
    executeReset();
    break;
  case ModemPagerMsg::START:
    executeStart();
    break;
  case ModemPagerMsg::STATUS:
    executeStatus();
    break;
  case ModemPagerMsg::STOP:
    executeStop();
    break;

  default:
    ThrowError("Unrecognized message type: " << msg->type);
    break;
  }
}

void ModemPager::executeAcknowledge()
{
  //  system("spt_acknowledge");
  spawnAndCapture("ssh sptnet spt_acknowledge");
}

void ModemPager::executeReset()
{
  //  system("spt_alive reset");
  spawnAndCapture("ssh sptnet spt_reset");
}

void ModemPager::executeStart()
{
  //  system("spt_alive start");
  spawnAndCapture("ssh sptnet spt_alive START");
}

void ModemPager::executeStatus()
{
  //  system("spt_alive status");
  spawnAndCapture("ssh sptnet spt_status", true);
}

void ModemPager::executeStop()
{
  //  system("spt_alive stop");
  spawnAndCapture("ssh sptnet spt_alive STOP");
}

void ModemPager::executeAlive()
{
  //  system("spt_alive");
  spawnAndCapture("ssh sptnet spt_alive");
}

void ModemPager::executeError()
{
  //  system("spt_error");
  ostringstream os;

  msgLock_.lock();
  os << "ssh sptnet spt_error \"" << errorMessage_ << "\"";
  msgLock_.unlock();

  spawnAndCapture(os.str());
}

void ModemPager::executeEnable(bool enable)
{
  if(enable) {
    // system("spt_enable");

    // Here we have to do -b (batch) or the scripts will prompt for
    // input

    spawnAndCapture("ssh sptnet spt_enable -b");
  } else {
    //    system("spt_disable");
    spawnAndCapture("ssh sptnet spt_disable -b");
  }
}

void ModemPager::sendAlive()
{
  ModemPagerMsg msg;
  msg.type = ModemPagerMsg::ALIVE;
  sendTaskMsg(&msg);
}

void ModemPager::reset()
{
  ModemPagerMsg msg;
  msg.type = ModemPagerMsg::RESET;
  sendTaskMsg(&msg);
}

void ModemPager::start()
{
  ModemPagerMsg msg;
  msg.type = ModemPagerMsg::START;
  sendTaskMsg(&msg);
}

void ModemPager::stop()
{
  ModemPagerMsg msg;
  msg.type = ModemPagerMsg::STOP;
  sendTaskMsg(&msg);
}

void ModemPager::requestStatus()
{
  ModemPagerMsg msg;
  msg.type = ModemPagerMsg::STATUS;
  sendTaskMsg(&msg);
}

void ModemPager::acknowledge()
{
  ModemPagerMsg msg;
  msg.type = ModemPagerMsg::ACK;
  sendTaskMsg(&msg);
}

void ModemPager::activate(std::string message)
{
  msgLock_.lock();
  errorMessage_ = message;
  msgLock_.unlock();

  ModemPagerMsg msg;
  msg.type = ModemPagerMsg::ERROR;
  sendTaskMsg(&msg);
}

void ModemPager::enable(bool enable)
{
  ModemPagerMsg msg;
  msg.type = ModemPagerMsg::ENABLE;
  msg.body.enable = enable;
  sendTaskMsg(&msg);
}

void ModemPager::spawnAndCapture(std::string script, bool log)
{
  CoProc proc(script);

  int stdIn  = proc.stdIn()->writeFd();
  int stdOut = proc.stdOut()->readFd();
  int stdErr = proc.stdErr()->readFd();

  close(stdIn);

  unsigned char c;
  std::ostringstream os;
  ModemPager::State state = STATE_NORM;

  while(read(stdOut, &c, 1) != 0) {

    switch (state) {

      // If the last read character was a normal one, check the
      // current one

    case STATE_NORM:
      if(c == ESC) {
	state = STATE_ESC;
      } else {
	os << c;
      }
      break;

      // If we are reading a color escape sequence, don't stop until
      // the terminating character is encountered

    case STATE_ESC:
      if(c == TERM) {
	state = STATE_NORM;
      } else {
      }
    default:
      break;
    }
  }

  if(log) { 
    ReportMessage(os.str());
  }

  os.str("");
  state = STATE_NORM;
  while(read(stdErr, &c, 1) != 0) {

    switch (state) {

      // If the last read character was a normal one, check the
      // current one

    case STATE_NORM:
      if(c == ESC) {
	state = STATE_ESC;
      } else {
	os << c;
      }
      break;

      // If we are reading a color escape sequence, don't stop until
      // the terminating character is encountered

    case STATE_ESC:
      if(c == TERM) {
	state = STATE_NORM;
      } else {
      }
    default:
      break;
    }
  }

  if(log) {
    ReportSimpleError(os.str());
  }
}
