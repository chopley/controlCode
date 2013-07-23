// $Id: Scan.h,v 1.4 2011/08/29 22:26:28 sjcm Exp $

#ifndef GCP_ANTENNA_CONTROL_SCAN_H
#define GCP_ANTENNA_CONTROL_SCAN_H

/**
 * @file Scan.h
 * 
 * Tagged: Fri Aug 12 14:16:25 PDT 2005
 * 
 * @version: $Revision: 1.4 $, $Date: 2011/08/29 22:26:28 $
 * 
 * @author Erik Leitch
 */
#include "gcp/control/code/unix/libunix_src/common/scancache.h"

namespace gcp {
  namespace antenna {
    namespace control {

      class Scan {
      public:

	/**
	 * Constructors
	 */
	Scan();
	Scan(Scan& scan);
	Scan(const Scan& scan);

	/**
	 * Destructor.
	 */
	virtual ~Scan();

	void setupForHalt();

	void resetName();
	void reset();

	void initialize(std::string name, unsigned ibody, 
			unsigned iend, unsigned nreps, unsigned seq,
			unsigned msPerStep=1000, unsigned msPerSample=1000,
			bool add=false);

	void extend(unsigned npt, unsigned* index, unsigned* flag,
		    int* azoff, int* eloff);

	void extend(unsigned npt, unsigned* index, 
		    int* azoff, int* eloff, int* dkoff);

	unsigned rep() {
	  return current_.irep;
	}

	unsigned nextRep() {
	  return offsets_.irep;
	}

	ScanCacheOffset& nextOffset(gcp::util::TimeVal& mjd);

	ScanCacheOffset& nextOffsetTimeJump(gcp::util::TimeVal& mjd);

	/**
	 * Return true if the scan finished on the last call to nextOffset()
	 */
	bool justFinished() {
	  return justFinished_;
	}

	unsigned& lastReq() {
	  return lastReq_;
	}

	ScanMode& currentState() {
	  return current_.currentState;
	}

	ScanMode& nextState() {
	  return current_.nextState;
	}
	
	bool add() {
	  return add_;
	}

	unsigned flag() {
	  return current_.flag;
	}

	unsigned index() {
	  return current_.index;
	}

	unsigned char* name() {
	  return name_;
	}

	void pack(signed* s_elements);

	void operator=(Scan& scan);
	void operator=(const Scan& scan);

	friend std::ostream& operator<<(std::ostream& os, Scan& scan);

      public:

	// Variable to keep track of the time elapsed since the last
	// call to get the scan offsets

	bool checkLastMjd_;
	gcp::util::TimeVal mjdLast_;
	gcp::util::TimeVal mjdDiff_;

	bool initialized_;

	unsigned char name_[SCAN_LEN];
	ScanCache offsets_;

	ScanCacheOffset current_;

	unsigned lastReq_;

	unsigned lastAck_;

	bool justFinished_;

	// True if the offsets are to be added to the current scan
	// offsets, or are absolute scan offsets

	bool add_;

      }; // End class Scan

      std::ostream& operator<<(std::ostream& os, Scan& scan);

    } // End namespace control
  } // End namespace antenna
} // End namespace gcp



#endif // End #ifndef GCP_ANTENNA_CONTROL_SCAN_H
