#include <math.h>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/PointingParameter.h"

#include "gcp/antenna/control/specific/AntennaDrive.h"
#include "gcp/antenna/control/specific/Date.h"
#include "gcp/antenna/control/specific/ServoCommsSa.h"
#include "gcp/antenna/control/specific/Tracker.h"

#include <iomanip>

using namespace std;
using namespace gcp::control;
using namespace gcp::util;

using namespace gcp::antenna::control;

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/**.......................................................................
 * Allocate all of the resources needed by the tracker task.
 * Note that no further resource allocation or re-allocation should
 * be peformed while the task is running. This means that the instrument
 * can be left unattended after start-up without the fear of resource
 * depletion causing unexpected failures.
 *
 * Input:
 *
 *  parent     AntennaDrive *  Resources of the parent object.
 */
Tracker::Tracker(AntennaDrive* parent) :
  SpecificTask(), gcp::util::GenericTask<TrackerMsg>::GenericTask()
{
  // Before attempting any operation that might fail, initialize the
  // container at least up to the point at which ~Tracker() can be
  // safely called.

  servo_   = 0;
  share_   = 0;
  tracker_ = 0;
  encoderCalPending_ = false;
  //------------------------------------------------------------
  // Sanity check arguments
  //------------------------------------------------------------

  if(parent == 0) {
    ThrowError("Tracker::Tracker: Received NULL parent argument");
  }

  parent_  = parent;
  share_   = parent_->getShare();
  simPmac_ = parent->simPmac();

  if(share_ == 0) {
    ThrowError("Share argument is NULL.");
  }

  lastTick_.setToCurrentTime();
  currentTick_.setToCurrentTime();

  //------------------------------------------------------------
  // Get the object which will manage servo resources
  //------------------------------------------------------------

  servo_    = new ServoCommsSa(share_, "servo", simPmac_);

  if(servo_ == 0) {
    ThrowError("Couldn't allocate servo_");
  }

  //------------------------------------------------------------
  // Lookup the register-map entry of the tracker statistics board.
  //------------------------------------------------------------

  tracker_ = new TrackerBoard(share_, "tracker");

  if(tracker_ == 0) {
    ThrowError("Tracker::Tracker: Couldn't allocate tracker_");
  }
  
  //------------------------------------------------------------
  // And call our initialization method
  //------------------------------------------------------------

  initialize();
}

/**.......................................................................
 * Delete the resources of a tracker-task.
 */
Tracker::~Tracker()
{
  if(tracker_ != 0)
    delete tracker_;
}

/**.......................................................................
 * Reset non-pointer members of the Tracker object
 */
void Tracker::initialize()
{
  COUT("initializing tracker");
  shutdownPending_ = false;
  year_            = 0;
  whatNext_        = IGNORE;
  archived_        = PointingParameter::NONE;
  lacking_         = PointingParameter::ALL;
  lastReq_         = 0;
  lastAck_         = 0;
  oldStatus_       = TrackingStatus::LACKING; 
  newStatus_       = TrackingStatus::LACKING;
  paramsUpdated_   = false;
  pmacTracking_    = false;

  // Connect to the pmac. During development we need to be able to
  // operate without the pmac, so ignore setup errors.

  try {

    servo_->connect();

    // Initialize the antenna

    COUT("initializing South African Antenna");
    servo_->initializeAntenna();

  } catch(...) {
    parent_->sendDriveConnectedMsg(false);
  }

}

/**.......................................................................
 * Reset non-pointer members of the Tracker object
 */
void Tracker::resetMembers()
{
  shutdownPending_ = false;
  year_            = 0;
  whatNext_        = IGNORE;
  archived_        = PointingParameter::NONE;
  lacking_         = PointingParameter::ALL;
  lastReq_         = 0;
  lastAck_         = 0;
  oldStatus_       = TrackingStatus::LACKING; 
  newStatus_       = TrackingStatus::LACKING;
  paramsUpdated_   = false;
  pmacTracking_    = false;
}

/**.......................................................................
 * This function is called by new_Tracker() and ini_Tracker() to clear
 * legacy pointing model terms before the thread is next run. It assumes
 * that all pointers either point at something valid or have been set
 * to NULL.
 */
void Tracker::reset()
{
  resetMembers();

  // Reset source parameters
  
  src_.reset();
  
  // Reset scan parameters

  scan_.reset();

  // Reset requested offset parameters  
  
  offset_.reset();
  
  // Reset pointing model parameters
  
  model_.reset();
  
  // Reset parameters of the refraction calculation
  
  atmos_.reset();
  
  // Rest site parameters
  
  site_.reset();
  
  // Reset commanded positions

  lastCommanded_.reset();
  prevCommanded_.reset();
  tertCommanded_.reset();

  nextTarget_.reset();
  nextTarget1_.reset();
  nextTarget2_.reset();
}

/**.......................................................................
 * Respond to a 1-second tick from the time-code reader ISR by
 * recording the new clock offset. If a resynchronization of the clock
 * has been requested, also check the time against the time-code
 * reader and correct it if necessary.  
 *
 *  Modification for South African Telescope -- need to calculate
 *  three positions to send to the telescope
 *
 * Input:
 *
 *   msg  TrackerMsg*  The message received on the Tracker message 
 *                          queue.
 */
void Tracker::addTick(TrackerMsg* msg)
{
  static TimeVal lastMjd, currMjd, diff;
  LogStream errStr;
  static bool startup=true;

  // Allowable limits for delta t between successive ticks sent to the
  // tracker

  double lowerDeltaLimit=0.9, upperDeltaLimit=1.1;

  // The timestamp gets set when the signal handler is called, on the
  // half-second boundary.  We store only the integral second of the
  // timestamp, since we want calculations to be done relative to the
  // absolute second boundary to which this tick corresponds.  
  //
  // We need to calculate the timestamp for three different times, and
  // pass the triplet to the servo controller.  We'll pass the
  // previous second and the next two, for continuity.  

  currMjd.setMjd(msg->body.tick.mjdDays, msg->body.tick.mjdSeconds, 0);

  try {

    // if the first time running the loop. no need to check the time
    // difference

    if(startup == true) {
      diff = 1;
      startup = false;
    } else {
      diff           = currMjd - lastMjd;
    };

    double secDiff = diff.getTimeInSeconds();

    // The new time should be roughly 1 second later than the previous
    // time.  But it's only crucial that we deliver the last position 
    // to the PMAC well before it is next needed.  

    if(secDiff > lowerDeltaLimit && secDiff < upperDeltaLimit) {

      // Record the new clock tick offset in the database.

      share_->setClock(currMjd);

      // Do we have enough information to roughly point the telescope?

      if(!okToPoint()) {
	newStatus_ = TrackingStatus::LACKING;

      } else {
	// Find out where the telescope is currently located and
	// record this in the archive.
	
	AxisPositions current;

	// Read the current position and set the pmac tracking status
	pmacTracking_ = servo_->readPosition(&current, &model_);

	// The sequence is as follows: updates to the pmac are made
	// once a second, on the three-quarter-second.  We calculate
	// three positions at that time, one for the integral previous
	// second, and the subsequent two seconds.  We need to pass
	// all of these (with their time) to the servo box.
	//
	//           Calculate       Calculate       Calculate 
	//            position        position        position    
	//           for second      for second      for second
	//            0,1,2            1,2,3           2,3,4
	//           Assert Pos      Assert Pos      Assert Pos
	//               |               |               |       
	//      Read     |      Read     |      Read     |    
	//      PMAC     |      PMAC     |      PMAC     |    
	//       |       |       |       |       |       |    
	//   |               |               |               | 
	//   |___|___|___|___|___|___|___|___|___|___|___|___|
	//   0               1               2               3
	//                   
	//                   |               |	             |	     
	//                   |               |	             |	     
	//              PMAC asserts    PMAC asserts    PMAC asserts 
	//               position 1      position 2      position 3  
	//
	//

	// Now archive the location read a during this second (
	// reflects instantaneous values) and the commanded
	// position from the past second

	tracker_->archivePosition(&current, &tertCommanded_);

	// If the pmac is ready for a new command, execute the next
	// state of the pmac state machine.  If the command is a halt
	// command, issue it right away.

	if(!servo_->isBusy()) {
	  // Use the (implicit) TimeVal copy constructor to create a
	  // copy of currentMjd, since updatePmac may modify the
	  // timestamp.

	  TimeVal mjd = currMjd;

	  // Send the updated position to the pmac.  

	  /*	  COUT("about to update pmac");
	  CTOUT("mjdays: " << mjd.getMjdDays());
	  CTOUT("mjdSeconds: " << mjd.getMjdSeconds());
	  CTOUT("mjdNanoSeconds: " << mjd.getMjdNanoSeconds());*/
	  
	  try {
	    updatePmac(mjd, &current);
	  } catch (Exception& err) {

	    ReportMessage("updatePmac threw an exception");

	    cout << err.what() << endl;
	    err.report();

	    // An error may be thrown if the UMAC interface does not
	    // respond in time, which has been seen to happen
	    // infrequently.  If we are tracking a source when such a
	    // timeout occurs, we should just queue a resync and
	    // continue tracking.  If not, we will just halt the
	    // telescope.

	    errStr.initMessage(true);
	    errStr << "No response from the PMAC." <<
	      " whatNext_ = " << whatNext_ << ", newStatus_ = " << newStatus_;

	    // If the pmac was tracking, syncing or targeting when the
	    // timeout occurred, re-target the source.
	    
	    if(whatNext_ == SLAVE || whatNext_ == SYNC || whatNext_ == TARGET) {

	      ReportMessage("whatNext_ = (one of 3 things)");

	      errStr << endl << "Re-targeting source";

	      whatNext_   = TARGET;
	      
	      // Else if we were in the process of slewing, just
	      // requeue the slew
	      
	    } else if(whatNext_ == HALT && 
		      newStatus_ == TrackingStatus::SLEWING) {

	      ReportMessage("whatNext_ = HALT");

	      errStr << endl << "Re-queuing the current slew";

	      whatNext_ = SLEW;
	      
	      // If the telescope was halted, and a slew was
	      // commanded, requeue the slew

	    } else if(whatNext_ == SLEW) {

	      ReportMessage("whatNext_ = SLEW");
	    
	      errStr << endl << "Re-queuing the new slew";

	      whatNext_ = SLEW;

	      // Else simply halt the telescope

	    } else {

	      ReportMessage("whatNext_ = (default)");
	      
	      errStr << endl << "Halting the telescope";

	      whatNext_   = HALT;
	    }
	    
	    // And set the new status to updating

	    newStatus_  = TrackingStatus::UPDATING;
	  }

	  // If the pmac is not ready and we are in the process of
	  // tracking a source, this means that pmac has lost sync, so
	  // queue a resync.

	} else if(whatNext_ == SLAVE) {

	  COUT("Tracker thinks pmac has lost time sync");
	  errStr.initMessage(true);
	  errStr << "The pmac appears to have lost its time synchronization.";

	  whatNext_   = TARGET;
	  newStatus_  = TrackingStatus::UPDATING;
	};
      
      };

      // If the time hasn't changed at all then this implies that a
      // spurious interrupt was received.
      
    } else if(secDiff < lowerDeltaLimit) {
      errStr.initMessage(true);
      errStr << "Discarding spurious time-code reader interrupt.";
    } else {

      // Did the clock jump? 

      errStr.initMessage(true);
      errStr << "Time appears to have gone "
	     << (secDiff < 0 ? "back":"forwar")
	     << " by "
	     << fabs(secDiff)
	     << "seconds.";
      
      // Record the time error for inclusion in the archive database.
      
      newStatus_ = TrackingStatus::TIME_ERROR;
    };
    
  } catch(Exception& err) {
    err.report();
    newStatus_ = TrackingStatus::TIME_ERROR;
  };
  
  // If the time is invalid and we are tracking then we should attempt
  // reacquire the current source on the next second tick.
  
  if(newStatus_ == TrackingStatus::TIME_ERROR && whatNext_ == SLAVE) {
    DBPRINT(true, Debug::DEBUG4, 
	    "About to enter TARGET because of a time error");
    whatNext_ = TARGET;
  }
  
  // Archive the current tracking status.
  
  archiveStatus();

  // Store the last mjd

  lastMjd = currMjd;

  // And simply log any errors that occurred.

  if(errStr.isError()){
    errStr.log();

    if(newStatus_ != TrackingStatus::TIME_ERROR) {
      ThrowError(errStr);
    }
  }

}

