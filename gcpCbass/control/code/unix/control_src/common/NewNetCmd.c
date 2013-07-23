#include "NewNetCmd.h"

using namespace std;

using namespace gcp::control;

/**.......................................................................
 * Constructor.
 */
NewNetCmd::NewNetCmd() 
{
  addMember(NET_INIT_CMD, &init);
  addMember(NET_SHUTDOWN_CMD, &shutdown); 
  addMember(NET_STROBE_CMD);
  addMember(NET_FEATURE_CMD, &feature);  
  addMember(NET_LOCATION_CMD, &location);
  addMember(NET_SITE_CMD, &site);        
  addMember(NET_GETREG_CMD, &getreg); 
  addMember(NET_SETREG_CMD, &setreg); 
  addMember(NET_HALT_CMD, &halt);     
  addMember(NET_STOP_CMD);
  addMember(NET_SCAN_CMD, &scan);     
  addMember(NET_SLEW_CMD, &slew);     
  addMember(NET_TRACK_CMD, &track);   
  addMember(NET_MOUNT_OFFSET_CMD, &mount_offset);
  addMember(NET_EQUAT_OFFSET_CMD, &equat_offset);
  addMember(NET_TV_OFFSET_CMD, &tv_offset);
  addMember(NET_TV_ANGLE_CMD, &tv_angle);
  addMember(NET_SKY_OFFSET_CMD, &sky_offset);
  addMember(NET_DECK_MODE_CMD, &deck_mode); 
  addMember(NET_ATMOS_CMD, &atmos);
  addMember(NET_UT1UTC_CMD, &ut1utc);
  addMember(NET_EQNEQX_CMD, &eqneqx);  
  addMember(NET_ENCODER_CALS_CMD, &encoder_cals);    
  addMember(NET_ENCODER_LIMITS_CMD, &encoder_limits);
  addMember(NET_ENCODER_ZEROS_CMD, &encoder_zeros);  
  addMember(NET_SLEW_RATE_CMD, &slew_rate);  
  addMember(NET_TILTS_CMD, &tilts);         
  addMember(NET_FLEXURE_CMD, &flexure);     
  addMember(NET_COLLIMATE_CMD, &collimate); 
  addMember(NET_MODEL_CMD, &model);         
  addMember(NET_YEAR_CMD, &year);           
  addMember(NET_GPIB_SEND_CMD, &gpib_send);  
  addMember(NET_GPIB_READ_CMD, &gpib_read);  
  addMember(NET_POWER_CMD, &power);         
  addMember(NET_PAGER_CMD, &pager);         
  addMember(NET_OPTCAM_CNTL_CMD, &optcam_cntl);
  addMember(NET_CONFIGURE_FG_CMD, &configureFrameGrabber); 
}

/**.......................................................................
 * Destructor
 */
NewNetCmd::~NewNetCmd() {}
