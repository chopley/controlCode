#ifndef AZELTARGET_H
#define AZELTARGET_H

/**
 * @file AzElTarget.h
 * 
 * Tagged: Thu Nov 13 16:53:46 UTC 2003
 * 
 * @author Erik Leitch
 */

#include "gcp/util/common/DriveMode.h"

#include "gcp/util/common/Axis.h"
#include "gcp/antenna/control/specific/DriveAxis.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      /**
       * Tracker will access private members of this class.
       */
      class Tracker;
      class SzaTracker;
      class SptTracker;
      
      class AzElTarget {
	
      public:
	
	/**
	 * Return a pointer to the appropriate axis
	 *
	 * @throws Exception
	 */
	gcp::antenna::control::DriveAxis* 
	  DriveAxis(gcp::util::Axis::Type axis);
	
	/**
	 * Return the current mode
	 */
	gcp::util::DriveMode::Mode getMode();
	
	/**
	 * Set the current mode
	 */
	void setMode(gcp::util::DriveMode::Mode mode);
	
	/**
	 * Pack the current mode for archival in the register
	 * database.
	 */
	void packMode(unsigned* u_elements);

	/**
         * Pack the raw azimuth and elevation in milli-arcseconds
         */
	void packRawAngles(double* s_elements);
	
	/**
	 * Pack the target encoder rates for archival in the register
	 * database.
	 */
	virtual void packRawRates(signed* s_elements);
	
      private:
	
	friend class Tracker;
	friend class SzaTracker;
	friend class SptTracker;
	
	gcp::util::DriveMode::Mode mode_;   // The pmac mode 
	
	// The encoder positions and rates of the azimuth drive

	gcp::antenna::control::DriveAxis az_; 
	
	// The encoder positions and rates of the elevation drive

	gcp::antenna::control::DriveAxis el_; 
	
      }; // End class AzElTarget
      
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp

#endif // End #ifndef 
