#define __FILEPATH__ "util/common/RegMapDataFrameManager.cc"

#include "gcp/util/common/RegAxisRange.h"
#include "gcp/util/common/RegMapDataFrameManager.h"
#include "gcp/util/common/BoardDataFrameManager.h"
#include "gcp/util/common/FrameFlags.h"
#include "gcp/util/common/DataFrameNormal.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"

using namespace gcp::util;
using namespace std;


/**.......................................................................
 * Constructor.
 */
RegMapDataFrameManager::RegMapDataFrameManager(string regmap, 
					       bool archivedOnly) 
{
  arrayMap_ = 0;

  if((arrayMap_ = new_ArrayMap())==0)
    ThrowError("Unable to allocate array map");

  regMap_ = 0;
  regMap_ = arrayMap_->findRegMap(regmap, false);

  if(regMap_ == 0)
    ThrowError("Register map " << regmap
	       << " not found in array map");

  // If we are only recording archived registers, frame size is the
  // size of the archived register map

  unsigned frameSize = SCAN_BUFF_BYTE_SIZE(regMap_->nByte(archivedOnly));

  // Make the frame large enough to accomodate the register map

  frame_ = new gcp::util::DataFrameNormal(frameSize);

  // Initialize the nBuffer variable

  nBuffer_ = frameSize;

  archivedOnly_ = archivedOnly;
}

/**.......................................................................
 * Constructor.
 */
RegMapDataFrameManager::RegMapDataFrameManager(bool archivedOnly) 
{
  record_ = 0;
  regMap_ = 0; // Just initialize the regmap pointer to null --
	       // inheritors should allocate this according to which
	       // regmap is appropriate for them.
  
  archivedOnly_ = archivedOnly;

  DBPRINT(true, Debug::DEBUG10, "archivedOnly = " << archivedOnly);
}

/**.......................................................................
 * Destructor.
 */
RegMapDataFrameManager::~RegMapDataFrameManager() 
{
  if(regMap_ != 0) {
    delete regMap_;
    regMap_ = 0;
  }
}

/**.......................................................................
 * Overloaded assignment operator from the base class
 */
void RegMapDataFrameManager::operator=(DataFrameManager& fm)
{
  // Just call our assignment operator
  
  operator=((RegMapDataFrameManager&) fm);
}

/**.......................................................................
 * Assignment operator
 */
void RegMapDataFrameManager::operator=(RegMapDataFrameManager& fm)
{
  // If both frames have the same internal specifications, the copy is
  // trivial
  
  if(fm.archivedOnly_ == archivedOnly_) {
    
    // Check that these frames are the same size
    
    if(fm.frame_->nByte() == frame_->nByte()) {
      
      // If they are, just call the base-class assignment operator
      
      DataFrameManager::operator=((DataFrameManager&) fm);
      
    } else {
      ThrowError("Frames are not from equivalent register maps.\n");
    }
    
    // Else we have to iterate over registers, checking which ones
    // from the source frame should be copied to the destination frame
    
  } else {
    
    lock();
    fm.lock();
    
    try {
      
      // Iterate over all boards in this register map.
      
      for(unsigned int iboard=0; iboard < regMap_->nboard_; iboard++) {
	RegMapBoard* brd = regMap_->boards_[iboard];
	
	// Only copy if this board is present in both source and
	// destination register maps.
	
	if(boardIsPresent(brd) && fm.boardIsPresent(brd)) {
	  
	  // If the board is marked as reachable, attempt to copy the
	  // contents of the board's registers into the frame buffer.
	  
	  if(!boardIsFlagged(brd)) {
	    
	    // Iterate over all register blocks of this board
	    
	    for(unsigned int iblock=0; iblock < brd->nblock; iblock++) {
	      RegMapBlock* blk = brd->blocks[iblock];
	      
	      // If this block is present in the register maps of both
	      // frames, copy it
	      
	      if(blockIsPresent(blk) && fm.blockIsPresent(blk)) {
		
		unsigned iStartDest = byteOffsetInFrameOf(blk);
		unsigned iStartSrc  = fm.byteOffsetInFrameOf(blk);
		
		// Pack without locking the frames, since they will
		// already be locked

		frame_->pack(fm.frame_->getPtr(iStartSrc, DataType::UCHAR), 
			     blk->nByte(), DataType::UCHAR, iStartDest, false);
	      }
	    }
	  }
	}
      }

      // If an exception was thrown, unlock the frames, and rethrow
      
    } catch (Exception& err) {
      
      unlock();
      fm.unlock();
      
      ThrowError(err.what());
      
    } catch (...) {
      unlock();
      fm.unlock();
    }
  }
}

