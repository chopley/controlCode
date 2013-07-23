#ifndef GCP_UTIL_REGMAPDATAFRAMEMANAGER_H
#define GCP_UTIL_REGMAPDATAFRAMEMANAGER_H

/**
 * @file RegMapDataFrameManager.h
 * 
 * Tagged: Sat Aug 14 13:12:19 UTC 2004
 * 
 * @author 
 */
#include <string>

#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

#include "gcp/util/common/Complex.h"
#include "gcp/util/common/DataFrameManager.h"


namespace gcp {
  namespace util {
    
    class BoardDataFrameManager;
    class ArrayMapDataFrameManager;
    class Date;
    
    class RegMapDataFrameManager : public DataFrameManager {
    public:
      
      /**
       * Destructor.
       */
      virtual ~RegMapDataFrameManager();
      
      /**
       * Overloaded base-class operator
       */
      void operator=(DataFrameManager& manager);
      
      /**
       * Inherited class assignment operator
       */
      virtual void operator=(RegMapDataFrameManager& manager);
      
      /**
       * Write a board
       */
      void writeBoard(BoardDataFrameManager& fm, bool lockFrame);

      //-------------------------------------------------------------
      // Methods to pack a named register.
      //-------------------------------------------------------------
      
      void writeReg(std::string board, std::string name, 
		    unsigned char* data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    signed char* data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    bool* data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    unsigned short* data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    signed short* data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    unsigned int* data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    signed int* data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    float* data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    double* data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    RegDate::Data* data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    Complex<float>::Data* data, CoordRange* range=0);
      
      //------------------------------------------------------------
      // Methods for writing single values
      //------------------------------------------------------------
      
      void writeReg(std::string board, std::string name, 
		    unsigned char data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    signed char data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    bool data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    unsigned short data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    signed short data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    unsigned int data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    signed int data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    float data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    double data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    RegDate::Data data, CoordRange* range=0);
      
      void writeReg(std::string board, std::string name, 
		    Complex<float>::Data data, CoordRange* range=0);
      
      //------------------------------------------------------------
      // Versions which don't lock
      //------------------------------------------------------------
      
      void writeRegNoLock(std::string board, std::string name, 
			  unsigned char* data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  signed char* data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  bool* data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  unsigned short* data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  signed short* data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  unsigned int* data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  signed int* data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  float* data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  double* data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  RegDate::Data* data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  Complex<float>::Data* data, CoordRange* range=0);
      
      //------------------------------------------------------------
      // Versions for writing a single value
      //------------------------------------------------------------
      
      void writeRegNoLock(std::string board, std::string name, 
			  unsigned char data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  signed char data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  bool data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  unsigned short data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  signed short data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  unsigned int data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  signed int data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  float data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  double data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  RegDate::Data data, CoordRange* range=0);
      
      void writeRegNoLock(std::string board, std::string name, 
			  Complex<float>::Data data, CoordRange* range=0);
      
      //-------------------------------------------------------------
      // Methods to pack a named register.
      //-------------------------------------------------------------
      
      void writeReg(RegMapBlock* blk,
		    unsigned char* data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    signed char* data, CoordRange* range=0);
      
      //void writeReg(RegMapBlock* blk,
		    //bool* data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    unsigned short* data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    signed short* data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    unsigned int* data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    signed int* data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    float* data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    double* data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    RegDate::Data* data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    Complex<float>::Data* data, CoordRange* range=0);
      
      //------------------------------------------------------------
      // Versions for writing a single value
      //------------------------------------------------------------
      
      void writeReg(RegMapBlock* blk,
		    unsigned char data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    signed char data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    bool data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    unsigned short data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    signed short data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    unsigned int data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    signed int data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    float data, CoordRange* range=0);
      
      void writeDcReg(RegMapBlock* blk,
		      float data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    double data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    RegDate::Data data, CoordRange* range=0);
      