/**.......................................................................
 * This is a private function of Tracker::addTick() used to update the
 * pmac on each one second tick when PmacBoard::isBusy() returns false.
 *
 * Input:
 *
 *  mjd      TimeVal&        The Julian Date (utc).
 *  current  AxisPositions*  The current position of the telescope.
 */ 
void Tracker::updatePmac(TimeVal& mjd, AxisPositions *current)
{
  LogStream logStr;

  TimeVal mjd0, mjd1, mjd2;
  mjd0 = mjd;
  mjd1 = mjd; mjd1.incrementSeconds(1.0);
  mjd2 = mjd; mjd2.incrementSeconds(2.0);

  // The following switch implements a state machine to command the
  // PMAC on each 1 second tick. The state to be adopted at the next 1
  // second tick is always recorded in whatNext_.

  //  COUT("WHATNEXT_: " << whatNext_);

  switch(whatNext_) { 

    //-----------------------------------------------------------------------
    // The telescope is idle 

  case IGNORE: 
    DBPRINT(true, gcp::util::Debug::DEBUG4,"whatNext is IGNORE: " << mjd);

    // If this is the end of a slew or halt command (including halting
    // after failing to start any other type of transaction), report the
    // transaction completion to the control program.

    DBPRINT(true, gcp::util::Debug::DEBUG4,"lastAck_ = " << lastAck_ << ", lastReq_ = " << lastReq_);
	    
    if(acknowledgeCompletion()){
      // Log a message back to the control program
      
      ReportMessage("The telescope is halted");
    };
    newStatus_ = TrackingStatus::HALTED;
    break;

    //-----------------------------------------------------------------------
    // Stop the telescope from moving 

  case HALT:   
    DBPRINT(true, gcp::util::Debug::DEBUG4,"whatNext is HALT: " << mjd);

    sourcePosition(mjd, nextTarget_);
    sourcePosition(mjd1, nextTarget1_);
    sourcePosition(mjd2, nextTarget2_);
    pmacNewPosition(PmacMode::HALT, current, mjd);
    whatNext_  = HALT;
    newStatus_ = TrackingStatus::UPDATING;
    break;

    //-----------------------------------------------------------------------
    // Initiate a pmac reboot 

  case REBOOT:   
    //    COUT("HERE 2");
    pmacNewPosition(PmacMode::REBOOT, current, mjd);
    whatNext_   = HALT;
    newStatus_  = TrackingStatus::SLEWING;
    break;

    //-----------------------------------------------------------------------
    // Slew the telescope to a given position 

  case SLEW:   
    //    COUT("HERE 3");

    DBPRINT(true, gcp::util::Debug::DEBUG4,"whatNext is SLEW: " << mjd);

    pmacNewPosition(PmacMode::SLEW, current, mjd);

    // Log a message back to the control program

    ReportMessage("The telescope is slewing");

    // Although the pmac will itself halt the telescope at the end of
    // the slew, arrange to halt it explicitly so that the archived
    // registers show the demanded position as halted instead of still
    // slewing.

    whatNext_   = HALT;
    newStatus_  = TrackingStatus::SLEWING;
    break;

    //-----------------------------------------------------------------------
    // Request a slew to steer the telescope close to the source position.

  case TARGET:
    //    COUT("HERE 4");

    DBPRINT(true, gcp::util::Debug::DEBUG4,"whatNext is TARGET: " << mjd);

    sourcePosition(mjd, nextTarget_);
    sourcePosition(mjd1, nextTarget1_);
    sourcePosition(mjd2, nextTarget2_);
    pmacNewPosition(PmacMode::SLEW, current, mjd);
    
    // Log a message back to the control program

    ReportMessage("The telescope is slewing to source: "  << (nextTarget_.isCenter() ? (char*)nextTarget_.getSrcName() : (char*)src_.getName()));

    // Queue a pmac synchronization so that the track starts on a
    // mutually agreed GPS 1-second boundary.
    
    whatNext_  = SYNC;
    newStatus_ = TrackingStatus::SLEWING;
    break;

    //-----------------------------------------------------------------------
    // Send the PMAC the position at which to start tracking the
    // current source, and arm the GPS time-code-reader to send the
    // PMAC a pulse 1 second before it is supposed to reach the
    // specified position.

  case SYNC:
    //    COUT("HERE 5");
    DBPRINT(true, gcp::util::Debug::DEBUG4,"whatNext is SYNC: " << mjd);


    sourcePosition(mjd, nextTarget_);
    sourcePosition(mjd1, nextTarget1_);
    sourcePosition(mjd2, nextTarget2_);
    pmacNewPosition(PmacMode::SYNC, current, mjd);

    // Until a new request is received from the user, or the PMAC
    // loses sync, updated tracking positions should be sent on
    // subsequent 1-second ticks.

    whatNext_      = SLAVE;
    newStatus_     = TrackingStatus::UPDATING;
    paramsUpdated_ = true;
    break;

    //-----------------------------------------------------------------------
    // Start or continue tracking the current source. This involves
    // sending the PMAC the position and rates that it should reach on
    // the next one-second tick. It already has the positions for the
    // current tick, so it can interpolate between the two positions
    // during the following second.

  case SLAVE:
    //    COUT("HERE 6");
    DBPRINT(true, gcp::util::Debug::DEBUG4,"whatNext is SLAVE: " << mjd);

    // Send the pmac the desired position of the track 1.0 seconds
    // from now.

    sourcePosition(mjd, nextTarget_);
    sourcePosition(mjd1, nextTarget1_);
    sourcePosition(mjd2, nextTarget2_);
    pmacNewPosition(PmacMode::TRACK, current, mjd);
    
    // Whenever parameters of the track get updated, we have to wait
    // until the next second tick to see whether the pmac accepted
    // the modifications.
    
    if(paramsUpdated_) {
      paramsUpdated_ = false;
      newStatus_ = TrackingStatus::UPDATING;
    } else {
      
      // Get the above-commanded elevation position and rate.
      
      double el_rate  = nextTarget2_.Position(Pointing::MOUNT_RATES)->
	get(Axis::EL);
      double el_angle = nextTarget2_.Position(Pointing::MOUNT_ANGLES)->
	get(Axis::EL);
      
      // Use the elevation rate to extrapolate back to the current elevation
      // of the source and its elevation one second from now. Note that
      // the above elevation was computed for 1.0 seconds in the future.
      
      double el_next = el_angle - el_rate;
      double el_now  = el_next  - el_rate;
      
      // Get the current min/max elevation limits.
      
      double el_min = model_.Encoder(Axis::EL)->getMountMin();
      double el_max = model_.Encoder(Axis::EL)->getMountMax();
      //	COUT("el_min, el_max: " << el_min << " , " << el_max);
      
      
      // Is the source going to exceed the lower elevation limit
      // within the next second? Note that the above position is for
      // 1.0 seconds in the future, so we have to use the elevation
      // rate to extrapolate back in time.
      
      if(el_now <= el_min || el_next <= el_min) {
	if(oldStatus_ != TrackingStatus::TOO_LOW) {
	  ReportSimpleError("The source is below "
			    "the lower elevation limit.");
	  parent_->sendSourceSetMsg(lastReq_);
	};
	newStatus_ = TrackingStatus::TOO_LOW;
	
	// Is the source going to exceed the upper elevation limit
	// within the next second? Note that the above position is
	// for 1.0 seconds in the future, so we have to use the
	// elevation rate to extrapolate back in time.
	
      } else if(el_now >= el_max || el_next >= el_max) {
	if(oldStatus_ != TrackingStatus::TOO_HIGH) 
	  ReportSimpleError("The source is above "
			    "the upper elevation limit.");
	newStatus_ = TrackingStatus::TOO_HIGH;
	
	// Is the pmac tracking within reasonable limits?
	
      } else if(pmacTracking_) {
	
	// Now that we are successfully on source report the
	// completion of any control-program commands that led to
	// this point.
	
	(void) acknowledgeCompletion();
	
	// Report the successful acquisition of the source?
	
	if(oldStatus_ != TrackingStatus::TRACKING) {
	  ReportMessage("The telescope is now tracking source: "  << (nextTarget_.isCenter() ? (char*)nextTarget_.getSrcName() : (char*)src_.getName()));
	};
	newStatus_ = TrackingStatus::TRACKING;
      };
    };
    break;
  }; // End switch(whatNext_)

  // Report any messages we chose to log.

  if(logStr.isError()) 
    ErrorDef(err, logStr);
}

/**.......................................................................
 * Write a given drive command into the drive dual-port-ram. Before invoking
 * this function, the caller must call DriveBoard::isBusy() to see whether the
 * drive is ready for a new command.
 *
 * Input:
 *
 *  mode         PmacMode    The command-type to send.
 *  current AxisPositions *  The current position of the telescope, as
 */
