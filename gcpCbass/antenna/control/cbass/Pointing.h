#ifndef POINTING_H
#define POINTING_H

/**
 * @file Pointing.h
 * 
 * Tagged: Thu Nov 13 16:53:46 UTC 2003
 * 
 * @author Erik Leitch
 */

// Needed for SRC_LEN

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

#include "gcp/util/common/Axis.h"
#include "gcp/util/common/RegDate.h"

#include "gcp/antenna/control/specific/TrackerMsg.h"
#include "gcp/antenna/control/specific/Encoder.h"
#include "gcp/antenna/control/specific/PmacAxis.h"
#include "gcp/antenna/control/specific/Position.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      
      class Tracker;
      
      /**
       * A class used by Tracker to handle pointing.
       */
      class Pointing {
	
      public:

	/**
	 * An enumerator for the Position members below
	 */
	enum PositionType {
	  MOUNT_ANGLES,
	  MOUNT_RATES,
	  TOPOCENTRIC,
	  GEOCENTRIC
	};
	
	/**
	 * Constructor
	 */
	Pointing();
	
	/**
	 * Reset method
	 */
	void reset();
	
	// Methods to set up pointing in preparation for various moves
	
	/**
	 * Prepare for a halt of the antenna
	 */
	void setupForHalt(SpecificShare* share);
	
	/**
	 * Prepare for a reboot (of the drive).
	 */
	void setupForReboot(SpecificShare* share);
	
	/**
	 * Prepare for a slew.
	 */
	void setupForSlew(SpecificShare* share, TrackerMsg* msg);

	/**
	 * Prepare for a track
	 */
	void setupForTrack();
	
	// Member modification methods
	
	/**
	 * Set the current time.
	 */
	void setTime(double utc);
	
	/**
	 * Set the current time.
	 */
	void setTime(int mjd, int sec);
	
	/**
	 * Set the current source name.
	 */
	void setSrcName(char* name);
	unsigned char* getSrcName();

	/**
	 * Set the current scan name
	 */
	void setScanName(char* name);
	unsigned char* getScanName();

	/**
	 * Install the set of axes to drive.
	 */
	void setAxes(gcp::util::Axis::Type axes);
	
	/**
	 * Set the RA of the source.
	 */
	void setRa(double ra);
	
	/**
	 * Set the Dec of the source.
	 */
	void setDec(double dec);
	
	/**
	 * Set the source distance.
	 */
	void setDist(double dist);
	
	/**
	 * Install the refraction correction.
	 */
	void setRefraction(double refraction);
	
	// Member access methods
	
	/**
	 * Get the mask of axes to be driven.
	 */
	gcp::util::Axis::Type getAxes();
	
	/**
	 * Return true if the passed axis is included in the set of
	 * axes to control.
	 */
	bool includesAxis(gcp::util::Axis::Type axis);
	
	/**
	 * Return the current UTC
	 */
	double getUtc();
	gcp::util::RegDate getDate();
	
	/**
	 * Return the refraction correction.
	 */
	double getRefraction();
	
	/**
	 * Convert a mount angle to encoder counts
	 */
	void convertMountToEncoder(Encoder* encoder,
				   PmacAxis* axis,
				   int current);
	void convertMountToEncoder(Encoder* encoder,
				   PmacAxis* axis, 
				   double current);
	/**
	 * Return a pointer to the requested pointing angle container
	 *
	 * @throws Exception
	 */
	gcp::antenna::control::Position* Position(PositionType type);
	
	/**
	 * Install the angles to which the axes will be driven.
	 */
	void setAngles(double az, double el, double pa);
	
	/**
	 * Install the rates with which the axes will be driven.
	 */
	void setRates(double az, double el, double pa);
	
	/**
	 * Compute the current geocentric position.
	 */
	void computeGeocentricPosition(double lst, PointingCorrections* f);
	
	/**
	 * Pack the UTC for archival into the register database.
	 */
	void packUtc(unsigned* u_elements);
	
	/**
	 * Pack the source name for archival in the register database.
	 *
	 * @throws Exception
	 */
	void packSourceName(unsigned* u_elements, int nel);
	
	/**
	 * Pack the scan name for archival in the register database.
	 */
	void packScanName(unsigned* u_elements, int nel);

	/**
	 * Pack geocentric equatorial coordinates.
	 */
	void packEquatGeoc(signed* s_elements);
	
	/**
	 * Pack geocentric horizon coordinates.
	 */
	void packHorizGeoc(signed* s_elements);
	
	/**
	 * Pack topocentric horizon coordinates.
	 */
	void packHorizTopo(signed* s_elements);
	
	/**
	 * Pack mount horizon coordinates.
	 */
	void packHorizMount(signed* s_elements);
	void packHorizMount(double* array);
	
	/**
	 * Return true if the current source is a center source
	 */
	bool isCenter() {
	  return isCenter_;
	}

	void resetAngles();

      private:
	
	friend class Tracker;
	
	/**
	 * Round an angle into the range -pi..pi.
	 */
	double wrapPi(double angle);
	
	/**
	 * Round an angle into the range 0..2pi
	 */
	double wrap2pi(double angle);
	
	/**
	 * The name of the source
	 */
	char srcName_[SRC_LEN];  

	/**
	 * The name of the scan
	 */
	char scanName_[SCAN_LEN];  
	
	/**
	 * If this is a center source, the tracking will behave differently
	 */
	bool isCenter_;

	/**
	 * The MJD in days and seconds
	 */
	int mjd_, sec_;       
	
	/**
	 * The geocentric right ascension and declination
	 */
	double ra_,dec_;      
	
	/**
	 * The distance to the source (or 0 if irrelevant)
	 */
	double dist_;         
	
	/**
	 * The applied refraction correction (radians).
	 */
	double refraction_;   
	
	/**
	 * The geocentric azimuth,elevation,parallactic angle.
	 */
	gcp::antenna::control::Position geocentric_;   
	
	/**
	 * The topocentric azimuth,elevation,parallactic angle
	 */
	gcp::antenna::control::Position topocentric_;  
	
	/**
	 * The telescope azimuth,elevation,parallactic angle
	 */
	gcp::antenna::control::Position mountAngles_; 
	gcp::antenna::control::Position saveAngles_; 
	
	/**
	 * The telescope az,el,pa move rates 
	 */
	gcp::antenna::control::Position mountRates_;  
	
	gcp::util::Axis::Type axes_;    // The set of axes to drive,
					// expressed as a bitwise
					// union of Axis enumerators.
      }; // End class Pointing
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
