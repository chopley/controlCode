#include "gcp/util/common/Exception.h"
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/antenna/control/specific/GpsIntHandler.h"

#include <unistd.h>
#include <fcntl.h>

using namespace std;

using namespace gcp::util;
using namespace gcp::antenna::control;

const std::string GpsIntHandler::devName_("/dev/tfp0");

/**.......................................................................
 * Constructor.
 */
GpsIntHandler::
GpsIntHandler(bool spawnThread, bool simulateTimer, bool simulateGpsCard, unsigned short priority) :
  gcp::util::Runnable(spawnThread, runFn, priority)
{
  fd_ = -1;
  stop_ = false;

  simulateTimer_   = simulateTimer;
  simulateGpsCard_ = simulateGpsCard;

  if(spawnThread)
    spawn(this);
}

GpsIntHandler::
GpsIntHandler(bool spawnThread, bool simulateTimer, bool simulateGpsCard) :
  gcp::util::Runnable(spawnThread, runFn)
{
  fd_ = -1;
  stop_ = false;

  simulateTimer_   = simulateTimer;
  simulateGpsCard_ = simulateGpsCard;

  if(spawnThread)
    spawn(this);
}

/**.......................................................................
 * Destructor.
 */
GpsIntHandler::~GpsIntHandler() 
{
  close();
}

/**.......................................................................
 * Open the device 
 */
void GpsIntHandler::open()
{
  // Make sure the file is closed before we try to reopen it.
  
  close();

  // Now attempt to open the device

  fd_ = ::open(devName_.c_str(), O_RDONLY);
  
  if(fd_ < 0)
      ThrowSysError("open()");
}

/**.......................................................................
 * Close the device
 */
void GpsIntHandler::close()
{
  if(fd_ > 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

/**.......................................................................
 * Register a callback function
 */
void GpsIntHandler::registerCallback(GPS_CALLBACK_FN(*fn), void* arg)
{
  handlers_.push_back(HandlerInfo(fn, arg));
}

/**.......................................................................
 * Call handlers
 */
void GpsIntHandler::callHandlers()
{
  for(unsigned ihand=0; ihand < handlers_.size(); ihand++)
    handlers_[ihand].fn_(handlers_[ihand].arg_);
}

/**.......................................................................
 * The static startup function used when this object is run in its own
 * thread
 */
RUN_FN(GpsIntHandler::runFn)
{
  GpsIntHandler* gps = (GpsIntHandler*) arg;
  gps->run();
}

/**.......................................................................
 * Stop this object from running
 */ 
void GpsIntHandler::stop()
{
  stop_ = true;
}

void GpsIntHandler::run() 
{
  if(simulateTimer_)
    runSim();
  else
    runReal();
}

/**.......................................................................
 * Sit in select(), watching for the file descriptor to become
 * readable
 */ 
void GpsIntHandler::runReal()
{
  FdSet fdSet;
  unsigned nready=0;
  TimeVal halfSec(0, 500000, 0);
  struct timeval* timeOut = 0;
  static unsigned counter = 0;

#if 1
  static TimeVal debugLast, debugCurr, diff;
#endif
  // Make sure the device is open

  open();

  fdSet.registerReadFd(fd_);

  try {

    while(!stop_) {
      COUT("ENtering select");
      nready=select(fdSet.size(), fdSet.readFdSet(), NULL, NULL,  
		    timeOut);
      
      COUT("Dropped out of select");
      // Received an interrupt?
      
      if(nready > 0) {

	
	//	gps_.getDate(lastTick_);

#if 1
	{
	  static unsigned counter;
	  unsigned nInt=1;

	  if(++counter % nInt == 0) {
	    debugCurr.setToCurrentTime();
	    diff = debugCurr - debugLast;
	    debugLast = debugCurr;
	    CTOUT("GpsIntHandler " << nInt << " interrupts: diff = " 
		  << (double)(diff.getTimeInMicroSeconds())/nInt);
	  }
	}
#endif

#if 0
	// Increment the timestamp by two seconds.  This will be the
	// time used in calculating the next position commanded of
	// the PMAC

	lastTick_.incrementSeconds(2);
	halfSec.reset();
	timeOut = halfSec.timeVal();
#endif
      }

#if 0
      // Timed out?
      
      else {
	callHandlers();
	timeOut = 0;
      }

#endif

    }
  } catch(Exception& err) {
    close();
    throw err;
  } catch(...) {
    close();
    ThrowError("Caught unknown error");
  }
  
  close();

  // If we exited from select() abnormally, report it

  if(!stop_)
    ThrowSysError("select()");
}

/**.......................................................................
 * Sit in select(), watching for the file descriptor to become
 * readable
 */ 
void GpsIntHandler::runTest()
{
  FdSet fdSet;
  unsigned nready=0;
  TimeVal timeOut(1, 0);

  // Make sure the device is open

  open();

  fdSet.registerReadFd(fd_);

  while(!stop_) {
    nready=select(0, NULL, NULL, NULL,  
		  timeOut.timeVal());
    
    callHandlers();
    timeOut.reset();
  }

  close();

  // If we exited from select() abnormally, report it

  if(!stop_)
    ThrowSysError("select()");
}

/**.......................................................................
 * Sit in select(), watching for the file descriptor to become
 * readable
 */ 
void GpsIntHandler::runSim()
{
  while(true) {
    sleep(1);
    //    gps_.getDate(lastTick_);
    lastTick_.setToCurrentTime();
    callHandlers();
  }
}

void GpsIntHandler::getDate(gcp::util::TimeVal& lastTick)
{
  //  gps_.getDate(lastTick);
  lastTick.setToCurrentTime();
}
