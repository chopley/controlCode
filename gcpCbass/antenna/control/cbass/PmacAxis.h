#ifndef PMACAXIS_H
#define PMACAXIS_H

/**
 * @file PmacAxis.h
 * 
 * Tagged: Thu Nov 13 16:53:43 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Angle.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      
      /**
       * Incomplete specification for Tracker lets us declare it as a
       * friend.
       */
      class Tracker;
      
      /**
       * Encapsulate the encoder counts and rates for a telescope
       * axis.
       */
      class PmacAxis {
	
      public:
	
	/**
	 * Constructor.
	 */ 
	PmacAxis();
	
	/**
	 * Reset internal members.
	 */
	void reset();
	
	/**
	 * Return the encoder count of this axis.
	 */
	signed getCount();
	
	/**
	 * Return the count rate of this axis.
	 */
	signed getRate();
	
	/**
	 * Return the position as an angle
	 */
	gcp::util::Angle getAngle();

	/**
	 * Set the encoder count for this axis.
	 */
	void setCount(signed count);
	
	/**
	 * Set the count rate for this axis.
	 */
	void setRate(signed rate);
	
	/**
	 * Set the position as an angle
	 */
	void setAngle(gcp::util::Angle& angle);
	void setRadians(double radians);

      private:
	
	/**
	 * Tracker will access private members of this class.
	 */
	friend class Tracker;
	
	/**
	 * The requested position (angle)
	 */
	gcp::util::Angle angle_;
	
	/**
	 * The encoder position (counts)
	 */
	signed count_; 
	
	/**
	 * The count rate.
	 */
	signed rate_;  
	
      }; // End class PmacAxis
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