//-----------------------------------------------------------------------
// Methods to get the byte offsets in the register map of a named board
// and block
//-----------------------------------------------------------------------

/**.......................................................................
 * Return the offset in a register map, of the data for this
 * register.
 */
int RegMapDataFrameManager::
byteOffsetInRegMapOf(RegMapBlock* blk, Coord* coord)
{
  if(regMap_ != 0) {
    return regMap_->byteOffsetInRegMapOf(archivedOnly_, blk, coord);
  } else {
    ThrowError("Register map is NULL");
  }
}

int RegMapDataFrameManager::
byteOffsetInRegMapOf(string board, string block, Coord* coord)
{
  RegMapBlock* blk = getReg(board, block);
  return byteOffsetInRegMapOf(blk, coord);
}

/**.......................................................................
 * Return the offset in a register map, of the data for this
 * register.
 */
int RegMapDataFrameManager::
byteOffsetInRegMapOf(RegMapBlock* blk, CoordRange* range)
{
  Coord coord, *coordPtr=0;

  if(range != 0) {
    coord = range->startCoord();
    coordPtr = &coord;
  }
  
  return regMap_->byteOffsetInRegMapOf(archivedOnly_, blk, coordPtr);
}

int RegMapDataFrameManager::
byteOffsetInRegMapOf(string board, string block, CoordRange* range)
{
  RegMapBlock* blk = getReg(board, block);
  return byteOffsetInRegMapOf(blk, range);
}

/**.......................................................................
 * Return the offset in bytes of the data for the requested
 * board, from the beginning of the frame buffer.
 */
int RegMapDataFrameManager::
byteOffsetInFrameOf(string board)
{
  RegMapBoard* brd = findRegMapBoard(board);
  return byteOffsetInFrameOf(brd);
}

/**.......................................................................
 * Return the offset in bytes of the data for the requested
 * board, from the beginning of the frame buffer.
 */
int RegMapDataFrameManager::
byteOffsetInFrameOf(RegMapBoard* brd)
{
  int regOffset = brd->byteOffsetInRegMap(archivedOnly_);

  
  if(regOffset < 0)
    return -1;
  
  return byteOffsetInFrameOfData() + regOffset;
}

/**.......................................................................
 * Return the offset in bytes of the data for the requested board
 * and register, from the beginning of the frame buffer.
 */
int RegMapDataFrameManager::
byteOffsetInFrameOf(RegMapBlock* blk, Coord* coord)
{
  int regOffset = byteOffsetInRegMapOf(blk, coord);
  
  if(regOffset < 0)
    return -1;
  
  return byteOffsetInFrameOfData() + regOffset;
}

int RegMapDataFrameManager::
byteOffsetInFrameOf(string board, string block, Coord* coord)
{
  int regOffset = byteOffsetInRegMapOf(board, block, coord);
  
  if(regOffset < 0)
    return -1;
  
  return byteOffsetInFrameOfData() + regOffset;
}

int RegMapDataFrameManager::
byteOffsetInFrameOf(RegMapBoard* brd, RegMapBlock* blk)
{
  int brdOffset = byteOffsetInFrameOf(brd);
  int blkOffset = blk->byteOffsetInBoardOf(archivedOnly_);

  if(brdOffset < 0 || blkOffset < 0)
    return -1;

  return brdOffset + blkOffset;
}

/**.......................................................................
 * Return the offset in bytes of the data for the requested
 * and register, from the beginning of the frame buffer.
 */
int RegMapDataFrameManager::
byteOffsetInFrameOf(RegMapBlock* blk, CoordRange* range)
{
  Coord coord, *coordPtr=0;

  if(range != 0) {
    coord = range->startCoord();
    coordPtr = &coord;
  }

  return byteOffsetInFrameOf(blk, coordPtr);
}

int RegMapDataFrameManager::
byteOffsetInFrameOf(string board, string block, CoordRange* range)
{
  Coord coord, *coordPtr=0;

  if(range != 0) {
    coord = range->startCoord();
    coordPtr = &coord;
  }
  return byteOffsetInFrameOf(board, block, coordPtr);
}

