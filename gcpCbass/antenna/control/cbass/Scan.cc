#include "gcp/antenna/control/specific/Scan.h"
#include "gcp/antenna/control/specific/OffsetBase.h"

#include <string.h>

using namespace std;

using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor.
 */
Scan::Scan() 
{
  offsets_.initialize();
  reset();
  initialized_ = true;
  add_ = false;
}

/**.......................................................................
 * Constructor.
 */
Scan::Scan(const Scan& scan) 
{
  initialized_ = false;
  *this = (Scan&)scan;
}

Scan::Scan(Scan& scan) 
{
  initialized_ = false;
  *this = scan;
}

/**.......................................................................
 * Destructor.
 */
Scan::~Scan() 
{
  offsets_.free();
}

/**.......................................................................
 * Setup for halt
 */
void Scan::setupForHalt()
{
  strncpy((char*)name_, "(none)", SCAN_LEN);
  name_[SCAN_LEN-1] = '\0';

  offsets_.empty();
  current_.reset();

  justFinished_ = false;
}

void Scan::resetName()
{
  strncpy((char*)name_, "(none)", SCAN_LEN);
  name_[SCAN_LEN-1] = '\0';
}

/**.......................................................................
 * Re/initialize the scan container
 */
void Scan::reset()
{
  resetName();

  lastReq_ = 0;
  lastAck_ = 0;

  offsets_.empty();
  current_.reset();

  justFinished_ = false;

  checkLastMjd_ = false;
}

/**.......................................................................
 * Extend the ephemeris for a scan
 */
void Scan::extend(unsigned npt, unsigned* index, 
		  int* azoff, int* eloff, int* dkoff)
{
  for(unsigned i=0; i < npt; i++)

    // Ignore errors for now

    offsets_.extend(index[i],
		    0,
		    OffsetBase::wrapPi(azoff[i] * mastor),
		    OffsetBase::wrapPi(eloff[i] * mastor),
		    OffsetBase::wrapPi(dkoff[i] * mastor));
}

/**.......................................................................
 * Extend the ephemeris for a scan
 */
void Scan::extend(unsigned npt, unsigned* index, unsigned* flag,
		  int* azoff, int* eloff)
{
  for(unsigned i=0; i < npt; i++)

    // Ignore errors for now

    offsets_.extend(index[i],
		    flag[i],
		    OffsetBase::wrapPi(azoff[i] * mastor),
		    OffsetBase::wrapPi(eloff[i] * mastor),
		    0.0);
}

/**.......................................................................
 * Reinitialize the scan cache
 */
void Scan::initialize(std::string name, 
		      unsigned ibody, unsigned iend, unsigned nreps, 
		      unsigned seq, unsigned msPerStep, unsigned msPerSample,
		      bool add)
{
  if(name.size() > SCAN_LEN)
    ThrowError("Scan name is too long");

  strcpy((char*)name_, name.c_str());

  offsets_.empty();
  lastReq_ = seq;

  add_ = add;

  if(offsets_.initialize(ibody, iend, nreps, msPerStep, msPerSample, add))
    ThrowError("Error in initialize_ScanCache()");
}

/**.......................................................................
 * Get the next set of offsets from the scan cache
 */ 
ScanCacheOffset& Scan::nextOffsetTimeJump(gcp::util::TimeVal& mjd)
{
  static gcp::util::TimeVal diff;
  unsigned nStep;

  if((currentState() != SCAN_INACTIVE) && offsets_.msPerStep) {
    if(checkLastMjd_) {
      mjdDiff_ = mjd - mjdLast_;
      nStep = mjdDiff_.getTimeInMilliSeconds()/offsets_.msPerStep;
    } else {
      mjdLast_ = mjd;
      checkLastMjd_ = true;
      nStep = 1;
    }
    
    // If time has jumped, increment by the equivalent number of scan
    // steps

    if(nStep > 1) {
      for(unsigned iStep=0; iStep < nStep; iStep++) {
	mjdLast_.incrementMilliSeconds(offsets_.msPerStep);
	nextOffset(mjdLast_);
      }

      // Else just process the step normally

    } else {
      nextOffset(mjd);
    }
  } else {
    nextOffset(mjd);
  }

  // Store the current mjd

  mjdLast_ = mjd;

  // And return the current offsets

  return current_;
}

/**.......................................................................
 * Get the next set of offsets from the scan cache
 */ 
ScanCacheOffset& Scan::nextOffset(gcp::util::TimeVal& mjd)
{
  // Initialize the finished flag to false

  justFinished_ = false;

  // If this scan is active, get the next set of offsets

  if(currentState() != SCAN_INACTIVE) {

    offsets_.getNextOffset(&current_, mjd);

    if(currentState() == SCAN_INACTIVE) {
      justFinished_ = true;
      resetName();
    }
  } else {
    current_.flag = 0;
  }

  // Return the current offsets

  return current_;
}

void Scan::pack(signed* s_elements)
{
  s_elements[0] = static_cast<signed>(current_.az * rtomas);
  s_elements[1] = static_cast<signed>(current_.el * rtomas);
  s_elements[2] = static_cast<signed>(current_.dk * rtomas);
}

void Scan::operator=(const Scan& scan)
{
  *this = (Scan&)scan;
}

void Scan::operator=(Scan& scan)
{
  strcpy((char*)name_, (char*)scan.name_);

  // Don't copy the offsets, but initialize them so that it is safe to
  // delete

  if(!initialized_) {
    offsets_.initialize();
    initialized_ = true;
  }

  current_      = scan.current_;
  lastReq_      = scan.lastReq_;
  lastAck_      = scan.lastAck_;
  justFinished_ = scan.justFinished_;
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::antenna::control::operator<<(ostream& os, Scan& scan)
{
  os << "Current scan iteration: "  << std::endl
     << " name  = " << scan.name()  << std::endl
     << " index = " << scan.index() << std::endl
     << " rep   = " << scan.rep()   << std::endl
     << " flag  = " << scan.flag()  << std::endl
     << " state = ";

  switch (scan.currentState()) {
  case SCAN_ACTIVE:
    os << "ACTIVE";
    break;
  case SCAN_INACTIVE:
    os << "INACTIVE";
    break;
  case SCAN_START:
    os << "START";
    break;
  case SCAN_BODY:
    os << "BODY";
    break;
  case SCAN_END:
    os << "END";
    break;
  default:
    os << "UNKNOWN";
    break;
  }

  os << std::endl;

  return os;
}