void Tracker::pmacNewPosition(PmacMode::Mode mode, 
			      AxisPositions *current, gcp::util::TimeVal& mjd) 
{
  unsigned new_position = 1;    // When the new command has been written to 
                                // the dual-port-ram, this value will be 
                                // assigned to the DRIVE new_position 
                                // flag. 
  PmacTarget pmac;              // The target encoder positions and rates 
                                // for each axis 
  PmacTarget pmac1;             // 1 second in future
  PmacTarget pmac2;             // 2 seconds in future

  Pointing* target  = &nextTarget_;
  Pointing* target1 = &nextTarget1_;
  Pointing* target2 = &nextTarget2_;
  
  DBPRINT(true, Debug::DEBUG4, "Inside pmacNewPosition()");

  // Act according to the type of pmac command being dispatched.
  
  switch(mode) {
  case PmacMode::HALT:
    target->setAxes(Axis::NONE); // Don't move any of the axes
				 // deliberate fall-through...
    target1->setAxes(Axis::NONE); 
    target2->setAxes(Axis::NONE); 
  case PmacMode::REBOOT:
  case PmacMode::SYNC:
  case PmacMode::SLEW:
  case PmacMode::TRACK:

    // Convert angles and rates to encoder positions and rates.  If we
    // have been asked to keep any axes unmoved, set the corresponding
    // target positions to the current positions of the axes and
    // assign target rates of zero.

    // Check the AZ axis
    
    if(target->includesAxis(Axis::AZ)) {
      // Convert the requested angle (stored in target) to a count and
      // set it in the PmacTarget container

      target->convertMountToEncoder(model_.Encoder(Axis::AZ), 
				    pmac.PmacAxis(Axis::AZ), 
				    current->az_.topo_);
      target1->convertMountToEncoder(model_.Encoder(Axis::AZ), 
				    pmac1.PmacAxis(Axis::AZ), 
				    current->az_.topo_);
      target2->convertMountToEncoder(model_.Encoder(Axis::AZ), 
				    pmac2.PmacAxis(Axis::AZ), 
				    current->az_.topo_);
      if(mode == PmacMode::SLEW)
	pmac.PmacAxis(Axis::AZ)->
	  setRate(model_.Encoder(Axis::AZ)->getSlewRate());
	pmac1.PmacAxis(Axis::AZ)->
	  setRate(model_.Encoder(Axis::AZ)->getSlewRate());
	pmac2.PmacAxis(Axis::AZ)->
	  setRate(model_.Encoder(Axis::AZ)->getSlewRate());
      
    } else {
      target->Position(Pointing::MOUNT_ANGLES)->
	set(Axis::AZ, current->az_.topo_);
      target->Position(Pointing::MOUNT_RATES)->
	set(Axis::AZ, 0);

      target1->Position(Pointing::MOUNT_ANGLES)->
	set(Axis::AZ, current->az_.topo_);
      target2->Position(Pointing::MOUNT_RATES)->
	set(Axis::AZ, 0);

      target1->Position(Pointing::MOUNT_ANGLES)->
	set(Axis::AZ, current->az_.topo_);
      target2->Position(Pointing::MOUNT_RATES)->
	set(Axis::AZ, 0);

      pmac.PmacAxis(Axis::AZ)->setCount(current->az_.count_);
      pmac.PmacAxis(Axis::PA)->setRadians(current->az_.topo_);
      pmac.PmacAxis(Axis::AZ)->setRate(0);
      pmac.PmacAxis(Axis::AZ)->setRadians(current->az_.topo_);

      pmac1.PmacAxis(Axis::AZ)->setCount(current->az_.count_);
      pmac1.PmacAxis(Axis::PA)->setRadians(current->az_.topo_);
      pmac1.PmacAxis(Axis::AZ)->setRate(0);
      pmac1.PmacAxis(Axis::AZ)->setRadians(current->az_.topo_);

      pmac2.PmacAxis(Axis::AZ)->setCount(current->az_.count_);
      pmac2.PmacAxis(Axis::PA)->setRadians(current->az_.topo_);
      pmac2.PmacAxis(Axis::AZ)->setRate(0);
      pmac2.PmacAxis(Axis::AZ)->setRadians(current->az_.topo_);
    };
    
    // Check the EL axis
    
    if(target->includesAxis(Axis::EL)) {
      target->convertMountToEncoder(model_.Encoder(Axis::EL), 
				    pmac.PmacAxis(Axis::EL), 
				    current->el_.topo_);

      target1->convertMountToEncoder(model_.Encoder(Axis::EL), 
				    pmac1.PmacAxis(Axis::EL), 
				    current->el_.topo_);

      target2->convertMountToEncoder(model_.Encoder(Axis::EL), 
				    pmac2.PmacAxis(Axis::EL), 
				    current->el_.topo_);

      if(mode == PmacMode::SLEW)
	pmac.PmacAxis(Axis::EL)->
	  setRate(model_.Encoder(Axis::EL)->getSlewRate());
	pmac1.PmacAxis(Axis::EL)->
	  setRate(model_.Encoder(Axis::EL)->getSlewRate());
	pmac2.PmacAxis(Axis::EL)->
	  setRate(model_.Encoder(Axis::EL)->getSlewRate());

    } else {

      target->Position(Pointing::MOUNT_ANGLES)->
	set(Axis::EL, current->el_.topo_);
      target->Position(Pointing::MOUNT_RATES)->
	set(Axis::EL, 0);

      target1->Position(Pointing::MOUNT_ANGLES)->
	set(Axis::EL, current->el_.topo_);
      target1->Position(Pointing::MOUNT_RATES)->
	set(Axis::EL, 0);
      
      target2->Position(Pointing::MOUNT_ANGLES)->
	set(Axis::EL, current->el_.topo_);
      target2->Position(Pointing::MOUNT_RATES)->
	set(Axis::EL, 0);

      pmac.PmacAxis(Axis::EL)->setCount(current->el_.count_);
      pmac.PmacAxis(Axis::PA)->setRadians(current->el_.topo_);
      pmac.PmacAxis(Axis::EL)->setRate(0);
      pmac.PmacAxis(Axis::EL)->setRadians(current->el_.topo_);

      pmac1.PmacAxis(Axis::EL)->setCount(current->el_.count_);
      pmac1.PmacAxis(Axis::PA)->setRadians(current->el_.topo_);
      pmac1.PmacAxis(Axis::EL)->setRate(0);
      pmac1.PmacAxis(Axis::EL)->setRadians(current->el_.topo_);

      pmac2.PmacAxis(Axis::EL)->setCount(current->el_.count_);
      pmac2.PmacAxis(Axis::PA)->setRadians(current->el_.topo_);
      pmac2.PmacAxis(Axis::EL)->setRate(0);
      pmac2.PmacAxis(Axis::EL)->setRadians(current->el_.topo_);
    };
    
    // Check the PA axis
    
    if(target->includesAxis(Axis::PA)) {
      target->convertMountToEncoder(model_.Encoder(Axis::PA), 
				    pmac.PmacAxis(Axis::PA),
				    current->pa_.topo_);

      target1->convertMountToEncoder(model_.Encoder(Axis::PA), 
				    pmac1.PmacAxis(Axis::PA),
				    current->pa_.topo_);

      target2->convertMountToEncoder(model_.Encoder(Axis::PA), 
				    pmac2.PmacAxis(Axis::PA),
				    current->pa_.topo_);

      if(mode == PmacMode::SLEW)
	pmac.PmacAxis(Axis::PA)->
	  setRate(model_.Encoder(Axis::PA)->getSlewRate());
	pmac1.PmacAxis(Axis::PA)->
	  setRate(model_.Encoder(Axis::PA)->getSlewRate());
	pmac2.PmacAxis(Axis::PA)->
	  setRate(model_.Encoder(Axis::PA)->getSlewRate());

    } else {

      target->Position(Pointing::MOUNT_ANGLES)->
	set(Axis::PA, current->pa_.topo_);
      target->Position(Pointing::MOUNT_RATES)->
	set(Axis::PA, 0);

      target1->Position(Pointing::MOUNT_ANGLES)->
	set(Axis::PA, current->pa_.topo_);
      target1->Position(Pointing::MOUNT_RATES)->
	set(Axis::PA, 0);

      target2->Position(Pointing::MOUNT_ANGLES)->
	set(Axis::PA, current->pa_.topo_);
      target2->Position(Pointing::MOUNT_RATES)->
	set(Axis::PA, 0);
      
      pmac.PmacAxis(Axis::PA)->setCount(current->pa_.count_);
      pmac.PmacAxis(Axis::PA)->setRadians(current->pa_.topo_);
      pmac.PmacAxis(Axis::PA)->setRate(0);
      pmac.PmacAxis(Axis::PA)->setRadians(current->pa_.topo_);

      pmac1.PmacAxis(Axis::PA)->setCount(current->pa_.count_);
      pmac1.PmacAxis(Axis::PA)->setRadians(current->pa_.topo_);
      pmac1.PmacAxis(Axis::PA)->setRate(0);
      pmac1.PmacAxis(Axis::PA)->setRadians(current->pa_.topo_);

      pmac2.PmacAxis(Axis::PA)->setCount(current->pa_.count_);
      pmac2.PmacAxis(Axis::PA)->setRadians(current->pa_.topo_);
      pmac2.PmacAxis(Axis::PA)->setRate(0);
      pmac2.PmacAxis(Axis::PA)->setRadians(current->pa_.topo_);
    };
    break;

  default:
    {
      ThrowError("Tracker::pmacNewPosition: Unknown mode.\n");
    }
    break;
  };

  cout.setf(ios::internal);

  // Record the new pmac operating mode.
  
  pmac.setMode(mode);
  pmac1.setMode(mode);
  pmac2.setMode(mode);

  // Set whether or not we are scanning
  
  pmac.setScanMode(scan_.currentState() != SCAN_INACTIVE);
  pmac1.setScanMode(scan_.currentState() != SCAN_INACTIVE);
  pmac2.setScanMode(scan_.currentState() != SCAN_INACTIVE);
  
  // Convert the commanded encoder counts back to topocentric mount
  // coordinates and record them for later comparison with the actual
  // telescope position. Note that the results can differ from
  // target->mount_angle because of the wrap regions.

  // First copy the last two commanded positions into the containers
  // for the previous two commanded positions

  tertCommanded_ = prevCommanded_;
  prevCommanded_ = lastCommanded_;

  // Now update the last commanded position to the current

  lastCommanded_.az_ = model_.Encoder(Axis::AZ)->
    convertCountsToSky(pmac.az_.count_);

  lastCommanded_.el_ = model_.Encoder(Axis::EL)->
    convertCountsToSky(pmac.el_.count_);

  lastCommanded_.pa_ = model_.Encoder(Axis::PA)->
    convertCountsToSky(pmac.pa_.count_);

  // If this command intiates a slew, set the prevCommand position to
  // the new expected position

  if(mode == PmacMode::SLEW) {
    prevCommanded_ = lastCommanded_;
    tertCommanded_ = lastCommanded_;
  }

  // Write the new values into the dual-port ram, then tell the PMAC
  // to read them by setting its new_position flag.

  servo_->commandNewPosition(&pmac, &pmac1, &pmac2, mjd);

  // Record details of the pointing in the archive.

  tracker_->archivePointing(&archived_, 
			    &atmos_, 
			    &model_, 
			    &pmac, 
			    &nextTarget_, 
			    &site_, 
			    &offset_,
			    &scan_);

  share_->switchBuffers();
}

/**.......................................................................
 * Arrange to slew the telescope to a given az,el,dk coordinate.
 *
 * Input:
 *   msg  TrackerMsg*  The message received on the Tracker message queue.
 */
void Tracker::slewTelescope(TrackerMsg* msg)
{
  // Record the control program transaction number.
  
  registerRequest(msg->body.slew.seq);
  
  // Set up the pointing parameters for the slew
  
  nextTarget_.setupForSlew(share_, msg);
  nextTarget1_.setupForSlew(share_, msg);
  nextTarget2_.setupForSlew(share_, msg);
  
  // Set current source name to the temporary slew source
  
  src_.reset();
  src_.setName(msg->body.slew.source);
  src_.setType(gcp::control::SRC_FIXED);
  src_.setAxis(gcp::util::Axis::BOTH, msg->body.slew.az, msg->body.slew.el, 0);
  
  // Queue the slew for the next 1-second boundary.
  
  //  whatNext_ = nextTarget_.isCenter() ? TARGET : SLEW;

  whatNext_ = TARGET;
}

/**.......................................................................
 * Arrange to halt the telescope. If the pmac new_position flag is set
 * this will be done immediately, otherwise it will be postponed to
 * the next 1-second tick.
 *
 * Input:
 *  msg   TrackerMsg  *  If the halt command was received from the
 *                         control program then this must contain
 *                         a transaction sequence number. Otherwise
 *                         send NULL.
 */
void Tracker::haltTelescope(TrackerMsg* msg)
{
  // If this command was received from the control program, record the
  // sequence
  
  DBPRINT(true, Debug::DEBUG4, "Got a HALT command");

  if(msg != 0)
    registerRequest(msg->body.halt.seq);
  
  // Zero the pointing flow parameters to start
  
  nextTarget_.setupForHalt(share_);
  nextTarget1_.setupForHalt(share_);
  nextTarget2_.setupForHalt(share_);
  
  // Queue the halt for the next 1-second boundary.
  
  whatNext_ = HALT;

  // Reset scan parameters

  scan_.setupForHalt();

  // Send a scan done message

  //  parent_->sendScanDoneMsg(scan_.lastReq());

  DBPRINT(true, Debug::DEBUG4, "Exiting HALT command");
}

/**.......................................................................
 * Extend the ephemeris of the current source.
 *
 * Input:
 *   msg  TrackerMsg*  The message received on the Tracker message queue.
 */
void Tracker::extendTrack(TrackerMsg* msg)
{
  // Convert to internal units.
  
  double mjd  = msg->body.track.mjd;

  HourAngle ra;
  ra.setRadians(msg->body.track.ra);

  DecAngle dec;
  dec.setRadians(msg->body.track.dec);

  double dist = msg->body.track.dist;

  // If this is the start of a new observation, discard legacy
  // positions of the previous source and arrange for the next call to
  // trk_update_pmac() to slew to the next source. Also arrange for
  // the start of the track to be reported.

  if(msg->body.track.seq) {
    
    src_.reset();
    src_.setName(msg->body.track.source);

    // The exact type of the source is not important for the antenna
    // (ie, SRC_J2000 vs. SRC_EPHEM), just that it knows this is not
    // an AZ/EL source

    src_.setType(gcp::control::SRC_EPHEM);
    
    whatNext_ = TARGET;
    
    // Record the control program transaction number.
    
    registerRequest(msg->body.track.seq);
  };

  // Extend the track of the current source.

  src_.extend(mjd, ra, dec, dist);

  nextTarget_.setupForTrack();
  nextTarget1_.setupForTrack();
  nextTarget2_.setupForTrack();
}

/**.......................................................................
 * Extend the ephemeris of the current scan
 *
 * Input:
 *   msg  TrackerMsg*  The message received on the Tracker message queue.
 */
