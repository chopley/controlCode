#include "SpecificTransactionStatus.h"

using namespace std;

using namespace gcp::control;

/**.......................................................................
 * Constructor.
 */
SpecificTransactionStatus::SpecificTransactionStatus() {}

/**.......................................................................
 * Destructor.
 */
SpecificTransactionStatus::~SpecificTransactionStatus() {}

unsigned int& SpecificTransactionStatus::done(unsigned transId)
{
  switch(transId) {
  default:
    return TransactionStatus::done(transId);
    break;
  }
}

const unsigned int SpecificTransactionStatus::seq(unsigned transId)
{
  switch(transId) {
  default:
    return TransactionStatus::seq(transId);
    break;
  }
}

unsigned int SpecificTransactionStatus::nextSeq(unsigned transId)
{
  switch(transId) {
  default:
    return TransactionStatus::nextSeq(transId);
    break;
  }
}
