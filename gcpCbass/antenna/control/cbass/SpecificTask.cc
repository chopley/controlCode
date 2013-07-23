#define __FILEPATH__ "antenna/control/specific/SzaTask.cc"

#include "gcp/util/common/Debug.h"
#include "gcp/antenna/control/specific/SpecificTask.h"

using namespace gcp::antenna::control;
using namespace gcp::util;

/**
 * Constructor just initializes the shared object pointer to
 * NULL.
 */
SpecificTask::SpecificTask()
{
  share_     = 0;
};

/**
 * Destructor.
 */
SpecificTask::~SpecificTask() {};

/**
 * Public method to get a pointer to our shared object.
 */
SpecificShare* SpecificTask::getShare() {
  return share_;
}