void Tracker::extendScan(TrackerMsg* msg)
{
  // Do nothing if we are not currently tracking

  if(whatNext_ != SLAVE)
    return;

  // seq < 0 is a flag to reset the scan offsets.  Just do it and
  // return

  if(msg->body.scan.seq < 0) {
    scan_.reset();
    return;
  }

  /*
   * If this is the start of a new scan, discard legacy positions of
   * the previous scan and arrange for the start of the scan to be
   * reported.
   */
  if(msg->body.scan.seq) {

    scan_.reset();

    /**
     * Initialize the scan cache fixed members if this is the start of
     * a new scan.
     */
    scan_.initialize(msg->body.scan.name,
		     msg->body.scan.ibody, msg->body.scan.iend, 
		     msg->body.scan.nreps,
		     msg->body.scan.seq,
		     10, // MS per evaluation of the scan offsets
		     msg->body.scan.msPerSample,
		     msg->body.scan.add);

    /**
     * Let the pointing routines know that the current scan is active.
     */
    scan_.currentState() = SCAN_ACTIVE;
  };

  /*
   * Extend the track of the current scan.
   */
  scan_.extend(msg->body.scan.npt,
	       msg->body.scan.index,
	       msg->body.scan.flag,
	       msg->body.scan.azoff,
	       msg->body.scan.eloff);
}

/**.......................................................................
 * Adjust the tracking offsets of specified drive axes, and round them
 * into the range -pi..pi.
 *
 * Input:
 *   msg  TrackerMsg*  The message received on the Tracker message queue.
 */
void Tracker::setOffset(TrackerMsg* msg)
{
  // Record the sequence number of the incomplete transaction, for
  // subsequent use when reporting completion to the control program.

  registerRequest(msg->body.offset.seq);
  
  offset_.Offset(msg->body.offset.offset.type)->
    set(msg->body.offset.offset);
}

/**.......................................................................
 * Install new encoder offsets.
 * 
 * Input:
 *   msg  TrackerMsg*  The message received on the Tracker message 
 *                          queue. 
 */
void Tracker::setEncoderZero(TrackerMsg* msg)
{
  // This operation could break an ongoing track, so to give the user
  // an opportunity to wait to get back on source, the tracker will
  // report the completion of this update to the control program when
  // we next get on source. Record the sequence number of this update
  // to allow it to be sent with the completion message.

  registerRequest(msg->body.encoderZeros.seq);

  // Record the new values.
  
  model_.Encoder(Axis::AZ)->setZero(msg->body.encoderZeros.az);
  model_.Encoder(Axis::EL)->setZero(msg->body.encoderZeros.el);
  model_.Encoder(Axis::PA)->setZero(msg->body.encoderZeros.dk);
  
  lacking_  &= ~PointingParameter::ZEROS;
  archived_ &= ~PointingParameter::ZEROS;
  
  // Update the mount-angle limits recorded in trk->model.az,el,dk.
  
  updateMountLimits();
}

/**.......................................................................
 * Record new refraction coefficients received from the weather
 * station task.
 *
 * Input:
 *
 *  msg  TrackerMsg*  The message received on the Tracker message 
 *                     queue.
 */
void Tracker::updateRefraction(TrackerMsg* msg)
{

  // Get the appropriate refraction container.

  Refraction *r = atmos_.Refraction(msg->body.refraction.mode);

  // Record the new terms and mark them as usable.

  r->setUsable(true);
  r->setA(msg->body.refraction.a);
  r->setB(msg->body.refraction.b);

  // If the modified refraction coefficients are those that are
  // currently being used, mark them as available, but so far
  // un-archived.

  if(atmos_.isCurrent(r)) {
    lacking_  &= ~PointingParameter::ATMOSPHERE;
    archived_ &= ~PointingParameter::ATMOSPHERE;
  };
}

/**.......................................................................
 * Update the refraction
 */
void Tracker::updateRefraction()
{
  if(refracCalculator_.canComputeRefraction()) {

    gcp::util::Atmosphere::RefractionCoefficients coeff = 
      refracCalculator_.refractionCoefficients();
    
    // Get the appropriate refraction container.
   
    Refraction* r = atmos_.Refraction(gcp::util::PointingMode::RADIO);

    // Record the new terms and mark them as usable.

    r->setUsable(true);
    r->setA(coeff.a);
    r->setB(coeff.b);

    // If the modified refraction coefficients are those that are
    // currently being used, mark them as available, but so far
    // un-archived.
    
    if(atmos_.isCurrent(r)) {
      lacking_  &= ~PointingParameter::ATMOSPHERE;
      archived_ &= ~PointingParameter::ATMOSPHERE;
    };
  }

  if(refracCalculator_.canComputeOpticalRefraction()) {

    gcp::util::Atmosphere::RefractionCoefficients coeff = 
      refracCalculator_.opticalRefractionCoefficients();
    
    // Get the appropriate refraction container.
    
    Refraction* r = atmos_.Refraction(gcp::util::PointingMode::OPTICAL);

    // Record the new terms and mark them as usable.

    r->setUsable(true);
    r->setA(coeff.a);
    r->setB(coeff.b);

    // If the modified refraction coefficients are those that are
    // currently being used, mark them as available, but so far
    // un-archived.
    
    if(atmos_.isCurrent(r)) {
      lacking_  &= ~PointingParameter::ATMOSPHERE;
      archived_ &= ~PointingParameter::ATMOSPHERE;
    };
  }
}

/**.......................................................................
 * Update the equation-of-the-equinoxes interpolator.
 */
void Tracker::extendEqnEqx(TrackerMsg* msg)
{
  // Convert to internal units.

  double tt     = msg->body.extendEqnEqx.mjd;
  double eqneqx = msg->body.extendEqnEqx.eqneqx;

  // Append the new parameters to the associated quadratic interpolation
  // tables.

  share_->extendEqnEqx(tt, eqneqx);
  
  // Mark the EQN-EQX parameters as available, but so far un-archived.
  
  lacking_  &= ~PointingParameter::EQNEQX;
  archived_ &= ~PointingParameter::EQNEQX;

  archiveStatus();
}

/**.......................................................................
 * Install new slew rates.
 *
 * Input:
 *   msg  TrackerMsg*  The message received on the Tracker message 
 *                          queue.
 */
void Tracker::setSlewRate(TrackerMsg* msg)
{
  // This operation could break an ongoing track (eg if the pmac
  // rejects the rates), so to give the user an opportunity to wait to
  // get back on source, the tracker will report the completion of
  // this update to the control program when we next get on
  // source. Record the sequence number of this update to allow it to
  // be sent with the completion message.

  registerRequest(msg->body.slewRate.seq);

  // Record the new values.
  
  if(msg->body.slewRate.axes & Axis::AZ) {
    model_.Encoder(Axis::AZ)->setSlewRate(msg->body.slewRate.az);
  }

  if(msg->body.slewRate.axes & Axis::EL) {
    model_.Encoder(Axis::EL)->setSlewRate(msg->body.slewRate.el);
  }
}

/**.......................................................................
 * Record new encoder offsets and multipliers.
 *
 * Input:
 *  msg  TrackerMsg* The message received on the Tracker message 
 *                   queue.
 */
void Tracker::calEncoders(TrackerMsg* msg)
{
  // This operation could break an ongoing track, so to give the user
  // an opportunity to wait to get back on source, the tracker will
  // report the completion of this update to the control program when
  // we next get on source. Record the sequence number of this update
  // to allow it to be sent with the completion message.

  registerRequest(msg->body.encoderCountsPerTurn.seq);

 // Record the new values

  model_.Encoder(Axis::AZ)->
    setCountsPerTurn(abs(msg->body.encoderCountsPerTurn.az));

  model_.Encoder(Axis::EL)->
    setCountsPerTurn(abs(msg->body.encoderCountsPerTurn.el));

  model_.Encoder(Axis::AZ)->
    setCountsPerRadian(msg->body.encoderCountsPerTurn.az / twopi);

  model_.Encoder(Axis::EL)->
    setCountsPerRadian(msg->body.encoderCountsPerTurn.el / twopi);

  // Mark the encoder calibration parameters as available, but so far
  // un-archived.

  lacking_  &= ~PointingParameter::ENCODERS;
  archived_ &= ~PointingParameter::ENCODERS;

  // Update the mount-angle limits recorded in model.az,el,dk.
  
  updateMountLimits();
}

/**.......................................................................
 * Record new encoder limits.
 *
 * Input:
 *
 *   msg TrackerMsg* The message received on the Tracker 
 *                        message queue.
 */
void Tracker::recordEncoderLimits(TrackerMsg* msg)
{
  // This operation could break an ongoing track, so to give the user
  // an opportunity to wait to get back on source, the tracker will
  // report the completion of this update to the control program when
  // we next get on source. Record the sequence number of this update
  // to allow it to be sent with the completion message.

  registerRequest(msg->body.encoderLimits.seq);

  // define the angles as angles:
  gcp::util::Angle az_min;
  gcp::util::Angle az_max;
  gcp::util::Angle el_min;
  gcp::util::Angle el_max;
  az_min.setDegrees(msg->body.encoderLimits.az_min);
  az_max.setDegrees(msg->body.encoderLimits.az_max);
  el_min.setDegrees(msg->body.encoderLimits.el_min);
  el_max.setDegrees(msg->body.encoderLimits.el_max);

  // setting limits to those in pointing.init, in degrees.
  model_.Encoder(Axis::AZ)->
    setLimits(az_min,az_max);

  model_.Encoder(Axis::EL)->
    setLimits(el_min,el_max);

  // Mark the limits as available, but so far un-archived.
  
  lacking_  &= ~PointingParameter::LIMITS;
  archived_ &= ~PointingParameter::LIMITS;
  
  // Update the mount-angle limits recorded in trk->model.az,el,dk.
  
  updateMountLimits();
}


/**.......................................................................
 * Update the year. Note that the time code reader doesn't supply the
 * year, so the year has to be provided by the control program.
 *
 * Input:
 *
 *   msg TrackerMsg* The message received on the Tracker 
 *                        message queue.
 */
void Tracker::changeYear(TrackerMsg* msg)
{
  // Record a specified new year?

  if(msg != 0) {
    year_ = msg->body.year.year;

    // At startup this function is called with msg==NULL. At that
    // point the control system clock has been initialized from the
    // Sun's clock, so the year can be extracted from it.

  } else {
    Date date;        // The broken down Gregorian version of 'utc' 
    date.convertMjdUtcToDate(share_->getUtc());
    year_ = date.getYear();
  };
}

/**.......................................................................
 * Update the local and system-wide site-location parameters.
 *
 * Input:
 *
 *   msg TrackerMsg* The message received on the Tracker 
 *                        message queue.
 */
void Tracker::locateSite(TrackerMsg* msg)
{
  Angle lon, lat;
  lon.setRadians(msg->body.site.lon);
  lat.setRadians(msg->body.site.lat);
  double altitude  = msg->body.site.alt;

  site_.setFiducial(lon, lat, altitude);

  // Update relevant site parameters in the refraction calculator

  refracCalculator_.setAltitude(site_.altitude());
  refracCalculator_.setLatitude(site_.latitude());

  share_->setSite(site_.getLongitude(), site_.getLatitude(), 
  		  site_.getAltitude());

  // Mark the site-info as available, but so far un-archived.
  
  lacking_  &= ~PointingParameter::SITE;
  archived_ &= ~PointingParameter::SITE;
}

/**.......................................................................
 * Update the local and system-wide site-location parameters.
 *
 * Input:
 *
 *   msg TrackerMsg* The message received on the Tracker 
 *                        message queue.
 */
void Tracker::locateAntenna(TrackerMsg* msg)
{
  double up    = msg->body.location.up;
  double east  = msg->body.location.east;
  double north = msg->body.location.north;

  site_.setOffset(up, east, north);

  // Update relevant site parameters in the refraction calculator

  refracCalculator_.setAltitude(site_.altitude());
  refracCalculator_.setLatitude(site_.latitude());

  // Mark the location-info as available, but so far un-archived.
  
  lacking_  &= ~PointingParameter::LOCATION;
  archived_ &= ~PointingParameter::LOCATION;

  // Mark the site as out of date too

  lacking_  &= ~PointingParameter::SITE;
  archived_ &= ~PointingParameter::SITE;
}