/**.......................................................................
 * Pack data of an arbitrary type into the underlying frame
 */
void RegMapDataFrameManager::
packData(RegMapBlock* blk, void* data, CoordRange* range, 
	 DataType::Type type, bool lockFrame)
{
  // Calculate the byte offset of the start element of this register
  // from the head of the frame.
  
  int byteOffset = byteOffsetInFrameOf(blk);
  
  // Do nothing if this register isn't archived
  
  if(byteOffset < 0)
    return;
  
  // If the number of bytes was passed as 0, use the default from the
  // block descriptor
  
  AxisRange axisRange(blk->axes_, range);

  //axisRange_.setTo(blk->axes_, range);

  frame_->pack(data, axisRange, type, byteOffset, lockFrame);
}

void RegMapDataFrameManager::
packData(string board, string block, void* data, CoordRange* range, 
	 DataType::Type type, bool lockFrame)
{
  RegMapBlock* blk = getReg(board, block);
  return packData(blk, data, range, type, lockFrame);
}

/**.......................................................................
 * Pack the same value for all elements of a range
 */
void RegMapDataFrameManager::
packValue(RegMapBlock* blk, void* data, CoordRange* range, 
	  DataType::Type type, bool lockFrame)
{
  // Calculate the byte offset of the start element of this register
  // from the head of the frame.
  
  int byteOffset = byteOffsetInFrameOf(blk);
  
  // Do nothing if this register isn't archived
  
  if(byteOffset < 0)
    return;
  
  // If the number of bytes was passed as 0, use the default from the
  // block descriptor
  
  AxisRange axisRange(blk->axes_, range);

  //axisRange_.setTo(blk->axes_, range);
 
  frame_->packValue(data, axisRange, type, byteOffset, lockFrame);
}

/**.......................................................................
 * Pack the same value for all elements of a range
 */
void RegMapDataFrameManager::
packDcValue(RegMapBlock* blk, void* data, CoordRange* range, 
	    DataType::Type type, bool lockFrame)
{
  // Calculate the byte offset of the start element of this register
  // from the head of the frame.
  
  int byteOffset = byteOffsetInFrameOf(blk);
  
  // Do nothing if this register isn't archived
  
  if(byteOffset < 0)
    return;
  
  // If the number of bytes was passed as 0, use the default from the
  // block descriptor

  AxisRange axisRange(blk->axes_, range);

  //axisRange_.setTo(blk->axes_, range);

  frame_->packValue(data, axisRange, type, byteOffset, lockFrame);
}

void RegMapDataFrameManager::
packValue(string board, string block, void* data, CoordRange* range, 
	  DataType::Type type, bool lockFrame)
{
  RegMapBlock* blk = getReg(board, block);
  return packValue(blk, data, range, type, lockFrame);
}

/**.......................................................................
 * Unpack data of an arbitrary type into the underlying frame
 */
void RegMapDataFrameManager::
unpackData(RegMapBlock* blk, void* data, CoordRange* range, 
	   DataType::Type type, bool lockFrame)
{
  // Calculate the byte offset of the start element of this register
  // from the head of the frame.
  
  int byteOffset = byteOffsetInFrameOf(blk);
  
  // Do nothing if this register isn't archived
  
  if(byteOffset < 0)
    return;
  
  // If the number of bytes was passed as 0, use the default from the
  // block descriptor
  
  AxisRange axisRange(blk->axes_, range);

  //axisRange_.setTo(blk->axes_, range);

  frame_->unpack(data, axisRange, type, byteOffset, lockFrame);
}

void RegMapDataFrameManager::
unpackData(string board, string block, void* data, CoordRange* range, 
	   DataType::Type type, bool lockFrame)
{
  RegMapBlock* blk = getReg(board, block);
  return unpackData(blk, data, range, type, lockFrame);
}

/**.......................................................................
 * Get the descriptor for this reg map block
 */
RegMapBlock* RegMapDataFrameManager::getReg(string board, string block)
{
  RegMapBlock* blk=0; 
  LogStream errStr;
  
  // Look up the requested block
  
  if(regMap_ != 0)
    blk = regMap_->findRegMapBlock(board, block, true);
  else {
    ThrowError("Register map is NULL");
  }
  
  // Check that the block requested was a valid one
  
  if(blk==0) {
    ThrowError("Register " << board << "." << block 
	       << " not found in the register map");
  }
  
  return blk;
}

