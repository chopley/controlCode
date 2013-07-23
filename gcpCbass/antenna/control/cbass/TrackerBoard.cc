#define __FILEPATH__ "antenna/control/specific/TrackerBoard.cc"

/**
 * With gcc.3.2.2, this must come first, or else the compiler gets
 * confused.
 */
#include "gcp/antenna/control/specific/TrackerBoard.h"

/**
 * Put these second.
 */
#include "gcp/util/common/PointingParameter.h"

#include "gcp/antenna/control/specific/Atmosphere.h"
#include "gcp/antenna/control/specific/AxisPositions.h"
#include "gcp/antenna/control/specific/Model.h"
#include "gcp/antenna/control/specific/PmacTarget.h"
#include "gcp/antenna/control/specific/Pointing.h"
#include "gcp/antenna/control/specific/Position.h"
#include "gcp/antenna/control/specific/SpecificShare.h"
#include "gcp/antenna/control/specific/Scan.h"
#include "gcp/antenna/control/specific/TrackerOffset.h"
#include "gcp/antenna/control/specific/OffsetBase.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TrackerFlags.h"
#include "gcp/util/common/TrackingStatus.h"

#include <cmath> // needed for modf

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

#define INIT_REG(regname) \
{\
  regname_ = share_->addBufferedBlock("tracker", "regname", share_);\
}

/**.......................................................................
 * Constructor for the Tracker board class.
 */
TrackerBoard::TrackerBoard(SpecificShare* share, char* name) : 
  Board(share, name)
{
  try {
  // Initialize pointers to registers of this board

  positionIndex_ = 0;

  // Slow registers

  lacking_          = share_->addBufferedBlock("tracker", "lacking");
  lst_              = share_->addBufferedBlock("tracker", "lst");
  utc_              = share_->addBufferedBlock("tracker", "utc");
  ut1utc_           = share_->addBufferedBlock("tracker", "ut1utc");
  eqneqx_           = share_->addBufferedBlock("tracker", "eqneqx");
  encoder_off_      = share_->addBufferedBlock("tracker", "encoder_off");
  encoder_mul_      = share_->addBufferedBlock("tracker", "encoder_mul");

  az_limits_        = share_->addBufferedBlock("tracker", "az_limits");
  el_limits_        = share_->addBufferedBlock("tracker", "el_limits");
  tilts_            = share_->addBufferedBlock("tracker", "tilts");
  flexure_          = share_->addBufferedBlock("tracker", "flexure");
  axis_             = share_->addBufferedBlock("tracker", "axis");
  ptel_             = share_->addBufferedBlock("tracker", "ptel");

  fixedCollimation_ = share_->addBufferedBlock("tracker", "fixedCollimation");
  siteActual_       = share_->addBufferedBlock("tracker", "siteActual");
  siteFiducial_     = share_->addBufferedBlock("tracker", "siteFiducial");
  location_         = share_->addBufferedBlock("tracker", "location");
  time_diff_        = share_->addBufferedBlock("tracker", "time_diff");

  // Fast registers

  //  utc_              = share_->addBufferedBlock("tracker", "utc");
  mode_             = share_->addBufferedBlock("tracker", "mode");
  refraction_       = share_->addBufferedBlock("tracker", "refraction");
  source_           = share_->addBufferedBlock("tracker", "source");
  scanName_         = share_->addBufferedBlock("tracker", "scan_name");
  scanOff_          = share_->addBufferedBlock("tracker", "scan_off");
  scanFlag_         = share_->addBufferedBlock("tracker", "scan_flag");
  scanInd_          = share_->addBufferedBlock("tracker", "scan_ind");
  scanState_        = share_->addBufferedBlock("tracker", "scan_mode");
  scanRep_          = share_->addBufferedBlock("tracker", "scan_rep");
  equat_geoc_       = share_->addBufferedBlock("tracker", "equat_geoc");
  equat_off_        = share_->addBufferedBlock("tracker", "equat_off");
  horiz_geoc_       = share_->addBufferedBlock("tracker", "horiz_geoc");
  horiz_topo_       = share_->addBufferedBlock("tracker", "horiz_topo");
  horiz_mount_      = share_->addBufferedBlock("tracker", "horiz_mount");
  horiz_off_        = share_->addBufferedBlock("tracker", "horiz_off");
  sky_xy_off_       = share_->addBufferedBlock("tracker", "sky_xy_off");

  expectedRates_    = share_->addBufferedBlock("tracker", "expectedRates");
  actualRates_      = share_->addBufferedBlock("tracker", "actualRates");
  actualCounts_     = share_->addBufferedBlock("tracker", "actualCounts");
  expectedCounts_   = share_->addBufferedBlock("tracker", "expectedCounts");

  motor_current_    = share_->addBufferedBlock("tracker", "motor_current");
  sum_tach_         = share_->addBufferedBlock("tracker", "sum_tach");
  tilt_xy_          = share_->addBufferedBlock("tracker", "tilt_xy");
  tilt_xy_avg_      = share_->addBufferedBlock("tracker", "tilt_xy_avg");
  tilt_enabled_     = share_->addBufferedBlock("tracker", "tilt_enabled");
  tilt_max_         = share_->addBufferedBlock("tracker", "tilt_max");
  tilt_xy_offset_   = share_->addBufferedBlock("tracker", "tilt_xy_offset");
  tilt_theta_       = share_->addBufferedBlock("tracker", "tilt_theta");
  tilt_average_interval_ = share_->addBufferedBlock("tracker", "tilt_average_interval");

  actual_           = share_->addBufferedBlock("tracker", "actual");
  expected_         = share_->addBufferedBlock("tracker", "expected");

  errors_           = share_->addBufferedBlock("tracker", "errors");
  state_            = share_->addBufferedBlock("tracker", "state");
  stateMask_        = share_->addBufferedBlock("tracker", "stateMask");
  offSource_        = share_->addBufferedBlock("tracker", "offSource");

  servoBusy_        = share_->findRegMapBoard("tracker")->findRegMapBlock("servoBusy");

  } catch(Exception& err) {
    COUT("Exception: " << err.what());
  } catch(...) {
    COUT("Exception: unknown");
  }
}

/**.......................................................................
 * Increment the position counter
 */
unsigned int TrackerBoard::incrementPosition()
{
  ++positionIndex_ %= POSITION_SAMPLES_PER_FRAME;
  return positionIndex_;
};

/**.......................................................................
 * Archive the current and expected positions of the telescope axes.
 *
 * Input:
 *
 *  current    AxisPositions *  The current position of the telescope.
 *  commanded  Position      *  The requested position of the telescope.
 */
void TrackerBoard::archivePosition(AxisPositions *current, 
				   Position* commanded)
{
  LogStream errStr;
  signed s_elements[3]; // Signed register values to record 

  try {
    
    // We want the following registers to be archived with contemporary
    // values, so prevent archiving while the tracker board is being
    // updated.
    
    share_->lock();

    // Record the current positions of the telescope axes as returned
    // from the pmac.

    {
      double coord[3];
      current->pack(coord);

      //      Debug::setLevel(Debug::DEBUG7);
      actual_->buffer(positionIndex_, coord);
      //      Debug::setLevel(Debug::DEBUGNONE);

      // Record the expected positions of the telescope axes commanded
      // two ticks ago.
      
      commanded->pack(coord);
      expected_->buffer(positionIndex_, coord);
    }
    
    // Record the difference between the expected and actual
    // positions, in mas
    

    double diff = (commanded->az_ - current->az_.topo_) * rtomas;
    s_elements[0] = static_cast<signed>(diff); 

    diff = (commanded->el_ - current->el_.topo_) * rtomas;
    s_elements[1] = static_cast<signed>(diff);

    diff = (commanded->pa_ - current->pa_.topo_) * rtomas;
    s_elements[2] = static_cast<signed>(diff);
    
    errors_->buffer(positionIndex_, s_elements);

  } catch (Exception& err) {
    errStr.appendMessage(true, err.what());
  } catch(...) {
    errStr.appendMessage(true, "Caught an unknown exception\n");
  }

  // Release the archive.
  
  try {
    share_->unlock();
  } catch (Exception& err) {
    errStr.appendMessage(true, err.what());
  } catch(...) {
    errStr.appendMessage(true, "Caught an unknown exception\n");
  }

  if(errStr.isError()) {
    ThrowError(errStr);
  }
}

