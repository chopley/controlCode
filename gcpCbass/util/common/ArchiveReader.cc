#include "gcp/util/common/ArchiveFileHandler.h"
#include "gcp/util/common/ArchiveReader.h"
#include "gcp/util/common/Date.h"
#include "gcp/util/common/DirList.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/RegDescription.h"
#include "gcp/util/common/RegParser.h"
#include "gcp/util/common/String.h"

#include "gcp/control/code/unix/libunix_src/common/pathname.h"

#include<iostream>
#include<iomanip>

using namespace std;

using namespace gcp::util;

/**
 * Declare a static array of valid format args
 */
ArchiveReader::Format ArchiveReader::formats_[] = {
  {"string",       DataType::STRING},
  {"bool",         DataType::BOOL},
  {"uchar",        DataType::UCHAR},
  {"char",         DataType::CHAR},
  {"float",        DataType::FLOAT},
  {"double",       DataType::DOUBLE},
  {"short",        DataType::SHORT},
  {"ushort",       DataType::USHORT},
  {"int",          DataType::INT},
  {"uint",         DataType::UINT},
  {"long",         DataType::LONG},
  {"ulong",        DataType::ULONG},
  {"date",         DataType::DATE},
  {"complex",      DataType::COMPLEX_FLOAT},
};

int ArchiveReader::nFormat_ = 
sizeof(ArchiveReader::formats_)/sizeof(ArchiveReader::Format);

void ArchiveReader::initialize(bool memMap, bool convert, bool read)
{
  arcDirIsSet_ = false;
  datesAreSet_ = false;
  nFile_       = 0;
  memMap_      = memMap;
  convert_     = convert;
  read_        = read;
  arrayMap_    = 0;

  regCal_      = 0;
  haveCalFile_ = false;
  iFrame_      = 0;
  nFrame_      = 0;
}

/**.......................................................................
 * Constructor.
 */
ArchiveReader::ArchiveReader(bool memMap, bool convert, bool read) 
{
  initialize(memMap, convert, read);
}

ArchiveReader::ArchiveReader(std::string arcDir, std::string calFile, std::string startUtc, 
			     std::string stopUtc, bool memMap, bool convert, bool read)
{
  initialize(memMap, convert, read);
  setArchiveDirectory(arcDir);
  setCalFile(calFile);
  setDates(startUtc, stopUtc);
}

/**.......................................................................
 * Const Copy Constructor.
 */
ArchiveReader::ArchiveReader(const ArchiveReader& objToBeCopied)
{
  *this = (ArchiveReader&)objToBeCopied;
};

/**.......................................................................
 * Copy Constructor.
 */
ArchiveReader::ArchiveReader(ArchiveReader& objToBeCopied)
{
  *this = objToBeCopied;
};

/**.......................................................................
 * Const Assignment Operator.
 */
