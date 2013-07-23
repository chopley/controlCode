#ifndef AXISPOSITIONS_H
#define AXISPOSITIONS_H

/**
 * @file AxisPositions.h
 * 
 * Tagged: Thu Nov 13 16:53:33 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/antenna/control/specific/AxisPos.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      // This needs to be cleaned up. Too many friends. - KAA
      
      /**
       * Incomplete type specification for classes lets us declare
       * them as friends below without defining them.
       */
      class Tracker;
      class SptTracker;
      class SptTrackerDebug;
      class TrackerBoard;
      class ServoComms;
      class PmacBoard;
      class AcuBoard;
      class AcuBoardDebug;

      
      /**
       * The AxisPositions class is used by the Tracker class to
       * record the positions of the telescope axes, as reported by
       * the pmac.
       */
      class AxisPositions {
	
      public:
	
	/**
	 * Constructor
	 */
	AxisPositions();
	
	/**
	 * Return a pointer to a particular AxisPos descriptor
	 *
	 * @throws Exception
	 */
	gcp::antenna::control::AxisPos* 
	  AxisPos(gcp::util::Axis::Type type);
	
	/**
	 * Pack relevant data for archival in the register database
	 */
	void pack(signed* s_elements);
	void pack(double* array);
	
      private:
	
	/**
	 * Friends of AxisPositions.
	 */
	friend class Tracker;
	friend class SptTracker;
	friend class SptTrackerDebug;
	friend class TrackerBoard;
	friend class ServoComms;
	friend class PmacBoard;
	friend class AcuBoard;
	friend class AcuBoardDebug;

	
      public:
	/**
	 * The position of the azimuth axis 
	 */
	gcp::antenna::control::AxisPos az_; 
	
	/**
	 * The position of the elevation axis 
	 */
	gcp::antenna::control::AxisPos el_; 
	
	/**
	 * The position of the pa axis 
	 */
	gcp::antenna::control::AxisPos pa_; 
	
      private:

      }; // End class AxisPositions
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