/**.......................................................................
 * Update the UT1-UTC interpolator.
 *
 * Input:
 *   msg TrackerMsg* The message received on the Tracker 
 *                        message queue.
 */
void Tracker::extendUt1Utc(TrackerMsg* msg)
{
  // Convert to internal units.

  double utc    = msg->body.extendUt1Utc.mjd;
  double ut1utc = msg->body.extendUt1Utc.ut1utc;

  // Append the new parameters to the associated quadratic
  // interpolation tables.

  share_->extendUt1Utc(utc, ut1utc);
  
  // Mark the UT1-UTC parameters as available, but so far un-archived.
  
  lacking_  &= ~PointingParameter::UT1UTC;
  archived_ &= ~PointingParameter::UT1UTC;

  archiveStatus();
}

/**.......................................................................
 * Update the mount-angle limits that correspond to revised values of
 * model.{az,el,dk}.{per_turn,per_radian,zero,min,max}, and mark them
 * as unarchived.
 */
void Tracker::updateMountLimits()
{
  // Do we have enough information to do this yet?

  if(!(lacking_ & (PointingParameter::ENCODERS | 
		   PointingParameter::ZEROS | 
		   PointingParameter::LIMITS)))
    model_.updateMountLimits();
  
  // Mark the archived limits as stale.
  
  archived_ &= ~PointingParameter::LIMITS;
}

/**.......................................................................
 * Compute the encoder positions and rates needed to track a given
 * source at a given utc.
 *
 * Input:
 *  mjd               int    The day component of the MJD utc at which
 *                           coordinates are required.
 *  sec               int    The MJD utc time of day, in seconds.
 */
void Tracker::sourcePosition(TimeVal& mjd)
{
  // Record the time stamp of the coordinates.

  double utc = mjd.getTimeInMjdDays();
  TimeVal mjd1 = mjd; mjd1.incrementSeconds(1.0);
  TimeVal mjd2 = mjd; mjd2.incrementSeconds(2.0);

  double utc1 = mjd1.getTimeInMjdDays();
  double utc2 = mjd2.getTimeInMjdDays();


  nextTarget_.setTime(utc);
  nextTarget1_.setTime(utc1);
  nextTarget2_.setTime(utc2);

  if(nextTarget_.isCenter())
    sourcePositionCenter(mjd);
  else
    sourcePositionTrack(mjd, utc);
}

/**.......................................................................
 * Compute the encoder positions and rates needed to track a given
 * source at a given utc.
 *
 * Input:
 *  mjd               int    The day component of the MJD utc at which
 *                           coordinates are required.
 *  target to modify
 *  sec               int    The MJD utc time of day, in seconds.
 */
void Tracker::sourcePosition(TimeVal& mjd, Pointing& thisTargetVal)
{
  // Record the time stamp of the coordinates.

  double utc = mjd.getTimeInMjdDays();

  thisTargetVal.setTime(utc);

  if(thisTargetVal.isCenter())
    sourcePositionCenter(mjd, thisTargetVal);
  else
    sourcePositionTrack(mjd, utc, thisTargetVal);
}


/**
 *
 */
void Tracker::sourcePositionCenter(TimeVal& mjd) 
{
  finalizePointing(mjd);
}

/**
 *
 */
void Tracker::sourcePositionCenter(TimeVal& mjd, Pointing& thisTargetVal) 
{
  finalizePointing(mjd, thisTargetVal);
}

/**
 *
 */
void Tracker::sourcePositionTrack(TimeVal& mjd, double utc) 
{
  PointingCorrections f;  // The current apparent az/el pointing 
  double pmra;    // The proper motion in Right Ascension (radians/sec) 
  double pmdec;   // The proper motion in Declination (radians/sec) 

  // Get the Terrestrial time that corresponds to 'utc'.

  double tt = share_->getTt(utc);
  
  // Determine the local apparent sidereal time.

  double lst = share_->getLst(utc);

  if(tt < 0 || lst < 0) {
    ThrowError("Tracker::sourcePosition: Illegal time received.\n");
  }
  
  // Store pointers to our internal objects
  
  gcp::util::Source* src = &src_;
  Pointing* p = &nextTarget_;
  
  // Record the time stamp of the coordinates.
  
  p->setTime(utc);
  
  // We want all axes to be commanded.
  
  p->setAxes(Axis::BOTH);
  
  // Get the location of this antenna.
  
  site_.updateLatitude(&f);
  
  // Record the name of the source.
  
  p->setSrcName(src->getName());
  
  // Interpolate for the geocentric apparent ra,dec and distance of
  // the source. Also add any equatorial tracking offsets that the
  // user has requested.

  p->setRa(src->getRa(tt).radians() + offset_.EquatOffset()->ra_);
  p->setDec(src->getDec(tt).radians() + offset_.EquatOffset()->dec_);

  double dist = src->getDist(tt);
  p->setDist(dist);
 
  // Estimate the proper motion of the source.
  
  pmra  = src->getGradRa(tt).radians() / daysec;
  pmdec = src->getGradDec(tt).radians() / daysec;
  
  // Compute the geocentric azimuth and elevation and record them in p.
  
  p->computeGeocentricPosition(lst, &f);
  
  // Account for horizontal parallax.
  
  site_.applyParallax(dist, &f);
  
  // Account for atmospheric refraction.
  
  double refraction = atmos_.applyRefraction(&f);
  p->setRefraction(refraction);
  
  // Account for diurnal aberration.
  
  site_.applyDiurnalAberration(&f);
  
  Position* position = p->Position(Pointing::TOPOCENTRIC);
  position->set(f.az, f.el, f.pa);
  
  // Correct for telescope flexure.
  
  model_.applyFlexure(&f);
  
  // Correct for the inevitable misalignment of the azimuth axis.

  model_.AxisTilt(Axis::AZ)->apply(&f);
  
  // Correct for misalignment of the elevation axis.
  
  model_.AxisTilt(Axis::EL)->apply(&f);
  
  // Correct for collimation errors.
  
  model_.applyCollimation(&f, offset_);
  
  // Add in any sky offset.
  
  offset_.SkyOffset()->apply(&f);
  
  // Get the final az,el,pa and associated drive rates.
  
  finalizePointing(pmra, pmdec, &f, p, mjd);
}

void Tracker::sourcePositionTrack(TimeVal& mjd, double utc, Pointing& thisTargetVal) 
{
  PointingCorrections f;  // The current apparent az/el pointing 
  double pmra;    // The proper motion in Right Ascension (radians/sec) 
  double pmdec;   // The proper motion in Declination (radians/sec) 

  // Get the Terrestrial time that corresponds to 'utc'.

  double tt = share_->getTt(utc);
  
  // Determine the local apparent sidereal time.

  double lst = share_->getLst(utc);

  if(tt < 0 || lst < 0) {
    ThrowError("Tracker::sourcePosition: Illegal time received.\n");
  }
  
  // Store pointers to our internal objects
  
  gcp::util::Source* src = &src_;
  Pointing* p = &thisTargetVal;
  
  // Record the time stamp of the coordinates.
  
  p->setTime(utc);
  
  // We want all axes to be commanded.
  
  p->setAxes(Axis::BOTH);
  
  // Get the location of this antenna.
  
  site_.updateLatitude(&f);
  
  // Record the name of the source.
  
  p->setSrcName(src->getName());
  
  // Interpolate for the geocentric apparent ra,dec and distance of
  // the source. Also add any equatorial tracking offsets that the
  // user has requested.

  p->setRa(src->getRa(tt).radians() + offset_.EquatOffset()->ra_);
  p->setDec(src->getDec(tt).radians() + offset_.EquatOffset()->dec_);

  double dist = src->getDist(tt);
  p->setDist(dist);
 
  // Estimate the proper motion of the source.
  
  pmra  = src->getGradRa(tt).radians() / daysec;
  pmdec = src->getGradDec(tt).radians() / daysec;
  
  // Compute the geocentric azimuth and elevation and record them in p.
  
  p->computeGeocentricPosition(lst, &f);
  
  // Account for horizontal parallax.
  
  site_.applyParallax(dist, &f);
  
  // Account for atmospheric refraction.
  
  double refraction = atmos_.applyRefraction(&f);
  p->setRefraction(refraction);
  
  // Account for diurnal aberration.
  
  site_.applyDiurnalAberration(&f);
  
  Position* position = p->Position(Pointing::TOPOCENTRIC);
  position->set(f.az, f.el, f.pa);
  
  // Correct for telescope flexure.
  
  model_.applyFlexure(&f);
  
  // Correct for the inevitable misalignment of the azimuth axis.

  model_.AxisTilt(Axis::AZ)->apply(&f);
  
  // Correct for misalignment of the elevation axis.
  
  model_.AxisTilt(Axis::EL)->apply(&f);
  
  // Correct for collimation errors.
  
  model_.applyCollimation(&f, offset_);
  
  // Add in any sky offset.
  
  offset_.SkyOffset()->apply(&f);
  
  // Get the final az,el,pa and associated drive rates.
  
  finalizePointing(pmra, pmdec, &f, p, mjd, thisTargetVal);
}

/**.......................................................................
 * Compute the final az,el,pa and associated drive rates.
 *
 * Input:
 * 
 *  pmra     double                 The proper motion in Right Ascension 
 *                                  (radians/sec).
 *  pmdec    double                 The proper motion in Declination 
 *                                  (radians/sec).
 *  f        PointingCorrections *  The corrected az,el and latitude.
 */
void Tracker::finalizePointing(gcp::util::TimeVal& mjd)
{
  // Get pointers to the mount angles and rates
  
  nextTarget_.resetAngles();

  Position* mount = nextTarget_.Position(Pointing::MOUNT_ANGLES);
  Position* rates = nextTarget_.Position(Pointing::MOUNT_RATES);
  
  // Get the pointing model.
  
  Model* model = &model_;
  
  // Get user-supplied tracking offsets.
  
  TrackerOffset* offset = &offset_;
  
  // Get the next set of scan offsets

  ScanCacheOffset& scanOffset = scan_.nextOffsetTimeJump(mjd);

  // Record the name of the scan

  strcpy((char*)nextTarget_.getScanName(), (char*)scan_.name());

  // Compute the azimuth, elevation and parallactic angle rotation
  // rates.  If we are at the zenith the az and pa rates will be
  // infinite, so set the rates to 0 to tell pmac_new_position() that
  // a slew will be needed to reacquire the source position.

  rates->set(Axis::EL, scanOffset.elrate);
  rates->set(Axis::AZ, scanOffset.azrate);

  // And add the AZ and EL offsets

  mount->increment(Axis::AZ, scanOffset.az);
  mount->increment(Axis::EL, scanOffset.el);

  // Apply user-supplied tracking offsets.  We can use increment()
  // here regardless of the deck mode, since the current deck position
  // will have been either incremented, or set above, with the current
  // scan offset, to which we should always add the tracking offset.

  mount->increment(offset_.MountOffset());
  mount->increment(offset_.SkyOffset());

  // If the scan just finished on this call, report its termination

  if(scan_.justFinished()) {
    parent_->sendScanDoneMsg(scan_.lastReq());
  }
}


void Tracker::finalizePointing(gcp::util::TimeVal& mjd, Pointing& thisTargetVal)
{
  // Get pointers to the mount angles and rates
  
  thisTargetVal.resetAngles();

  Position* mount = thisTargetVal.Position(Pointing::MOUNT_ANGLES);
  Position* rates = thisTargetVal.Position(Pointing::MOUNT_RATES);
  
  // Get the pointing model.
  
  Model* model = &model_;
  
  // Get user-supplied tracking offsets.
  
  TrackerOffset* offset = &offset_;
  
  // Get the next set of scan offsets

  ScanCacheOffset& scanOffset = scan_.nextOffsetTimeJump(mjd);

  // Record the name of the scan

  strcpy((char*)nextTarget_.getScanName(), (char*)scan_.name());

  // Compute the azimuth, elevation and parallactic angle rotation
  // rates.  If we are at the zenith the az and pa rates will be
  // infinite, so set the rates to 0 to tell pmac_new_position() that
  // a slew will be needed to reacquire the source position.

  rates->set(Axis::EL, scanOffset.elrate);
  rates->set(Axis::AZ, scanOffset.azrate);

  // And add the AZ and EL offsets

  mount->increment(Axis::AZ, scanOffset.az);
  mount->increment(Axis::EL, scanOffset.el);

  // Apply user-supplied tracking offsets.  We can use increment()
  // here regardless of the deck mode, since the current deck position
  // will have been either incremented, or set above, with the current
  // scan offset, to which we should always add the tracking offset.

  mount->increment(offset_.MountOffset());
  mount->increment(offset_.SkyOffset());

  // If the scan just finished on this call, report its termination

  if(scan_.justFinished()) {
    parent_->sendScanDoneMsg(scan_.lastReq());
  }
}

