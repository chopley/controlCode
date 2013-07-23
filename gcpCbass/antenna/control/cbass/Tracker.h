#ifndef TRACKER_H
#define TRACKER_H

/**
 * @file Tracker.h
 * 
 * Tagged: Thu Nov 13 16:53:55 UTC 2003
 * 
 * @author Erik Leitch
 */
#include <list>

// Class includes

#include "gcp/util/common/Atmosphere.h"
#include "gcp/util/common/PmacMode.h"
#include "gcp/util/common/Source.h"
#include "gcp/util/common/TrackingStatus.h"

#include "gcp/util/common/TimeVal.h"

#include "gcp/antenna/control/specific/Atmosphere.h"
#include "gcp/antenna/control/specific/EmlDebug.h"
#include "gcp/antenna/control/specific/Site.h"
#include "gcp/antenna/control/specific/SpecificTask.h"
#include "gcp/antenna/control/specific/Pointing.h"
#include "gcp/antenna/control/specific/PointingCorrections.h"
#include "gcp/antenna/control/specific/Position.h"
#include "gcp/antenna/control/specific/Scan.h"
#include "gcp/antenna/control/specific/TrackerBoard.h"
#include "gcp/antenna/control/specific/TrackerMsg.h"
#include "gcp/antenna/control/specific/TrackerOffset.h"

// Needed for various gcp::control enumerators

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

namespace gcp {
  namespace antenna {
    namespace control {

      class AntennaDrive;
      class ServoCommsSa;

