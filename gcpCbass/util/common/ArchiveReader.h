// $Id: ArchiveReader.h,v 1.5 2011/08/29 22:35:22 sjcm Exp $

#ifndef GCP_UTIL_ARCHIVEREADER_H
#define GCP_UTIL_ARCHIVEREADER_H

/**
 * @file ArchiveReader.h
 * 
 * Tagged: Thu Jan 29 20:08:27 NZDT 2009
 * 
 * @version: $Revision: 1.5 $, $Date: 2011/08/29 22:35:22 $
 * 
 * @author username: Command not found.
 */

#include <iostream>
#include <vector>
#include <valarray>

#include "gcp/util/common/ArchiveConvFn.h"
#include "gcp/util/common/ArchiveFileHandler.h"
#include "gcp/util/common/RegDescription.h"

#include "gcp/control/code/unix/libunix_src/common/arcfile.h"
#include "gcp/control/code/unix/libunix_src/common/arraymap.h"

namespace gcp {
  namespace util {

    // A class for optimized reading from the archive

    class ArchiveReader {
    public:
      
      //------------------------------------------------------------
      // Define a struct that will encapsulate a data format, and
      // its corresponding string identifier
      //------------------------------------------------------------

      struct Format {
	std::string format_;
	DataType::Type type_;
      };

      enum ArchiveTransposeType {
	NONE  = 0x0,
	FIRST = 0x2,
	LAST  = 0x4,	
      };

      //------------------------------------------------------------
      // Define a struct that will encapsulate a single monitor file
      //------------------------------------------------------------

      struct ArchiveFile {
	ArcTimeStamp ts_;  // The timestamp corresponding to the start
			   // of this file
	std::string name_; // The full name (including path) of this file

	unsigned startFrame_;  // The index of the first frame to read in
			       // this file
	unsigned stopFrame_;   // The number of frames to read from this file

	unsigned currFrame_;


	ArchiveFile() {
	  startFrame_ = 0;
	  stopFrame_  = 0;
	  currFrame_  = 0;
	}

	bool atEnd() {
	  return (currFrame_ > stopFrame_);
	}

	void incr() {
	  currFrame_++;
	}
      };

      //------------------------------------------------------------
      // Define a struct that will encapsulate a register to read
      //------------------------------------------------------------

      struct ArchiveRegister {

	// The native data type of this register

	DataType::Type type_;

	// The requested output data type for this register

	DataType::Type outputType_;

	// If printing, the width and precision with which to print

	int width_;
	int prec_;

	ArrRegMap*   arregmap_;
	RegMapBoard* board_;
	RegMapBlock* block_;

	// A vector of byte ranges in this register selection

	std::vector<Range<unsigned> > byteRanges_;

	// A vector of slot ranges corresponding to this register
	// selection

	std::vector<Range<unsigned> > slotRanges_;

	// The number of bytes, summed over all elements in the
	// specified byte ranges

	unsigned nTotalBytes_;

	// The total number of elements

	unsigned nEl_;

	// The number of elements in the fastest-changing dimension of
	// this register

	unsigned nEl1_;

	// Which aspect of this register was requested?

	RegAspect aspect_;

	// An array into which the raw bytes for all byte ranges of
	// this register will be copied

	std::valarray<unsigned char> buf_;

	// External pointers to memory into which the calibrated
	// values will be written

	void* cRePtr_;
	void* cImPtr_;
	void* args_;

	// Arrays of input/output indices

	std::valarray<unsigned int> inInds_;
	std::valarray<unsigned int> outInds_;

	// A function for converting values read from the archive to
	// the output type

	ARC_CONV_FN(*convFn_);
	ARC_PRINT_FN(*printFn_);

	// An array of register calibration factors, one for each
	// element in this register

	std::vector<RegCal::RegCalSlot> regCalSlots_;
	
	// If true, transpose the indices of this register

	ArchiveTransposeType transpose_;

	// A string to hold the converted value

	std::ostringstream strVal_;

	// Convert from input values to output values

	void convertVals(unsigned iFrame, unsigned nFrame) {
	  convFn_(&buf_[0], cRePtr_, cImPtr_, args_, &regCalSlots_[0], nEl_, &inInds_[0], 
		  &outInds_[0], 
		  (transpose_ == ArchiveReader::LAST) ? iFrame*nEl_ : 
		  (transpose_ == ArchiveReader::FIRST ? iFrame*nEl1_ : iFrame));
	}

	// Print input values

	void printVals(std::ostringstream& os) {
	  printFn_(&buf_[0], os, args_, &regCalSlots_[0], nEl_, width_, prec_);
	}