/**.......................................................................
 * Check the type and element number of a regmap block
 */
void RegMapDataFrameManager::
checkType(RegMapBlock* blk, DataType::Type type,
	  CoordRange* range) 
{
  bool match=false;
  
  switch (type) {
  case DataType::BOOL:
    match = blk->isBool();
    break;
  case DataType::UCHAR:
    match = blk->isUchar();
    break;
  case DataType::CHAR:
    match = blk->isChar();
    break;
  case DataType::USHORT:
    match = blk->isUshort();
    break;
  case DataType::SHORT:
    match = blk->isShort();
    break;
  case DataType::UINT:
    match = blk->isUint();
    break;
  case DataType::INT:
    match = blk->isInt();
    break;
  case DataType::FLOAT:
    match = (blk->isFloat() && !blk->isComplex());
    break;
  case DataType::DOUBLE:
    match = blk->isDouble();
    break;
  case DataType::DATE:
    match = blk->isUtc();
    break;
  case DataType::COMPLEX_FLOAT:
    match = (blk->isFloat() && blk->isComplex());
    break;
  default:
    match = false;
  }

  
  if(!match) {
    ThrowError("Register " << blk->brd_->name << "." << blk->name_
	       << " does not match the requested type: " 
	       << type << " " << blk->isInt());
  }
  
  if(range !=0 && !blk->axes_->rangeIsValid(range)) {
    ThrowError("Invalid range: " << blk->brd_->name << "." << blk->name_
	       << range);
  }

}

/**.......................................................................
 * Check the type and element number of a regmap block
 */
DataType::Type RegMapDataFrameManager::
typeOf(RegMapBlock* blk)
{
  if(blk->isBool()) {
    return DataType::BOOL;
  
  } else if(blk->isUchar()) {
    return DataType::UCHAR;
    
  } else if(blk->isChar()) {
    return DataType::CHAR;
    
  } else if(blk->isUshort()) {
    return DataType::USHORT;
    
  } else if(blk->isShort()) {
    return DataType::SHORT;
    
  } else if(blk->isUint()) {
    return DataType::UINT;
    
  } else if(blk->isInt()) {
    return DataType::INT;

  } else if(blk->isFloat() && !blk->isComplex()) {
    return DataType::FLOAT;
 
  } else if(blk->isDouble()) {
    return DataType::DOUBLE;
    
  } else if(blk->isUtc()) {
    return DataType::DATE;
    
  } else if(blk->isFloat() && blk->isComplex()) {
    return DataType::COMPLEX_FLOAT;
    
  } else {
    ThrowError("Unhandled data type");
  }
}

void RegMapDataFrameManager::
checkType(string board, string block, DataType::Type type,
	  CoordRange* range) 
{
  bool match;
  LogStream errStr;
  RegMapBlock* blk=getReg(board, block);
  
  return checkType(blk, type, range);
}

//-----------------------------------------------------------------------
// Methods to write to a named register
//-----------------------------------------------------------------------

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 bool* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::BOOL, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::BOOL);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 unsigned char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UCHAR, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::UCHAR);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 signed char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::CHAR, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::CHAR);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 unsigned short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::USHORT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::USHORT);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 signed short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::SHORT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::SHORT);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 unsigned int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UINT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::UINT);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 signed int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::INT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::INT);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 float* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::FLOAT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::FLOAT);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 double* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DOUBLE, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::DOUBLE);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 RegDate::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DATE, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::DATE);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 Complex<float>::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::COMPLEX_FLOAT);
}

//------------------------------------------------------------
// Versions for writing single register values
//------------------------------------------------------------

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 bool data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::BOOL, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::BOOL);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 unsigned char data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UCHAR, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::UCHAR);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 signed char data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::CHAR, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::CHAR);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 unsigned short data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::USHORT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::USHORT);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 signed short data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::SHORT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::SHORT);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 unsigned int data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UINT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::UINT);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 signed int data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::INT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::INT);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 float data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::FLOAT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::FLOAT);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 double data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DOUBLE, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::DOUBLE);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 RegDate::Data data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DATE, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::DATE);
}

void RegMapDataFrameManager::
writeReg(string board, string block, 
	 Complex<float>::Data data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::COMPLEX_FLOAT);
}

