#include "gcp/control/code/unix/libunix_src/common/scancache.h"
#include "gcp/control/code/unix/libunix_src/common/lprintf.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/QuadraticInterpolatorNormal.h"

#define MS_PER_SEC 1000

using namespace gcp::control;

/**.......................................................................
 * Empty a cache.
 */
int ScanCache::empty()
{
  unsigned zero = 0;
  initialize(zero, zero, zero, zero, zero, false);
  return 0;
}

/**.......................................................................
 * Initialize cache object
 */
int ScanCache::initialize(unsigned int in_ibody, unsigned int in_iend,
			  unsigned int in_nreps, unsigned int in_msPerStep, 
			  unsigned int in_msPerSample, bool add)
{
  size    = 0;
  istart  = 0;
  ibody   = in_ibody;
  iend    = in_iend;

  midIndex     = 0;
  startIndex_  = 0;
  
  nreps        = in_nreps;
  irep         = 0;

  msPerStep    = in_msPerStep;
  msPerSample  = in_msPerSample;

  lastOffset.reset();

  currentState = SCAN_INACTIVE;

  add_ = add;

  return 0;
}

/**.......................................................................
 * Add an element to the current cache.
 */
int ScanCache::extend(unsigned int index, unsigned int flag,
		      double azoff, double eloff, double dkoff)
{
  // We can only add entries up to the maximum cache size.

  if(size == SCAN_CACHE_SIZE) {
    lprintf(stderr, "Unable to add more points to the current cache.\n");
    return 1;
  }

  /**
   * Check that an out-of-order entry is not being added.
   */
  if(size > 0 && index != offsets[size-1].index + 1) {
    lprintf(stderr, "Attempt to add a non-consecutive entry to the cache.\n");
    return 1;
  }

  /**
   * If everything checks out, add the requested element to the end of
   * the cache.
   */
  offsets[size].index = index;
  offsets[size].flag  = flag;
  offsets[size].azoff = azoff;
  offsets[size].eloff = eloff;
  offsets[size].dkoff = dkoff;

  /*
   * And increment the cache size.
   */
  size++;

  return 0;
}

/**.......................................................................
 * Get the next set of offsets from the cache.
 */
void ScanCache::getNextOffset(ScanCacheOffset* offset, gcp::util::TimeVal& mjd)
{
  static gcp::util::TimeVal nextMjd;

  if(offset->currentState == SCAN_ACTIVE)
    currentState = SCAN_ACTIVE;

  // Set what the state of this offset will be on exit

  offset->currentState = getState(mjd);

  // If the scan is over, just return

  if(offset->currentState == SCAN_INACTIVE)
    return;

  // Clear the current offset.  We clear it after the state check
  // above, so that if the scan is inactive, it remains at the last
  // offset position

  offset->reset();

  // update the interpolators

  updateInterpolators(mjd);
  
  // And evaulate the offset

  computeOffset(offset, mjd);

  // Finally, set the next state for this offset

  nextMjd = mjd;
  nextMjd.incrementMilliSeconds(msPerStep);

  offset->nextState = getState(mjd);
  offset->irep      = irep;

  // And store the current state

  currentState = offset->currentState;
}

/**.......................................................................
 * Get the scan offsets and rates corresponding to this mjd
 */
void ScanCache::setUpForNewInterpolation(gcp::util::TimeVal& mjd, unsigned index)
{
  // Set the start mjd to the passed value

  *startMjd_  = mjd;
  startIndex_ = index;

  // Set the current index to the start of the scan

  midIndex = index;

  // Empty the interpolation containers

  clearInterpolationContainers();

  // Fill with values, starting from the passed index

  fillInterpolationContainers(index);
}

/**.......................................................................
 * Get the scan offsets and rates corresponding to this mjd
 */
void ScanCache::setUpForNextIteration(gcp::util::TimeVal& mjd)
{
  setUpForNewInterpolation(mjd, ibody);
  ++irep;
}

/**.......................................................................
 * Empty the interpolation containers
 */
void ScanCache::clearInterpolationContainers()
{
  azInt_->empty();
  elInt_->empty();
  dkInt_->empty();
}

/**.......................................................................
 * Extend the interpolation containers in preparation for the next
 * evaluation
 */
void ScanCache::extendInterpolation(gcp::util::TimeVal& mjd)
{
  static gcp::util::TimeVal diff;
  diff = mjd - *startMjd_;

  // Compute the difference between the current time and the start
  // time of the current interpolation, in milliseconds

  unsigned msDiff = diff.getTimeInMilliSeconds();

  // Compute the difference between the current midpoint of the
  // interpolation and the start

  unsigned msComp = (midIndex - startIndex_)*msPerSample;

  // If the next interpolation will exceed the midpoint, extend the
  // interpolation container, unless we have already pushed the last
  // point into it.

  if(msDiff > msComp) {
    ++midIndex;
    extend(midIndex+1);
  }
}