//-----------------------------------------------------------------------
// Private methods for packing data for archiving

/**.......................................................................
 * Pack the lst for archival into the register database.
 */
void TrackerBoard::packLst(unsigned* u_elements, double lst)
{
  u_elements[0] = static_cast<unsigned int>(lst * (daysec / twopi) * 1000.0);
}

/**.......................................................................
 * Pack the ut1-utc correction.
 */
void TrackerBoard::packUt1Utc(signed* s_elements, double ut1utc)
{
  s_elements[0] = static_cast<signed>(ut1utc * 1000.0);
}

/**.......................................................................
 * Pack the equation of the equinoxes.
 */
void TrackerBoard::packEqnEqx(signed* s_elements, double eqneqx)
{
  s_elements[0] = static_cast<signed>(eqneqx * daysec / twopi * 1000.0);
}

/*.......................................................................
 * Record the latest pointing parameters for archival.
 */
void TrackerBoard::archivePointing(unsigned* archived_ptr,
				   Atmosphere* atmosphere,
				   Model* model, 
				   PmacTarget *pmac,
				   Pointing* pointing, 
				   Site* site,
				   TrackerOffset* offset,
				   Scan* scan,
				   bool servoBusy)
{

  LogStream errStr;
  unsigned u_elements[10];           // Unsigned register values to record 
  signed s_elements[10];             // Signed register values to record 
  double eqneqx;                     // The equation of the equinoxes 
  double ut1utc;                     // The value of UT1 - UTC 
  //  unsigned archived = *archived_ptr; // Store a copy of the bitmask
  unsigned archived = PointingParameter::NONE;  // we don't want the stored
				       // copy because we're flipping
				       // between buffers

  // Get the UTC, the local apparent sidereal time and the terrestrial
  // time at the given time.

  RegDate date = pointing->getDate();
  double utc = date.mjd();
  double lst = share_->getLst(utc);
  double tt  = share_->getTt(utc);
  
  // Get the equation of the equinoxes and UT1-UTC.

  eqneqx = share_->getEqnEqx(tt);
  ut1utc = share_->getUt1Utc(utc);

  // We want all registers to be archived with contemporary values, so
  // prevent archival while the tracker board is being updated.
  //
  // Enclose this in a try{} construction so that a thrown exception
  // won't result in a lock remaining on the register database

  DBPRINT(true, Debug::DEBUG14, "Inside archivePointing()");

  try {

    share_->lock();

    // Record the MJD UTC in days and milli-seconds.

    utc_->buffer(positionIndex_, date.data());

    // Record the local apparent sidereal time in milli-seconds.
    packLst(u_elements, lst);

    lst_->buffer(positionIndex_, u_elements);

    // Record the value of UT1-UTC in milli-seconds.

    packUt1Utc(s_elements, ut1utc);
    ut1utc_->buffer(positionIndex_, s_elements);

    // Record the value of the equation of the equinoxes from radians
    // to milliseconds of time (lst).

    packEqnEqx(s_elements, eqneqx);
    eqneqx_->buffer(positionIndex_, s_elements);

    // Record the type of pointing.

    pmac->packMode(u_elements);
    mode_->buffer(positionIndex_, u_elements);

    // Record the refraction parameters in micro-arcseconds.

    if(~archived & PointingParameter::ATMOSPHERE) {

      atmosphere->currentRefraction()->pack(s_elements);

      // Add in the applied refraction corrections

      s_elements[2] = static_cast<signed>(pointing->getRefraction() * rtomas);

      refraction_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::ATMOSPHERE;

    } else {
      CoordRange range(2);
      int refracCorr = static_cast<signed>(pointing->getRefraction() * rtomas);
      refraction_->buffer(positionIndex_, &refracCorr, &range);
    };

    // Record the encoder zero points in milli-arcseconds.

    if(~archived & PointingParameter::ZEROS) {
      model->packEncoderZeros(s_elements);
      encoder_off_->buffer(positionIndex_, s_elements);
    };

    // Record encoder counts per turn.

    if(~archived & PointingParameter::ENCODERS) {
      model->packEncoderMultipliers(s_elements);
      encoder_mul_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::ENCODERS;
    };

    // Record the encoder limits as topocentric mount angles.

    if(~archived & PointingParameter::LIMITS) {

      // The azimuth limits (milli-arcsec).

      model->Encoder(Axis::AZ)->packLimits(s_elements);
      az_limits_->buffer(positionIndex_, s_elements);
      
      // The elevation limits (milli-arcsec).

      model->Encoder(Axis::EL)->packLimits(s_elements);
      el_limits_->buffer(positionIndex_, s_elements);

      archived |= PointingParameter::LIMITS;
    };

    // Record the axis tilts in milli-arcsec.

    if(~archived & PointingParameter::TILTS) {
      model->packTilts(s_elements);
      tilts_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::TILTS;
    };

    // Record the gravitational flexure in milli-arcsec.

    if(~archived & PointingParameter::FLEXURE) {
      model->packFlexure(s_elements);
      flexure_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::FLEXURE;
    };

    // Record whether radio collimation (else optical) is being used.

    model->packCollimationMode(u_elements);
    axis_->buffer(positionIndex_, u_elements);

    // Record the collimation parameters in milli-arcseconds.

    if(~archived & PointingParameter::COLLIMATION) {

      model->packCollimation(s_elements, gcp::util::Collimation::FIXED);
      fixedCollimation_->buffer(positionIndex_, s_elements);

      archived |= PointingParameter::COLLIMATION;
    };

    // Record the site-location parameters.

    if(~archived & PointingParameter::SITE) {

      site->packActual(s_elements);
      CoordRange range(0,2);
      siteActual_->buffer(s_elements, &range);

      DBPRINT(true, Debug::DEBUG14, "Site is: " 
	      << s_elements[0] << " "  
	      << s_elements[1] << " "  
	      << s_elements[2] << " "  
	      << s_elements[3]);

      site->packFiducial(s_elements);
      siteFiducial_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::SITE;
    };

    // Record the antenna-location parameters.

    if(~archived & PointingParameter::LOCATION) {
      site->packOffset(s_elements);
      location_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::LOCATION;
    };

    // Pack the scan name

    scanName_->buffer(positionIndex_, scan->name());

    // Pack what mode the scan is in.

    scanState_->buffer(positionIndex_, &(scan->currentState()));

    // Pack what rep the scan is in.

    unsigned rep = scan->rep();
    scanRep_->buffer(positionIndex_, &rep);

    // Pack the scan index corresponding to these offsets

    unsigned index = scan->index();
    scanInd_->buffer(positionIndex_, &index);

    // Pack the scan offsets

    scan->pack(s_elements);
    scanOff_->buffer(positionIndex_, s_elements);

    // Pack the source name

    source_->buffer(positionIndex_, pointing->getSrcName());

    // Record the geocentric apparent equatorial coordinates of the
    // source.

    pointing->packEquatGeoc(s_elements);
    equat_geoc_->buffer(positionIndex_, s_elements);

    // Record any temporary equatorial tracking offsets.

    offset->packEquatOffset(s_elements);
    equat_off_->buffer(positionIndex_, s_elements);

    // Record the geocentric apparent az,el,pa of the source.

    pointing->packHorizGeoc(s_elements);
    horiz_geoc_->buffer(positionIndex_, s_elements);

    // Record the topocentric az,el,pa of the source.

    pointing->packHorizTopo(s_elements);
    horiz_topo_->buffer(positionIndex_, s_elements);

    // Record the instrumental-apparent az,el,pa of the source.

    {
      double coord[3];
      pointing->packHorizMount(coord);
      horiz_mount_->buffer(positionIndex_, coord);

    // Record any temporary az,el,dk tracking offsets.
      offset->packHorizOffset(coord);
      horiz_off_->buffer(positionIndex_, coord);
    }

    // Record any temporary sky-based tracking offsets.

    offset->packSkyOffset(s_elements);
    sky_xy_off_->buffer(positionIndex_, s_elements);

    // Record the target encoder counts verbatim.

    pmac->packCounts(s_elements);
    expectedCounts_->buffer(positionIndex_, s_elements);

    // Record the target encoder rates (milli-counts/second).

    pmac->packRates(s_elements);
    expectedRates_->buffer(positionIndex_, s_elements);

    // Record whether the servo is busy
    share_->writeReg(servoBusy_, (bool) servoBusy);

  } catch (Exception& err) {
    errStr.appendMessage(true, err.what());
  } catch(...) {
    errStr.appendMessage(true, "Caught an unknown exception\n");
  }


  // Release the archive.  Enclose in a try-catch clause so we don't
  // lose an exception we may ahve caught earlier if another is thrown
  // in the process.
  
  try {
    
    share_->unlock();
    
    // Return the new bitmask of archived parameters
    
    *archived_ptr = static_cast<PointingParameter::Parameter>(archived);
    
  } catch (Exception& err) {
    errStr.appendMessage(true, err.what());
  } catch(...) {
    errStr.appendMessage(true, "Caught an unknown exception\n");
  }

  // If we caught an exception, throw another.

  if(errStr.isError())
    throw Error(errStr);
}

