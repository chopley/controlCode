#define __FILEPATH__ "util/common/DelayLocation.cc"

#include "gcp/util/common/DelayLocation.h"
#include "gcp/util/common/Location.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
DelayLocation::DelayLocation() {}

/**.......................................................................
 * Destructor.
 */
DelayLocation::~DelayLocation() {}

/**
 * A function called whenever an outside caller changes a location
 */
void DelayLocation::locationChanged(Location* loc) {};
