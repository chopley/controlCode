#include "gcp/util/specific/Directives.h"

#if DIR_USE_ANT_CORBA

#include "carma/util/CorbaUtils.h"

#include "gcp/util/common/AntennaDataFrameCorba.h"

using namespace carma::util;

#endif

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"

#include "gcp/antenna/control/specific/FrameSender.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor.
 */
FrameSender::FrameSender() {};

/**.......................................................................
 * Destructor.
 */
FrameSender::~FrameSender() {};

#if 0
/**.......................................................................
 * Get a reference to the consumer on the requested event channel
 */
void FrameSender::attachConsumer(string eventChannelName)
{
  consumer_ = 
    carma::util::CorbaUtils::getProxyPushConsumer(eventChannelName);

  // Now obtain a NULL reference to a PushSupplier
  
  CosEventComm::PushSupplier_var nil_supplier = 
    CosEventComm::PushSupplier::_nil();

  // And register ourselves as a supplier, passing our nil reference
  // to indicate that we don't care to be notified of being
  // disconnected
  
  consumer_->connect_push_supplier(nil_supplier);
}

/**.......................................................................
 * CORBA method to push data onto an event channel.
 */
void FrameSender::push(CORBA::Any& any)
{
  consumer_->push(any);
}

/**.......................................................................
 * Return a reference to the consumer.
 */
CosEventChannelAdmin::ProxyPushConsumer_var FrameSender::getConsumer()
{
  return consumer_;
}

/**.......................................................................
 * Method to send an event via a CORBA event channel
 */
void FrameSender::
sendFrameViaEventChannel(gcp::util::DataFrameManager* fm)
{
  AntennaDataFrameCorba* frame = 
    dynamic_cast<AntennaDataFrameCorba*>(fm->frame());
  CORBA::Any any;
  
  any <<= *(frame->frame());
  
  // Finally, send it
  
  try{
    consumer_->push(any);
  } catch (...) {
    throw Error("FrameBuffer::sendNextFrame: Consumer is disconnected.\n");
  }
}
#endif

#if DIR_USE_ANT_CORBA
/**.......................................................................
 * Get a reference to the consumer on the requested notification channel
 */
void FrameSender::initNotification(string notifyChannelName)
{
  // Store the notification channel name

  notifyChannelName_ = notifyChannelName;

  DBPRINT(true, Debug::DEBUG3, "Initializing notification channel: " <<
	  notifyChannelName);

  // Create the Notification event

  event_ = CorbaUtils::createEventForm("SZA Antenna Data", "Ok");
  event_->filterable_data.length(1);
}

/**.......................................................................
 * Method to send an event via a CORBA notification channel
 */
void FrameSender::
sendFrameViaNotificationChannel(gcp::util::DataFrameManager* fm)
{
  AntennaDataFrameCorba* frame = 
    dynamic_cast<AntennaDataFrameCorba*>(fm->frame());

  DBPRINT(true, Debug::DEBUG3, "Sending an event");

  event_->filterable_data[0].name = CORBA::string_dup("SZA Antenna Data");
  event_->filterable_data[0].value <<= *(frame->frame());

  CorbaUtils::sendNotification(notifyChannelName_, event_);
}
#endif

/**.......................................................................
 * Send a data frame back to the outside world
 */
void FrameSender::sendFrame(gcp::util::DataFrameManager* fm)
{
#if DIR_USE_ANT_CORBA
  sendFrameViaNotificationChannel(fm);
#else
  // Do something else for non-corba, not yet written
#endif
}