      void writeReg(RegMapBlock* blk,
		    Complex<float>::Data data, CoordRange* range=0);
      
      //------------------------------------------------------------
      // Versions which don't lock
      //------------------------------------------------------------
      
      void writeRegNoLock(RegMapBlock* blk,
			  unsigned char* data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  signed char* data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  bool* data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  unsigned short* data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  signed short* data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  unsigned int* data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  signed int* data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  float* data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  double* data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  RegDate::Data* data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  Complex<float>::Data* data, CoordRange* range=0);
      
      //------------------------------------------------------------
      // Versions for writing a single value
      //------------------------------------------------------------
      
      void writeRegNoLock(RegMapBlock* blk,
			  unsigned char data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  signed char data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  bool data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  unsigned short data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  signed short data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  unsigned int data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  signed int data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  float data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  double data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  RegDate::Data data, CoordRange* range=0);
      
      void writeRegNoLock(RegMapBlock* blk,
			  Complex<float>::Data data, CoordRange* range=0);
      
      //------------------------------------------------------------
      // Methods to read a named register
      //------------------------------------------------------------
      
      void readReg(std::string board, std::string name, 
		   unsigned char* data, CoordRange* range=0);
      
      void readReg(std::string board, std::string name, 
		   signed char* data, CoordRange* range=0);
      
      void readReg(std::string board, std::string name, 
		   bool* data, CoordRange* range=0);
      
      void readReg(std::string board, std::string name, 
		   unsigned short* data, CoordRange* range=0);
      
      void readReg(std::string board, std::string name, 
		   signed short* data, CoordRange* range=0);
      
      void readReg(std::string board, std::string name, 
		   unsigned int* data, CoordRange* range=0);
      
      void readReg(std::string board, std::string name, 
		   signed int* data, CoordRange* range=0);
      
      void readReg(std::string board, std::string name, 
		   float* data, CoordRange* range=0);
      
      void readReg(std::string board, std::string name, 
		   double* data, CoordRange* range=0);
      
      void readReg(std::string board, std::string name, 
		   RegDate::Data* data, CoordRange* range=0);
      
      void readReg(std::string board, std::string name, 
		   Complex<float>::Data* data, CoordRange* range=0);
      
      //------------------------------------------------------------
      // Versions which don't lock
      //------------------------------------------------------------
      
      void readRegNoLock(std::string board, std::string name, 
			 unsigned char* data, CoordRange* range=0);
      
      void readRegNoLock(std::string board, std::string name, 
			 signed char* data, CoordRange* range=0);
      
      void readRegNoLock(std::string board, std::string name, 
			 bool* data, CoordRange* range=0);
      
      void readRegNoLock(std::string board, std::string name, 
			 unsigned short* data, CoordRange* range=0);
      
      void readRegNoLock(std::string board, std::string name, 
			 signed short* data, CoordRange* range=0);
      
      void readRegNoLock(std::string board, std::string name, 
			 unsigned int* data, CoordRange* range=0);
      
      void readRegNoLock(std::string board, std::string name, 
			 signed int* data, CoordRange* range=0);
      
      void readRegNoLock(std::string board, std::string name, 
			 float* data, CoordRange* range=0);
      
      void readRegNoLock(std::string board, std::string name, 
			 double* data, CoordRange* range=0);
      
      void readRegNoLock(std::string board, std::string name, 
			 RegDate::Data* data, CoordRange* range=0);
      
      void readRegNoLock(std::string board, std::string name, 
			 Complex<float>::Data* data, CoordRange* range=0);
      
      //------------------------------------------------------------
      // Methods to read a named register
      //------------------------------------------------------------
      
      void readReg(RegMapBlock* blk,
		   unsigned char* data, CoordRange* range=0);
      
      void readReg(RegMapBlock* blk,
		   signed char* data, CoordRange* range=0);
      
      void readReg(RegMapBlock* blk,
		   bool* data, CoordRange* range=0);
      
