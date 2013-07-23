#ifndef POSITION_H
#define POSITION_H

/**
 * @file Position.h
 * 
 * Tagged: Thu Nov 13 16:53:49 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Axis.h"
#include "gcp/antenna/control/specific/MountOffset.h"
#include "gcp/antenna/control/specific/SkyOffset.h"
#include "gcp/antenna/control/specific/AzTiltMeter.h"
#include "gcp/antenna/control/specific/Model.h"
#include "gcp/antenna/control/specific/TrackerOffset.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      /**
       * Incomplete type specification for Pointing lets us declare
       * these as friends below without defining them.
       */
      class Pointing;
      class Tracker;
      class SptTracker;
      class TrackerBoard;
      
      /**
       * The following class is used to record the horizon
       * coordinates of a source.
       */
      class Position {
	
      public:
	
	/**
	 * Constructor.
	 */
	Position();
	
	/**
	 * Reset internal data members.
	 */
	void reset();
	
	/**
	 * Set an axis position.
	 */
	void set(gcp::util::Axis::Type axis, double val);
	
	/**
	 * Set an axis position
	 */
	void set(double az, double el, double pa);
	
	/**
	 * Increment the requested position with mount offsets
	 */
	void increment(gcp::util::Axis::Type axis, double val);

	/**
	 * Increment the requested position with mount offsets
	 */
	void increment(MountOffset* offset);
	/**
	 * Increment the requested position with sky offsets
	 */
	void increment(SkyOffset* offset);
	/**
	 * Increment the requested position with tilt meter corrections
	 */
	void increment(AzTiltMeter* offset);

	/**
	 * Apply a collimation model
	 */
	void applyCollimation(Model& model, TrackerOffset& offset);
	
	/**
	 * Get an axis position.
	 */
	double get(gcp::util::Axis::Type axis);
	
	/**
	 * Pack this position for archival in the register database.
	 */
	void pack(signed* s_elements);
	void pack(double* array);
	
      private:
	
	/**
	 * Declare Pointing as a friend so we can manipulate Position
	 * members in pack methods
	 */
	friend class Tracker;
	friend class SptTracker;
	friend class Pointing;
	friend class TrackerBoard;
	
	/**
	 * The azimuth of the source
	 */
	double az_;      
	
	/**
	 * The elevation of the source
	 */
	double el_;      
	
	/**
	 * The parallactic angle of the source
	 */
	
	double pa_;      
	
      }; // End class Position
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