	void printVals() {
	  strVal_.str("");
	  printFn_(&buf_[0], strVal_, args_, &regCalSlots_[0], nEl_, width_, prec_);
	}

	// Copy constructors & operators to be sure we don't have
	// undefined behavior by not explicitly defining these

	ArchiveRegister() {
	  nEl_       = 0;
	  nEl1_      = 0;
	  arregmap_  = 0;
	  board_     = 0;
	  block_     = 0;
	  convFn_    = 0;
	  printFn_   = 0;
	  cRePtr_    = 0;
	  cImPtr_    = 0;
	  args_      = 0;
	  transpose_ = ArchiveReader::NONE;
	  width_     = 10;
	  prec_      = 4;
	}

	ArchiveRegister(RegDescription& desc) {
	  *this = desc;
	}

	void initialize(RegDescription& desc, DataType::Type outputType, 
			ArchiveTransposeType transpose, int width, int prec,
			bool convert=true, bool read=false) {

	  *this = desc;
	  outputType_ = outputType;
	  transpose_  = transpose;
	  width_      = width;
	  prec_       = prec;

	  try {

	    if(convert) {
	      ArchiveConvFn::setConvFn(type_, outputType_, &convFn_);

	      if(convFn_ == 0) {
		ReportError("No conv function for register: " << desc);
		ThrowError("No conv function for register: "  << desc);
	      }

	    } else {
	      ArchiveConvFn::setPrintFn(type_, outputType_, &printFn_);

	      if(printFn_ == 0) {
		ReportError("No print function for register: " << desc);
		ThrowError("No print function for register: "  << desc);
	      }

	    }

	  } catch(Exception& err) {

	    ReportError(err.what() << " (" << desc << ")");
	    ThrowError(err.what()  << " (" << desc << ")");

	  }
	}

	void operator=(RegDescription& desc) {
	  nEl_         = desc.nEl();
	  nEl1_        = desc.axes().nEl(desc.axes().nAxis()-1);
	  arregmap_    = desc.regMap();
	  board_       = arregmap_->regmap->boards_[desc.iBoard()];
	  block_       = board_->blocks[desc.iBlock()];
	  type_        = DataType::typeOf(block_);
	  byteRanges_  = desc.getByteRanges();
	  slotRanges_  = desc.getSlotRanges();
	  nTotalBytes_ = desc.nByte();
	  aspect_      = desc.aspect();

	  buf_.resize(nTotalBytes_);
	  regCalSlots_.resize(nEl_);
	}

	ArchiveRegister(const ArchiveRegister& reg) {
	  *this = (ArchiveRegister&)reg;
	}

	ArchiveRegister(ArchiveRegister& reg) {
	  *this = reg;
	}

	void operator=(const ArchiveRegister& reg) {
	  *this = (ArchiveRegister&)reg;
	}

	void operator=(ArchiveRegister& reg) {
	  nEl_         = reg.nEl_;
	  nEl1_        = reg.nEl1_;
	  arregmap_    = reg.arregmap_;
	  board_       = reg.board_;
	  block_       = reg.block_;
	  convFn_      = reg.convFn_;
	  printFn_     = reg.printFn_;
	  cRePtr_      = reg.cRePtr_;
	  cImPtr_      = reg.cImPtr_;
	  args_        = reg.args_;

	  type_        = reg.type_;
	  byteRanges_  = reg.byteRanges_;
	  slotRanges_  = reg.slotRanges_;
	  nTotalBytes_ = reg.nTotalBytes_;
	  aspect_      = reg.aspect_;
	  outputType_  = reg.outputType_;
	  transpose_   = reg.transpose_;
	  width_       = reg.width_;
	  prec_        = reg.prec_;

	  buf_.resize(reg.buf_.size());
	  regCalSlots_.resize(nEl_);

	}

	// External method for setting output indices

	void setInputIndices(std::valarray<unsigned int>& inds) {
	  inInds_.resize(inds.size());

	  for(unsigned i=0; i < inds.size(); i++)
	    inInds_[i] = inds[i];
	}

	void setOutputIndices(std::valarray<unsigned int>& inds) {
	  outInds_.resize(inds.size());

	  for(unsigned i=0; i < inds.size(); i++)
	    outInds_[i] = inds[i];
	}

	void setExternalMemory(void* rePtr, void* imPtr, void* args) {
	  cRePtr_ = rePtr;
	  cImPtr_ = imPtr;
	  args_   = args;
	}

      };

