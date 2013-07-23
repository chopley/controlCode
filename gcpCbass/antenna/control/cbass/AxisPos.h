#ifndef AXISPOS_H
#define AXISPOS_H

/**
 * @file AxisPos.h
 * 
 * Tagged: Thu Nov 13 16:53:32 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Axis.h"
#include "gcp/util/common/Angle.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      
      /**
       * Incomplete type specifications lets us declare these as
       * friends without defining them.
       */
      class AxisPositions;
      class ServoComms;
      class ServoCommsSa;
      class PmacComms;
      class AcuBoard;
      class AcuBoardDebug;
      class Tracker;
      class SptTracker;
      class SptTrackerDebug;
      class TrackerBoard;
      
      /**
       * A class to encapsulate the topocentric and encoder position
       * of a telescope axis
       */
      class AxisPos {
	
      public:
	
	/**
	 * Constructor
	 */
	AxisPos(gcp::util::Axis::Type type);
	
	/**
	 * Reset internal members
	 *
	 * @throws Exception
	 */
	void reset();
	
      private:
	
	/**
	 * Friends of this class -- why don't I just make the data
	 * members public?  Yes, why not? - KAA
	 */
	friend class AxisPositions;
	friend class ServoComms;
	friend class PmacBoard;
	friend class AcuBoard;
	friend class AcuBoardDebug;
	friend class Tracker;
	friend class SptTracker;
	friend class SptTrackerDebug;
	friend class TrackerBoard;
	
	/**
	 * The axis this container represents.
	 */
	gcp::util::Axis axis_; 
	
	/**
	 * The topocentric position of the axis in
	 * radians.
	 */
      public:
	double topo_;     
	
      private:
	/**
	 * The encoder count of the axis.
	 */
	signed count_;    
	
	gcp::util::Angle rawAngle_;
	
      }; // End class AxisPos
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