      /**
       * Define a class that will handle pointing and tracking for
       * this antenna.
       */
      class Tracker :  
	public SpecificTask,
	public gcp::util::GenericTask<TrackerMsg> {
		
	//============================================================
	// Public members
	//============================================================
	
	public:
	
	//------------------------------------------------------------
	// Protected members
	//------------------------------------------------------------
	
	protected:
	
	bool simPmac_;
	bool simGps_;

	/**
	 * The following identifiers define stages in the state
	 * machine that controls the drive.
	 */
	enum NextDriveState {
	  IGNORE,   // No further action needed (pmac halted)
	  HALT,     // Tell the pmac to stop the telescope drives
	  SLEW,     // Tell the pmac to slew
	  WAIT,     // Waiting for the pmac to signal completion of a
		    // slew
	  TARGET,   // Prepare to change source
	  SYNC,     // Prepare the pmac to start a new track
	  SLAVE,    // Send the next 1-second position of an ongoing
		    // track
	  REBOOT    // Tell the pmac to reboot itself 
	};
	
	//------------------------------------------------------------
	// Protected methods & members
	//------------------------------------------------------------
	
	friend class AntennaDrive;
	
	/**
	 * Pointer to the resources of the parent task.
	 */
	AntennaDrive* parent_;
	
	/**
	 * Constructor function requires the shared resource object
	 * which will maintain pertinent registers of this antenna's
	 * state.
	 *
	 * @throws Exception
	 */
	Tracker(AntennaDrive* parent);
	
	/**
	 * Destructor.
	 */
	virtual ~Tracker();	
	
	/**
	 * Public method to initate a response to a message received on the
	 * AntennaDrive message queue
	 *
	 * @throws Exception
	 */
	virtual void processMsg(TrackerMsg* msg);
	
	/**
	 * True when a shutdown request has been received.
	 */
	bool shutdownPending_; 
	
	// The current year - received from the control program.

	int year_;              
	
	// The next PMAC action to perform.

	NextDriveState whatNext_;
	
	// The set of externally provided pointing parameters that
	// have been recorded in the archive since their last
	// change. This is a bit set of Pointing::Parameter values
	// above

	unsigned archived_; 
	
	// The set of externally provided pointing parameters which
	// have not been received yet. This is a bitwise union of
	// Pointing::Parameter values

	unsigned lacking_;     

	// Calibration of encoders is pending;
	
	bool encoderCalPending_;
	
	// The sequence number of the last telescope positioning
	// request received from the control program.

	volatile long lastReq_;       
	
	// The last sequence number that was acknowledged to the
	// control program on completion.

	volatile long lastAck_;        
	
	// The tracking status at the previous 1-second tick.

	gcp::util::TrackingStatus::Status oldStatus_; 
	
	// The current tracking status

	gcp::util::TrackingStatus::Status newStatus_; 
	
	// This should be set to non-zero whenever any parameters that
	// could break the current track are changed.

	bool paramsUpdated_;   
	
	// The last value of the "acquired" status of the drive

	int pmacTracking_;     
	
	// The latest atmospheric refraction terms

	Atmosphere atmos_;      
	
	// The telescope pointing model

	Model model_;           
	
	// Registers of the servo box

	ServoCommsSa* servo_;

	// The position to request on the next 1-sec pulse

	Pointing nextTarget_;  
	Pointing nextTarget1_;
	Pointing nextTarget2_;
	
	// The last three commanded mount positions

	Position lastCommanded_;    // Position commanded on the last tick
	Position prevCommanded_;    // Position commanded two ticks ago
	Position tertCommanded_;    // Position commanded three ticks ago

	// The location of the telescope

	Site site_;             
	
	// An object for handling atmospheric calculations

	gcp::util::Atmosphere refracCalculator_;

	// The current source

	gcp::util::Source src_;            
	
	// The current scan

	Scan scan_;            
	
	/**
	 * The time of the last time-code-reader interrupt
	 */
	gcp::util::TimeVal currentTick_;

	// The time of the previous time-code-reader interrupt

	gcp::util::TimeVal lastTick_;   
	
	/**
	 * A virtual board of tracker registers
	 */
	TrackerBoard* tracker_; 
	
	/**
	 * The current mount tracking offsets
	 */
	TrackerOffset offset_;  
	
	//------------------------------------------------------------
	// Protected Methods
	//------------------------------------------------------------
	
	void resetMembers();
	
	/**
	 * This function is called by new_Tracker() and ini_Tracker() to clear
	 * legacy pointing model terms before the thread is next run. It assumes
	 * that all pointers either point at something valid or have been set
	 * to NULL.
	 */
	void reset();
	
	/**
	 * Reset non-pointer members of the Tracker object
	 *
	 * @throws Exception
	 */
	virtual void initialize();
	
	/**
	 * Record the current tracking status (newStatus_) in the
	 * archive database and in oldStatus_.
	 */
	void archiveStatus();
	
	void updatePmac(TimeVal& mjd, AxisPositions *current);

	/**
	 * Update the mount-angle limits that correspond to revised
	 * values of
	 * model.{az,el,dk}.{per_turn,per_radian,zero,min,max}, and
	 * mark them as unarchived.
	 */
	void updateMountLimits();
	
	/**
	 * Compute the encoder positions and rates needed to track a given
	 * source at a given utc.
	 *
	 * @param mjd int    The day component of the MJD utc at which
	 *                           coordinates are required.
	 * @param sec int    The MJD utc time of day, in seconds.
	 */
	void sourcePosition(gcp::util::TimeVal& mjd);
	void sourcePosition(gcp::util::TimeVal& mjd, Pointing& thisTargetVal);
	void sourcePositionTrack(gcp::util::TimeVal& mjd, double utc);
	void sourcePositionTrack(gcp::util::TimeVal& mjd, double utc, Pointing& thisTargetVal);
	void sourcePositionCenter(gcp::util::TimeVal& mjd);
	void sourcePositionCenter(gcp::util::TimeVal& mjd, Pointing& thisTargetVal);
	
	/**
	 
	 * @param current AxisPositions *  The current position of the 
	 *                                 telescope, as readPosition().
	 * @throws Exception
	 */
	virtual void pmacNewPosition(gcp::util::PmacMode::Mode mode, 
				     AxisPositions *current, gcp::util::TimeVal& mjd); 
	
	/**
	 * Prototype the functions that implement the various stages
	 * of pointing calculations
	 */
	void finalizePointing(double pmra, double pmdec, 
			      PointingCorrections *f,
			      Pointing* p,
			      gcp::util::TimeVal& mjd);
	void finalizePointing(double pmra, double pmdec, 
			      PointingCorrections *f,
			      Pointing* p,
			      gcp::util::TimeVal& mjd, Pointing& thisTargetVal);

	void finalizePointing(gcp::util::TimeVal& mjd);
	void finalizePointing(gcp::util::TimeVal& mjd, Pointing& thisTargetVal);
	
	/**
	 * Register the receipt of a control-program command that
	 * needs to be acknowledged when has taken effect. Also mark
	 * the tracking parameters as modified.
	 *
	 * Input:
	 *  seq   unsigned    The sequence number assigned to the command by
	 *                    the control program.
	 */
	void registerRequest(unsigned seq);
	
	//............................................................
	// Methods which are called in response to messages from the
	// Drive Task
	//............................................................
	
	/**
	 * Respond to a tick from the time-code reader ISR by
	 * recording the new clock offset. If a resynchronization of
	 * the clock has been requested, also check the time against
	 * the time-code reader and correct it if necessary.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */
	virtual void addTick(TrackerMsg* msg);
	virtual void addTickOvro(TrackerMsg* msg);
	
	/**
	 * Calibrate the collimation of the telescope.
	 */	
	void calCollimation(TrackerMsg* msg);
	
	/**
	 * Record new encoder offsets and multipliers.
	 */
	void calEncoders(TrackerMsg* msg);
	
	/**
	 * Calibrate the gravitational flexure of the telescope.
	 */
	void calFlexure(TrackerMsg* msg);
	
	/**
	 * Calibrate the axis tilts of the telescope.
	 */
	void calTilts(TrackerMsg* msg);
	
	/**
	 * Update the year. Note that the time code reader doesn't supply the
	 * year, so the year has to be provided by the control program.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */
	void changeYear(TrackerMsg* msg);
	
	/**
	 * Update the equation-of-the-equinoxes interpolator.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 *
	 * @throws Exception
	 */
	void extendEqnEqx(TrackerMsg* msg);
	
	/**
	 * Extend the ephemeris of the current source.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */
	void extendTrack(TrackerMsg* msg);
	/**
	 * Extend the ephemeris of the current scan
	 */
	void extendScan(TrackerMsg* msg);
	
	/**
	 * Update the UT1-UTC interpolator.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 *
	 * @throws Exception
	 */
	void extendUt1Utc(TrackerMsg* msg);
	
	/**
	 * Arrange to halt the telescope. If the pmac new_position flag is set
	 * this will be done immediately, otherwise it will be postponed to
	 * the next 1-second tick.
	 *
	 * @param msg TrackerMsg * If the halt command was received
	 * from the control program then this must contain a
	 * transaction sequence number. Otherwise send NULL.
	 */
	void haltTelescope(TrackerMsg* msg=0);
	
	/**
	 * Update the local and system-wide site-location parameters.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */
	void locateSite(TrackerMsg* msg);
	
	/**
	 * Update the antenna-specific offset
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */
	void locateAntenna(TrackerMsg* msg);

	/**
	 * Arrange to reboot the pmac. If the pmac new_position flag is set
	 * this will be done immediately, otherwise it will be postponed to
	 * the next 1-second tick.
	 *
	 * @param msg TrackerMsg * If the halt command was received
	 * from the control program then this must contain a
	 * transaction sequence number. Otherwise send NULL.
	 */
	void rebootDrive(TrackerMsg* msg);
	
	/**
	 * Record new encoder limits.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */
	void recordEncoderLimits(TrackerMsg* msg);
	
	/**
	 * Select between the optical and radio pointing models.
	 */
	void selectModel(TrackerMsg* msg);
	
	/**
	 * Install new encoder offsets.
	 * 
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */	
	void setEncoderZero(TrackerMsg* msg);
	
	/**
	 * Adjust the tracking offsets of specified drive axes, and round them
	 * into the range -pi..pi.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */	
	void setOffset(TrackerMsg* msg);
	
	/**
	 * Install new slew rates.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */	
	void setSlewRate(TrackerMsg* msg);
	
	/**
	 * Arrange to slew the telescope to a given az,el,dk coordinate.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */	
	void slewTelescope(TrackerMsg* msg);
	
	/**
	 * Record new refraction coefficients received from the weather
	 * station task.
	 *
	 * @param msg TrackerMsg* The message received on the
	 * AntennaDrive message queue.
	 */
	void updateRefraction(TrackerMsg* msg);
	void updateRefraction();
	
	/**
	 * Attempt to connect to the pmac.
	 */
	virtual void connectPmac();
	
	/**
	 * A function called when an error occurs talking to the pmac.
	 */
	virtual void disconnectPmac();

	/**
	 * Return true if we have enough information to roughly point
	 * the telescope.
	 */
	bool okToPoint();

	void setNextState(NextDriveState state);

	void strobeServo(TrackerMsg* msg);

	// acknowledges cmpletion of a motion command
	bool acknowledgeCompletion();

	// executes a Servo Specific command
	void executeServoCmd(TrackerMsg* msg);

      }; // End class Tracker
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif
