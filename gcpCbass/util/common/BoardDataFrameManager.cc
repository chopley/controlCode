#include "gcp/util/common/BoardDataFrameManager.h"
#include "gcp/util/common/DataFrameNormal.h"
#include "gcp/util/common/FrameFlags.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
BoardDataFrameManager::BoardDataFrameManager(string regmap, string board,
					     bool archivedOnly) :
  RegMapDataFrameManager(archivedOnly)
{
  record_ = 0;
  arrayMap_ = 0;

  if((arrayMap_ = new_ArrayMap())==0)
    ThrowError("Unable to allocate array map");

  arrRegMap_ = 0;
  arrRegMap_ = arrayMap_->findArrRegMap(regmap);

  if(arrRegMap_ == 0)
    ThrowError("Register map " << regmap << " not found in array map");

  brd_ = 0;
  brd_ = arrayMap_->findArrayMapBoard(regmap, board);

  if(brd_ == 0)
    ThrowError("Board " << regmap << "." << board 
	       << " not found in array map");

  // If we are only recording archived registers, frame size is the
  // size of the archived register map

  unsigned frameSize = SCAN_BUFF_BYTE_SIZE(brd_->nByte(archivedOnly));

  // Make the frame large enough to accomodate the register map

  frame_ = new gcp::util::DataFrameNormal(frameSize);

  // Initialize the nBuffer variable

  nBuffer_ = frameSize;
}

/**.......................................................................
 * Destructor.
 */
BoardDataFrameManager::~BoardDataFrameManager() 
{
  if(arrayMap_) {
    arrayMap_ = del_ArrayMap(arrayMap_);
  }
}

/**.......................................................................
 * Get the descriptor for this reg map block
 */
RegMapBlock* BoardDataFrameManager::getReg(string block)
{
  RegMapBlock* blk=0; 
  
  // Look up the requested block
  
  if(brd_ == 0)
    ThrowError("Board is NULL");

  blk = brd_->findRegMapBlock(block);

  // Check that the block requested was a valid one
  
  if(blk==0) {
    ThrowError("Register " << block 
	       << " not found in board: " << brd_->name);
  }
  
  return blk;
}


/**.......................................................................
 * Return the offset in bytes of the data for the requested
 * and register, from the beginning of the frame buffer.
 */
int BoardDataFrameManager::
byteOffsetInFrameOf(RegMapBlock* blk, Coord* coord)
{
  int regOffset = blk->byteOffsetInBoardOf(archivedOnly_, coord);
  
  if(regOffset < 0)
    return -1;
  
  return byteOffsetInFrameOfData() + regOffset;
}

/**.......................................................................
 * Return the offset in bytes of the data for the requested
 * and register, from the beginning of the frame buffer.
 */
int BoardDataFrameManager::
byteOffsetInFrameOf(RegMapBlock* blk, CoordRange* range)
{
  Coord coord, *coordPtr=0;

  if(range != 0) {
    coord = range->startCoord();
    coordPtr = &coord;
  }

  return byteOffsetInFrameOf(blk, coordPtr);
}

void BoardDataFrameManager::setReceived(bool received)
{
  unsigned char recv = received ? FrameFlags::RECEIVED : 
    FrameFlags::NOT_RECEIVED;

  writeBoardReg("received", &recv);
}

void BoardDataFrameManager::setMjd(RegDate& date)
{
  writeBoardReg("utc", date.data());
}

void BoardDataFrameManager::incrementRecord()
{
  ++record_;
  writeBoardReg("record", &record_);
}

