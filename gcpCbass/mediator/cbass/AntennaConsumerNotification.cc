#define __FILEPATH__ "mediator/specific/AntennaConsumerNotification.cc"

#include "gcp/util/common/AntennaDataFrameManager.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/specific/Directives.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include "gcp/mediator/specific/AntennaConsumerNotification.h"
#include "gcp/mediator/specific/Scanner.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::mediator;

/**.......................................................................
 * Constructor.
 */
AntennaConsumerNotification::
AntennaConsumerNotification(Scanner* parent,
			    std::string notifyChannel) :
  AntennaConsumer(parent), notifyChannel_(notifyChannel) {}

/**.......................................................................
 * Destructor.
 */
AntennaConsumerNotification::~AntennaConsumerNotification() {}

/**.......................................................................
 * Run method blocks in NotificationObserver::run()
 */
void AntennaConsumerNotification::run()
{
}
