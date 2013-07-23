#ifndef BOARD_H
#define BOARD_H

/**
 * @file Board.h
 * 
 * Tagged: Thu Nov 13 16:53:34 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/control/code/unix/libunix_src/common/regmap.h"
#include "gcp/util/common/AntNum.h"

namespace gcp {
  namespace antenna {
    namespace control {
      
      class SpecificShare;

      /**
       * A class which encapsulates resources of a board of the shared
       * register map.
       */     
      class Board {
	
      public:
	
	/**
	 * Constructor looks up a board by name and stores a pointer
	 * to it in private member board_ (below).
	 *
	 * @throws Exception
	 */
	Board(SpecificShare* share, std::string name);
	
	/**
	 * Constructor looks up the rx board corresponding to the
	 * requested antenna and stores a pointer to it in private
	 * member board_ (below)
	 *
	 * @throws Exception
	 */
	Board(SpecificShare* share, gcp::util::AntNum ant);
	
	/**
	 * Constructor for a virtual board
	 *
	 * @throws Exception
	 */
	Board(SpecificShare* share);
	Board();

	/**
	 * Declaration of destructor as pure virtual prevents
	 * instantiation of this base class
	 */
	virtual ~Board();
	
	/**
	 * Return a pointer to a register of the board managed by this
	 * object.
	 *
	 * @throws Exception
	 */
	RegMapBlock* findReg(char* name);
	
	/**
	 * Verify that this board is reachable.
	 *
	 * @throws (indirectly) Exception
	 */
	bool isReachable();
	
	/**
	 * Function to reset private members of a board-management
	 * object.  Should be overwritten by classes which inherit
	 * from Board
	 *
	 * @throws Exception
	 */
	virtual void reset() {};
	
	/**
	 * Public function to return the index of this board in the
	 * register database
	 */
	int getIndex();
	
	/**
	 * Method to read a register from this board.
	 */
	virtual void readRegNoLock(RegMapBlock* blk, unsigned int first, 
			     unsigned int nreg, unsigned int* value);
	
	virtual void readReg(RegMapBlock* blk, unsigned int first, 
			     unsigned int nreg, unsigned int* value);
	
	virtual void readReg(RegMapBlock* blk, unsigned int first, 
			     unsigned int nreg, float* value);

	/**
	 * Methods to write to a register of this board.
	 */
	//virtual void writeReg(RegMapBlock* blk, unsigned int first, unsigned int nreg, 
	           //   bool* value);
			      
	virtual void writeRegNoLock(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      signed char* value);

	virtual void writeRegNoLock(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      unsigned char* value);

	virtual void writeRegNoLock(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      signed short* value);

	virtual void writeRegNoLock(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      unsigned short* value);

	virtual void writeRegNoLock(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      int* value);

	virtual void writeRegNoLock(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      unsigned int* value);
/*
	virtual void writeReg(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      long* value);

	virtual void writeReg(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      unsigned long* value);
*/
	virtual void writeRegNoLock(RegMapBlock *blk, unsigned first, unsigned nreg, 
		      float* value);

	virtual void writeRegNoLock(RegMapBlock *blk, unsigned first, unsigned nreg, 
		      double* value);
		      
		      
	//virtual void writeReg(RegMapBlock* blk, unsigned int first, unsigned int nreg, 
	           //   bool* value);
			      
	virtual void writeReg(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      signed char* value);

	virtual void writeReg(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      unsigned char* value);

	virtual void writeReg(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      signed short* value);

	virtual void writeReg(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      unsigned short* value);

	virtual void writeReg(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      int* value);

	virtual void writeReg(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      unsigned int* value);
/*
	virtual void writeReg(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      long* value);

	virtual void writeReg(RegMapBlock* blk, unsigned first, unsigned nreg, 
		      unsigned long* value);
*/
	virtual void writeReg(RegMapBlock *blk, unsigned first, unsigned nreg, 
		      float* value);

	virtual void writeReg(RegMapBlock *blk, unsigned first, unsigned nreg, 
		      double* value);	      
		      
      protected:
	
	/**
	 * The resource object of the shared memory database
	 */
	SpecificShare* share_;
	
	/**
	 * A pointer to the board this object refers to.
	 */
	RegMapBoard* board_;
	
	/**
	 * True if this Board has a real board
	 * corresponding to it.  
	 */
	bool hasBoard_;  
	
      }; // End class Board
      
    }; // End namespace control
  }; // End namespace antenna
} // End namespace gcp

#endif
