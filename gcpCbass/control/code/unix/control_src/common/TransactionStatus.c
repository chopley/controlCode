#define __FILEPATH__ "control/code/unix/control_src/common/TransactionStatus.c"

#include "gcp/util/common/Debug.h"
#include "TransactionStatus.h"

using namespace std;

using namespace gcp::control;

/**.......................................................................
 * Constructor.
 */
TransactionStatus::TransactionStatus() {}

/**.......................................................................
 * Destructor.
 */
TransactionStatus::~TransactionStatus() {}

/**.......................................................................
 * Initialize to 0
 */
TransactionStatus::Transaction::Transaction() {
  seq_  = 0;
  done_ = 0x0;
}

/**.......................................................................
 * Return the completion state of the corresponding transaction
 */
unsigned int& TransactionStatus::done(unsigned transId)
{
  switch(transId) {
  case TRANS_FRAME:
    return frame_.done_;
    break;
  case TRANS_GRAB:
    return grab_.done_;
    break;
  case TRANS_PMAC:
    return pmac_.done_;
    break;
  case TRANS_BENCH:
    return bench_.done_;
    break;
  case TRANS_MARK:
    return mark_.done_;
    break;
  case TRANS_SCAN:
    return scan_.done_;
    break;
  case TRANS_SETREG:
    return setreg_.done_;
    break;
  case TRANS_TVOFF:
    return tvoff_.done_;
    break;
  case TRANS_SCRIPT:
    return script_.done_;
    break;
  default:
    break;
  }
}

/**.......................................................................
 * Return the completion state of the corresponding transaction
 */
const unsigned int TransactionStatus::seq(unsigned transId)
{
  switch(transId) {
  case TRANS_FRAME:
    return frame_.seq_;
    break;
  case TRANS_GRAB:
    return grab_.seq_;
    break;
  case TRANS_PMAC:
    return pmac_.seq_;
    break;
  case TRANS_BENCH:
    return bench_.seq_;
    break;
  case TRANS_MARK:
    return mark_.seq_;
    break;
  case TRANS_SCAN:
    return scan_.seq_;
    break;
  case TRANS_SETREG:
    return setreg_.seq_;
    break;
  case TRANS_TVOFF:
    return tvoff_.seq_;
    break;
  case TRANS_SCRIPT:
    return script_.seq_;
    break;
  default:
    break;
  }
}

/**.......................................................................
 * Return the completion state of the corresponding transaction
 */
unsigned int TransactionStatus::nextSeq(unsigned int transId)
{
  switch(transId) {
  case TRANS_FRAME:
    frame_.done_ = 0x0;
    return ++frame_.seq_;
    break;
  case TRANS_GRAB:
    grab_.done_ = 0x0;
    return ++grab_.seq_;
    break;
  case TRANS_PMAC:
    pmac_.done_ = 0x0;
    return ++pmac_.seq_;
    break;
  case TRANS_BENCH:
    bench_.done_ = 0x0;
    return ++bench_.seq_;
    break;
  case TRANS_MARK:
    mark_.done_ = 0x0;
    return ++mark_.seq_;
    break;
  case TRANS_SCAN:
    scan_.done_ = 0x0;
    return ++scan_.seq_;
    break;
  case TRANS_SETREG:
    setreg_.done_ = 0x0;
    return ++setreg_.seq_;
    break;
  case TRANS_TVOFF:
    tvoff_.done_ = 0x0;
    return ++tvoff_.seq_;
    break;
  case TRANS_SCRIPT:
    script_.done_ = 0x0;
    return ++script_.seq_;
    break;
  default:
    break;
  }
}