//------------------------------------------------------------
// Versions which don't lock
//------------------------------------------------------------

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       bool* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::BOOL, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::BOOL, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       unsigned char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UCHAR, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::UCHAR, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       signed char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::CHAR, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::CHAR, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       unsigned short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::USHORT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::USHORT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       signed short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::SHORT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::SHORT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       unsigned int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UINT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::UINT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       signed int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::INT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::INT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       float* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::FLOAT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::FLOAT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       double* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DOUBLE, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::DOUBLE, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       RegDate::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DATE, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::DATE, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       Complex<float>::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  packData(board, block, (void*) data, range, DataType::COMPLEX_FLOAT, false);
}

//------------------------------------------------------------
// Versions for writing single register values
//------------------------------------------------------------

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       bool data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::BOOL, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::BOOL, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       unsigned char data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UCHAR, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::UCHAR, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       signed char data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::CHAR, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::CHAR, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       unsigned short data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::USHORT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::USHORT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       signed short data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::SHORT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::SHORT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       unsigned int data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UINT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::UINT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       signed int data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::INT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::INT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       float data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::FLOAT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::FLOAT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       double data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DOUBLE, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::DOUBLE, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       RegDate::Data data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DATE, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::DATE, false);
}

void RegMapDataFrameManager::
writeRegNoLock(string board, string block, 
	       Complex<float>::Data data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  packValue(board, block, (void*) &data, range, DataType::COMPLEX_FLOAT, false);
}

//-----------------------------------------------------------------------
// Methods to write to a named register
//-----------------------------------------------------------------------
/*
  void RegMapDataFrameManager::
  writeReg(RegMapBlock* blk,
  bool* data, CoordRange* range)
  {
  // Sanity check the register type
  
  checkType(blk, DataType::BOOL, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::BOOL);
  }
*/

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 unsigned char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UCHAR, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::UCHAR);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 signed char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::CHAR, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::CHAR);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 unsigned short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::USHORT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::USHORT);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 signed short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::SHORT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::SHORT);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 unsigned int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UINT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::UINT);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 signed int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::INT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::INT);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 float* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::FLOAT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::FLOAT);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 double* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DOUBLE, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::DOUBLE);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 RegDate::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DATE, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::DATE);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 Complex<float>::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::COMPLEX_FLOAT);
}

//------------------------------------------------------------
// Versions for writing single register values
//------------------------------------------------------------

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 bool data, CoordRange* range)
{
  // Sanity check the register type

  checkType(blk, DataType::BOOL, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::BOOL);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 unsigned char data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UCHAR, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::UCHAR);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 signed char data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::CHAR, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::CHAR);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 unsigned short data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::USHORT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::USHORT);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 signed short data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::SHORT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::SHORT);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 unsigned int data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UINT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::UINT);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 signed int data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::INT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::INT);
}

void RegMapDataFrameManager::
writeDcReg(RegMapBlock* blk,
	   float data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::FLOAT, range);

  // And pack the data 
  
  packDcValue(blk, (void*) &data, range, DataType::FLOAT);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 float data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::FLOAT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::FLOAT);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 double data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DOUBLE, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::DOUBLE);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 RegDate::Data data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DATE, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::DATE);
}

void RegMapDataFrameManager::
writeReg(RegMapBlock* blk,
	 Complex<float>::Data data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::COMPLEX_FLOAT);
}

//------------------------------------------------------------
// Versions which don't lock
//------------------------------------------------------------
/*
  void RegMapDataFrameManager::
  writeRegNoLock(RegMapBlock* blk,
  bool* data, CoordRange* range)
  {
  // Sanity check the register type
  
  checkType(blk, DataType::BOOL, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::BOOL, false);
  }
*/

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       unsigned char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UCHAR, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::UCHAR, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       signed char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::CHAR, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::CHAR, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       unsigned short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::USHORT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::USHORT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       signed short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::SHORT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::SHORT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       unsigned int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UINT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::UINT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       signed int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::INT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::INT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       float* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::FLOAT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::FLOAT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       double* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DOUBLE, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::DOUBLE, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       RegDate::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DATE, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::DATE, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       Complex<float>::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  packData(blk, (void*) data, range, DataType::COMPLEX_FLOAT, false);
}

//------------------------------------------------------------
// Versions for writing single register values
//------------------------------------------------------------

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       bool data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::BOOL, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::BOOL, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       unsigned char data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UCHAR, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::UCHAR, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       signed char data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::CHAR, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::CHAR, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       unsigned short data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::USHORT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::USHORT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       signed short data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::SHORT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::SHORT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       unsigned int data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UINT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::UINT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       signed int data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::INT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::INT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       float data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::FLOAT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::FLOAT, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       double data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DOUBLE, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::DOUBLE, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       RegDate::Data data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DATE, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::DATE, false);
}

void RegMapDataFrameManager::
writeRegNoLock(RegMapBlock* blk,
	       Complex<float>::Data data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  packValue(blk, (void*) &data, range, DataType::COMPLEX_FLOAT, false);
}


//------------------------------------------------------------
// Methods to read a named register
//------------------------------------------------------------

void RegMapDataFrameManager::
readReg(string board, string block, 
	bool* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::BOOL, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::BOOL);
}

void RegMapDataFrameManager::
readReg(string board, string block, 
	unsigned char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UCHAR, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::UCHAR);
}

void RegMapDataFrameManager::
readReg(string board, string block, 
	signed char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::CHAR, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::CHAR);
}

void RegMapDataFrameManager::
readReg(string board, string block, 
	unsigned short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::USHORT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::USHORT);
}

void RegMapDataFrameManager::
readReg(string board, string block, 
	signed short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::SHORT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::SHORT);
}

void RegMapDataFrameManager::
readReg(string board, string block, 
	unsigned int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UINT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::UINT);
}

void RegMapDataFrameManager::
readReg(string board, string block, 
	signed int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::INT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::INT);
}

void RegMapDataFrameManager::
readReg(string board, string block, 
	float* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::FLOAT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::FLOAT);
}

void RegMapDataFrameManager::
readReg(string board, string block, 
	double* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DOUBLE, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::DOUBLE);
}

void RegMapDataFrameManager::
readReg(string board, string block, 
	RegDate::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DATE, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::DATE);
}

void RegMapDataFrameManager::
readReg(string board, string block, 
	Complex<float>::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::COMPLEX_FLOAT);
}

//------------------------------------------------------------
// Versions which don't lock
//------------------------------------------------------------

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      bool* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::BOOL, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::BOOL, false);
}

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      unsigned char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UCHAR, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::UCHAR, false);
}

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      signed char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::CHAR, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::CHAR, false);
}

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      unsigned short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::USHORT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::USHORT, false);
}

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      signed short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::SHORT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::SHORT, false);
}

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      unsigned int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::UINT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::UINT, false);
}

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      signed int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::INT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::INT, false);
}

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      float* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::FLOAT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::FLOAT, false);
}

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      double* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DOUBLE, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::DOUBLE, false);
}

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      RegDate::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::DATE, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::DATE, false);
}

void RegMapDataFrameManager::
readRegNoLock(string board, string block, 
	      Complex<float>::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(board, block, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  unpackData(board, block, (void*) data, range, DataType::COMPLEX_FLOAT, false);
}


//------------------------------------------------------------
// Methods to read a named register
//------------------------------------------------------------

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	bool* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::BOOL, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::BOOL);
}

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	unsigned char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UCHAR, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::UCHAR);
}

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	signed char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::CHAR, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::CHAR);
}

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	unsigned short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::USHORT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::USHORT);
}

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	signed short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::SHORT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::SHORT);
}

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	unsigned int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UINT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::UINT);
}

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	signed int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::INT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::INT);
}

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	float* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::FLOAT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::FLOAT);
}

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	double* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DOUBLE, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::DOUBLE);
}

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	RegDate::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DATE, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::DATE);
}

void RegMapDataFrameManager::
readReg(RegMapBlock* blk,
	Complex<float>::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::COMPLEX_FLOAT);
}

//------------------------------------------------------------
// Versions which don't lock
//------------------------------------------------------------

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      bool* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::BOOL, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::BOOL, false);
}

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      unsigned char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UCHAR, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::UCHAR, false);
}

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      signed char* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::CHAR, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::CHAR, false);
}

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      unsigned short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::USHORT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::USHORT, false);
}

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      signed short* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::SHORT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::SHORT, false);
}

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      unsigned int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::UINT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::UINT, false);
}

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      signed int* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::INT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::INT, false);
}

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      float* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::FLOAT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::FLOAT, false);
}

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      double* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DOUBLE, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::DOUBLE, false);
}

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      RegDate::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::DATE, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::DATE, false);
}