/**.......................................................................
 * Compute the final az,el,pa and associated drive rates.
 *
 * Input:
 * 
 *  pmra     double                 The proper motion in Right Ascension 
 *                                  (radians/sec).
 *  pmdec    double                 The proper motion in Declination 
 *                                  (radians/sec).
 *  f        PointingCorrections *  The corrected az,el and latitude.
 */
void Tracker::finalizePointing(double pmra, double pmdec, 
			       PointingCorrections *f,
			       Pointing* p, gcp::util::TimeVal& mjd)
{
  // Get pointers to the mount angles and rates
  
  Position* mount = nextTarget_.Position(Pointing::MOUNT_ANGLES);
  Position* rates = nextTarget_.Position(Pointing::MOUNT_RATES);
  
  // Get the pointing model.
  
  Model* model = &model_;
  
  // Get user-supplied tracking offsets.
  
  TrackerOffset* offset = &offset_;
  
  // Get the next set of scan offsets

  ScanCacheOffset& scanOffset = scan_.nextOffsetTimeJump(mjd);

  // Get local copies of cached values.
  
  double cos_el  = f->cos_el;
  double sin_el  = f->sin_el;
  double cos_az  = f->cos_az;
  double sin_az  = f->sin_az;
  double cos_lat = f->cos_lat;
  double sin_lat = f->sin_lat;
  
  // Compute the rate of change of hour angle wrt sidereal time in
  // radians of sidereal time per second of UT1.
  
  double dhdt = rot_to_ut * twopi / daysec - pmra;
  
  // Precompute pertinent spherical trig equations.
  
  double cos_dec_cos_pa = sin_lat * cos_el - cos_lat * sin_el * cos_az;
  double sin_az_cos_lat = sin_az * cos_lat;
  double sin_dec = sin_lat * sin_el + cos_lat * cos_el * cos_az;
  
  // Record the name of the scan

  strcpy((char*)p->getScanName(), (char*)scan_.name());

  // Record the already computed azimuth, elevation and parallactic
  // values.
  
  mount->set(f->az, f->el, f->pa);
  
  // Compute the azimuth, elevation and parallactic angle rotation
  // rates.  If we are at the zenith the az and pa rates will be
  // infinite, so set the rates to 0 to tell pmac_new_position() that
  // a slew will be needed to reacquire the source position.

  if(cos_el != 0.0) {
    double cos_pa = cos(mount->pa_);
    double sin_pa = sin(mount->pa_);
    rates->set(Axis::EL, dhdt * sin_az_cos_lat + pmdec * cos_pa + scanOffset.elrate);

    rates->set(Axis::AZ, (dhdt * cos_dec_cos_pa + pmdec * sin_pa) / cos_el + scanOffset.azrate);

    rates->set(Axis::PA, (dhdt * -cos_lat * cos_az + pmdec*sin_pa*sin_el) / 
	       cos_el - pmdec * sin_dec * sin_pa * sin_pa + scanOffset.dkrate);
  } else {
    rates->set(0.0, 0.0, 0.0);
  };

  // Update the az and el pointing offsets to include any new offsets
  // measured by the user from the TV monitor of the optical-pointing
  // telescope.

  offset_.mergeTvOffset(f);
  
  // And add the AZ and EL offsets

  mount->increment(Axis::AZ, scanOffset.az);
  mount->increment(Axis::EL, scanOffset.el);

  // Apply user-supplied tracking offsets.  We can use increment()
  // here regardless of the deck mode, since the current deck position
  // will have been either incremented, or set above, with the current
  // scan offset, to which we should always add the tracking offset.

  mount->increment(offset_.MountOffset());
  mount->increment(offset_.SkyOffset());

  // If the scan just finished on this call, report its termination

  if(scan_.justFinished())
    parent_->sendScanDoneMsg(scan_.lastReq());
}

void Tracker::finalizePointing(double pmra, double pmdec, 
			       PointingCorrections *f,
			       Pointing* p, gcp::util::TimeVal& mjd, Pointing& thisTargetVal)
{
  // Get pointers to the mount angles and rates
  
  Position* mount = thisTargetVal.Position(Pointing::MOUNT_ANGLES);
  Position* rates = thisTargetVal.Position(Pointing::MOUNT_RATES);
  
  // Get the pointing model.
  
  Model* model = &model_;
  
  // Get user-supplied tracking offsets.
  
  TrackerOffset* offset = &offset_;
  
  // Get the next set of scan offsets

  ScanCacheOffset& scanOffset = scan_.nextOffsetTimeJump(mjd);

  // Get local copies of cached values.
  
  double cos_el  = f->cos_el;
  double sin_el  = f->sin_el;
  double cos_az  = f->cos_az;
  double sin_az  = f->sin_az;
  double cos_lat = f->cos_lat;
  double sin_lat = f->sin_lat;
  
  // Compute the rate of change of hour angle wrt sidereal time in
  // radians of sidereal time per second of UT1.
  
  double dhdt = rot_to_ut * twopi / daysec - pmra;
  
  // Precompute pertinent spherical trig equations.
  
  double cos_dec_cos_pa = sin_lat * cos_el - cos_lat * sin_el * cos_az;
  double sin_az_cos_lat = sin_az * cos_lat;
  double sin_dec = sin_lat * sin_el + cos_lat * cos_el * cos_az;
  
  // Record the name of the scan

  strcpy((char*)p->getScanName(), (char*)scan_.name());

  // Record the already computed azimuth, elevation and parallactic
  // values.
  
  mount->set(f->az, f->el, f->pa);
  
  // Compute the azimuth, elevation and parallactic angle rotation
  // rates.  If we are at the zenith the az and pa rates will be
  // infinite, so set the rates to 0 to tell pmac_new_position() that
  // a slew will be needed to reacquire the source position.

  if(cos_el != 0.0) {
    double cos_pa = cos(mount->pa_);
    double sin_pa = sin(mount->pa_);
    rates->set(Axis::EL, dhdt * sin_az_cos_lat + pmdec * cos_pa + scanOffset.elrate);

    rates->set(Axis::AZ, (dhdt * cos_dec_cos_pa + pmdec * sin_pa) / cos_el + scanOffset.azrate);

    rates->set(Axis::PA, (dhdt * -cos_lat * cos_az + pmdec*sin_pa*sin_el) / 
	       cos_el - pmdec * sin_dec * sin_pa * sin_pa + scanOffset.dkrate);
  } else {
    rates->set(0.0, 0.0, 0.0);
  };

  // Update the az and el pointing offsets to include any new offsets
  // measured by the user from the TV monitor of the optical-pointing
  // telescope.

  offset_.mergeTvOffset(f);
  
  // And add the AZ and EL offsets

  mount->increment(Axis::AZ, scanOffset.az);
  mount->increment(Axis::EL, scanOffset.el);

  // Apply user-supplied tracking offsets.  We can use increment()
  // here regardless of the deck mode, since the current deck position
  // will have been either incremented, or set above, with the current
  // scan offset, to which we should always add the tracking offset.

  mount->increment(offset_.MountOffset());
  mount->increment(offset_.SkyOffset());

  // If the scan just finished on this call, report its termination

  if(scan_.justFinished())
    parent_->sendScanDoneMsg(scan_.lastReq());
}

/**.......................................................................
 * Register the receipt of a control-program command that needs to be
 * acknowledged when has taken effect. Also mark the tracking parameters
 * as modified.
 *
 * Input:
 *  seq   unsigned    The sequence number assigned to the command by
 *                    the control program.
 */
void Tracker::registerRequest(unsigned seq)
{
  lastReq_ = seq;
  paramsUpdated_ = true;
}

/**.......................................................................
 * Record the current tracking status (trk->new_status) in the archive
 * database and in trk->old_status.
 */
void Tracker::archiveStatus()
{
  unsigned state = (int) newStatus_;
  unsigned off_source =
    (newStatus_ != TrackingStatus::TRACKING || !pmacTracking_) ? 1 : 0;
  
  // Record the new tracking status.
  
  tracker_->archiveStatus(state, off_source, lacking_);
  
  // Keep a record of the status for comparison with that of the next tick.
  
  oldStatus_ = newStatus_;
}

/**.......................................................................
 * Process a message received on the Tracker message queue
 *
 * Input:
 *
 *   msg TrackerMsg* The message received on the Tracker 
 *                   message queue.
 */
void Tracker::processMsg(TrackerMsg* msg)
{
  switch (msg->type) {

  case TrackerMsg::COLLIMATION:
    calCollimation(msg);
    break;

  case TrackerMsg::CONNECT_DRIVE:
    connectPmac();
    break;

  case TrackerMsg::ENCODER_CALS:
    calEncoders(msg);
    break;

  case TrackerMsg::ENCODER_LIMITS:
    recordEncoderLimits(msg);
    break;

  case TrackerMsg::ENCODER_ZEROS:
    setEncoderZero(msg);
    break;

  case TrackerMsg::EXTEND_EQNEQX:
    extendEqnEqx(msg);
    break;

  case TrackerMsg::EXTEND_UT1UTC:
    extendUt1Utc(msg);
    break;

  case TrackerMsg::FLEXURE:
    calFlexure(msg);
    break;

  case TrackerMsg::HALT:
    haltTelescope(msg);
    break;

  case TrackerMsg::OFFSET:
    setOffset(msg);
    break;

  case TrackerMsg::REBOOT_DRIVE:
    rebootDrive(msg);
    break;

  case TrackerMsg::SHUTDOWN:
    break;

  case TrackerMsg::SELECT_MODEL:
    selectModel(msg);
    break;

  case TrackerMsg::REFRACTION:
    updateRefraction(msg);
    break;    

  case TrackerMsg::LOCATION:
    locateAntenna(msg);
    break;

  case TrackerMsg::SCAN:
    extendScan(msg);
    break;

  case TrackerMsg::SITE:
    locateSite(msg);
    break;

  case TrackerMsg::SLEW:
    slewTelescope(msg);
    break;

  case TrackerMsg::SLEWRATE:
    setSlewRate(msg);
    break;

  case TrackerMsg::STROBE_SERVO:
    //    CTOUT("strobing servo");
    strobeServo(msg);
    break;

  case TrackerMsg::TICK:
    {
      try {
	addTick(msg);
      } catch(...) {
	disconnectPmac();
      }
    }
    break;    

  case TrackerMsg::TILTS:
    calTilts(msg);
    break;

  case TrackerMsg::TRACK:
    extendTrack(msg);
    break;

  case TrackerMsg::YEAR:
    changeYear(msg);
    break;

  case TrackerMsg::SPECIFIC_SERVO_COMMAND:
    executeServoCmd(msg);
    break;

  case TrackerMsg::WEATHER:

    refracCalculator_.
      setAirTemperature(gcp::util::Temperature(gcp::util::Temperature::Kelvin(), 
					       msg->body.weather.airTemperatureInK));

    refracCalculator_.
      setHumidity(gcp::util::Percent(gcp::util::Percent::Max1(),
				     msg->body.weather.relativeHumidityInMax1));
    
    refracCalculator_.setPressure(gcp::util::Pressure(gcp::util::Pressure::MilliBar(), 
						      msg->body.weather.pressureInMbar));
    
    // Set the frequency to clear lacking flag
    
    refracCalculator_.setFrequency(gcp::util::Frequency(gcp::util::Frequency::GigaHz(), 5.0));

    updateRefraction();

    break;

  default:
    {
      ThrowError("Tracker::processMsg: Unrecognized message type" << msg->type);
    }
    break;
  }

}

/**.......................................................................
 * Attempt to connect to the pmac.
 */