/**.......................................................................
 * Add a new point to the interpolation containers
 */
void ScanCache::extend(unsigned index)
{
  if(index > size-1)
    return;

  double x = (double)(index - startIndex_)*msPerSample;
  azInt_->extend(x, offsets[index].azoff);
  elInt_->extend(x, offsets[index].eloff);
  dkInt_->extend(x, offsets[index].dkoff);
}

/**.......................................................................
 * Fill the interpolation containers, starting from the passed index
 */
void ScanCache::fillInterpolationContainers(unsigned startIndex)
{
  extend(startIndex);
  extend(startIndex+1);
  extend(startIndex+2);

  midIndex = startIndex+1;
}

/**.......................................................................
 * Compute the offset for the passed mjd
 */
void ScanCache::computeOffset(ScanCacheOffset* offset, gcp::util::TimeVal& mjd)
{
  static gcp::util::TimeVal diff;

  // Determine the x-value to interpolate

  diff   = mjd - *startMjd_;
  double msDiff = (double)diff.getTimeInMilliSeconds();

  // Evaluate the offsets

  offset->az = azInt_->evaluate(msDiff);
  offset->el = elInt_->evaluate(msDiff);
  offset->dk = dkInt_->evaluate(msDiff);

  // If we are incrementing offsets, add the new offset to the last
  // position

  if(add_) {
    offset->az += lastOffset.az;
    offset->el += lastOffset.el;
    offset->dk += lastOffset.dk;
  }

  // Differentiate to get the rates.  If this is the first step of the
  // scan, the last offset will be zero. Note that these will be in
  // whatever units were used when extend_ScanCache() was called to
  // install the offsets, per second.
  //
  // Compute rates from the last commanded position, converting to
  // units/second
  
  offset->azrate = (offset->az - lastOffset.az)/(msPerStep)*MS_PER_SEC;
  offset->elrate = (offset->el - lastOffset.el)/(msPerStep)*MS_PER_SEC;
  offset->dkrate = (offset->dk - lastOffset.dk)/(msPerStep)*MS_PER_SEC;
  
  // And store the last offset, for rate calculations

  lastOffset.az = offset->az;
  lastOffset.el = offset->el;
  lastOffset.dk = offset->dk;

  // Set which effective millisecond of the scan we are on

  offset->ms = (startIndex_ * msPerStep) + (unsigned)msDiff;

  // And set the flag from the lower bracketing offset

  offset->flag = offsets[midIndex > 0 ? midIndex-1 : 0].flag;
}

/**.......................................................................
 * Do whatever is appropriate to extend the interpolation containers in 
 * preparation for the next computation
 */
ScanMode ScanCache::getState(gcp::util::TimeVal& mjd)
{
  switch(currentState) {
  case SCAN_ACTIVE:
    updateInterpolators(mjd);
  case SCAN_START:
    if(nextStepIsBody(mjd))
      return SCAN_BODY;
    else
      return SCAN_START;
    break;
  case SCAN_BODY:
    if(nextStepIsBody(mjd))
      return SCAN_BODY;
    else
      return SCAN_END;
    break;
  case SCAN_END:
    if(nextStepEndsScan(mjd))
      return SCAN_INACTIVE;
    else
      return SCAN_END;
  default:
    return SCAN_INACTIVE;
    break;
  }
}

/**.......................................................................
 * Do whatever is appropriate to extend the interpolation containers in 
 * preparation for the next computation
 */
void ScanCache::updateInterpolators(gcp::util::TimeVal& mjd)
{
  switch (currentState) {
  case SCAN_ACTIVE:
    //    COUT("Scan is active -- setting up for new interpolation: ");
    setUpForNewInterpolation(mjd, 0);
    break;
  case SCAN_BODY:
    if(nextStepStartsNewIteration(mjd)) {
      //      COUT("Scan is in body -- setting up for next iteration");
      setUpForNextIteration(mjd); 
    } else {
      //      COUT("Scan is in body -- extending interpolation");
      extendInterpolation(mjd);
    }
    break;
  default:
    //    COUT("Extending interpolation");
    extendInterpolation(mjd);
    break;
  }
}

/**.......................................................................
 * Return true if the next step will end the scan
 */
bool ScanCache::nextStepEndsScan(gcp::util::TimeVal& mjd)
{
  static gcp::util::TimeVal diff;
  diff = mjd - *startMjd_;

  // Compute the difference between the current time and the start
  // time of the current interpolation, in milliseconds

  unsigned msDiff = diff.getTimeInMilliSeconds();
  unsigned msComp = (size - startIndex_) * msPerSample;

  // If this time is beyond the end time, return true

  //  COUT("End test: msDIff = " << msDiff << " msComp = " << msComp);
  //  COUT("End test: size = " << size << " startIndex = " << startIndex_);

  return (msDiff > msComp);
}

/**.......................................................................
 * Return true if the next step is part of the body segment
 */