      void readReg(RegMapBlock* blk,
		   unsigned short* data, CoordRange* range=0);
      
      void readReg(RegMapBlock* blk,
		   signed short* data, CoordRange* range=0);
      
      void readReg(RegMapBlock* blk,
		   unsigned int* data, CoordRange* range=0);
      
      void readReg(RegMapBlock* blk,
		   signed int* data, CoordRange* range=0);
      
      void readReg(RegMapBlock* blk,
		   float* data, CoordRange* range=0);
      
      void readReg(RegMapBlock* blk,
		   double* data, CoordRange* range=0);
      
      void readReg(RegMapBlock* blk,
		   RegDate::Data* data, CoordRange* range=0);
      
      void readReg(RegMapBlock* blk,
		   Complex<float>::Data* data, CoordRange* range=0);
      
      //------------------------------------------------------------
      // Versions which don't lock
      //------------------------------------------------------------
      
      void readRegNoLock(RegMapBlock* blk,
			 unsigned char* data, CoordRange* range=0);
      
      void readRegNoLock(RegMapBlock* blk,
			 signed char* data, CoordRange* range=0);
      
      void readRegNoLock(RegMapBlock* blk,
			 bool* data, CoordRange* range=0);
      
      void readRegNoLock(RegMapBlock* blk,
			 unsigned short* data, CoordRange* range=0);
      
      void readRegNoLock(RegMapBlock* blk,
			 signed short* data, CoordRange* range=0);
      
      void readRegNoLock(RegMapBlock* blk,
			 unsigned int* data, CoordRange* range=0);
      
      void readRegNoLock(RegMapBlock* blk,
			 signed int* data, CoordRange* range=0);
      
      void readRegNoLock(RegMapBlock* blk,
			 float* data, CoordRange* range=0);
      
      void readRegNoLock(RegMapBlock* blk,
			 double* data, CoordRange* range=0);
      
      void readRegNoLock(RegMapBlock* blk,
			 RegDate::Data* data, CoordRange* range=0);
      
      void readRegNoLock(RegMapBlock* blk,
			 Complex<float>::Data* data, CoordRange* range=0);
      
      // Return true if this frame only contains archived registers
      
      inline bool archivedOnly() {return archivedOnly_;};
      
      /**
       * Get a unique frame id based on integral intervals of the MJD
       */
      unsigned int getId(unsigned nanoSecondInterval);
      
      //------------------------------------------------------------
      // Methods for setting registers maintained by all register maps
      //------------------------------------------------------------

      /**
       * Set the mjd of this frame
       */

      // Set to the current mjd

      void setMjd();

      // Set  to passed mjd

      void setMjd(RegDate& regDate);
      void setMjd(TimeVal& mjd);
      void setMjd(double mjd);

      // Increment the record counter for this frame

      void incrementRecord();
      void setRecord(unsigned record);

      // Set the received flag to true for this frame

      void setReceived(bool received);

      //------------------------------------------------------------
      // Methods for looking up registers
      //------------------------------------------------------------

      RegMapBlock* findReg(char* boardName, char* blockName);
      
      RegMapBoard* findRegMapBoard(std::string boardName);
      
      //------------------------------------------------------------
      // Methods for calculating byte offsets
      //------------------------------------------------------------

      /**
       * Return the offset in bytes of the data for the requested
       * register, from the beginning of the frame buffer.
       */
      int byteOffsetInFrameOf(RegMapBlock* blk, Coord* coord=0);
      int byteOffsetInFrameOf(std::string board, std::string block, 
			      Coord* coord=0);
      
      int byteOffsetInFrameOf(RegMapBlock* blk, CoordRange* range);
      int byteOffsetInFrameOf(std::string board, std::string block, 
			      CoordRange* range);
      
      int byteOffsetInFrameOf(RegMapBoard* brd, RegMapBlock* blk);

