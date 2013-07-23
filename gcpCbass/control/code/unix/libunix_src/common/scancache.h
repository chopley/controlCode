#ifndef SCANCACHE_H
#define SCANCACHE_H

#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/QuadraticInterpolator.h"

/*
 * Set the number of scan entries per cache.  This should be large
 * enough to read all points for any scan.  Currently, each step of a
 * scan pattern can be as little as 10ms second long, so a SCAN_CACHE
 * of 100000 relects a scan of about 17 minutes.
 */
#define SCAN_CACHE_SIZE 100000

/**.......................................................................
 * When get_ScanCache() is called with a pointer to this type, on
 * return, the struct will contain offsets and rates for the current
 * step of the scan pattern.
 */
struct ScanCacheOffset { 
  double az;       /* The AZ offset */
  double el;       /* The EL offset */
  double dk;       /* The PA offset */
  double azrate;   /* The AZ rate */
  double elrate;   /* The EL rate */
  double dkrate;   /* The PA rate */
  ScanMode currentState;   /* The current mode of this scan */
  ScanMode nextState;   /* The current mode of this scan */
  unsigned irep;   /* The current rep */
  unsigned index;  /* The current index */
  unsigned flag;   // The current flag
  unsigned ms;
  
  // Reset an offset container.
  
  void reset();

  // Print method

  // The gcc 3.2.3 compiler on raki doesn't like this at all -- I
  // don't know why.  Commenting out for now...
  
  //  friend std::ostream& operator<<(std::ostream& os, ScanCacheOffset& offset);
};

/**.......................................................................
 * A single entry from a file off scan offsets.
 */
typedef struct {
  unsigned int flag;  // The flag
  unsigned int index; /* The integer counter of this point in the scan */
  double azoff;       /* The AZ offset (degrees) */
  double eloff;       /* The EL offset (degrees) */
  double dkoff;       /* The DK offset (degrees) */
} ScanCacheEntry;

/**.......................................................................
 * A struct for managing offsets read in from a scan offset file.
 */
struct ScanCache {
  ScanCacheEntry offsets[SCAN_CACHE_SIZE];
  ScanCacheOffset lastOffset; // The last calculated offset

  unsigned int size;     /* The number of offsets in the offset buffer. */

  unsigned int istart;
  unsigned int ibody; // The starting index of the body portion of this scan
  unsigned int iend;  // The starting index of the end portion of this scan

  // The current 1-second index midpoint of the interpolation
  // container.  For SPT, which updates on 10-ms timescales, this will
  // represent the next 1-s index needed to bracket the mjd
  // corresponding to the current 10-ms tick

  unsigned midIndex; 
  unsigned startIndex_; 
  unsigned nreps;        // The number of times we should run through
			 // the current scan pattern
  unsigned irep;         // The current rep

  // The number of milliseconds per step of the scan

  unsigned msPerStep;

  // The number of milliseconds per offset in the cache

  unsigned msPerSample;
  
  ScanMode currentState;

  gcp::util::TimeVal* startMjd_;
  gcp::util::QuadraticInterpolator* azInt_;
  gcp::util::QuadraticInterpolator* elInt_;
  gcp::util::QuadraticInterpolator* dkInt_;

  // True if we are first starting a scan, or if we are restarting the
  // body portion

  bool start; 

  // True if scan offsets should be 

  bool add_;

  // Initialize this object

  void initialize();

  // Free any allocated members

  void free();

  // Empty a cache.

  int empty();

  // Initialize fixed members.

  int initialize(unsigned int ibody, unsigned int iend,   
		 unsigned int nreps, unsigned int msPerStep, 
		 unsigned int msPerSample, bool add);

  // Add an element to the current cache.

  int extend(unsigned int index, unsigned flag, 
	     double azoff, double eloff, double dkoff);

  // Return the next offset 
  
  void getNextOffset(ScanCacheOffset* offset, gcp::util::TimeVal& mjd);

  // Get the scan offsets and rates corresponding to this mjd

  void setUpForNewInterpolation(gcp::util::TimeVal& mjd, unsigned index);

  // Get the scan offsets and rates corresponding to this mjd

  void setUpForNextIteration(gcp::util::TimeVal& mjd);

  // Empty the interpolation containers

  void clearInterpolationContainers();

  // Extend the interpolation containers in preparation for the next
  // evaluation

  void extendInterpolation(gcp::util::TimeVal& mjd);

  // Add a new point to the interpolation containers

  void extend(unsigned index);

  // Fill the interpolation containers, starting from the passed index

  void fillInterpolationContainers(unsigned startIndex);

  // Private method to compute the next offset

  void computeOffset(ScanCacheOffset* offset, gcp::util::TimeVal& mjd);

  // Update the state of the offset

  ScanMode getState(gcp::util::TimeVal& mjd);

  // Do whatever is appropriate to extend the interpolation containers
  // in preparation for the next computation

  void updateInterpolators(gcp::util::TimeVal& mjd);

  // Return true if the next step will end the scan

  bool nextStepEndsScan(gcp::util::TimeVal& mjd);

  // Return true if the next step is part of the body segment

  bool nextStepIsBody(gcp::util::TimeVal& mjd);

  // Return true if the next step is part of the body segment

  bool nextStepStartsNewIteration(gcp::util::TimeVal& mjd);

  // Print method

  friend std::ostream& operator<<(std::ostream& os, ScanCache& cache);
};


#endif
