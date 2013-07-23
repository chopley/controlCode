#define __FILEPATH__ "antenna/control/specific/Board.cc"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include "gcp/antenna/control/specific/Board.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor function
 */
Board::Board() 
{
  board_ = 0;
  share_ = 0;
  hasBoard_ = false;
}

/**.......................................................................
 * Constructor function
 */
Board::Board(SpecificShare* share, string name)
{
  // Initialize the private elements to something safe
  
  board_    = 0;
  share_    = 0;
  hasBoard_ = true;
  
  // Sanity check arguments
  
  if(share == 0)
    ThrowError("Received NULL SpecificShare descriptor");
  
  share_ = share;
  
  board_ = share_->findRegMapBoard(name);

  if(board_==0)
    ThrowError("Lookup of board: " << name << " failed");
}

/**.......................................................................
 * Constructor function
 */
Board::Board(SpecificShare* share, AntNum ant)
{
  // Initialize the private elements to something safe
  
  board_    = 0;
  share_    = 0;
  hasBoard_ = true;
  
  // Sanity check arguments
  
  if(share == 0)
    throw Error("Board::Board: Received NULL SpecificShare descriptor.\n");
  
  // Check that this antenna descriptor refers to a valid single
  // Antenna (AntNum descriptors can also be unions of several
  // antennas)
  
  if(!ant.isValidSingleAnt())
    ThrowError("Received invalid antenna designation");
  
  share_ = share;
  
  board_ = share_->findRegMapBoard((char*)(ant.getAntennaName().data()));
  
  if(board_ == 0) {
    ThrowError("Lookup of board \"" << ant.getAntennaName().data()
	       << "\" failed.");
  }
}

/**.......................................................................
 * Constructor function for a virtual board
 */
Board::Board(SpecificShare* share)
{
  // Initialize the private elements to something safe
  
  board_    = 0;
  share_    = 0;
  hasBoard_ = false;
  
  if(share == 0)
    throw Error("Board::Board: Received NULL SpecificShare descriptor.\n");
  
  share_ = share;
}

/**.......................................................................
 * Empty function body for the virtual destructor
 */
Board::~Board() {};

/**.......................................................................
 * Look up a register of this board
 */
RegMapBlock* Board::findReg(char* name)
{
  RegMapBlock* block = 0;
  
  block = find_RegMapBoard_Block(board_, name);
  
  // find_RegMapBoard_Block() returns NULL on error
  
  if(block == 0) {
    ostringstream os;
    os << "Board::findReg: Lookup of register \"" << name
       << "\" on board: " << board_->name
       << " failed." << ends;
    throw Error(os.str());
  }
  
  return block;
}

/**.......................................................................
 * A public function to verify that this board is reachable
 */
bool Board::isReachable()
{
  // SpecificShare::verifyBoard() returns true if an error occurred trying
  // to reach the board, so a false value signifies the board is
  // reachable
  
  return !share_->verifyBoard(board_->number);
}

/**.......................................................................
 * Return the index of this board in the register database
 */
int Board::getIndex()
{
  return board_->number;
}

/**.......................................................................
 * Method to read a register from this board.
 */
void Board::readReg(RegMapBlock* blk, unsigned int first, 
		    unsigned int nreg, unsigned int* value)
{
  CoordRange range(first, first+nreg-1);
  share_->readReg(blk, value, &range);
}

void Board::readReg(RegMapBlock* blk, unsigned int first, 
		    unsigned int nreg, float* value)
{
  CoordRange range(first, first+nreg-1);
  share_->readReg(blk, value, &range);
}

void Board::readRegNoLock(RegMapBlock* blk, unsigned int first, 
		    unsigned int nreg, unsigned int* value)
{
  CoordRange range(first, first+nreg-1);
  share_->readRegNoLock(blk, value, &range);
}

void Board::Board::writeRegNoLock(RegMapBlock* blk, unsigned int first, unsigned int nreg, signed char* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeRegNoLock(blk, value, &range);  
}

void Board::Board::writeRegNoLock(RegMapBlock* blk, unsigned int first, unsigned int nreg, unsigned char* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeRegNoLock(blk, value, &range);  
}

void Board::Board::writeRegNoLock(RegMapBlock* blk, unsigned int first, unsigned int nreg, short* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeRegNoLock(blk, value, &range);  
}

void Board::Board::writeRegNoLock(RegMapBlock* blk, unsigned int first, unsigned int nreg, unsigned short* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeRegNoLock(blk, value, &range);  
}

void Board::Board::writeRegNoLock(RegMapBlock* blk, unsigned int first, unsigned int nreg, int* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeRegNoLock(blk, value, &range);  
}

void Board::Board::writeRegNoLock(RegMapBlock* blk, unsigned int first, unsigned int nreg, unsigned int* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeRegNoLock(blk, value, &range);  
}

void Board::Board::writeRegNoLock(RegMapBlock* blk, unsigned int first, unsigned int nreg, float* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeRegNoLock(blk, value, &range);  
}

void Board::Board::writeRegNoLock(RegMapBlock* blk, unsigned int first, unsigned int nreg, double* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeRegNoLock(blk, value, &range);  
}

void Board::Board::writeReg(RegMapBlock* blk, unsigned int first, unsigned int nreg, signed char* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeReg(blk, value, &range);  
}

void Board::Board::writeReg(RegMapBlock* blk, unsigned int first, unsigned int nreg, unsigned char* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeReg(blk, value, &range);  
}

void Board::Board::writeReg(RegMapBlock* blk, unsigned int first, unsigned int nreg, short* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeReg(blk, value, &range);  
}

void Board::Board::writeReg(RegMapBlock* blk, unsigned int first, unsigned int nreg, unsigned short* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeReg(blk, value, &range);  
}

void Board::Board::writeReg(RegMapBlock* blk, unsigned int first, unsigned int nreg, int* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeReg(blk, value, &range);  
}

void Board::Board::writeReg(RegMapBlock* blk, unsigned int first, unsigned int nreg, unsigned int* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeReg(blk, value, &range);  
}

void Board::Board::writeReg(RegMapBlock* blk, unsigned int first, unsigned int nreg, float* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeReg(blk, value, &range);  
}

void Board::Board::writeReg(RegMapBlock* blk, unsigned int first, unsigned int nreg, double* value)
{
  CoordRange range(first, first+nreg-1);
  share_->writeReg(blk, value, &range);  
}

