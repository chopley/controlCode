#include "script.h"
#include "genericcontrol.h"
#include "genericscheduler.h"

#include "SpecificTransactionStatus.h"
#include "gcp/util/specific/SpecificNetMsg.h"
#include "gcp/util/common/Debug.h"

using namespace gcp::control;
using namespace gcp::util;

/**.......................................................................
 * Return an experiment-specific instantiation of a transaction manager
 */
TransactionStatus* new_SpecificTransactions()
{
  return new SpecificTransactionStatus();
}

/**.......................................................................
 * Process a message specifically for this experiment
 */
void processSpecificSchedulerMessage(unsigned id, gcp::util::NewNetMsg* msg, 
				     TransactionStatus* trans,
				     unsigned antenna)
{
  SpecificNetMsg* netmsg = (SpecificNetMsg*)msg;

  switch(id) {
  default:
    DBPRINT(true, Debug::DEBUGANY, "Scheduler: Unexpected network message.");
    break;
  };
}


