#ifndef DRIVEMODE_H
#define DRIVEMODE_H

/**
 * @file DriveMode.h
 * 
 * Tagged: Thu Nov 13 16:53:45 UTC 2003
 * 
 * @author Erik Leitch
 */

namespace gcp {
  namespace util {
    
    /**
     * Class to define valid Drive command modes.
     */
    class DriveMode {

    public:
      /**
       * Enumerate the drive control modes
       */
      enum Mode {
	
	// Follow tracking targets
	
	TRACK,
	
	// Slew the telescope to a given position then stop
	
	SLEW,
	
	// Bring the telesope to a stop ASAP
	
	HALT,
	
	// Start a new track when a pulse is received from the
	// time-code reader.
	
	SYNC,
	
	// Tell the drive to reboot itself
	
	REBOOT
      };
      
    }; // End class DriveMode
    
  }; // End namespace util
}; // End namespace gcp

#endif // End #ifndef 
