#ifndef GCP_ANTENNA_CONTROL_FIXEDCOLLIMATION_H
#define GCP_ANTENNA_CONTROL_FIXEDCOLLIMATION_H

/**
 * @file FixedCollimation.h
 * 
 * Tagged: Thu Nov 13 16:53:35 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Angle.h"

#include "gcp/antenna/control/specific/PointingCorrections.h"
#include "gcp/antenna/control/specific/SkyOffset.h"
#include "gcp/antenna/control/specific/Collimation.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      
      /**
       * Objects of the following type contain the collimation
       * components of the pointing model.
       */
      class FixedCollimation : public Collimation, public SkyOffset {
	
      public:
	
	/**
	 * Constructor trivially calls reset() method, below.
	 */
	FixedCollimation();
	
	/**
	 * Update the tilt associated with this collimation correction.
	 */
	void setXOffset(gcp::util::Angle x);
	
	/**
	 * Update the azimuth associated with this collimation
	 * correction.
	 */
	void setYOffset(gcp::util::Angle y);

	/**
	 * Increment the tilt associated with this collimation correction.
	 */
	void incrXOffset(gcp::util::Angle x);
	
	/**
	 * Increment the azimuth associated with this collimation
	 * correction.
	 */
	void incrYOffset(gcp::util::Angle y);

	void reset();

	void apply(PointingCorrections* f, TrackerOffset& offset);

	void pack(signed* s_elements);


	void print();

      }; // End class FixedCollimation
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
