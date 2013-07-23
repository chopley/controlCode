// $Id: BoardDataFrameManager.h,v 1.1.1.1 2009/07/06 23:57:25 eml Exp $

#ifndef GCP_UTIL_BOARDDATAFRAMEMANAGER_H
#define GCP_UTIL_BOARDDATAFRAMEMANAGER_H

/**
 * @file BoardDataFrameManager.h
 * 
 * Tagged: Fri Feb  2 06:40:27 NZDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:25 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Complex.h"
#include "gcp/util/common/RegMapDataFrameManager.h"

#include "gcp/control/code/unix/libunix_src/common/scanner.h"

#include <iostream>

namespace gcp {
  namespace util {

    //-----------------------------------------------------------------------
    // Class definition for BoardDataFrameManager.  Manages a data
    // frame which represents the data for a single board of a
    // register map
    //-----------------------------------------------------------------------

    class BoardDataFrameManager : public RegMapDataFrameManager {
    public:

      /**
       * Constructor.
       */
      BoardDataFrameManager(std::string regmap, std::string board, 
			    bool archivedOnly=false);

      /**
       * Destructor.
       */
      virtual ~BoardDataFrameManager();

      /**.......................................................................
       * Get the descriptor for this reg map block
       */
      RegMapBlock* getReg(std::string block);
	
      // Pack data of an arbitrary type into the underlying frame

      template<class type>
	void writeBoardReg(RegMapBlock* blk, type* data, CoordRange* range=0, 
			   bool lock=true);

      template<class type>
	void writeBoardReg(std::string block, type* data, CoordRange* range=0, 
			   bool lock=true);

      template<class type>
	void readBoardReg(RegMapBlock* blk, type* data, CoordRange* range=0, 
			  bool lock=true);

      template<class type>
	void readBoardReg(std::string block, type* data, CoordRange* range=0, 
			  bool lock=true);


      // Return the offset in bytes of the data for the requested and
      // register, from the beginning of the frame buffer.
      
      int byteOffsetInFrameOf(RegMapBlock* blk, Coord* coord);
      int byteOffsetInFrameOf(RegMapBlock* blk, CoordRange* range=0);

      RegMapBoard* board() {return brd_;}
      ArrRegMap*   regmap() {return arrRegMap_;}

      void incrementRecord();

      void setReceived(bool recv);

      void setMjd(RegDate& date);

    protected:

      ArrRegMap* arrRegMap_;
      RegMapBoard* brd_;

    private:

      unsigned record_;

    }; // End class BoardDataFrameManager


    //-----------------------------------------------------------------------
    // Implementation
    //-----------------------------------------------------------------------

    /**.......................................................................
     * Pack data of an arbitrary type
     */
    template <class type>
      void BoardDataFrameManager::
      writeBoardReg(std::string block, type* data, CoordRange* range, 
	       bool lockFrame)
      {
	RegMapBlock* blk = 0;
	blk = brd_->findRegMapBlock(block);

	if(blk==0) {
	  ThrowError("Block: " << block << " not found in board: " <<
		     brd_->name);
	}

	writeBoardReg(blk, data, range, lockFrame);
      }

    /**.......................................................................
     * Pack data of an arbitrary type
     */
    template <class type>
      void BoardDataFrameManager::
      writeBoardReg(RegMapBlock* blk, type* data, CoordRange* range, 
	       bool lockFrame)
      {
	DataType::Type dataType = DataType::typeOf(data);

	// Calculate the byte offset of the start element of this
	// register
	// from the head of the frame.
  
	int byteOffset = byteOffsetInFrameOf(blk);
  
	// Do nothing if this register isn't archived
  
	if(byteOffset < 0)
	  return;
  
	// If the number of bytes was passed as 0, use the default
	// from the block descriptor
	
	AxisRange axisRange(blk->axes_, range);
	frame_->pack((void*)data, axisRange, dataType, byteOffset, lockFrame);
      }
    
    /**.......................................................................
     * UnPack data of an arbitrary type
     */
    template <class type>
      void BoardDataFrameManager::
      readBoardReg(std::string block, type* data, CoordRange* range, 
	      bool lockFrame)
      {
	RegMapBlock* blk = 0;
	blk = brd_->findRegMapBlock(block);

	if(blk==0) {
	  ThrowError("Block: " << block << " not found in board: " <<
		     brd_->name);
	}

	readBoardReg(blk, data, range, lockFrame);
      }

    /**.......................................................................
     * Pack data of an arbitrary type
     */
    template <class type>
      void BoardDataFrameManager::
      readBoardReg(RegMapBlock* blk, type* data, CoordRange* range, 
	       bool lockFrame)
      {
	DataType::Type dataType = DataType::typeOf(data);

	// Calculate the byte offset of the start element of this
	// register
	// from the head of the frame.
  
	int byteOffset = byteOffsetInFrameOf(blk);
  
	// Do nothing if this register isn't archived
  
	if(byteOffset < 0)
	  return;
  
	// If the number of bytes was passed as 0, use the default
	// from the block descriptor
	
	AxisRange axisRange(blk->axes_, range);
	frame_->unpack((void*)data, axisRange, dataType, byteOffset, lockFrame);
      }

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_BOARDDATAFRAMEMANAGER_H
