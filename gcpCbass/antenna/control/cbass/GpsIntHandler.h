// $Id: GpsIntHandler.h,v 1.1.1.1 2009/07/06 23:57:04 eml Exp $

#ifndef GCP_ANTENNA_CONTROL_GPSINTHANDLER_H
#define GCP_ANTENNA_CONTROL_GPSINTHANDLER_H

/**
 * @file GpsIntHandler.h
 * 
 * Tagged: Fri Jul 29 18:20:35 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:04 $
 * 
 * 
 * @author BICEP Software Development
 */
#define GPS_CALLBACK_FN(fn) void (fn)(void* args)

#include "gcp/util/common/Runnable.h"
#include "gcp/util/common/TimeVal.h"

#include <string>
#include <vector>

namespace gcp {
  namespace antenna {
    namespace control {

      class GpsIntHandler : public gcp::util::Runnable {
      public:

	/**
	 * Constructor.
	 */
	GpsIntHandler(bool spawn, bool simulateTimer, 
		 bool simulateGpsCard, unsigned short priority);

	GpsIntHandler(bool spawn, bool simulateTimer, 
		 bool simulateGpsCard=false);

	/**
	 * Destructor.
	 */
	virtual ~GpsIntHandler();

	/**
	 * Register a callback function
	 */
	void registerCallback(GPS_CALLBACK_FN(*fn), void* arg);

	/**
	 * Run this object
	 */
	void run();
	void runReal();
	void runSim();
	void runTest();

	// Stop this object from running

	void stop();

	// Read the current date from the gps board

	void getDate(gcp::util::TimeVal& lastTick);

	gcp::util::TimeVal& lastTick() {
	  return lastTick_;
	}

	int fd() {
	  return fd_;
	}
	
      private:
	
	struct HandlerInfo {
	  GPS_CALLBACK_FN(*fn_);
	  void* arg_;
	  
	  HandlerInfo(GPS_CALLBACK_FN(*fn), void* arg) :
	    fn_(fn), arg_(arg) {}
	};

	/**
	 * The name by which the device is accessible from user-space
	 */
	static const std::string devName_;

	/**
	 * A vector of handlers to be called when the GPS card
	 * generates an interrupt.
	 */
	std::vector<HandlerInfo> handlers_;

	// If true, stop running this object

	volatile bool stop_;

	// The file descriptor associated with the gps interrupt
	// driver

	int fd_;

	bool simulateTimer_;
	bool simulateGpsCard_;

	/**
	 * If spawned in its own thread, this will be the startup
	 * function for this object
	 */
	static RUN_FN(runFn);
	
      public:

	// Open the device

	void open();

      private:
	
	// Close the device

	void close();

	// Call handlers when the device becomes readable

	void callHandlers();

	gcp::util::TimeVal lastTick_;

      }; // End class GpsIntHandler

    } // End namespace control
  } // End namespace antenna
} // End namespace gcp



#endif // End #ifndef GCP_ANTENNA_CONTROL_GPSINTHANDLER_H
