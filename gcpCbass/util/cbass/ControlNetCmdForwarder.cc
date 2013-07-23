#define __FILEPATH__ "util/common/ControlNetCmdForwarder.cc"

#include "gcp/util/specific/ControlNetCmdForwarder.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
ControlNetCmdForwarder::ControlNetCmdForwarder() {}

/**.......................................................................
 * Destructor.
 */
ControlNetCmdForwarder::~ControlNetCmdForwarder() {}

/**.......................................................................
 * Forward a network command for the delay subsystem.
 */
void ControlNetCmdForwarder::forwardNetCmd(gcp::util::NetCmd* netCmd) {};
