#ifndef OFFSETBASE_H
#define OFFSETBASE_H

/**
 * @file OffsetBase.h
 * 
 * Tagged: Thu Nov 13 16:53:42 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/OffsetMsg.h"
#include "gcp/antenna/control/specific/PointingCorrections.h"

// Share includes for constants mastor, pi, twopi

#include "gcp/control/code/unix/libunix_src/common/const.h"

namespace gcp {
  namespace antenna {
    namespace control {

      class PointingCorrections;

      /**
       * Define a class to encapsulate a variety of offsets
       */
      class OffsetBase {
	
      public:

	/**
	 * Constructor.
	 */
	OffsetBase();
	
	/**
	 * Pure virtual destructor prevents instantiation of
	 * OffsetBase.
	 */
	virtual ~OffsetBase() = 0;
	
	// Define methods which should be over-written by classes
	// which inherit from Offset
	//
	// These will be stubbed out so they only need to be defined
	// by inheritors where relevant
	
	/**
	 * Apply these offsets to the pointing model.
	 */
	virtual void apply(PointingCorrections* f);
	
	/**
	 * Install new offsets received from the control program.
	 */
	virtual void set(gcp::util::OffsetMsg);
	
	/**
	 * Install new offsets received from the control program, with
	 * sequence number.
	 */
	virtual void set(gcp::util::OffsetMsg, unsigned seq);
	
	/**
	 * Set an angle.
	 */
	virtual void setAngle(double angle);
	
	// Some utility functions we want all inheritors of this class
	// to have.
	
	/**
	 * Round an angle into the range -pi..pi.
	 */
	static double wrapPi(double angle);
	
	/**
	 * Round an angle into the range 0-2.pi.
	 */
	static double wrap2pi(double angle);
      };
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif
