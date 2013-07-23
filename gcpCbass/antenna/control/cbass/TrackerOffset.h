#ifndef TRACKEROFFSET_H
#define TRACKEROFFSET_H

/**
 * @file TrackerOffset.h
 * 
 * Tagged: Thu Nov 13 16:53:57 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/antenna/control/specific/SkyOffset.h"

#include "gcp/antenna/control/specific/MountOffset.h"
#include "gcp/antenna/control/specific/EquatOffset.h"
#include "gcp/antenna/control/specific/TvOffset.h"


#include "gcp/util/common/OffsetMsg.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      /**
       * Collect together the various offset types used by the Tracker
       * class.
       */
      class TrackerOffset {
	
      public:
	
	/**
	 * Constructor.
	 */
	TrackerOffset();
	
	/**
	 * Reset internal data members.
	 */
	void reset();
	
	// Inline these functions to reduce overhead of frequent
	// function calls
	
	/**
	 * Return a pointer to the mount offsets.
	 */
	inline gcp::antenna::control::MountOffset* MountOffset() {
	  return &mount_;
	};
	
	/**
	 * Return a pointer to the equatorial offsets.
	 */
	inline gcp::antenna::control::EquatOffset* EquatOffset() {
	  return &equat_;
	};
	
	/**
	 * Return a pointer to the sky offsets.
	 */
	inline gcp::antenna::control::SkyOffset* SkyOffset() {
	  return &sky_;
	};
	
	/**
	 * Return a pointer to the TV offsets.
	 */
	inline gcp::antenna::control::TvOffset* TvOffset() {
	  return &tv_;
	};
	
	/**
	 * A public method for obtaining a pointer to the right offset type
	 */
	OffsetBase* Offset(gcp::util::OffsetMsg::Type type);
	
	// Public methods for packing data for archival
	
	/**
	 * Pack equatorial offsets for archival in the register
	 * database.
	 */
	void packEquatOffset(signed* s_elements);
	
	/**
	 * Pack mount horizon offsets for archival in the register
	 * database.
	 */
	void packHorizOffset(signed* s_elements);
	void packHorizOffset(double* array);
	
	/**
	 * Pack sky offsets for archival in the register database.
	 */
	void packSkyOffset(signed* s_elements);
	
	/**
	 * Update the az and el pointing offsets to include any new
	 * offsets measured by the user from the TV monitor of the
	 * optical-pointing telescope.
	 *
	 * @param f   PointingCorrections *  The corrected az,el and latitude.
	 */
	void mergeTvOffset(PointingCorrections *f);
	
      private:
	
	/**
	 * Mount offsets
	 */
	gcp::antenna::control::MountOffset mount_; 
	
	/**
	 * Equatorial offsets
	 */
	gcp::antenna::control::EquatOffset equat_; 
	
	/**
	 * Sky-based tracking offsets
	 */
	gcp::antenna::control::SkyOffset sky_;     
	
	/**
	 * Pointing offsets derived from the image of the sky
	 * displayed on the TV monitor of the optical telescope.
	 */
	gcp::antenna::control::TvOffset tv_;       
	
      }; // End class TrackerOffset
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