void Tracker::connectPmac()
{
  // If we successfully connected, disable the connect timer.

  if(simPmac_ || servo_->connect())
    parent_->sendDriveConnectedMsg(true);
}

/**.......................................................................
 * This function will be called if an error occurs talking to the
 * pmac.
 */
void Tracker::disconnectPmac()
{
  servo_->disconnect();
  parent_->sendDriveConnectedMsg(false);
}


/**.......................................................................
 * Return true if we have enough information to roughly point the
 * telescope.
 */
bool Tracker::okToPoint() 
{
  return !(lacking_ &
	   (PointingParameter::SITE   | PointingParameter::UT1UTC   | 
	    PointingParameter::EQNEQX | PointingParameter::ENCODERS | 
	    PointingParameter::ZEROS  | PointingParameter::LIMITS));
}

/**.......................................................................
 * Arrange to reboot the drive.
 *
 * Input:
 *  msg    TrackerMsg *   If the halt command was received from the
 *                          control program then this must contain
 *                          a transaction sequence number. Otherwise
 *                          send NULL.
 */
void Tracker::rebootDrive(TrackerMsg* msg)
{
  // If this command was received from the control program, record its
  // transaction number.

  if(msg != 0)
    registerRequest(msg->body.rebootDrive.seq);

  // Set up pointing in preparation for the reboot

  nextTarget_.setupForReboot(share_);

  // Queue the halt for the next 1-second boundary.

  whatNext_ = REBOOT;
}

/**.......................................................................
 * Select between the optical and radio pointing models.
 */
void Tracker::selectModel(TrackerMsg* msg)
{
  // This operation could break an ongoing track, so to give the user
  // an opportunity to wait to get back on source, the tracker will
  // report the completion of this update to the control program when
  // we next get on source. Record the sequence number of this update
  // to allow it to be sent with the completion message.

  registerRequest(msg->body.selectModel.seq);

  // Select the associated collimation, flexure and refraction
  // parameters, all of which are model-dependent (optical or radio).
  // Additionally, the collimation and flexure terms are per-pointing
  // telescope

  model_.setCurrentCollimation(msg->body.selectModel.mode,
			       msg->body.selectModel.ptelMask);

  model_.setCurrentFlexure(msg->body.selectModel.mode,
			   msg->body.selectModel.ptelMask);

  // Refraction is the same for all optical telescopes, and depends
  // only on radio vs. optical

  atmos_.setCurrentRefraction(msg->body.selectModel.mode);

  // The new parameters need to be archived.

  archived_ &= ~(PointingParameter::COLLIMATION |
		 PointingParameter::FLEXURE |
		 PointingParameter::ATMOSPHERE);

  // Are the new refraction terms usable?

  Refraction* refrac = atmos_.currentRefraction();

  if(refrac->isUsable())
    lacking_ &= ~PointingParameter::ATMOSPHERE;
  else
    lacking_ |= PointingParameter::ATMOSPHERE;

  //-----------------------------------------------------------------------
  // Is at least one of the collimation terms usable?
  //-----------------------------------------------------------------------

  Collimation* fixedCollim = 
    model_.currentCollimation(gcp::util::Collimation::FIXED);

  if(fixedCollim->isUsable())
    lacking_ &= ~PointingParameter::COLLIMATION;
  else
    lacking_ |= PointingParameter::COLLIMATION;

  //-----------------------------------------------------------------------
  // Are the new flexure terms usable?
  //-----------------------------------------------------------------------

  Flexure* flexure = model_.currentFlexure();

  if(flexure->isUsable())
    lacking_ &= ~PointingParameter::FLEXURE;
  else
    lacking_ |= PointingParameter::FLEXURE;
}

/**.......................................................................
 * Calibrate the gravitational flexure of the telescope.
 */
void Tracker::calFlexure(TrackerMsg* msg)
{
  // This operation could break an ongoing track, so to give the user
  // an opportunity to wait to get back on source, the tracker will
  // report the completion of this update to the control program when
  // we next get on source. Record the sequence number of this update
  // to allow it to be sent with the completion message.

  registerRequest(msg->body.flexure.seq);
  
  if(msg->body.flexure.mode == gcp::util::PointingMode::OPTICAL) {

    for(unsigned iPtel=0; iPtel < PointingTelescopes::nPtel_; iPtel++) {
      
      PointingTelescopes::Ptel ptel = PointingTelescopes::intToPtel(iPtel);
      if(msg->body.flexure.ptelMask & ptel) {
	
	// Record the new flexure parameters for subsequent use.
	
	Flexure* flexure = model_.Flexure(msg->body.flexure.mode, ptel);
	
	flexure->setSineElFlexure(msg->body.flexure.sFlexure);
	flexure->setCosElFlexure(msg->body.flexure.cFlexure);
	flexure->setUsable(true);
	
	// If the modified flexure terms are the ones that are
	// currently being used, mark them as available, but so far
	// un-archived.
	
	if(model_.isCurrent(flexure)) {
	  lacking_  &= ~PointingParameter::FLEXURE;
	  archived_ &= ~PointingParameter::FLEXURE;
	}
      }
    }
  } else {

    // Record the new flexure parameters for subsequent use.
	
    Flexure* flexure = model_.Flexure(msg->body.flexure.mode);
    
    flexure->setSineElFlexure(msg->body.flexure.sFlexure);
    flexure->setCosElFlexure(msg->body.flexure.cFlexure);
    flexure->setUsable(true);
    
    // If the modified flexure terms are the ones that are
    // currently being used, mark them as available, but so far
    // un-archived.
    
    if(model_.isCurrent(flexure)) {
      lacking_  &= ~PointingParameter::FLEXURE;
      archived_ &= ~PointingParameter::FLEXURE;
    }
  }
}

/**.......................................................................
 * Calibrate the collimation of the telescope.
 */
void Tracker::calCollimation(TrackerMsg* msg)
{
  // This operation could break an ongoing track, so to give the user
  // an opportunity to wait to get back on source, the tracker will
  // report the completion of this update to the control program when
  // we next get on source. Record the sequence number of this update
  // to allow it to be sent with the completion message.

  registerRequest(msg->body.collimation.seq);

  if(msg->body.collimation.mode == gcp::util::PointingMode::OPTICAL) {

    for(unsigned iPtel=0; iPtel < PointingTelescopes::nPtel_; iPtel++) {
      
      PointingTelescopes::Ptel ptel = PointingTelescopes::intToPtel(iPtel);
      if(msg->body.collimation.ptelMask & ptel) {
	
	Collimation* c = model_.Collimation(msg->body.collimation.mode,
					    ptel,
					    msg->body.collimation.type);
	
	// Record the new parameters and mark them as usable.
	
	if(msg->body.collimation.type == gcp::util::Collimation::FIXED) {
	  
	  FixedCollimation* f = (FixedCollimation*) c;
	  
	  Angle x = Angle(Angle::Radians(), msg->body.collimation.x);
	  Angle y = Angle(Angle::Radians(), msg->body.collimation.y);
	  
	  if(msg->body.collimation.addMode == gcp::util::OffsetMsg::SET) {
	    f->setXOffset(x);
	    f->setYOffset(y);
	  } else {
	    f->incrXOffset(x);
	    f->incrYOffset(y);
	  }
	  
	  // If the modified collimation terms are the ones that are currently
	  // being used, mark them as available, but so far un-archived.
	  
	  if(model_.isCurrent(c, gcp::util::Collimation::FIXED)) {
	    lacking_  &= ~PointingParameter::COLLIMATION;
	    archived_ &= ~PointingParameter::COLLIMATION;
	  }
	} else {
	  ThrowError("Polar Collimation Not Supported");
	}
	
	// Mark the collimation for the current model as usable
	
	c->setUsable(true);
      }
    }
  } else {

    Collimation* c = model_.Collimation(msg->body.collimation.mode);
	
    // Record the new parameters and mark them as usable.
	
    if(msg->body.collimation.type == gcp::util::Collimation::FIXED) {
	  
      FixedCollimation* f = (FixedCollimation*) c;
	  
      Angle x = Angle(Angle::Radians(), msg->body.collimation.x);
      Angle y = Angle(Angle::Radians(), msg->body.collimation.y);
	  
      if(msg->body.collimation.addMode == gcp::util::OffsetMsg::SET) {
	f->setXOffset(x);
	f->setYOffset(y);
      } else {
	f->incrXOffset(x);
	f->incrYOffset(y);
      }
	  
      // If the modified collimation terms are the ones that are currently
      // being used, mark them as available, but so far un-archived.
	  
      if(model_.isCurrent(c, gcp::util::Collimation::FIXED)) {
	lacking_  &= ~PointingParameter::COLLIMATION;
	archived_ &= ~PointingParameter::COLLIMATION;
      }
    } else {
      ThrowError("Polar Collimation Not Supported");
    }
	
    // Mark the collimation for the current model as usable
	
    c->setUsable(true);
  }
}

/**.......................................................................
 * Calibrate the axis tilts of the telescope.
 */
void Tracker::calTilts(TrackerMsg* msg)
{
  // This operation could break an ongoing track, so to give the user
  // an opportunity to wait to get back on source, the tracker will
  // report the completion of this update to the control program when
  // we next get on source. Record the sequence number of this update
  // to allow it to be sent with the completion message.

  registerRequest(msg->body.tilts.seq);

  // Record the new parameters for subsequent use.

  AxisTilt* az = model_.AxisTilt(Axis::AZ);
  
  az->setHaTilt(msg->body.tilts.ha);
  az->setLatTilt(msg->body.tilts.lat);
  
  model_.AxisTilt(Axis::EL)->setTilt(msg->body.tilts.el);

  // Mark the tilt parameters as available, but so far un-archived.

  lacking_  &= ~PointingParameter::TILTS;
  archived_ &= ~PointingParameter::TILTS;
}

void Tracker::setNextState(NextDriveState state)
{
  whatNext_ = state;
}

void Tracker::strobeServo(TrackerMsg* msg)
{
  if(servo_->isSim()){
    return;
  }

  TimeVal currMjd;

  currMjd.setMjd(msg->body.strobe.mjdDays, msg->body.strobe.mjdSeconds, 0);
  //  COUT("in strobeServo");
  //  CTOUT("mjdays: " << msg->body.strobe.mjdDays);
  //  CTOUT("mjdSeconds: " << msg->body.strobe.mjdSeconds);

  try {
    /* do nothing if servo not connected */
    if(servo_->servoIsConnected()) {
      servo_->queryStatus(currMjd);
      currMjd.setMjd(msg->body.strobe.mjdDays, msg->body.strobe.mjdSeconds, 0);
      
      servo_->queryAntPositions(currMjd);

      // If the current state is IGNORE, halt
      // the telescope indicate that a valid position has now been
      // read
      if(whatNext_ == IGNORE){
	haltTelescope();
      }

      // if initialization complete, we check the status query results
      if(servo_->isInitialized()) {

	if(servo_->isLidOpen()) {
	  ReportMessage("Lid Open, Halting Antenna");
	  servo_->hardStopAntenna();
	}
	
	if(servo_->isBrakeOn()) {
	  ReportMessage("Brakes On, Halting Antenna");
	  servo_->hardStopAntenna();
	}

	if(servo_->isCircuitBreakerTripped()) {
	  ReportMessage("Breaker Tripped, Halting Antenna");
	  servo_->hardStopAntenna();
	}

	if(servo_->isThermalTripped()) {
	  ReportMessage("Thermal Tripped, Halting Antenna");
	  servo_->hardStopAntenna();
	}

      }
    }
  } catch(Exception& err) {
    COUT("Caught an exception in strobeServo(): " << err.what());
    disconnectPmac();
    whatNext_ = IGNORE;
  } catch(...) {
    COUT("Caught an unknown exception in strobeServo()");
    disconnectPmac();
    whatNext_ = IGNORE;
  }
}


bool Tracker::acknowledgeCompletion()
{
  if(lastAck_ < lastReq_) {
    parent_->sendDriveDoneMsg(lastReq_);
    lastAck_ = lastReq_;
    return true;
  };
  return false;
}


/**............................................................
 * Execute a Servo specific command
 */

