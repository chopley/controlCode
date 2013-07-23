#ifndef AZTILT_H
#define AZTILT_H

/**
 * @file AzTilt.h
 * 
 * Tagged: Thu Nov 13 16:53:34 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/antenna/control/specific/AxisTilt.h"
#include "gcp/antenna/control/specific/PointingCorrections.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      
      /**
       * Define a class for handling AZ tilts
       */
      class AzTilt : public AxisTilt {
	
      public:
	
	/**
	 * Constructor trivially calls reset(), below.
	 */
	AzTilt();
	
	/**
	 * Reset internal data members to something sensible.
	 */
	void reset();
	
	/**
	 * Install a HA tilt
	 */
	void setHaTilt(double ha_tilt);
	
	/**
	 * Install a latitude tilt
	 */
	void setLatTilt(double lat_tilt);
	
	/**
	 * Correct the tilt of the azimuth axis.
	 */
	void apply(PointingCorrections* f);
	
	/**
	 * Pack an HA tilt for archival in the register database.
	 */
	void packHaTilt(signed* s_elements);
	
	/**
	 * Pack a latitude tilt for archival in the register database.
	 */
	void packLatTilt(signed* s_elements);
	
      private:
	
	/**
	 * The component of the azimuth tilt in the
	 * direction of increasing hour angle
	 * (radians)
	 */
	double haTilt_;  
	
	/**
	 * The component of the azimuth tilt in the
	 * direction of increasing geodetic latitude
	 * (radians)
	 */
	double latTilt_; 
	
      }; // End class AzTilt
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
