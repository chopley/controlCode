#ifndef grabber_h
#define grabber_h

#include "gcp/control/code/unix/libunix_src/common/control.h"

#include "gcp/grabber/common/Channel.h"

#include "gcp/util/common/PointingTelescopes.h"

#ifndef genericcontrol_h
#define genericcontrol_h
#endif

#include "gcp/util/common/Angle.h"

#include "gcp/grabber/common/Channel.h"

void grabber_offset_info(ControlProg *cp, 
			 gcp::util::Angle& xoff, 
			 gcp::util::Angle& yoff, 
			 unsigned& ipeak, unsigned& jpeak,
			 unsigned& chanMask);

void grabber_peak_info(ControlProg *cp, double& peak, double& snr,
		       unsigned& chanMask);

namespace gcp {
  namespace util {
    class Angle;
  }
}

/**.......................................................................
 * Public method to set the optical camera FOV
 */
int setOpticalCameraFov(ControlProg* cp, gcp::util::Angle& fov, 
			unsigned chanMask);

int setOpticalCameraFov(ControlProg* cp, unsigned chanMask);

/**.......................................................................
 * Public method to set the optical camera FOV
 */
int setOpticalCameraAspectRatio(ControlProg* cp, double aspect=0.0,
				unsigned chanMask = 
				gcp::util::PointingTelescopes::getDefaultFgChannels());

/**.......................................................................
 * Public method to set the optical camera x,y-directions
 */
int setOpticalCameraXImDir(ControlProg* cp, gcp::control::ImDir dir,
			   unsigned chanMask);

int setOpticalCameraYImDir(ControlProg* cp, gcp::control::ImDir dir, 
			   unsigned chanMask);

int setDeckAngleRotationSense(ControlProg* cp, 
			      gcp::control::RotationSense sense, 
			      unsigned chanMask);

/**.......................................................................
 * Public method to set the optical camera collimation
 */
int setOpticalCameraRotationAngle(ControlProg* cp, 
				  gcp::util::Angle& collimation, 
				  unsigned chanMask);

int setOpticalCameraRotationAngle(ControlProg* cp,
				  unsigned chanMask);

/**.......................................................................
 * Public method to tell clients about the grabber COMBINE
 */
int setGrabberCombine(ControlProg* cp, unsigned combine, 
		      unsigned chanMask);

/**.......................................................................
 * Public method to set the grabber FLATFIELD
 */
int setGrabberFlatfieldType(ControlProg* cp,
			    unsigned flatfield,
			    unsigned chanMask);

/**.......................................................................
 * Public method to send a frame grabber channel - pointing telescope
 * association
 */
int sendFgChannelAssignment(ControlProg* cp, 
			    gcp::grabber::Channel::FgChannel chan,
			    gcp::util::PointingTelescopes::Ptel ptel);

/**.......................................................................
 * Send the current grabber config to all connected clients.
 */
int sendCurrentGrabberConfiguration(Grabber* grabber);

/**.......................................................................
 * Public method to set search box
 */
int setFrameGrabberSearchBox(ControlProg* cp, 
			     unsigned ixmin, unsigned iymin, 
			     unsigned ixmax, unsigned iymax, 
			     bool inc,
			     unsigned chanMask);

/**.......................................................................
 * Public method to delete a search box
 */
int remFrameGrabberSearchBox(ControlProg* cp, 
			     unsigned ix, unsigned iym, 
			     unsigned chanMask);

/**.......................................................................
 * Public method to delete all search boxes
 */
int remAllFrameGrabberSearchBoxes(ControlProg* cp, unsigned chanMask);

#endif