      friend std::ostream& gcp::util::operator<<(std::ostream& os, 
						 ArchiveReader::ArchiveRegister& reg);

      //------------------------------------------------------------
      // A struct to encapsulate a single byte range to read out
      // of the archive
      //------------------------------------------------------------

      struct ArchiveByteRange {
	ArchiveRegister* reg_;
	unsigned char* ptr_;
	unsigned byteOffsetFromStartOfFrame_;
	unsigned nByte_;
      };

      //------------------------------------------------------------
      // Methods of the ArchiveReader class
      //------------------------------------------------------------

      /**
       * Constructor.
       */
      void initialize(bool memMap, bool convert, bool read);

      ArchiveReader(bool memMap=false, bool convert=true, bool read=false);
      ArchiveReader(std::string arcDir, std::string calFile, 
		    std::string startUtc, std::string stopUtc,
		    bool memMap=false, bool convert=true, bool read=false);

      /**
       * Copy Constructor.
       */
      ArchiveReader(const ArchiveReader& objToBeCopied);

      /**
       * Copy Constructor.
       */
      ArchiveReader(ArchiveReader& objToBeCopied);

      /**
       * Const Assignment Operator.
       */
      void operator=(const ArchiveReader& objToBeAssigned);

      /**
       * Assignment Operator.
       */
      void operator=(ArchiveReader& objToBeAssigned);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, ArchiveReader& obj);

      /**
       * Destructor.
       */
      virtual ~ArchiveReader();

      //-----------------------------------------------------------------------
      // User Methods
      //-----------------------------------------------------------------------

      // Set the top-level archive directory.  Note that this class
      // will read files in subdirectories below the top-level

      void setArchiveDirectory(std::string arcDir);

      void getFileList();

      unsigned countFrames();

      unsigned readTimeStamps();

      void setCalFile(std::string calFile);

      void setDates(std::string startUtc, std::string stopUtc);

      bool readNextFrame();

      bool updateArrayMap();

      void updateRegisterCalibration();

      void updateRegSelection();

      void updateRegBufPtrCache(unsigned nByteRanges);

      void readFirstArrayMap();

      void printAddresses();

    private:

      bool convert_;
      bool read_;

      static ArchiveReader::Format formats_[];
      static int nFormat_;

      unsigned iFrame_;
      unsigned nFrame_;

    public:
      RegCal* regCal_;
    private:
      std::string calFile_;
      bool haveCalFile_;

      std::vector<ArchiveByteRange> byteRanges_;

      ArrayMap* arrayMap_;

      bool memMap_;
      std::string arcDir_;
      bool arcDirIsSet_;

      std::string startUtc_;
      std::string stopUtc_;
      bool datesAreSet_;

      std::vector<ArchiveReader::ArchiveFile> fileList_;
      std::vector<ArchiveReader::ArchiveFile>::iterator currFile_;
      unsigned nFile_;

      ArchiveFileHandler handler_;

      // The array of registers and format specifiers input by the user

    public:

      std::vector<bool> regSpecValid_;
      std::vector<std::string> regSpecs_;
      std::vector<std::string> formatSpecs_;
      std::vector<ArchiveTransposeType> transposeTypes_;
      std::vector<int> widths_;
      std::vector<int> precs_;

      // The array of registers and associated information derived
      // from the above.  Note, however, that these arrays are not in
      // general of the same length as the above, since some registers
      // may have errors in the specification, or may not exist in
      // the register map

      void printRegsSize();

      std::vector<ArchiveRegister>& getRegs();
      std::vector<RegDescription>&  getRegDescs();

    public:

      std::vector<ArchiveRegister> regs_;
      std::vector<DataType::Type> formatTypes_;
      std::vector<RegDescription> regDescs_;

    private:

      static bool compArchiveFile(ArchiveReader::ArchiveFile f1, 
				  ArchiveReader::ArchiveFile f2);

      static bool compArchiveByteRange(ArchiveByteRange r1, ArchiveByteRange r2);

      static DataType::Type parseFormat(std::string format);

      bool unique(ArchiveRegister& reg);

      void checkFirstFile();

    public:

      void addRegister(std::string regSpec);
      void addRegisterOnly(std::string regSpec);
  
      bool stageNextFile();

      void resetToBeginning();

      bool advanceFile();

      void iterateFiles();

      void readRegs();
      void printRegs(std::ostringstream& os);

      // Convenience methods for external use of this object

      std::vector<RegDescription> selectedRegs();

      std::vector<DataType::Type> selectedFormats();

    }; // End class ArchiveReader

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_ARCHIVEREADER_H
