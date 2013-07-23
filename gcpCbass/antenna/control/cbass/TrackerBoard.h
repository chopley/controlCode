#ifndef TRACKERBOARD_H
#define TRACKERBOARD_H

/**
 * @file TrackerBoard.h
 * 
 * Tagged: Thu Nov 13 16:53:56 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

#include "gcp/antenna/control/specific/Board.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/RegDate.h"

#define NEW_SCAN

namespace gcp {
  namespace antenna {
    namespace control {
      
      class Atmosphere;
      class AxisPositions;
      class Model;
      class PmacTarget;
      class Pointing;
      class Position;
      class Site;
      class Scan;
      class TrackerOffset;

      /**
       * The registers of the virtual tracker board. This contains
       * details of the tracking computations performed by the Drive
       * Task.
       */
      class TrackerBoard : public Board {
	
      public:
	
	/**
	 * Constructor for the Tracker board class.
	 *
	 * @throws (via Board::findReg or Board::Board) Exception
	 */
	TrackerBoard(SpecificShare* share, char* name);
	
	/**
	 * Return true if the current index is the first in a new frame
	 */
	inline bool isNewFrame() {
	  return (positionIndex_ == 0);
	}
	
	/**
	 * Archive the currently requested position.
	 *
	 * @throws Exception
	 */
	void archivePosition(AxisPositions *current, 
			     Position* commanded);
			
	/*.......................................................................
	 * Record the latest pointing parameters for archival.
	 */
	void archivePointing(unsigned* archived_ptr,
			     Atmosphere* atmosphere,
			     Model* model, 
			     PmacTarget* pmac,
			     Pointing* pointing, 
			     Site* site,
			     TrackerOffset* offset,
			     Scan* scan);

	void archivePointing(unsigned* archived_ptr,
			     Atmosphere* atmosphere,
			     Model* model, 
			     PmacTarget* pmac,
			     Pointing* pointing, 
			     Site* site,
			     TrackerOffset* offset,
			     Scan* scan, 
			     bool servoBusy);
	
	/**
	 * Archive some status information.
	 */
	void archiveStatus(unsigned state, unsigned offSoruce, unsigned lacking);
	
	/**
	 * Convert from integer state to a bit mask
	 */
	unsigned char trackerStateToBit(int state);

      private:
      
	// Increment the position counter

	unsigned int incrementPosition();
	
	// Pack the LST for archival into the register database

	void packLst(unsigned* u_elements, double lst);
	
	// Pack the Ut1Utc

	void packUt1Utc(signed* s_elements, double ut1utc);
	
	// Pack the equation of the equinoxes.

	void packEqnEqx(signed* s_elements, double eqneqx);
	
	// We store SAMPLES_PER_FRAME samples in each data frame This
	// is the telescope position index of the current sample *
	// which is multiplied by SAMPLES_PER_POSITION to get the
	// sample index

	unsigned int positionIndex_;

	//------------------------------------------------------------
	// Block definitions
	//------------------------------------------------------------

	/**
	 * Bit-mask of unreceived pointing parameters: site |
	 * refraction | ut1utc | eqneqx | encoders | tilts | optical |
	 * radio | limits | flexure
	 */
	SpecificShare::Block* lacking_;      

	SpecificShare::Block* offSource_;

	RegMapBlock* servoBusy_;
	
	/**
	 * Local Sidereal Time (msec).
	 */
	SpecificShare::Block* lst_;        
	  
	/**
	 * The value of UT1-UTC (milli-seconds).
	 */
	SpecificShare::Block* ut1utc_;       

	/**
	 * The value of the equation of the equinoxes
	 * (milli-arcseconds).
	 */
	SpecificShare::Block* eqneqx_;       

	/**
	 * az,el encoder zero points.
	 */
	SpecificShare::Block* encoder_off_;  
	
	/**
	 * az,el encoder multipliers
	 */
	SpecificShare::Block* encoder_mul_;  

	/**
	 * Azimuth limits as topocentric mount angles.
	 */
	SpecificShare::Block* az_limits_;    

	/**
	 * Elevation limits as topocentric mount angles.
	 */
	SpecificShare::Block* el_limits_;    

	/**
	 * Hour-angle,latitude,elevation tilts.
	 */
	SpecificShare::Block* tilts_;        

	/**
	 * Elevation flexure.
	 */
	SpecificShare::Block* flexure_;      

	/**
	 * Radio collimation (else optical)?
	 */
	SpecificShare::Block* axis_;         

	/**
	 * If optical model, which ptel?
	 */
	SpecificShare::Block* ptel_;         

	/**
	 * Collimation tilt,direction.
	 */
	SpecificShare::Block* fixedCollimation_;  

	/**
	 * Actual latitude, longitude, altitude.
	 */
	SpecificShare::Block* siteActual_;         

	/**
	 * Fiducial latitude, longitude, altitude.
	 */
	SpecificShare::Block* siteFiducial_;         

	/**
	 * Antenna offsets (N, E, Up)
	 */
	SpecificShare::Block* location_;         

	SpecificShare::Block* time_diff_;         

	//------------------------------------------------------------
	// Fast registers
	//------------------------------------------------------------

	/**
	 * MJD UTC of each position in this record
	 */
	SpecificShare::Block* utc_;
	
	/**
	 * MJD UTC when position command sent to ACU for each position
	 * in this record
	 */
	SpecificShare::Block* tick_utc_;
		
	/**
	 * MJD UTC when position command sent to ACU for each position
	 * in this record
	 */
	SpecificShare::Block* tick_nanoseconds_;
		
	/**
	 * The pointing mode (a DriveMode enumerator).
	 */
	SpecificShare::Block* mode_;   

	/**
	 * The A and B refraction terms.
	 */
	SpecificShare::Block* refraction_;   

	/**
	 * Source name
	 */
	SpecificShare::Block* source_;  

	/*
	 * Scan monitor points
	 */
	SpecificShare::Block* scanName_;   
	
	SpecificShare::Block* scanOff_;     
	
	SpecificShare::Block* scanFlag_;     

	SpecificShare::Block* scanInd_;     

	SpecificShare::Block* scanState_;      
	
	SpecificShare::Block* scanRep_;       

	/**
	 * Geocentric apparent ra,dec,distance.
	 */
	SpecificShare::Block* equat_geoc_;   

	/**
	 * User specified ra,dec offsets.
	 */
	SpecificShare::Block* equat_off_;  
	 
	/**
	 * Geocentric apparent az, el.
	 */
	SpecificShare::Block* horiz_geoc_;   
	
	/**
	 * Topocentric az, el, pa.
	 */
	SpecificShare::Block* horiz_topo_;   
	
	/**
	 * Telescope-mount az, el.
	 */
	SpecificShare::Block* horiz_mount_; 

	/**
	 * User supplied az,el offsets.
	 */
	SpecificShare::Block* horiz_off_;   

	/**
	 * User supplied sky-based x,y offsets.
	 */
	SpecificShare::Block* sky_xy_off_;  

	/**
	 * Demanded az,el encoder move rates.
	 */
	SpecificShare::Block* expectedRates_;     
	
	/**
	 * The current reported rates of the telescope axes
	 */
	SpecificShare::Block* actualRates_;
	
	/**
	 * The reported positions of the telescope axes.
	 */
	SpecificShare::Block* actual_;       

	/**
	 * The current reported motor currents
	 */
        SpecificShare::Block* motor_current_;

	/**
	 * The current reported tachometer readings
	 */
        SpecificShare::Block* sum_tach_;

	/**
	 * The current reported tiltmeter readings
	 */
        SpecificShare::Block* tilt_xy_;

	/**
	 * The current moving average of reported tiltmeter readings
	 */
        SpecificShare::Block* tilt_xy_avg_;

	/**
	 * The current tilt meter correction enabled status
	 */
        SpecificShare::Block* tilt_enabled_;

	/**
	 * The current maximum allowed absolute tilt meter value
	 */
        SpecificShare::Block* tilt_max_;

	/**
	 * The current tilt meter zero offsets
	 */
        SpecificShare::Block* tilt_xy_offset_;

	/**
	 * The current angle between Grid North and tilt meter x axis
	 */
        SpecificShare::Block* tilt_theta_;

	/**
	 * The current tilt meter moving average interval 
	 */
        SpecificShare::Block* tilt_average_interval_;

	/**
	 * The current reported tiltmeter temperature 
	 */
        SpecificShare::Block* tilt_temp_;

        /**
	 * The expected position
         */
        SpecificShare::Block* expected_;

        /**
	 * The expected encoder counts
         */
        SpecificShare::Block* expectedCounts_;

	/**
	 * The expected positions of the telescope axes.
	 */
	SpecificShare::Block* actualCounts_;

	/**
	 * The difference between the expected and actual positions of
	 * the telescope axes.
	 */
	SpecificShare::Block* errors_;   

	/**
	 * The current tracking status.
	 */
	SpecificShare::Block* state_;
	    
	SpecificShare::Block* stateMask_;      

      }; // End class TrackerBoard
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
