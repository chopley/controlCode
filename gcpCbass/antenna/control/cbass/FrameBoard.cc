#define __FILEPATH__ "antenna/control/specific/FrameBoard.cc"

#include <cmath>

#include "gcp/antenna/control/specific/FrameBoard.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor for a FrameBoard object
 */
FrameBoard::FrameBoard(SpecificShare* share, string name) : 
  Board(share, name)
{
  // Initialize everything to NULL

  nsnap_      = 0;
  record_     = 0;
  utc_        = 0;
  features_   = 0;
  markSeq_    = 0;

  // Look up registers of the frame board

  nsnap_      = findReg("nsnap");
  record_     = findReg("record");
  utc_        = findReg("utc");
  features_   = findReg("features");
  markSeq_    = findReg("markSeq");
}

/**.......................................................................
 * Update the record number
 */
void FrameBoard::archiveRecordNumber(unsigned record)
{
  writeReg(record_, 0, 1, &record);
}

/**.......................................................................
 * Record the time in the register database
 */
void FrameBoard::setTime()
{
  share_->setClock();
}

/**.......................................................................
 * Record the time in the register database
 */
void FrameBoard::archiveTime()
{
  unsigned utc[2];   // The current UTC as a Modified Julian Day
                     // number and the time of day in milli-seconds.
  double days;       // The MJD day number
  unsigned lst;      // The local sidereal time in milliseconds

  // Get the current UTC as a Modified Julian Date.
 
  double mjd = share_->getUtc();

  // Split the MJD UTC into integral days and milliseconds parts.
 
  // Milli-seconds

  utc[1] = static_cast<unsigned int>(modf(mjd, &days) * daysec * 1000.0);  
  
  utc[0] = static_cast<unsigned int>(days); // Days

  gcp::util::RegDate date(utc[0], utc[1]);

  // Get the Local Sidereal Time in milliseconds.
 
  lst = static_cast<unsigned int>(share_->getLst(mjd) * (daysec / twopi) 
				  * 1000.0);

  // Record the new values.
 
  share_->writeReg(utc_, date.data());
}

/**.......................................................................
 * Update the features bitmask
 */
void FrameBoard::archiveFeatures(unsigned features, unsigned seq)
{
  writeReg(features_, 0, 1, &features);
  writeReg(markSeq_, 0, 1, &seq);
}

/**.......................................................................
 * Update the local register that contains a count of the number
 * of integrated snapshots per frame.
 */
void FrameBoard::archiveNsnap(unsigned nsnap)
{
  writeReg(nsnap_, 0, 1, &nsnap);
}