void ArchiveReader::operator=(const ArchiveReader& objToBeAssigned)
{
  *this = (ArchiveReader&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void ArchiveReader::operator=(ArchiveReader& objToBeAssigned)
{
  std::cout << "Calling default assignment operator for class: ArchiveReader" << std::endl;
};

/**.......................................................................
 * Output Operator.
 */
std::ostream& gcp::util::operator<<(std::ostream& os, ArchiveReader& obj)
{
  os << "Default output operator for class: ArchiveReader" << std::endl;
  return os;
};

/**.......................................................................
 * Destructor.
 */
ArchiveReader::~ArchiveReader() 
{
  if(arrayMap_) {
    arrayMap_ = del_ArrayMap(arrayMap_);
  }

  if(regCal_) {
    delete regCal_;
    regCal_ = 0;
  }
}

void ArchiveReader::setDates(std::string startUtc, std::string stopUtc)
{
  startUtc_    = startUtc;
  stopUtc_     = stopUtc;
  datesAreSet_ = true;
}

void ArchiveReader::setCalFile(std::string calFile)
{
  calFile_     = calFile;
  haveCalFile_ = true;
}

/**.......................................................................
 * Construct a sorted list of archive files from a given directory
 */
void ArchiveReader::getFileList()
{
  if(!arcDirIsSet_)
    ThrowError("No archive directory has been specified");

  if(!datesAreSet_)
    ThrowError("No dates have been specified");

  double daysec = 24*3600;

  Date start;
  start.setToDateAndTime(startUtc_);

  Date stop;
  stop.setToDateAndTime(stopUtc_);

  double ta = start.getMjd();
  double tb = stop.getMjd();

  // Attempt to open the named directory

  gcp::util::DirList dirList(arcDir_, true);

  // List all files under the named directory, including recursive
  // searches into subdirectories.  We return the list of all files,
  // including files that are symlinks

  std::list<gcp::util::DirList::DirEnt> fileList;
  fileList = dirList.getFiles(true);
  fileList_.resize(fileList.size());

  // Store a pointer to the file immediately preceding the start time
  // of interest

  gcp::util::DirList::DirEnt* latestPrecedingFile=0;
  ArcTimeStamp latestPrecedingTs;
  double latestPrecedingUtc = 0;

  std::list<gcp::util::DirList::DirEnt>::iterator entry;
  for(entry = fileList.begin(); entry != fileList.end(); entry++) {

    ArcFileType type; // The type of archive file 
    ArcTimeStamp ts;  // The archive-filename timestamp 
    double utc;       // The Modified Julian Date of the file timestamp 
    
    // Try to decode the file name as that of an archive file.  Skip
    // this file if it isn't an archive file

    if(arc_file_id((char*)entry->name_.c_str(), 0, &ts, &type) 
       || type != ARC_DAT_FILE)
      continue;
    
    // Combine the Modified Julian Day number and time of day into a
    // Modified Julian Date.

    utc = ts.mjd + (double) ts.sec/daysec;
    
    // If the timestamp is later than the latest time that we are
    // interested in skip it.

    if(tb > 0.0 && utc > tb)
      continue;

    // Check whether the file is a regular file and whether we have
    // read access to it.

    char* errmsg=0;
    if((errmsg=test_pathname((char*)entry->fullName().c_str(), 
			     PATH_IS_REG, PATH_READ))) {
      ReportError("Ignoring " << entry->fullName().c_str() << " (" 
		  <<  errmsg << ")");
      continue;
    };

    // The latest timestamp that precedes the earliest time of
    // interest may refer to a file who's later entries are in the
    // range of interest.  latest_preceding refers to the timestamp of
    // the latest timestamp found so far that precedes ta

    if(ta > 0.0 && utc <= ta) {

      if(utc > latestPrecedingUtc) {
	latestPrecedingUtc    = utc;
	latestPrecedingTs.mjd = ts.mjd;
	latestPrecedingTs.sec = ts.sec;
	latestPrecedingFile   = &(*entry);
      }

      // Don't unnecessarily add this file to the list (we may find a
      // later one) until the end

      continue;
    };
    
    // Record the time-stamp of the new file.

    fileList_[nFile_].ts_.mjd = ts.mjd;
    fileList_[nFile_].ts_.sec = ts.sec;
    fileList_[nFile_].name_   = entry->fullName();

    ++nFile_;
  };
  
  if(latestPrecedingFile != 0) {

    fileList_[nFile_].ts_.mjd = latestPrecedingTs.mjd;
    fileList_[nFile_].ts_.sec = latestPrecedingTs.sec;
    fileList_[nFile_].name_   = latestPrecedingFile->fullName();

    ++nFile_;
  }
  
  // Did we fail to find any readable archive files?
  
  if(nFile_ == 0) {
    ThrowError("Directory " << arcDir_ 
	       << " doesn't contain any suitable files");
  };
  
  fileList_.resize(nFile_);
  currFile_ = fileList_.begin();

  // Sort the list of time-stamps.

  std::sort(fileList_.begin(), fileList_.end(), compArchiveFile);
}

/**.......................................................................
 * A function for passing to std::sort() to sort a list of ArchiveFile
 * objects
 */
bool ArchiveReader::compArchiveFile(ArchiveReader::ArchiveFile f1, 
				    ArchiveReader::ArchiveFile f2) 
{
  if(f1.ts_.mjd < f2.ts_.mjd)
    return true;
  else if(f1.ts_.mjd > f2.ts_.mjd)
    return false;
  else if(f1.ts_.sec < f2.ts_.sec)
    return true;
  else if(f1.ts_.sec > f2.ts_.sec)
    return false;
  else 
    return false;
}

/**.......................................................................
 * Set the top-level archive directory.  Note that this class
 * will read files in subdirectories below the top-level
 */
void ArchiveReader::setArchiveDirectory(std::string arcDir)
{
  arcDir_      = arcDir;
  arcDirIsSet_ = true;
}

/**.......................................................................
 * Count the number of frames spanned by the date range
 */
unsigned ArchiveReader::countFrames()
{
  ArchiveFileHandler fh;
  unsigned nFrame = 0, iFrame;
  std::vector<ArchiveReader::ArchiveFile>::iterator file;

  // Check the last timestamp in the first file.  This is to catch the
  // case where our startutc falls between the last actual timestamp
  // in the first file, and the first timestamp of the next file

  checkFirstFile();

  // Iterate over files within the date range

  unsigned iFile=0;
  for(file=fileList_.begin(); file != fileList_.end(); ) {

    fh.setTo(file->name_);

    // Open the file for reading.  If there is an error reading this
    // file, simply skip to the next one and delete it from the list
    // of files in this date range

    try {
      fh.openForRead(memMap_);
    } catch(...) {
      file = fileList_.erase(file);
      nFile_ = fileList_.size();
      continue;
    }

    // If this file contains less than 1 usable frame, discard it

    if(fh.nFrame() == 0) {
      file = fileList_.erase(file);
      nFile_ = fileList_.size();
      continue;
    }

    // We have four possible cases:
    
    // 1) This file could be the one and only file

    if(iFile==0 && iFile == fileList_.size()-1) {

      iFrame = fh.findFirstFrameAfter(startUtc_);
      file->startFrame_ = iFrame;

      iFrame = fh.findFirstFrameBefore(stopUtc_);
      file->stopFrame_  = iFrame;
      
      file->currFrame_  = 0;

      nFrame += file->stopFrame_ - file->startFrame_ + 1;

      // 2) Else if could be the first file, including data prior to
      //    the start date

    } else if(iFile==0) {

      iFrame = fh.findFirstFrameAfter(startUtc_);
      nFrame += fh.nFrame() - iFrame;

      file->startFrame_ = iFrame;
      file->stopFrame_  = fh.nFrame()-1;
      file->currFrame_  = 0;

      // 3) Else if could be the last file, including data after the
      //    stop date

    } else if(iFile==nFile_ - 1) {

      iFrame = fh.findFirstFrameBefore(stopUtc_);
      nFrame += iFrame + 1;

      file->startFrame_ = 0;
      file->stopFrame_  = iFrame;
      file->currFrame_  = 0;

      // 4) Else it could a file completely spanned by the date range

    } else {

      nFrame += fh.nFrame();

      file->startFrame_ = 0;
      file->stopFrame_  = fh.nFrame()-1;
      file->currFrame_  = 0;

    }

    fh.close();

    file++;
    iFile++;

  }

  nFile_  = fileList_.size();
  nFrame_ = nFrame;
  iFrame_ = 0;

  return nFrame;
}

/**.......................................................................
 * Iterate through files, reading the date
 */
unsigned ArchiveReader::readTimeStamps()
{
  ArchiveFileHandler fh;
  unsigned nFrame = 0, iFrame;

  // Iterate over files within the date range

  for(unsigned iFile=0; iFile < nFile_; iFile++) {
    fh.setTo(fileList_[iFile].name_);
    fh.openForRead(memMap_);
    nFrame = fh.nFrame();

    fh.readTimestamps();

    fh.close();
  }

  return nFrame;
}

/**.......................................................................
 * Read the next frame of registers
 */
bool ArchiveReader::readNextFrame()
{
  // If we are at the end of the list of files return false;

  if(currFile_ == fileList_.end()) {

    if(!read_) {
      std::cout << "\rReading..." << std::fixed << std::setprecision(0) << "100%";
      fflush(stdout);
    }

    return false;

    // Else read the next frame

  } else {

    // If we are at the end of the current file, advance to the next
    // file before reading

    if(currFile_->atEnd()) {
      advanceFile();
    }

    if(currFile_ == fileList_.end()) {

      if(!read_) {
	std::cout << "\rReading..." << std::fixed << std::setprecision(0) << "100%";
	fflush(stdout);
      }

      return false;

    } else {

      handler_.advanceToFrame(currFile_->currFrame_);

      currFile_->incr();

      if(!read_) {
	if(iFrame_ < 10 || iFrame_ % (nFrame_/10) == 0) {
	  std::cout << "\rReading..." << std::fixed << std::setprecision(0) 
		    << (double)(iFrame_)/nFrame_*100 << "%";
	  fflush(stdout);
	}
      }

      return true;

    }
  }
}

/**.......................................................................
 * Stage the next file for reading.
 *
 * Return true if the array map was updated on this call, false if
 * not.
 */
bool ArchiveReader::stageNextFile()
{
  currFile_->currFrame_ = currFile_->startFrame_;
  handler_.close();
  handler_.setTo(currFile_->name_);
  handler_.openForRead(memMap_);

  // Update the array map if it has changed

  return updateArrayMap();
}

/**.......................................................................
 * Reset to the first file in the list
 */
void ArchiveReader::resetToBeginning()
{
  regs_.resize(0);
  formatTypes_.resize(0);
  regDescs_.resize(0);

  iFrame_ = 0;

  arrayMap_  = del_ArrayMap(arrayMap_);
  currFile_  = fileList_.begin();

  stageNextFile();
}

/**.......................................................................
 * Advance to the next file in the list.
 *
 * Return true if ths arraymap was updated on this call, false if not
 */
bool ArchiveReader::advanceFile()
{
  if(currFile_ != fileList_.end()) {
    currFile_++;
  }

  if(currFile_ != fileList_.end()) {
    return stageNextFile();
  }

  return false;
}

void ArchiveReader::iterateFiles()
{
  std::vector<ArchiveReader::ArchiveFile>::iterator test;
  for(test = fileList_.begin(); test != fileList_.end(); test++) {
    COUT(test->name_);
  }
}

/**.......................................................................
 * When a new array map is encountered, this function is called to update
 * our internal members that depend on the array map
 */
void ArchiveReader::readFirstArrayMap()
{
  // Try to read the array map out of the first file.  If an error
  // occurs, delete this file from the list of valid files, and try to
  // read from the next one

  bool success=false;

  do {

    try {
      resetToBeginning();
      updateArrayMap();
      success = true;
    } catch(...) {
      fileList_.erase(fileList_.begin());
      nFile_ = fileList_.size();
    }
    
  } while(!(success && nFile_ > 0));
}

void ArchiveReader::printRegsSize()
{
}

/**.......................................................................
 * When a new array map is encountered, this function is called to
 * update internal members that depend on the array map
 *
 * Return true if the array map was updated on this call. False if not
 */
bool ArchiveReader::updateArrayMap()
{
  bool wasUpdated=false;

  if(arrayMap_ == 0 || !equiv_ArrayMap(arrayMap_, handler_.getArrayMap())) {

    // Decremement the reference count of the arrayMap_ pointer we are
    // currently holding

    if(arrayMap_) {
      arrayMap_ = del_ArrayMap(arrayMap_);
    }

    // And increment the ref count for the new one

    arrayMap_ = alias_ArrayMap(handler_.getArrayMap());

    updateRegisterCalibration();
    updateRegSelection();
    wasUpdated = true;
  }

  return wasUpdated;
}

/**.......................................................................
 * Add a register to the list of registers to be read
 */
void ArchiveReader::addRegisterOnly(std::string regSpec)
{
  ArchiveTransposeType transpose = ArchiveReader::NONE;

  String regSpecStr(regSpec);
  String regStr    = regSpecStr.findNextInstanceOf("", false, " ", false, false);

  if(regStr[regStr.size()-1] == '^') {
    regStr.strip("^");
    transpose = ArchiveReader::LAST;
  } else if(regStr[regStr.size()-1] == '~') {
    regStr.strip("~");
    transpose = ArchiveReader::FIRST;
  }

  String formatStr = regSpecStr.findNextInstanceOf(" ", true, " ", false, false);
  regStr.strip(" ");
  formatStr.strip(" ");

  regSpecs_.push_back(regStr.str());
  formatSpecs_.push_back(formatStr.str());
  transposeTypes_.push_back(transpose);
  regSpecValid_.push_back(false); // Reg spec not valid until parsed
				  // against a register map
}

/**.......................................................................
 * Add a register to the list of registers to be read
 */
void ArchiveReader::addRegister(std::string regSpec)
{
  String regSpecStr(regSpec);
  String keyValPair, key, val;
  int width=10, prec=4;
  bool first=true;

  while(!regSpecStr.atEnd()) {
      
    keyValPair = regSpecStr.findNextInstanceOf("", false, ",", false, true);

    // If this is the first string, it is the register specification
    
    if(first) {
      first = false;
      addRegisterOnly(keyValPair.str());
      
      // Else it should be a keyword=val pair
      
    } else {
      
      // Do something with this keyword-value pair
      
      if(!keyValPair.isEmpty()) {
	if(!keyValPair.contains("=")) {
	  ThrowError("Badly formed keyword/value pair: '" << keyValPair << "'.  Should be 'keyword=value'");
	}
	
	key = keyValPair.findNextInstanceOf(" ", false, "=", true,  false);
	val = keyValPair.findNextInstanceOf("=", true,  " ", false, true);     
	
	key.strip(" ");
	
	// Parse optional keywords, ignoring unrecognized keywords
	
	if(key.str() == "width") {
	  width = val.toInt();
	} else if(key.str() == "prec") {
	  prec = val.toInt();
	}
	
      }
    }
  }

  // If a register was specified, store the requested field widths and
  // precision as well

  widths_.push_back(width);
  precs_.push_back(prec);
}

/**.......................................................................
 * If the array map changed, update the register calibration
 */
void ArchiveReader::updateRegisterCalibration()
{
  if(haveCalFile_) {

    if(regCal_) {
      delete regCal_;
      regCal_ = 0;
    }

    regCal_ = new RegCal(arrayMap_, true);
    regCal_->loadCalFile("", calFile_, false);
  }
}

/**.......................................................................
 * If the array map changed, update the register selection
 */
void ArchiveReader::updateRegSelection()
{
  bool first = (regs_.size() == 0);
  unsigned iArcReg=0;

  // If this is the first array map we've encountered, parse the reg
  // descriptions.  Note that some reg descriptions can specify more
  // than one register (antenna*.tracker.actual, for example)

  RegParser parser(true);
  std::vector<RegDescription> regDescs;
  ArchiveRegister reg;

  unsigned nByteRange = 0;
  
  //------------------------------------------------------------
  // Iterate over distinct register specifications
  //------------------------------------------------------------
  
  for(unsigned iSpec=0; iSpec < regSpecs_.size(); iSpec++) {
    
    // Get the (possibly more than one) register descriptions that
    // match this specification
    
    regDescs = parser.inputRegs(regSpecs_[iSpec], arrayMap_, REG_INPUT_RANGE, 
				true, true, false, false);

    // For now, set the validity flag to false if the register
    // specification doesn't correspond to any register in the array
    // map.  
    //
    // This will have the effect of invalidating the register
    // specification for all time, however, and doesn't account for
    // register maps that subsequently remove the register.
    // 
    // Need a more general solution that parses the register
    // specification for all array maps, and if it matches any,
    // creates a valid entry in the regs_ array.  

    if(regDescs.size() == 0) {

      if(first) {
	regSpecValid_[iSpec] = false;
      }

    } else {

      if(first) {
	regSpecValid_[iSpec] = true;
      }

    }

    
    if(!regSpecValid_[iSpec]) {
      continue;
    }

    // If the format specifier was present, parse it now.  Else set
    // the output type to the input type of this register

    DataType::Type formatType;
    if(formatSpecs_[iSpec].size() > 0)
      formatType = parseFormat(formatSpecs_[iSpec]);
    else
      formatType = DataType::typeOf(regDescs[0].block());

    // Check that the register and format specifications were valid

    if(parser.validity() != REG_VALID) {
      ReportError("Invalid register specified: " << regSpecs_[iSpec] << "(ignoring)");
    } else if(formatType == DataType::UNKNOWN) {
      ReportError("Invalid format specified: " << formatSpecs_[iSpec] << " (ignoring)");
      
      //------------------------------------------------------------
      // If everything checks out, add the register to the list of
      // regs we will monitor
      //------------------------------------------------------------

    } else {

      for(unsigned iReg=0; iReg < regDescs.size(); iReg++) {
	reg.initialize(regDescs[iReg], formatType, transposeTypes_[iSpec], widths_[iSpec], precs_[iSpec], 
		       convert_, read_);

	//------------------------------------------------------------
	// Check that this register hasn't already been specified
	//------------------------------------------------------------

	if(unique(reg)) {

	  // If the register specifications have already been parsed,
	  // just update the information related to the new array
	  // map

	  if(first) {
	    regs_.push_back(reg);
	    formatTypes_.push_back(formatType);
	    regDescs_.push_back(regDescs[iReg]);
	  } else {
	    regs_[iArcReg].initialize(regDescs[iReg], formatType, transposeTypes_[iSpec], widths_[iSpec], precs_[iSpec],
				      convert_, read_);
	    ++iArcReg;
	  }
	  
	  // Keep track of the number of distinct byte ranges encountered
	  
	  nByteRange += reg.byteRanges_.size();
	} else {
	  //	  ReportError("Register: " << regSpecs_[iSpec] << " has already been specified: ignoring");
	}
      }
    }
  }

  // And re-cache the register buf pointers

  updateRegBufPtrCache(nByteRange);
}

bool ArchiveReader::unique(ArchiveRegister& reg)
{
  for(unsigned i=0; i < regs_.size(); i++) {
    if(reg.arregmap_ == regs_[i].arregmap_ &&
       reg.board_    == regs_[i].board_ &&
       reg.block_    == regs_[i].block_) {
      return false;
    }
  }

  return true;
}

std::vector<RegDescription> ArchiveReader::selectedRegs()
{
  return regDescs_;
}

std::vector<DataType::Type> ArchiveReader::selectedFormats()
{
  return formatTypes_;
}

/**.......................................................................
 * Output a registere selection
 */
std::ostream& gcp::util::operator<<(std::ostream& os, ArchiveReader::ArchiveRegister& reg)
{
  os << reg.arregmap_->name << "." << reg.board_->name << "." << reg.block_->name_;
  for(unsigned i=0; i < reg.byteRanges_.size(); i++) {
    os << reg.byteRanges_[i];
  }
  return os;
}

/**.......................................................................
 * Iterate through all distinct byte ranges in the current register
 * selection
 */
void ArchiveReader::updateRegBufPtrCache(unsigned nByteRanges)
{
  byteRanges_.resize(nByteRanges);

  for(unsigned iReg=0, iByteRange=0; iReg < regs_.size(); iReg++) {

    ArchiveRegister& reg = regs_[iReg];
    unsigned bufInd  = 0;
    unsigned slotInd = 0;

    for(unsigned iRange=0; iRange < regs_[iReg].byteRanges_.size(); iRange++, iByteRange++) {

      Range<unsigned>& range      = reg.byteRanges_[iRange];
      Range<unsigned>& slotRange  = reg.slotRanges_[iRange];
      ArchiveByteRange& byteRange = byteRanges_[iByteRange];

      // Set the pointer for this byte range pointing to the
      // corresponding address in the register's buffer

      byteRange.reg_ = &reg;
      byteRange.ptr_ = &reg.buf_[bufInd];
      byteRange.byteOffsetFromStartOfFrame_ = range.start();

      byteRange.nByte_ = range.stop() - range.start() + 1;

      // Increment the buffer index by the number of bytes in this range

      bufInd += (range.stop() - range.start()) + 1;

      // Also fill the calibration data with information for the
      // appropriate slots.  If no cal file was specified, just set
      // all calibration parameters to 1.0 and 0.0 (reset).

      for(unsigned iSlot=slotRange.start(); iSlot <= slotRange.stop(); iSlot++, slotInd++) {
	if(regCal_) {
	  reg.regCalSlots_[slotInd] = regCal_->getRegCalSlot(iSlot);
	} else {
	  reg.regCalSlots_[slotInd].reset();
	}
      }

    }
  }

  // Now sort the byte ranges into increasing order, so that file
  // reading will proceed by the smallest memory offsets possible

  std::sort(byteRanges_.begin(), byteRanges_.end(), compArchiveByteRange);
}

/**.......................................................................
 * A function for passing to std::sort() to sort a list of
 * ArchiveByteRange objects
 */
bool ArchiveReader::compArchiveByteRange(ArchiveByteRange r1, ArchiveByteRange r2)
{
  if(r1.byteOffsetFromStartOfFrame_ < r2.byteOffsetFromStartOfFrame_)
    return true;
  else
    return false;
}

/**.......................................................................
 * Read register values from the current frame.  The data stream
 * should be positioned at the head of the current frame when this
 * method is called
 */
void ArchiveReader::readRegs()
{
  off_t offset;
  ArchiveByteRange* curr=0;
  ArchiveByteRange* last=0;

  // Iterate over all distinct byte ranges that were specified

  for(unsigned i=0; i < byteRanges_.size(); i++) {

    curr = &byteRanges_[i];

    // If this is the first reg, the file pointer is now pointing to
    // the head of the frame
    // 
    // Else it is pointing just past the last byte range that was read

    offset = (i==0) ? curr->byteOffsetFromStartOfFrame_ : 
      curr->byteOffsetFromStartOfFrame_ - 
      last->byteOffsetFromStartOfFrame_ - last->nByte_;

    handler_.advanceByNbytes(offset);
    handler_.read(curr->ptr_, curr->nByte_);

    curr->reg_->convertVals(iFrame_, nFrame_);

    last = curr;
  }

  ++iFrame_;
}

/**.......................................................................
 * Read register values from the current frame.  The data stream
 * should be positioned at the head of the current frame when this
 * method is called
 */
void ArchiveReader::printRegs(std::ostringstream& os)
{
  off_t offset;
  ArchiveByteRange* curr=0;
  ArchiveByteRange* last=0;

  // Iterate over all distinct byte ranges that were specified

  for(unsigned i=0; i < byteRanges_.size(); i++) {

    curr = &byteRanges_[i];

    // If this is the first reg, the file pointer is now pointing to
    // the head of the frame
    // 
    // Else it is pointing just past the last byte range that was read

    offset = (i==0) ? curr->byteOffsetFromStartOfFrame_ : 
      curr->byteOffsetFromStartOfFrame_ - 
      last->byteOffsetFromStartOfFrame_ - last->nByte_;

    handler_.advanceByNbytes(offset);
    handler_.read(curr->ptr_, curr->nByte_);

    curr->reg_->printVals();

    last = curr;
  }

  for(unsigned i=0; i < regs_.size(); i++) {
    if(i==0) {
      os << regs_[i].strVal_.str();
    } else {
      os << " " << regs_[i].strVal_.str();
    }
  }

  ++iFrame_;
}

/**.......................................................................
 * Parse a register format specification
 */
DataType::Type ArchiveReader::parseFormat(std::string str) 
{
  for(unsigned iformat=0; iformat < nFormat_; iformat++)
    if(formats_[iformat].format_.compare(str.c_str())==0)
      return formats_[iformat].type_;
  
  return DataType::UNKNOWN;
}

void ArchiveReader::checkFirstFile()
{
  Date targetDate;
  targetDate.setToDateAndTime(startUtc_);
  double mjd = targetDate.getMjd();

  std::vector<ArchiveReader::ArchiveFile>::iterator file = fileList_.begin();

  ArchiveFileHandler fh;

  fh.setTo(file->name_);
  
  double mjdLast;

  try {
    fh.openForRead(memMap_);
    mjdLast = fh.getMjd(fh.nFrame()-1);

    if(mjdLast < mjd) {
      fileList_.erase(file);
      nFile_ -= 1;
    }
  } catch(...) {
    fileList_.erase(file);
    nFile_ -= 1;
  }

  // And close the file

  fh.close();

  if(nFile_ == 0) {
    ThrowError("Directory " << arcDir_ 
	       << " doesn't contain any suitable files");
  }
}

void ArchiveReader::printAddresses()
{
  COUT("&formats_  = " << &formats_);
  COUT("&nFormat_  = " << &nFormat_);
  COUT("&iFrame_  = " <<  &iFrame_);
  COUT("&nFrame_  = " <<  &nFrame_);

  COUT("&regCal_  = " << &regCal_);
  COUT("&byteRanges_  = " << &byteRanges_);
  COUT("&startUtc_ = " << &startUtc_);
  COUT("&currFile_ = " << &currFile_);
  COUT("&nFile_ = " << &nFile_);
  COUT("&handler_ = " << &handler_);

  COUT("&regSpecs_ = " << &regSpecs_);
  COUT("&regDescs_ = " << &regDescs_);
}

std::vector<ArchiveReader::ArchiveRegister>& ArchiveReader::getRegs()
{
  return regs_;
}

std::vector<RegDescription>& ArchiveReader::getRegDescs()
{
  return regDescs_;
}