/*.......................................................................
 * Record the latest pointing parameters for archival.
 */
void TrackerBoard::archivePointing(unsigned* archived_ptr,
				   Atmosphere* atmosphere,
				   Model* model, 
				   PmacTarget *pmac,
				   Pointing* pointing, 
				   Site* site,
				   TrackerOffset* offset,
				   Scan* scan)
{
  LogStream errStr;
  unsigned u_elements[10];           // Unsigned register values to record 
  signed s_elements[10];             // Signed register values to record 
  double eqneqx;                     // The equation of the equinoxes 
  double ut1utc;                     // The value of UT1 - UTC 
  //  unsigned archived = *archived_ptr; // Store a copy of the bitmask
  unsigned archived = PointingParameter::NONE;  // we don't want the stored
				       // copy because we're flipping
				       // between buffers
  bool servoBusy = false;

  // Get the UTC, the local apparent sidereal time and the terrestrial
  // time at the given time.

  RegDate date = pointing->getDate();
  double utc = date.mjd();
  double lst = share_->getLst(utc);
  double tt  = share_->getTt(utc);
  
  // Get the equation of the equinoxes and UT1-UTC.

  eqneqx = share_->getEqnEqx(tt);
  ut1utc = share_->getUt1Utc(utc);

  // We want all registers to be archived with contemporary values, so
  // prevent archival while the tracker board is being updated.
  //
  // Enclose this in a try{} construction so that a thrown exception
  // won't result in a lock remaining on the register database

  DBPRINT(true, Debug::DEBUG14, "Inside archivePointing()");

  try {

    share_->lock();

    // Record the MJD UTC in days and milli-seconds.

    utc_->buffer(positionIndex_, date.data());

    // Record the local apparent sidereal time in milli-seconds.
    packLst(u_elements, lst);

    lst_->buffer(positionIndex_, u_elements);

    // Record the value of UT1-UTC in milli-seconds.

    packUt1Utc(s_elements, ut1utc);
    ut1utc_->buffer(positionIndex_, s_elements);

    // Record the value of the equation of the equinoxes from radians
    // to milliseconds of time (lst).

    packEqnEqx(s_elements, eqneqx);
    eqneqx_->buffer(positionIndex_, s_elements);

    // Record the type of pointing.

    pmac->packMode(u_elements);
    mode_->buffer(positionIndex_, u_elements);

    // Record the refraction parameters in micro-arcseconds.

    if(~archived & PointingParameter::ATMOSPHERE) {

      atmosphere->currentRefraction()->pack(s_elements);

      // Add in the applied refraction corrections

      s_elements[2] = static_cast<signed>(pointing->getRefraction() * rtomas);

      refraction_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::ATMOSPHERE;

    } else {
      CoordRange range(2);
      int refracCorr = static_cast<signed>(pointing->getRefraction() * rtomas);
      refraction_->buffer(positionIndex_, &refracCorr, &range);
    };

    // Record the encoder zero points in milli-arcseconds.

    if(~archived & PointingParameter::ZEROS) {
      model->packEncoderZeros(s_elements);
      encoder_off_->buffer(positionIndex_, s_elements);
    };

    // Record encoder counts per turn.

    if(~archived & PointingParameter::ENCODERS) {
      model->packEncoderMultipliers(s_elements);
      encoder_mul_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::ENCODERS;
    };

    // Record the encoder limits as topocentric mount angles.

    if(~archived & PointingParameter::LIMITS) {

      // The azimuth limits (milli-arcsec).

      model->Encoder(Axis::AZ)->packLimits(s_elements);
      az_limits_->buffer(positionIndex_, s_elements);
      
      // The elevation limits (milli-arcsec).

      model->Encoder(Axis::EL)->packLimits(s_elements);
      el_limits_->buffer(positionIndex_, s_elements);

      archived |= PointingParameter::LIMITS;
    };

    // Record the axis tilts in milli-arcsec.

    if(~archived & PointingParameter::TILTS) {
      model->packTilts(s_elements);
      tilts_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::TILTS;
    };

    // Record the gravitational flexure in milli-arcsec.

    if(~archived & PointingParameter::FLEXURE) {
      model->packFlexure(s_elements);
      flexure_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::FLEXURE;
    };

    // Record whether radio collimation (else optical) is being used.

    model->packCollimationMode(u_elements);
    axis_->buffer(positionIndex_, u_elements);

    // Record the collimation parameters in milli-arcseconds.

    if(~archived & PointingParameter::COLLIMATION) {

      model->packCollimation(s_elements, gcp::util::Collimation::FIXED);
      fixedCollimation_->buffer(positionIndex_, s_elements);

      archived |= PointingParameter::COLLIMATION;
    };

    // Record the site-location parameters.

    if(~archived & PointingParameter::SITE) {

      site->packActual(s_elements);
      CoordRange range(0,2);
      siteActual_->buffer(s_elements, &range);

      DBPRINT(true, Debug::DEBUG14, "Site is: " 
	      << s_elements[0] << " "  
	      << s_elements[1] << " "  
	      << s_elements[2] << " "  
	      << s_elements[3]);

      site->packFiducial(s_elements);
      siteFiducial_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::SITE;
    };

    // Record the antenna-location parameters.

    if(~archived & PointingParameter::LOCATION) {
      site->packOffset(s_elements);
      location_->buffer(positionIndex_, s_elements);
      archived |= PointingParameter::LOCATION;
    };

    // Pack the scan name

    scanName_->buffer(positionIndex_, scan->name());

    // Pack what mode the scan is in.

    scanState_->buffer(positionIndex_, &(scan->currentState()));

    // Pack what rep the scan is in.

    unsigned rep = scan->rep();
    scanRep_->buffer(positionIndex_, &rep);

    // Pack the scan index corresponding to these offsets

    unsigned index = scan->index();
    scanInd_->buffer(positionIndex_, &index);

    // Pack the scan offsets

    scan->pack(s_elements);
    scanOff_->buffer(positionIndex_, s_elements);

    // Pack the source name

    source_->buffer(positionIndex_, pointing->getSrcName());

    // Record the geocentric apparent equatorial coordinates of the
    // source.

    pointing->packEquatGeoc(s_elements);
    equat_geoc_->buffer(positionIndex_, s_elements);

    // Record any temporary equatorial tracking offsets.

    offset->packEquatOffset(s_elements);
    equat_off_->buffer(positionIndex_, s_elements);

    // Record the geocentric apparent az,el,pa of the source.

    pointing->packHorizGeoc(s_elements);
    horiz_geoc_->buffer(positionIndex_, s_elements);

    // Record the topocentric az,el,pa of the source.

    pointing->packHorizTopo(s_elements);
    horiz_topo_->buffer(positionIndex_, s_elements);

    // Record the instrumental-apparent az,el,pa of the source.

    {
      double coord[3];
      pointing->packHorizMount(coord);
      horiz_mount_->buffer(positionIndex_, coord);

    // Record any temporary az,el,dk tracking offsets.
      offset->packHorizOffset(coord);
      horiz_off_->buffer(positionIndex_, coord);
    }

    // Record any temporary sky-based tracking offsets.

    offset->packSkyOffset(s_elements);
    sky_xy_off_->buffer(positionIndex_, s_elements);

    // Record the target encoder counts verbatim.

    pmac->packCounts(s_elements);
    expectedCounts_->buffer(positionIndex_, s_elements);

    // Record the target encoder rates (milli-counts/second).

    pmac->packRates(s_elements);
    expectedRates_->buffer(positionIndex_, s_elements);

    // Record whether the servo is busy
    share_->writeReg(servoBusy_, (bool) servoBusy);

  } catch (Exception& err) {
    errStr.appendMessage(true, err.what());
  } catch(...) {
    errStr.appendMessage(true, "Caught an unknown exception\n");
  }


  // Release the archive.  Enclose in a try-catch clause so we don't
  // lose an exception we may ahve caught earlier if another is thrown
  // in the process.
  
  try {
    
    share_->unlock();
    
    // Return the new bitmask of archived parameters
    
    *archived_ptr = static_cast<PointingParameter::Parameter>(archived);
    
  } catch (Exception& err) {
    errStr.appendMessage(true, err.what());
  } catch(...) {
    errStr.appendMessage(true, "Caught an unknown exception\n");
  }

  // If we caught an exception, throw another.

  if(errStr.isError())
    throw Error(errStr);
}

