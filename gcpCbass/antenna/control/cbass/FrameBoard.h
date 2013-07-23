#ifndef FRAMEBOARD_H
#define FRAMEBOARD_H

/**
 * @file FrameBoard.h
 * 
 * Tagged: Thu Nov 13 16:53:38 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/antenna/control/specific/Board.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      
      /**
       * A class to encapsulate details of the frame software board.
       */
      class FrameBoard : public Board {
	
      public:
	
	/**
	 * Constructor function for this board
	 *
	 * @throws Exception (via Board::findReg) 
	 */
	FrameBoard(SpecificShare* share, std::string name);
	
	/**
	 * Update the record number
	 *
	 * @throws Exception (via SpecificShare::writeReg) 
	 */
	void archiveRecordNumber(unsigned record);
	
	/**
	 * Set the time in the register database
	 *
	 * @throws Exception (via SpecificShare::writeReg) 
	 */
	void setTime();

	/**
	 * Record the time in the register database
	 *
	 * @throws Exception (via SpecificShare::writeReg) 
	 */
	void archiveTime();
	
	/**
	 * Update the features bitmask
	 *
	 * @throws Exception (via SpecificShare::writeReg)
	 */
	void archiveFeatures(unsigned features, unsigned seq);
	
	/**
	 * Update the Walsh state in effect during the last frame.
	 *
	 * @throws Exception (via SpecificShare::writeReg) 
	 */
	void archiveWalshState(unsigned walshState);
	
	/**
	 * Update the local register that contains a count of the
	 * number of integrated snapshots per frame.
	 *
	 * @throws Exception;
	 */
	void archiveNsnap(unsigned nsnap);
	
	/**
	 * Stub this out until needed.
	 */
	void reset() {};
	
      private:
	
	/**
	 * The integration count 
	 */
	RegMapBlock* nsnap_;     
	/**
	 * The snapshot number 
	 */
	RegMapBlock* record_;    
	/**
	 * The current MJD UTC (days,ms) 
	 */
	RegMapBlock* utc_;       
	/**
	 * A bit-mask union of feature markers. 
	 */
	RegMapBlock* features_;  
	/**
	 * The sequence number of the last feature-mark command
	 * received from the control program.
	 */
	RegMapBlock* markSeq_;  
	
	/**
	 * A bit-mask of Walsh states for the various receivers.
	 */
	RegMapBlock* walshstate_;
	
      }; // End class FrameBoard
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif // End #ifndef 
