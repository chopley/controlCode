#ifndef FRAMESENDER_H
#define FRAMESENDER_H

/**
 * @file FrameSender.h
 * 
 * Started: Fri Feb 27 14:40:56 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/util/specific/Directives.h"

#if DIR_USE_ANT_CORBA
#include <OB/CORBA.h>
#include <OB/CosEventChannelAdmin.h>
#include <OB/CosNotification.h>
#endif

#include "gcp/util/common/DataFrameManager.h"

namespace gcp {
  namespace antenna {
    namespace control {

      class FrameSender {
      public:

	/**
	 * Constructor.
	 */
	FrameSender();

	/**
	 * Destructor.
	 */
	virtual ~FrameSender();
	
#if DIR_USE_ANT_CORBA

	//------------------------------------------------------------
	// CORBA event service
	//------------------------------------------------------------

	/**
	 * Get a reference to the consumer on the requested event channel
	 */
	void FrameSender::attachConsumer(std::string eventChannelName);

	/**
	 * CORBA method to push data onto an event channel.
	 */
	void FrameSender::push(CORBA::Any& any);

	/**
	 * Return a reference to the consumer.
	 */
	CosEventChannelAdmin::ProxyPushConsumer_var getConsumer();

	/**
	 * Send a frame via the notification service
	 */
	void sendFrameViaEventChannel(gcp::util::DataFrameManager* frame);

	//------------------------------------------------------------
	// CORBA notification service
	//------------------------------------------------------------

	/**
	 * Set up the notification channel
	 */
	void initNotification(std::string notifyChannelName);

	/**
	 * Send a frame via the notification service
	 */
	void sendFrameViaNotificationChannel(gcp::util::DataFrameManager* frame);
#endif
	/**
	 * Send a frame
	 */
	void sendFrame(gcp::util::DataFrameManager* frame);

      private:

#if DIR_USE_ANT_CORBA

	//------------------------------------------------------------
	// CORBA event service
	//------------------------------------------------------------

	/**
	 * A pointer to the consumer we will use to send data back to
	 * the outside world.  This is initialized by this task, and
	 * used by the Monitor task, whose resources are managed by
	 * the AntennaMonitor object allocated by the AntennaMonitor
	 * thread.
	 */ 
	CosEventChannelAdmin::ProxyPushConsumer_var consumer_;

	//------------------------------------------------------------
	// CORBA notification service
	//------------------------------------------------------------

	/**
	 * The notification channel name
	 */
	std::string notifyChannelName_;

	/**
	 * A reference to the notification event
	 */
	CosNotification::StructuredEvent_var event_;

#endif
      }; // End class FrameSender

    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 