void RegMapDataFrameManager::
readRegNoLock(RegMapBlock* blk,
	      Complex<float>::Data* data, CoordRange* range)
{
  // Sanity check the register type
  
  checkType(blk, DataType::COMPLEX_FLOAT, range);
  
  // And pack the data 
  
  unpackData(blk, (void*) data, range, DataType::COMPLEX_FLOAT, false);
}

/**.......................................................................
 * Return true if this board is flagged.
 */
bool RegMapDataFrameManager::boardIsFlagged(RegMapBoard* brd)
{
  // The first register of each board is a scalar "status" register.
  // This has the value 0 when the board is ok, or non-zero when
  // broken.
  
  unsigned status;
  readReg(brd->blocks[0], &status);
  return status==1;
}

/**.......................................................................
 * Return true if the passed board is present in the register map
 */
bool RegMapDataFrameManager::boardIsPresent(RegMapBoard* brd)
{
  // Board will be present if this frame encompasses all registers, or
  // if the board contains any archived registers
  
  return !archivedOnly_ || brd->nArcByte_ > 0;
}

/**.......................................................................
 * Return true if the block is present in the register map
 */
bool RegMapDataFrameManager::blockIsPresent(RegMapBlock* blk)
{
  // Block is present if this frame encompasses all registers, or
  // if the register is archived
  
  return !archivedOnly_ || blk->isArchived();
}

/**.......................................................................
 * Get a unique frame id based on integral MJD half-seconds.
 */
unsigned int RegMapDataFrameManager::getId(unsigned nanoSecondInterval)
{
  RegDate date;

  readReg("frame", "utc", date.data());

  return date.timeVal().getMjdId(nanoSecondInterval);
}

/**.......................................................................
 * Get a unique frame id based on integral MJD half-seconds.
 */
void RegMapDataFrameManager::setMjd(double mjd)
{
  TimeVal timeVal;
  timeVal.setMjd(mjd);

  setMjd(timeVal);
}

/**.......................................................................
 * Set the passed MJD in the frame
 */
void RegMapDataFrameManager::setMjd()
{
  RegDate utc;
  writeReg("frame", "utc", utc.data());
}

/**.......................................................................
 * Set the passed MJD in the frame
 */
void RegMapDataFrameManager::incrementRecord()
{
  ++record_;
  writeReg("frame", "record", &record_);
}

/**.......................................................................
 * Set the passed MJD in the frame
 */
void RegMapDataFrameManager::setRecord(unsigned record)
{
  writeReg("frame", "record", &record);
}

/**.......................................................................
 * Set the received flag to true for this frame
 */
void RegMapDataFrameManager::setReceived(bool received)
{
  unsigned char recv = received ? FrameFlags::RECEIVED : 
    FrameFlags::NOT_RECEIVED;

  writeReg("frame", "received", &recv);
}

/**.......................................................................
 * Set the passed MJD in the frame
 */
void RegMapDataFrameManager::setMjd(RegDate& regDate)
{
  writeReg("frame", "utc", regDate.data());
}

/**.......................................................................
 * Set the passed MJD in the frame
 */
void RegMapDataFrameManager::setMjd(TimeVal& timeVal)
{
  RegDate date(timeVal);

  writeReg("frame", "utc", date.data());
}

RegMapBlock* RegMapDataFrameManager::findReg(char* boardName, char* blockName)
{
  return regMap_->findRegMapBlock(boardName, blockName);
}

RegMapBoard* RegMapDataFrameManager::findRegMapBoard(std::string boardName)
{
  return find_RegMapBoard(regMap_, (char*)boardName.c_str());
}

unsigned char* RegMapDataFrameManager::getUcharPtr(char* boardName, char* blockName)
{
  RegMapBlock* blk = findReg(boardName, blockName);
  checkType(blk, DataType::UCHAR);
  return frame()->getUcharPtr(byteOffsetInFrameOf(blk));
}

char* RegMapDataFrameManager::getCharPtr(char* boardName, char* blockName)
{
  RegMapBlock* blk = findReg(boardName, blockName);
  checkType(blk, DataType::CHAR);
  return frame()->getCharPtr(byteOffsetInFrameOf(blk));
}

unsigned short* RegMapDataFrameManager::getUshortPtr(char* boardName, char* blockName)
{
  RegMapBlock* blk = findReg(boardName, blockName);
  checkType(blk, DataType::USHORT);
  return frame()->getUshortPtr(byteOffsetInFrameOf(blk));
}