/**.......................................................................
 * Record the current tracking status in the archive database
 */
void TrackerBoard::archiveStatus(unsigned state, unsigned offSource, unsigned lacking)
{
  state_->buffer(positionIndex_, (unsigned char*)&state);

  unsigned char stateMask = trackerStateToBit(state);
  stateMask_->buffer(positionIndex_, &stateMask);
  
  offSource_->buffer(positionIndex_, &offSource);

  // Record the bit-mask of missing resources.
  
  lacking_->buffer(positionIndex_, &lacking);
}

/**.......................................................................
 * Convert from integer state to a bit mask
 */
unsigned char TrackerBoard::trackerStateToBit(int state)
{
  switch(state) {
  case TrackingStatus::LACKING:
    return TrackerFlags::LACKING;
    break;
  case TrackingStatus::TIME_ERROR:
    return TrackerFlags::TIME_ERROR;
    break;
  case TrackingStatus::UPDATING:
    return TrackerFlags::UPDATING;
    break;
  case TrackingStatus::HALTED:
    return TrackerFlags::HALT;
    break;
  case TrackingStatus::SLEWING:
    return TrackerFlags::SLEW;
    break;
  case TrackingStatus::TRACKING:
    return TrackerFlags::TRACK;
    break;
  case TrackingStatus::TOO_LOW:
    return TrackerFlags::TOO_LOW;
    break;
  case TrackingStatus::TOO_HIGH:
    return TrackerFlags::TOO_HIGH;
    break;
  default:
    ThrowError("Unrecognized state");
    break;
  }
}
