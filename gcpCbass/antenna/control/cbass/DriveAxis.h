#ifndef DRIVEAXIS_H
#define DRIVEAXIS_H

#include "gcp/util/common/Angle.h"

/**
 * @file DriveAxis.h
 * 
 * Tagged: Thu Nov 13 16:53:43 UTC 2003
 * 
 * @author Erik Leitch
 */
namespace gcp {
  namespace antenna {
    namespace control {
      
      
      /**
       * Incomplete specification for Tracker lets us declare it as a
       * friend.
       */
      class Tracker;
      class SzaTracker;
      class SptTracker;
      
      /**
       * Encapsulate the encoder counts and rates for a telescope
       * axis.
       */
      class DriveAxis {
	
      public:
	
	/**
	 * Constructor.
	 */ 
	DriveAxis();
	
	/**
	 * Reset internal members.
	 */
	void reset();
	
	/**
	 * Return the encoder count of this axis.
	 */
	signed getCount();
	
	gcp::util::Angle getRawAngle() {return rawAngle_;};
	
	/**
	 * Return the count rate of this axis.
	 */
	signed getRate();
	
	gcp::util::Angle getRawRate() {return rawRate_;};
	/**
	 * Set the encoder count for this axis.
	 */
	void setCount(signed count);
	
	void setRawAngle(gcp::util::Angle& angle) {rawAngle_ = angle;};
	
	/**
	 * Set the count rate for this axis.
	 */
	void setRate(signed rate);
	
	void setRawRate(gcp::util::Angle& rate) {rawRate_ = rate;};
	
      private:
	
	/**
	 * Tracker will access private members of this class.
	 */
	friend class Tracker;
	friend class SzaTracker;
	friend class SptTracker;
	
	/**
	 * The encoder position (counts)
	 */
	signed count_; 
	
	/**
	 * The position as an angle in degrees
	 */
	gcp::util::Angle rawAngle_;
	
	/**
	 * The count rate.
	 */
	signed rate_;  
	
	/**
	 * The rate in degrees per second
	 */
	gcp::util::Angle rawRate_;
	
      }; // End class DriveAxis
      
    }; // End namespace control
  }; // End namespace antenna
}; // End namespace gcp

#endif // End #ifndef 