      int byteOffsetInFrameOf(RegMapBoard* brd);
      int byteOffsetInFrameOf(std::string board);
      
      /**
       * Return a pointer to our internal data
       */
      char*           getCharPtr(  char* boardName, char* blockName);
      unsigned char*  getUcharPtr( char* boardName, char* blockName);
      short*          getShortPtr( char* boardName, char* blockName);
      unsigned short* getUshortPtr(char* boardName, char* blockName);
      unsigned int*   getUintPtr(  char* boardName, char* blockName);
      int*            getIntPtr(   char* boardName, char* blockName);
      float*          getFloatPtr( char* boardName, char* blockName);
      double*         getDoublePtr(char* boardName, char* blockName);
      RegDate::Data*  getDatePtr(  char* boardName, char* blockName);

    protected:
      
      friend class ArrayMapDataFrameManager;
      
      ArrayMap* arrayMap_;

      RegMap*   regMap_;        // A register map 
      
      /**
       * A flag which will determine if this manager's frame contains
       * all registers, or only archived registers.
       */
      bool archivedOnly_; 
      
      /**
       * Return the offset in the register map, of the data for this
       * register.
       */
      int byteOffsetInRegMapOf(RegMapBlock* blk, Coord* coord=0);
      int byteOffsetInRegMapOf(std::string board, std::string block, 
			       Coord* coord=0);
      int byteOffsetInRegMapOf(RegMapBlock* blk, CoordRange* range);
      int byteOffsetInRegMapOf(std::string board, std::string block, 
			       CoordRange* range);
      
      /**
       * Pack data of an arbitrary type into the underlying frame
       */
      void packData(RegMapBlock* blk, void* data, CoordRange* range, 
		    DataType::Type type, bool lock=true);
      
      void packData(std::string board, std::string block, void* data, 
		    CoordRange* range, DataType::Type type, bool lock=true);
      
      void packValue(RegMapBlock* blk, void* data, CoordRange* range, 
		     DataType::Type type, bool lock=true);
      
      void packDcValue(RegMapBlock* blk, void* data, CoordRange* range, 
		       DataType::Type type, bool lock=true);
      
      void packValue(std::string board, std::string block, void* data, 
		     CoordRange* range, DataType::Type type, bool lock=true);
      /**
       * Unpack data of an arbitrary type from the underlying frame
       */
      void unpackData(RegMapBlock* blk, void* data, CoordRange* range, 
		      DataType::Type type, bool lock=true);
      void unpackData(std::string board, std::string block, void* data, 
		      CoordRange* range, DataType::Type type, bool lock=true);
      
      /**
       * Private constructors.  Only inheritors should be instantiated
       */
      
      // Constructor.

      RegMapDataFrameManager(std::string regmap, bool archivedOnly=false);

      RegMapDataFrameManager(bool archivedOnly=false);
      
      /**
       * Check the type and element number of a regmap block
       */
      void checkType(std::string board, std::string block, 
		     DataType::Type type, CoordRange* range=0);
      
      void checkType(RegMapBlock* blk, DataType::Type type,
		     CoordRange* range=0);
      
      /**
       * Return the type of a regmap block
       */
      DataType::Type typeOf(RegMapBlock* blk);
	
      /**
       * Get the descriptor for this reg map block
       */
      RegMapBlock* getReg(std::string board, std::string block);
      
      /**
       * Return true if the passed board is present in the register map
       */
      bool boardIsPresent(RegMapBoard* brd);
      
      /**
       * Return true if the block is present in the register map
       */
      bool blockIsPresent(RegMapBlock* blk);
      
      /**
       * Return true if this board is flagged.
       */
      bool boardIsFlagged(RegMapBoard* brd);
      
    private:
      
      unsigned record_;
      AxisRange axisRange_;
      
    }; // End class RegMapDataFrameManager
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_REGMAPDATAFRAMEMANAGER_H
