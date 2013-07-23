#include "gcp/util/common/CondVar.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"

#include <errno.h>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
CondVar::CondVar() 
{
  errno = 0;

  if(pthread_cond_init(&cond_, 0) != 0) {
    ThrowSysError("initializing cond var");
  }
}

void CondVar::wait()
{
  errno = 0;

  // pthread_cond_wait() requires a locked mutex to be passed in with
  // the condition variable.  The system will release the lock on the
  // caller's behalf when the wait begins

  DBPRINT(true, Debug::DEBUG7, "About to lock in wait " << pthread_self());
  mutex_.lock();
  if(pthread_cond_wait(&cond_, mutex_.getPthreadVarPtr()) != 0) {
    ThrowSysError("Broadcasting");
  }
  DBPRINT(true, Debug::DEBUG7, "About to lock in wait... done " << pthread_self());
}

void CondVar::broadcast()
{
  errno = 0;

  // Lock the mutex before calling broadcast.  This ensures that the
  // broadcast is not called before a thread is waiting for it

  DBPRINT(true, Debug::DEBUG7, "About to lock in broadcast... done " << pthread_self());
  mutex_.lock();
  if(pthread_cond_broadcast(&cond_) != 0) {
    mutex_.unlock();
    ThrowSysError("Broadcasting");
  }
  DBPRINT(true, Debug::DEBUG7, "About to broadcast... done " << pthread_self());
  mutex_.unlock();
  DBPRINT(true, Debug::DEBUG7, "Done unlocking " << pthread_self());


}

/**.......................................................................
 * Destructor.
 */
CondVar::~CondVar() 
{
  errno = 0;

  // Ensure that no-one is holding a lock on our mutex before we exit

  //  mutex_.lock();

  if(pthread_cond_destroy(&cond_) != 0) {
    ReportSysError("Destroying cond var");
  }
}
