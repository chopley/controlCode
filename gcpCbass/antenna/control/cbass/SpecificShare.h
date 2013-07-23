#ifndef SPECIFICSHARE_H
#define SPECIFICSHARE_H

/**
 * @file SpecificShare.h
 * 
 * Tagged: Thu Nov 13 16:53:54 UTC 2003
 * 
 * @author Erik Leitch
 */
#include <string>

#include <pthread.h>

#include "gcp/util/common/DoubleBufferVec.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/util/common/AntennaDataFrameManager.h"
#include "gcp/util/common/QuadPath.h"

#include "gcp/antenna/control/specific/PolarEncoderPos.h"
#include "gcp/antenna/control/specific/Site.h"

// C header files from the array control code

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"
#include "gcp/control/code/unix/libunix_src/common/astrom.h"

#include "gcp/util/common/Directives.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      class AcuPort;

      /**
       * An instance of this class is created by AntennaMaster and
       * passed to the constructors of other tasks. It contains
       * resources that are shared between all antenna tasks.
       */
      class SpecificShare : public gcp::util::AntennaDataFrameManager {
	
      public:
	
	/**
	 * Enumerate supported timeouts.
	 */
	enum TimeOut {
	  NO_WAIT, 
	  WAIT_FOREVER
	};
	    
	// Define the fundamental hardware tick rate used to
	// synchronize all antenna tracking A pulse with this period
	// is distributed to all tracking and receiver hardware to
	// synchronize antenna positions and data acquisition samples
	
        static const unsigned int NANOSECONDS_PER_POSITION = 10000000;
	
	//------------------------------------------------------------
	// An object to manage buffered registers
	//------------------------------------------------------------

	struct Block {

	  // Public members

	  SpecificShare* parent_;
	  RegMapBlock* blk_;
	  unsigned nElPerSample_;
	  unsigned nFastElPerFrame_;
	  unsigned nBytePerSample_;
	  unsigned nBytePerEl_;
	  unsigned nBytePerFrame_;
	  gcp::util::DoubleBufferVec<unsigned char> buf_;
	  unsigned char* bufPtr_;
	  unsigned char* framePtr_;

	  // Methods

	  Block(char* boardName, char* blockName, 
		SpecificShare* parent);

	  virtual ~Block();

	  void buffer(unsigned iSamp, void* data, bool print=false);
	  void buffer(void* data, bool print=false);

	  void switchBuffers();
	  void copyDataToFrame();
	};


	//------------------------------------------------------------
	// SpecificRegDb Object
	//------------------------------------------------------------
	
	/**
	 * Access to the register database and its shadow registers is
	 * provided through an object of the following type.
	 */
	class SpecificRegDb {
	  
	public:
	  
	  /**
	   * Constructor.
	   *
	   * @throws Exception
	   */
	  SpecificRegDb();
	  
	  /**
	   * Destructor.
	   */
	  ~SpecificRegDb();
	  
	  /**
	   * Acquire exclusive access to the database.
	   *
	   * @throws Exception
	   */
	  void grabRegs(TimeOut timeout);
	  
	  /**
	   * Release the database.
	   *
	   * @throws Exception
	   */
	  void ungrabRegs();
	  
	  /**
	   * Look up a board in the register map
	   */
	  RegMapBoard* findRegMapBoard(std::string boardName);
	  
	  /**
	   * Flag a given register board as unreachable. This sets the
	   * status register of the specified board to 1. Note that
	   * readReg() and writeReg() don't check the status of this
	   * register before accessing other registers, so if you want
	   * to avoid I/O errors, call verifyBoard() before calling
	   * readReg() or writeReg().
	   *
	   *  @param board int  The index of the board to flag.
	   *
	   *  @throws Exception
	   */
	  void flagBoard(int board);
	  
	  /**
	   * Mark a given register board as reachable. This sets the
	   * status register of the specified board to 0.
	   *
	   * @param board int The index of the board to unflag.
	   *
	   * @throws Exception
	   */
	  void unflagBoard(int board);
	  
	  /**
	   * Return the value of the status register of a given
	   * register board.  This will be zero if the board is
	   * reachable, or non-zero otherwise.
	   *
	   * returns    bool    true  - The board is marked as ok.
	   *                   false - The board is marked as unreachable, or
	   *                           an error occured in the function.
	   *
	   * @param board  int The index of the board to verify.
	   *
	   * @throws Exception
	   */
	  bool verifyBoard(int board);
	  
	private:
	  
	  /**
	   * SpecificShare will access our members directly.
	   */
	  friend class SpecificShare;
	  
	  /**
	   * The EXPSTUB register map
	   */
	  RegMap *regmap_;      

	  /**
	   * Database mutual-exclusion guard semaphore
	   */
	  pthread_mutex_t guard_;  
	  
	  /**
	   * Shadow and local register buffer
	   */
	  unsigned *shadow_;       
	  /**
	   * The dimension of shadow[]
	   */
	  int nshadow_;            
	  
	  /**
	   * This is a private function that returns a pointer to the
	   * shadow status register of a given register board. The
	   * caller should lock the database and have checked the
	   * validity of share and board before calling this function.
	   *
	   * returns  unsigned *  The pointer alias of the shadow 
	   *                      status register.
	   * @param board  int  The index of the board.
	   */
	  unsigned* boardStatusReg(int board);
	  
	};
	
	//------------------------------------------------------------
	// SpecificClock Object
	//------------------------------------------------------------
	
	/**
	 * The following structure is used by the EXPSTUB UTC clock.
	 */
	class SpecificClock {
	  
	public:
	  
	  /** 
	   * Constructor.
	   */
	  SpecificClock();
	  
	  /**
	   * Destructor.
	   *
	   * @throws Exception
	   */
	  ~SpecificClock();
	  
	  /**
	   * Set the current time.
	   *
	   * @throws Exception
	   */
	  void setClock(unsigned long mjd, unsigned long sec, 
			unsigned long nanoSeconds);
	  
	  /**
	   * Set the current time via a TimeVal ref.
	   *
	   * @throws Exception
	   */
	  void setClock(gcp::util::TimeVal& time);
	  
	  /**
	   * Fill the internal time representation with the current
	   * time.
	   *
	   * @throws Exception
	   */
	  void setClock();

	  /**
	   * Get the current UTC as a Modified Julian Date.
	   *
	   * @throws Exception
	   */
	  double getUtc();

	  
	private:
	  
	  /**
	   * The mutual exclusion guard of the clock
	   */
	  pthread_mutex_t guard_; 
	  
	  gcp::util::TimeVal time_;
	};
	
	//----------------------------------------------------------------------
	// SpecificAstrom Object
	//------------------------------------------------------------
	
	/**
	 * Encapsulate the astrometry details of the array in this
	 * class.
	 */
	class SpecificAstrom {
	  
	public:
	  
	  /**
	   * Constructor.
	   *
	   * @throws Exception
	   */
	  SpecificAstrom();
	  
	  /**
	   * Destructor.
	   */
	  ~SpecificAstrom();
	  
	  /**
	   * Record new site-location details in share->site.
	   *
	   * @throws Exception
	   */
	  void setSite(double longitude, double latitude, double altitude);
	  
	  /**
	   * Get a copy of the EXPSTUB site-specification object.
	   *
	   * @throws Exception
	   */
	  void getSite(gcp::antenna::control::Site *site);
	  
	  /**
	   * Extend the quadratic interpolation table of ut1 - utc
	   * versus MJD UTC.
	   *
	   * @throws Exception
	   */
	  void extendUt1Utc(double utc, double ut1utc);
	  
	  /**
	   * Extend the quadratic interpolation table of the equation
	   * of the equinoxes versus Terrestrial Time (as a Modified
	   * Julian Date).
	   *
	   * @throws Exception
	   */
	  void extendEqnEqx(double tt, double eqneqx);
	  
	  /**
	   * Get the value of UT1-UTC for a given UTC.
	   *
	   * @throws Exception
	   */
	  double getUt1Utc(double utc);
	  
	  /**
	   * Get the value of the equation of the equinoxes for a
	   * given terrestrial time.
	   *
	   * @throws Exception
	   */
	  double getEqnEqx(double tt);
	  
	private:
	  
	  /**
	   * The mutual exclusion guard of this shared object
	   */
	  pthread_mutex_t guard_;   
	  
	  /**
	   * The location of the EXPSTUB
	   */
	  gcp::antenna::control::Site site_; 
	  
	  /**
	   * Quadratic interpolation table of UT1-UTC in seconds,
	   * versus UTC, expressed as a Modified Julian Date.
	   */
	  gcp::util::QuadPath* ut1utc_;     
	  
	  /**
	   * Quadratic interpolation table of the equation of the
	   * equinoxes in radians, versus Terrestrial Time expressed
	   * as a Modified JulianDate.
	   */
	  gcp::util::QuadPath* eqneqx_;     
	  
	}; // Class SpecificAstrom
	
	//------------------------------------------------------------
	// SpecificPmacLock Object
	//------------------------------------------------------------
	
	/**
	 * The following structure encapsulates the reader lock used
	 * to prevent clashes when freezing and unfreezing the pmac
	 * monitor registers.
	 */
	class SpecificPmacLock { 
	  
	public:
	  
	  /**
	   * Constructor.
	   *
	   * @throws Exception
	   */
	  SpecificPmacLock(SpecificShare *share);
	  
	  /**
	   * Destructor.
	   *
	   * @throws Exception
	   */
	  ~SpecificPmacLock();
	  
	private:
	  
	  /**
	   * The mutex that protects access to the counting semaphore
	   */
	  pthread_mutex_t guard_;  
	  
	  /**
	   * The number of readers
	   */
	  int count_;              
	  
	  /**
	   * The pmac register-map entry
	   */
	  RegMapBoard *pmac_;      
	  
	  /**
	   * The pmac.host_read register
	   */
	  RegMapBlock *host_read_; 

	}; // End class SpecificPmacLock
	
	//------------------------------------------------------------
	// SpecificShare Methods
	//------------------------------------------------------------
	
	/**
	 * Struct used to store names and IP addresses of trusted
	 * hosts.
	 */
	struct HostAddress {
	  char* name;
	  char* address;
	};
	
	// Static members and functions
	
	/**
	 * Static pointer for use in signal handlers.
	 */
	static SpecificShare* share;
	
	/**
	 * An array of trusted hosts.
	 */
	static HostAddress host_address[];
	
	/**
	 * Validate a specified IP address or host-name alias, and
	 * return a malloc'd copy of the resulting IP address.
	 *
	 * returns  char *      The IP address of the host, or NULL on error.
	 *                      This should be free'd when no longer required.
	 *
	 * @param  host char *  The IP address or host-name alias of the host.
	 */
	static std::string hostIpAddress(std::string host);
	
	/**
	 * This is a signal handler for trapping bus and address
	 * errors while readReg() and writeReg() access PCI
	 * registers. It simply calls longjmp to abort the I/O
	 * operation and return to an error catching part of the
	 * originating function.
	 */
	static void pciBusErrorHandler(int sig);
	
	/**
	 * This is a signal handler for trapping bus and address
	 * errors while readReg() and writeReg() access PCI
	 * registers. It simply calls longjmp to abort the I/O
	 * operation and return to an error catching part of the
	 * originating function.
	 */
	static void pciSegvErrorHandler(int sig);
	
	/**
	 * Constructor.
	 *
	 * @throws Exception
	 */
	SpecificShare(std::string host);
	
	/**
	 * Destructor.
	 */
	~SpecificShare();
	
	/**
	 * Acquire exlusive use of the register database.  The
	 * following grab and ungrab calls can be used to bracket
	 * multiple calls to (raw_)read_sza_reg() and/or
	 * (raw_)write_sza_reg() to prevent other tasks from gaining
	 * access to the specific registers between calls. These functions
	 * must be called by users of the raw_ read and write
	 * functions.
	 *
	 * @throws Exception
	 */
	void grabRegs(TimeOut timeout);
	
	/**
	 * Relinquish exclusive use to the register database.
	 *
	 * @throws Exception
	 */
	void ungrabRegs();
	
	/*
	 * Flag a board as unreachable. Note that this is called
	 * automatically by read_regdb() and write_regdb() if their
	 * 'check' argument is true and an exception occurs. If
	 * 'check' is false then the caller is expected to perform
	 * exception handling and to call the following function if an
	 * exception occurs.
	 *
	 * @throws Exception
	 */
	void flagBoard(int board);
	
	/*
	 * This function will be called by the scanner task after
	 * successfully reading all registers of a previously flagged
	 * board without incuring an exception. It marks the board as
	 * usable by other tasks.
	 *
	 * @throws Exception
	 */
	void unflagBoard(int board);
	
	/**
	 * Return false if a board is flagged as unreachable.
	 *
	 * @throws Exception
	 */
	bool verifyBoard(int board);
	
	/**
	 * Get the Local Sidereal Time that corresponds to a given MJD UTC.
	 *
	 * @throws Exception
	 */
	double getLst(double utc);
	
	/**
	 * Get the terrestrial time as MJD
	 */
	double getTt(double lst);
	
	/**
	 * Return the Utc as MJD
	 *
	 * @throws Exception
	 */
	double getUtc();
	
	/**
	 * Set the current time.
	 *
	 * @throws Exception
	 */
	void setClock(unsigned long mjd, unsigned long sec, 
		      unsigned long nanoSeconds);
	
	/**
	 * Set the current time via a TimeVal ref.
	 *
	 * @throws Exception
	 */
	void setClock(gcp::util::TimeVal& time);

	/**
	 * Set the current time.
	 *
	 * @throws Exception
	 */
	void setClock();
	
	public:

	/**
	 * Public function to set the site parameters
	 *
	 * @throws Exception
	 */
	void setSite(double longitude, double latitude, double altitude);
	
	/**
	 * Get the value of UT1-UTC for a given UTC.
	 *
	 * @throws Exception
	 */
	double getUt1Utc(double utc);
	
	/**
	 * Get the value of the equation of the equinoxes for a given
	 * terrestrial time.
	 *
	 * @throws Exception
	 */
	double getEqnEqx(double tt);
	
	/**
	 * Extend the quadratic interpolation table of ut1 - utc
	 * versus MJD UTC.
	 *
	 * @throws Exception
	 */
	void extendUt1Utc(double utc, double ut1utc);
	
	/**
	 * Extend the quadratic interpolation table of the equation of
	 * the equinoxes versus Terrestrial Time (as a Modified Julian
	 * Date).
	 *
	 * @throws Exception
	 */
	void extendEqnEqx(double tt, double eqneqx);
	
	/**
	 * Return a pointer to the requested register
	 */
	RegMapBoard* findRegMapBoard(std::string boardName);
	
	/**
	 * Freeze the pmac readout
	 *
	 * @throws Exception
	 */
	void freezePmacReadout();
	
	/**
	 * Un-freeze the pmac readout
	 *
	 * @throws Exception
	 */
	void unfreezePmacReadout();
	
	/**
	 * Return the number of boards in the register map
	 */
	unsigned int getNboard();
	
	/**
	 * Return the number of archived registers in the register map
	 */
	unsigned int getNarchived();
	
	/**
	 * Return the total number of registers in the register map
	 */
	unsigned int getNreg();

	/**
	 * Return the number of bytes
	 */
	unsigned int getNbyte();

	/**
	 * Public method to pack a frame
	 *
	 * @throws Exception
	 */
	void packFrame(gcp::util::DataFrameManager* frame);
	
	void debugTrigger();
	
	void setNewFramePending() {newFramePending_ = true;};
	
	void clearNewFramePending() {newFramePending_ = false;};
	
	bool isNewFramePending() {return newFramePending_;};
	
	// Method to add an element to the list of registers being
	// buffered

	Block* addBufferedBlock(char* boardName, char* blockName);

	// Method to copy data to the frame

	void copyBufferedBlocksToFrame();

	// Method to switch buffers

	void switchBuffers();

      private:
	
	// A vector of blocks that are currently being buffered

	std::vector<Block*> bufferedBlocks_;

	/**
	 * A temporary buffer for use in read() methods
	 */
	unsigned* tmpbuf_;         
	
	/**
	 * The IP address of the control host
	 */
	std::string controlHost_;       
	
	/**
	 * The specific register database
	 */
	SpecificRegDb *regdb_;      
	
	/**
	 * True if tracker has requested a new frame but it has not
	 * yet been provided Needs to be volatile because accessors
	 * are inline, and otherwise compiler optimizes away the
	 * writes
	 */
	volatile bool newFramePending_;    
	
	/**
	 * The Specific UTC clock
	 */
	SpecificClock *clock_;          
	  	
	/**
	 * Astrometry information
	 */
	SpecificAstrom *astrom_;        
	
	/**
	 * PMAC dual-port-ram read-lock
	 */
	SpecificPmacLock *pmac_lock_;   
	
	gcp::util::TimeVal startTime_;
	gcp::util::TimeVal endTime_;

      }; // class SpecificShare
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif
