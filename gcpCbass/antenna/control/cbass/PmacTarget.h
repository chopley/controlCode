#ifndef PMACTARGET_H
#define PMACTARGET_H

/**
 * @file PmacTarget.h
 * 
 * Tagged: Thu Nov 13 16:53:46 UTC 2003
 * 
 * @author Erik Leitch
 */

#include "gcp/util/common/PmacMode.h"

#include "gcp/util/common/Axis.h"
#include "gcp/antenna/control/specific/PmacAxis.h"

#include "gcp/control/code/unix/libunix_src/common/scancache.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      /**
       * Tracker will access private members of this class.
       */
      class Tracker;
      
      class PmacTarget {
	
      public:
	
	/**
	 * Return a pointer to the appropriate axis
	 *
	 * @throws Exception
	 */
	gcp::antenna::control::PmacAxis* 
	  PmacAxis(gcp::util::Axis::Type axis);
	
	/**
	 * Return the current mode
	 */
	gcp::util::PmacMode::Mode getMode();
	
	/**
	 * Set the current mode
	 */
	void setMode(gcp::util::PmacMode::Mode mode);

	/**
	 * Return the current scan mode
	 */
	bool getScanMode();

	/**
	 * Set the current scan
	 */
	void setScanMode(bool isScanning);
	
	/**
	 * Pack the current mode for archival in the register
	 * database.
	 */
	void packMode(unsigned* u_elements);

	/**
	 * Pack the target encoder count for archival in the register
	 * database.
	 */
	void packCounts(signed* s_elements);
	
	/**
	 * Pack the target encoder rates for archival in the register
	 * database.
	 */
	void packRates(signed* s_elements);
	
      private:
	
	friend class Tracker;
	
	bool isScanning_;

	gcp::util::PmacMode::Mode mode_;   // The pmac mode 
	
	// The encoder positions and rates of the azimuth drive

	gcp::antenna::control::PmacAxis az_; 
	
	// The encoder positions and rates of the elevation drive

	gcp::antenna::control::PmacAxis el_; 
	
	// The encoder positions and rates of the PA drive

	gcp::antenna::control::PmacAxis pa_; 
	
      }; // End class PmacTarget
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