void Tracker::executeServoCmd(TrackerMsg* msg)
{

  switch(msg->body.servoCmd.cmdId){
  case gcp::control::SERVO_ENGAGE:
    {
      std::vector<float> values(1);
      values[0] = msg->body.servoCmd.intVal;
      servo_->issueCommand(ServoCommandSa::SERVO_ENGAGE, values);
    }
  break;
    
  case gcp::control::SERVO_INITIALIZE_ANTENNA:
    {
      servo_->initializeAntenna();
    }
  break;
  
  case gcp::control::SERVO_ENABLE_CLUTCHES:
    {
      switch(msg->body.servoCmd.intVal){
      case 0:
	servo_->issueCommand(ServoCommandSa::CLUTCHES_OFF);
	break;

      case 1:
	servo_->issueCommand(ServoCommandSa::CLUTCHES_ON);
	break;	
      }
    }
  break;
  
  
  case gcp::control::SERVO_ENABLE_BRAKES:
    {
      switch( (int) msg->body.servoCmd.fltVal){
      case 1:
	// do it for az
	if(msg->body.servoCmd.intVal == 1){
	  servo_->issueCommand(ServoCommandSa::AZ_BRAKE_ON);
	} else {
	  servo_->issueCommand(ServoCommandSa::AZ_BRAKE_OFF);
	};
	break;

      case 2:
	// elevation
	if(msg->body.servoCmd.intVal == 1){
	  servo_->issueCommand(ServoCommandSa::EL_BRAKE_ON);
	} else {
	  servo_->issueCommand(ServoCommandSa::EL_BRAKE_OFF);
	};
	break;
	
      case 3:
	// both
	if(msg->body.servoCmd.intVal == 1){
	  servo_->issueCommand(ServoCommandSa::AZ_BRAKE_ON);
	  servo_->issueCommand(ServoCommandSa::EL_BRAKE_ON);
	} else {
	  servo_->issueCommand(ServoCommandSa::AZ_BRAKE_OFF);
	  servo_->issueCommand(ServoCommandSa::EL_BRAKE_OFF);
	};
	break;
      }
    }
  break;

  case gcp::control::SERVO_ENABLE_CONTACTORS:
    {
      switch( (int) msg->body.servoCmd.fltVal){
      case 1:
	// do it for az
	if(msg->body.servoCmd.intVal == 1){
	  servo_->issueCommand(ServoCommandSa::AZ_CONTACTORS_ON);
	} else {
	  servo_->issueCommand(ServoCommandSa::AZ_CONTACTORS_OFF);
	};
	break;
	
      case 2:
	// elevation
	if(msg->body.servoCmd.intVal == 1){
	  servo_->issueCommand(ServoCommandSa::EL_CONTACTORS_ON);
	} else {
	  servo_->issueCommand(ServoCommandSa::EL_CONTACTORS_OFF);
	};
	break;
	
      case 3:
	// both
	if(msg->body.servoCmd.intVal == 1){
	  servo_->issueCommand(ServoCommandSa::AZ_CONTACTORS_ON);
	  servo_->issueCommand(ServoCommandSa::EL_CONTACTORS_ON);
	} else {
	  servo_->issueCommand(ServoCommandSa::AZ_CONTACTORS_OFF);
	  servo_->issueCommand(ServoCommandSa::EL_CONTACTORS_OFF);
	};
	break;
      }
    }
  break;


  case gcp::control::SERVO_LOAD_PARAMETERS:
    {
      COUT("GOT A LOAD PARAMETERS");
      std::vector<float> vals(7);
      for (unsigned i=0; i<7; i++){
	vals[i] = msg->body.servoCmd.fltVals[i];
      };

      switch(msg->body.servoCmd.intVal){
      case 0:
	//	COUT("got values for all lines");
	servo_->issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_A, vals);
	servo_->issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_B, vals);
	servo_->issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_C, vals);
	servo_->issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_D, vals);
	break;

      case 1:
	//	COUT("got values for loop 1");
	servo_->issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_A, vals);
	break;

      case 2:
	//	COUT("got values for loop 2");
	servo_->issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_B, vals);
	break;

      case 3:
	//	COUT("got values for loop 3");
	servo_->issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_C, vals);
	break;

      case 4:
	//	COUT("got values for loop 4");
	servo_->issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_D, vals);
	break;
      }
    }
    break;
  };

}
    


/**------------------------------------------------------------
 *  OLD ADDTICK
 */

/**.......................................................................
 * Respond to a 1-second tick from the time-code reader ISR by
 * recording the new clock offset. If a resynchronization of the clock
 * has been requested, also check the time against the time-code
 * reader and correct it if necessary.  
 *
 *
 * Input:
 *
 *   msg  TrackerMsg*  The message received on the Tracker message 
 *                          queue.
 */
void Tracker::addTickOvro(TrackerMsg* msg)
{
  static TimeVal lastMjd, currMjd, diff;
  LogStream errStr;
  static bool startup=true;

  // Allowable limits for delta t between successive ticks sent to the
  // tracker

  double lowerDeltaLimit=0.9, upperDeltaLimit=1.1;

  // The timestamp gets set when the signal handler is called, on the
  // half-second boundary.  We store only the integral second of the
  // timestamp, since we want calculations to be done relative to the
  // absolute second boundary to which this tick corresponds.  
  //
  // The timestamp for which we want to calculate the next telescope
  // position should be 2 seconds later than the current 1-second
  // boundary, since it is the position we want the telescope to
  // achieve two 1-PPS ticks from now (see below)

  currMjd.setMjd(msg->body.tick.mjdDays, msg->body.tick.mjdSeconds, 0);
  currMjd.incrementSeconds(2.0);

  try {

    // if the first time running the loop. no need to check the time
    // difference

    if(startup == true) {
      diff = 1;
      startup = false;
    } else {
      diff           = currMjd - lastMjd;
    };

    double secDiff = diff.getTimeInSeconds();

    // The new time should be roughly 1 second later than the previous
    // time.  But it's only crucial that we deliver the current
    // position to the PMAC well before it is next needed.  

    if(secDiff > lowerDeltaLimit && secDiff < upperDeltaLimit) {

      // Record the new clock tick offset in the database.

      share_->setClock(currMjd);

      // Do we have enough information to roughly point the telescope?

      if(!okToPoint()) {
	newStatus_ = TrackingStatus::LACKING;

      } else {
	// Find out where the telescope is currently located and
	// record this in the archive.
	
	AxisPositions current;

	// Read the current position and set the pmac tracking status
	pmacTracking_ = servo_->readPosition(&current, &model_);

	// The sequence is as follows: updates to the pmac are made
	// once a second, on the three-quarter-second.  The positions written
	// to the pmac on each 3/4-second are for the integral second
	// tick 1.25 seconds in the future.  Reads of the PMAC DPRAM
	// are made every second, on the quarter second boundaries.
	// When the current and commanded positions are compared on
	// the half-second, the current position for the pmac will be
	// the position read 0.25 seconds ago, which will reflect
	// the position commanded not 1 second ago, but two seconds
	// ago.
	//
	//           Commanded       Commanded       Commanded    
	//            position        position        position    
	//           for second      for second      for second
	//               2               3               4
	//               |               |               |       
	//      Read     |      Read     |      Read     |    
	//      PMAC     |      PMAC     |      PMAC     |    
	//       |       |       |       |       |       |    
	//   |               |               |               | 
	//   |___|___|___|___|___|___|___|___|___|___|___|___|
	//   0               1               2               3
	//                   
	//                   |               |	             |	     
	//                   |               |	             |	     
	//              PMAC asserts    PMAC asserts    PMAC asserts 
	//               position 1      position 2      position 3  
	//
	//

	// Now archive the location read a quarter-second ago (should
	// reflect the values from the previous second) and the commanded
	// position from two ticks ago

	tracker_->archivePosition(&current, &tertCommanded_);

	// If the pmac is ready for a new command, execute the next
	// state of the pmac state machine.  If the command is a halt
	// command, issue it right away.

	if(!servo_->isBusy()) {
	  // Use the (implicit) TimeVal copy constructor to create a
	  // copy of currentMjd, since updatePmac may modify the
	  // timestamp.

	  TimeVal mjd = currMjd;

	  // Send the updated position to the pmac.  

	  /*	  COUT("about to update pmac");
	  CTOUT("mjdays: " << mjd.getMjdDays());
	  CTOUT("mjdSeconds: " << mjd.getMjdSeconds());
	  CTOUT("mjdNanoSeconds: " << mjd.getMjdNanoSeconds());*/
	  
	  try {
	    updatePmac(mjd, &current);
	  } catch (Exception& err) {

	    ReportMessage("updatePmac threw an exception");

	    cout << err.what() << endl;
	    err.report();

	    // An error may be thrown if the UMAC interface does not
	    // respond in time, which has been seen to happen
	    // infrequently.  If we are tracking a source when such a
	    // timeout occurs, we should just queue a resync and
	    // continue tracking.  If not, we will just halt the
	    // telescope.

	    errStr.initMessage(true);
	    errStr << "No response from the PMAC." <<
	      " whatNext_ = " << whatNext_ << ", newStatus_ = " << newStatus_;

	    // If the pmac was tracking, syncing or targeting when the
	    // timeout occurred, re-target the source.
	    
	    if(whatNext_ == SLAVE || whatNext_ == SYNC || whatNext_ == TARGET) {

	      ReportMessage("whatNext_ = (one of 3 things)");

	      errStr << endl << "Re-targeting source";

	      whatNext_   = TARGET;
	      
	      // Else if we were in the process of slewing, just
	      // requeue the slew
	      
	    } else if(whatNext_ == HALT && 
		      newStatus_ == TrackingStatus::SLEWING) {

	      ReportMessage("whatNext_ = HALT");

	      errStr << endl << "Re-queuing the current slew";

	      whatNext_ = SLEW;
	      
	      // If the telescope was halted, and a slew was
	      // commanded, requeue the slew

	    } else if(whatNext_ == SLEW) {

	      ReportMessage("whatNext_ = SLEW");
	    
	      errStr << endl << "Re-queuing the new slew";

	      whatNext_ = SLEW;

	      // Else simply halt the telescope

	    } else {

	      ReportMessage("whatNext_ = (default)");
	      
	      errStr << endl << "Halting the telescope";

	      whatNext_   = HALT;
	    }
	    
	    // And set the new status to updating

	    newStatus_  = TrackingStatus::UPDATING;
	  }

	  // If the pmac is not ready and we are in the process of
	  // tracking a source, this means that pmac has lost sync, so
	  // queue a resync.

	} else if(whatNext_ == SLAVE) {

	  COUT("Tracker thinks pmac has lost time sync");
	  errStr.initMessage(true);
	  errStr << "The pmac appears to have lost its time synchronization.";

	  whatNext_   = TARGET;
	  newStatus_  = TrackingStatus::UPDATING;
	};
      
      };

      // If the time hasn't changed at all then this implies that a
      // spurious interrupt was received.
      
    } else if(secDiff < lowerDeltaLimit) {
      errStr.initMessage(true);
      errStr << "Discarding spurious time-code reader interrupt.";
    } else {

      // Did the clock jump? 

      errStr.initMessage(true);
      errStr << "Time appears to have gone "
	     << (secDiff < 0 ? "back":"forwar")
	     << " by "
	     << fabs(secDiff)
	     << "seconds.";
      
      // Record the time error for inclusion in the archive database.
      
      newStatus_ = TrackingStatus::TIME_ERROR;
    };
    
  } catch(Exception& err) {
    err.report();
    newStatus_ = TrackingStatus::TIME_ERROR;
  };
  
  // If the time is invalid and we are tracking then we should attempt
  // reacquire the current source on the next second tick.
  
  if(newStatus_ == TrackingStatus::TIME_ERROR && whatNext_ == SLAVE) {
    DBPRINT(true, Debug::DEBUG4, 
	    "About to enter TARGET because of a time error");
    whatNext_ = TARGET;
  }
  
  // Archive the current tracking status.
  
  archiveStatus();

  // Store the last mjd

  lastMjd = currMjd;

  // And simply log any errors that occurred.

  if(errStr.isError()){
    errStr.log();

    if(newStatus_ != TrackingStatus::TIME_ERROR) {
      ThrowError(errStr);
    }
  }

}