bool ScanCache::nextStepIsBody(gcp::util::TimeVal& mjd)
{
  static gcp::util::TimeVal diff;
  diff = mjd - *startMjd_;

  // Compute the difference between the current time and the start
  // time of the current interpolation, in milliseconds

  unsigned msDiff = diff.getTimeInMilliSeconds();
  unsigned msEndComp  = (iend  - startIndex_) * msPerSample;
  unsigned msBodyComp = (ibody - startIndex_) * msPerSample;

  //  COUT("msDiff, msBodyComp, msEndComp: " << msDiff << "," << msBodyComp << "," << msEndComp); 

  // If the time is less than the start time for the body, we are
  // still on the start section

  //  COUT("ibody = " << ibody << " iend = " << iend << " " << startIndex_);
  //  COUT("msDiff = " << msDiff << " " << msBodyComp << " " << msEndComp);

  if(msDiff < msBodyComp)
    return false;
  else {

    // If this time is less than the start of the end segment, or we
    // are not done iterating on the body, we are still on the body

    return (msDiff < msEndComp) || irep < nreps-1;
  }
}

/**.......................................................................
 * Return true if the next step is part of the body segment
 */
bool ScanCache::nextStepStartsNewIteration(gcp::util::TimeVal& mjd)
{
  static gcp::util::TimeVal diff;
  diff = mjd - *startMjd_;

  // Compute the difference between the current time and the start
  // time of the current interpolation, in milliseconds

  unsigned msDiff = diff.getTimeInMilliSeconds();
  unsigned msComp = (iend - startIndex_) * msPerSample;

  // If this time is less than the start of the end segment, or we are
  // not done iterating on the body

  return (msDiff >= msComp) && irep < nreps-1;
}

/**.......................................................................
 * Print method for ScanCache class
 */
std::ostream& operator<<(std::ostream& os, ScanCache& cache)
{
  os << std::endl 
     << "nreps =  " << cache.nreps << " irep= " << cache.irep << " msps = " << cache.msPerStep
    //     << " lastoff = " << cache.lastOffset 
     << " istart  = " << cache.istart << " ibody = " << cache.ibody << " iend = " << cache.iend
     << " currInd = " << cache.midIndex;
  return os;
}

// The gcc 3.2.3 compiler on raki doesn't like this at all -- I
// don't know why.  Commenting out for now...

#if 0
std::ostream& operator<<(std::ostream& os, ScanCacheOffset& offs)
{
  gcp::util::Angle az(gcp::util::Angle::Radians(), offs.az);
  gcp::util::Angle el(gcp::util::Angle::Radians(), offs.el);
  gcp::util::Angle dk(gcp::util::Angle::Radians(), offs.dk);

  gcp::util::Angle azr(gcp::util::Angle::Radians(), offs.azrate);
  gcp::util::Angle elr(gcp::util::Angle::Radians(), offs.elrate);
  gcp::util::Angle dkr(gcp::util::Angle::Radians(), offs.dkrate);

  os << "azo = " << az.degrees() << " elo = " << el.degrees() << " dko = " << dk.degrees() 
     << " azr = " << azr.degrees() << " elr = " << elr.degrees() << " dkr = " << dkr.degrees()
     << " currentState= ";

  switch(offs.currentState) {
  case SCAN_START:
    os << "start";
    break;
  case SCAN_BODY:
    os << "body";
    break;
  case SCAN_END:
    os << "end";
    break;
  case SCAN_INACTIVE:
    os << "inactive";
    break;
  default:
    os << "unknown";
    break;
  }

  os << " irep= " << offs.irep   << " ind = " << offs.index; 
  return os;
}
#endif

//-----------------------------------------------------------------------
// Methods of ScanCacheOffset
//-----------------------------------------------------------------------


/**.......................................................................
 * Reset an offset container.
 */
void ScanCacheOffset::reset()
{
  az = 0.0;
  el = 0.0;
  dk = 0.0;

  azrate = 0.0;
  elrate = 0.0;
  dkrate = 0.0;

  irep  = 0;
  index = 0;  
  ms    = 0;
}

void ScanCache::initialize()
{
  azInt_ = 0;
  elInt_ = 0;
  dkInt_ = 0;
  startMjd_ = 0;

  azInt_ = new gcp::util::QuadraticInterpolatorNormal(0.0);
  elInt_ = new gcp::util::QuadraticInterpolatorNormal(0.0);
  dkInt_ = new gcp::util::QuadraticInterpolatorNormal(0.0);
  startMjd_ = new gcp::util::TimeVal();

  empty();
}

void ScanCache::free()
{
  if(azInt_) {
    delete azInt_;
    azInt_ = 0;
  }

  if(elInt_) {
    delete elInt_;
    elInt_ = 0;
  }

  if(dkInt_) {
    delete dkInt_;
    dkInt_ = 0;
  }

  if(startMjd_) {
    delete startMjd_;
    startMjd_ = 0;
  }
}