short* RegMapDataFrameManager::getShortPtr(char* boardName, char* blockName)
{
  RegMapBlock* blk = findReg(boardName, blockName);
  checkType(blk, DataType::SHORT);
  return frame()->getShortPtr(byteOffsetInFrameOf(blk));
}

unsigned int* RegMapDataFrameManager::getUintPtr(char* boardName, char* blockName)
{
  RegMapBlock* blk = findReg(boardName, blockName);
  checkType(blk, DataType::UINT);
  return frame()->getUintPtr(byteOffsetInFrameOf(blk));
}

int* RegMapDataFrameManager::getIntPtr(char* boardName, char* blockName)
{
  RegMapBlock* blk = findReg(boardName, blockName);
  checkType(blk, DataType::INT);
  return frame()->getIntPtr(byteOffsetInFrameOf(blk));
}

float* RegMapDataFrameManager::getFloatPtr(char* boardName, char* blockName)
{
  RegMapBlock* blk = findReg(boardName, blockName);
  checkType(blk, DataType::FLOAT);
  return frame()->getFloatPtr(byteOffsetInFrameOf(blk));
}

double* RegMapDataFrameManager::getDoublePtr(char* boardName, char* blockName)
{
  RegMapBlock* blk = findReg(boardName, blockName);
  checkType(blk, DataType::DOUBLE);
  return frame()->getDoublePtr(byteOffsetInFrameOf(blk));
}

RegDate::Data* RegMapDataFrameManager::getDatePtr(char* boardName, char* blockName)
{
  RegMapBlock* blk = findReg(boardName, blockName);
  checkType(blk, DataType::DATE);
  frame()->getDatePtr(byteOffsetInFrameOf(blk));
}

/**.......................................................................
 * Copy the contents of a RegMapDataFrameManager to the appropriate
 * place in the array map.
 */
void RegMapDataFrameManager::writeBoard(BoardDataFrameManager& fm, 
					bool lockFrame)
{
  // Do nothing if the board is not present in our frame
  
  if(!boardIsPresent(fm.board()))
    return;

  if(lockFrame)
    lock();

  fm.lock();

  RegMapBoard* brd = findRegMapBoard(fm.board()->name);

  try {
    
    // If both frames have the same internal specifications, just copy
    // the whole block at once
    
    if(fm.archivedOnly() == archivedOnly_) {
      
      // Starting byte index to which we will write will be the offset
      // of this board in the frame
      
      unsigned iStartDest = byteOffsetInFrameOf(brd);
      
      // Starting byte in the source frame will just be the start of the
      // data
      
      unsigned iStartSrc  = fm.byteOffsetInFrameOfData();

      // Pack the array into our frame
      
      frame_->pack(fm.frame_->getPtr(iStartSrc, DataType::UCHAR), 
		   brd->nByte(archivedOnly_),
		   DataType::UCHAR, iStartDest, false);

      // Else we have to iterate over registers, checking which ones
      // from the source frame should be copied to the destination frame
      
    } else {
      
      // If the board is marked as reachable, attempt to copy the
      // contents of the board's registers into the frame buffer.
	  
      // Iterate over all register blocks of this board
	  
      for(unsigned int iblock=0; iblock < brd->nblock; iblock++) {
	RegMapBlock* blk = brd->blocks[iblock];
	    
	// If this block is present in the register maps of both
	// frames, copy it
	    
	if(blockIsPresent(blk) && fm.blockIsPresent(blk)) {
	      
	  unsigned iStartSrc  = fm.byteOffsetInFrameOf(blk);
	  unsigned iStartDest = byteOffsetInFrameOf(brd, blk);
	      
	  frame_->pack(fm.frame_->getPtr(iStartSrc, DataType::UCHAR), 
		       blk->nByte(), DataType::UCHAR, iStartDest, false);
	      
	}; // End if(blockIsPresent(blk) && fm.blockIsPresent(blk))

      }; // End iteration over blocks

    } // End if we can do a block copy

    // Unlock

    if(lockFrame)
      unlock();

    fm.unlock();

  } catch(const Exception& err) {
    if(lockFrame)
      unlock();
    fm.unlock();
    throw(err);
  } catch(...) {
    if(lockFrame)
      unlock();
    fm.unlock();
  }
}
