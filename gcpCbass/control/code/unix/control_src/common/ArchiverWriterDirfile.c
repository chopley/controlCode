#include "gcp/control/code/unix/control_src/common/ArchiverWriterDirfile.h"

#include "gcp/control/code/unix/control_src/common/archiver.h"
#include "gcp/control/code/unix/libunix_src/common/arcfile.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include<iostream>
#include<fstream>

#include<string.h>

// Needed for mkdir()

#include <sys/stat.h>
#include <sys/types.h>

// Needed for open()/close()/write()

#include <fcntl.h>
#include <unistd.h>
#include <iomanip>

using namespace std;
using namespace gcp::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
ArchiverWriterDirfile::ArchiverWriterDirfile(Archiver* arc, unsigned nBuffer) : 
  ArchiverWriter(arc) 
{
  isOpen_ = false;
  maxlen_ = 0;

  ArrayMap* arrayMap = arc_->fb.arraymap;

  // Iterate through the array map, getting pointers to the location
  // in the network buffer that will correspond to this register

  for(std::vector<ArrRegMap*>::iterator iRegMap=arrayMap->regmaps.begin(); 
      iRegMap != arrayMap->regmaps.end(); iRegMap++) {

    ArrRegMap* arrRegMap = *iRegMap;

    for(std::vector<RegMapBoard*>::iterator iBoard=arrRegMap->regmap->boards_.begin();
	iBoard != arrRegMap->regmap->boards_.end(); iBoard++) {
      RegMapBoard* board = *iBoard;
      for(std::vector<RegMapBlock*>::iterator iBlock=board->blocks.begin();
	  iBlock != board->blocks.end(); iBlock++) {

	insert(*iBlock, arrRegMap, arc_->net->buf, nBuffer);
      }
    }
  }
}

/**.......................................................................
 * Insert a register block
 */
void ArchiverWriterDirfile::insert(RegMapBlock* block, 
				   ArrRegMap* arrRegMap, 
				   unsigned char* basePtr,
				   unsigned nBuffer)
{
  // We are only interested in archived registers

  if(block->flags_ & REG_EXC)
    return;

  // If this is a fast register, then we will write each of the slow
  // indices out as a separate file

  if(block->flags_ & REG_FAST && block->axes_->nAxis() > 2) {
    ThrowError("I don't know how I should deal with nAxis > 2!");
  } else if((block->flags_ & REG_FAST) && block->axes_->nAxis() == 2 
	    && block->axes_->nEl(0) > 1) {

    unsigned nSlowIndex = block->axes_->nEl(0);
    unsigned nFastIndex = block->axes_->nEl(1);

    for(unsigned iSlow=0; iSlow < nSlowIndex; iSlow++) 
      insertReg(block, arrRegMap, iSlow, nFastIndex, basePtr, true, nBuffer);
 
    // Else just write out the register in its entirety

 } else
   insertReg(block, arrRegMap, 0, block->nEl(), basePtr, false, nBuffer);
}

/**.......................................................................
 * Insert a block of register elements into the list
 */
void ArchiverWriterDirfile::insertReg(RegMapBlock* block, 
				      ArrRegMap* arrRegMap, 
				      unsigned startEl,
				      unsigned nEl,
				      unsigned char* basePtr,
				      bool writeIndex,
				      unsigned nBuffer) 
{
  Register reg(block, arrRegMap, startEl, nEl, basePtr, writeIndex, nBuffer);
  
  // Insert the new element in the list
  
  registers_.insert(registers_.end(), reg);
  
  // Increment the maximum name length
  
  maxlen_ = (reg.name_.size() > maxlen_) ? reg.name_.size() : maxlen_;
}

/**.......................................................................
 * Destructor
 */
ArchiverWriterDirfile::~ArchiverWriterDirfile() {}

/**.......................................................................
 * Const Copy Constructor.
 */
ArchiverWriterDirfile::ArchiverWriterDirfile(const ArchiverWriterDirfile& objToBeCopied)
{
  *this = (ArchiverWriterDirfile&)objToBeCopied;
};

/**.......................................................................
 * Copy Constructor.
 */
ArchiverWriterDirfile::ArchiverWriterDirfile(ArchiverWriterDirfile& objToBeCopied)
{
  *this = objToBeCopied;
};

/**.......................................................................
 * Const Assignment Operator.
 */
void ArchiverWriterDirfile::operator=(const ArchiverWriterDirfile& objToBeAssigned)
{
  *this = (ArchiverWriterDirfile&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void ArchiverWriterDirfile::operator=(ArchiverWriterDirfile& objToBeAssigned)
{
  std::cout << "Warning: calling default assignment operator for class: "
    "ArchiverWriterDirfile" << std::endl;
};

/**.......................................................................
 * Open dir files for all archived registers
 */
int ArchiverWriterDirfile::openArcfile(char* dir)
{
  char *path=0;   // The path name of the file 
  int waserr=0;

  // Record a new directory?

  if(dir && *dir!='\0')
    (void) chdir_archiver(arc_, dir);
  else
    dir = arc_->dir;
  
  // Compose the full pathname of the archive file.

  path = arc_path_name(dir, NULL, ARC_DAT_FILE);
  if(!path)
    return 1;
  
  // Close the current archive file if open.

  closeArcfile();

  // For dir files, the path is a new directory that must be created:

  if(createDir(path))
    return 1;

  // Write out the format file

  if(outputFormatFile(path))
    return 1;

  // Attempt to open the new data files.

  for(std::list<Register>::iterator iReg=registers_.begin();
      iReg != registers_.end(); iReg++)
    waserr |= iReg->open(path);
  
  // Return error if an error occurred opening any of the files

  if(waserr) 
    return 1;

  // Install the new archive file.

  arc_->path = path;
  arc_send_ccmsg(arc_, NULL, "OPEN %s", arc_->path);
  
  // Report the successful opening of the file.

  lprintf(stdout, "Starting new archive file: %s\n", path);

  isOpen_ = true;

  return 0;
}

/**.......................................................................
 * Create the named directory
 */
int ArchiverWriterDirfile::createDir(char* path)
{
  return (mkdir(path, 0755) < 0);
}

/**.......................................................................
 * Close all open files
 */
void ArchiverWriterDirfile::closeArcfile() 
{
  for(std::list<Register>::iterator iReg=registers_.begin();
      iReg != registers_.end(); iReg++)
    iReg->close();

  if(arc_->path)
    free(arc_->path);

  arc_->path = NULL;
  arc_->nrecorded = 0;
  arc_send_ccmsg(arc_, NULL, "CLOSE");

  isOpen_ = false;
}

/**.......................................................................
 * Flush all open files
 */
void ArchiverWriterDirfile::flushArcfile()
{
  for(std::list<Register>::iterator iReg=registers_.begin();
      iReg != registers_.end(); iReg++)
    iReg->flush();
}

/**.......................................................................
 * Write the latest integration
 */
int ArchiverWriterDirfile::writeIntegration() 
{
  int waserr=0;
  for(std::list<Register>::iterator iReg=registers_.begin();
      iReg != registers_.end(); iReg++)
    waserr |= iReg->write();
  return waserr;
}

/**.......................................................................
 * Return true if an archive "file" is currently open
 */
bool ArchiverWriterDirfile::isOpen()
{
  return isOpen_;
}

/**.......................................................................
 * Output the dirfile format file needed by things like KST
 */
int ArchiverWriterDirfile::outputFormatFile(char* dir)
{
  if(dir==0) 
    return 1;

  std::stringstream os;
  os << dir << "/format";
  std::ofstream fout(os.str().c_str(), ios::out);

  if(!fout)
    return 1;

  for(std::list<Register>::iterator iReg=registers_.begin();
      iReg != registers_.end(); iReg++) 
    fout << std::setw(maxlen_ + 1) << std::left << iReg->name_ << " " << "RAW" << " " << iReg->format() << " " << iReg->nEl_ << std::endl;

  fout.close();
  return 0;
}

//-----------------------------------------------------------------------
// Methods of Register class
//-----------------------------------------------------------------------

/**.......................................................................
 * Constructor for Register
 */
ArchiverWriterDirfile::Register::Register(RegMapBlock* block, 
					  ArrRegMap* arrRegMap,
					  unsigned startEl,
					  unsigned nEl,
					  unsigned char* base,
					  bool writeIndex,
					  unsigned nBuffer)
			
{
  std::ostringstream os;
  fd_    = -1;
  block_ = block;
  
  // Byte offset in the network buffer will be the header size
  // (2*sizeof(unsigned int)), followed by the byte offset of
  // this register in the frame
  
  byteOffset_ = 
    2*sizeof(unsigned int)
    + arrRegMap->byteOffsetInArcArrayMap()
    + block->byteOffsetInArcRegMapOf()
    + startEl * nEl * block->nBytePerEl();
  
  ptr_ = (void*)(base + byteOffset_);
  
  // Store the total number of bytes in this register
  
  unsigned nBytePrefac = 
    (block->flags_ & (REG_BOOL|REG_CHAR|REG_UCHAR)) ? 2 : 1;

  nByte_ = nBytePrefac * nEl * block->nBytePerEl();

  // nEl_ will only be used if we have to write elements separately
  // (rather than block copy bytes from the data frame).  This will be
  // done only for data types unsupported by the dirfile data format
  // (8-bit types).  
  //
  //  Furthermore, if the data type being written is a date or complex
  //  type, then as far as dirfile format is concerned, there will be
  //  two elements written per frame

  unsigned nElPrefac = (block->flags_ & REG_COMPLEX) ? 2 : 1;
  nEl_ = nElPrefac * nEl;
  
  // A format string for this register
  
  format_ = format();
  
  // And the name of this register
  
  os.str("");
  os << arrRegMap->name << "." << block->brd_->name << "." << block->name_;

  // If writing indices separately, format the index as part of the
  // name too

  if(writeIndex) {
    os << "[" << startEl << "]";
  }
  
  name_ = os.str();

  nBuffer_ = nBuffer;

  iBuffer_ = 0;

  // And resize the byte array

  bytes_.resize(nByte_ * nBuffer);
}

/**.......................................................................
 * Output a dirfile-style format flag for this register
 */
std::string ArchiverWriterDirfile::
Register::format()
{
  std::string format;
  
  // Dirfiles don't support 8-bit types.  Convert these to 16-bit types

  if(block_->flags_ & (REG_BOOL|REG_UCHAR|REG_USHORT))
    format = "u";
  else if(block_->flags_ & (REG_CHAR|REG_SHORT))
    format = "s";

  // Write supported types as is

  else if(block_->flags_ & (REG_UINT))
    format = "U";
  else if(block_->flags_ & (REG_INT))
    format = "S";
  else if(block_->flags_ & (REG_FLOAT))
    format = "f";

  // REG_UTC will get converted to a double mjd in dirfile format

  else if(block_->flags_ & (REG_DOUBLE|REG_UTC))
    format = "d";

  return format;
}

/**.......................................................................
 * Copy constructor for Register
 */
ArchiverWriterDirfile::Register::Register(const Register& reg)
{
  name_       = reg.name_;
  ptr_        = reg.ptr_;
  byteOffset_ = reg.byteOffset_;
  nByte_      = reg.nByte_;
  nEl_        = reg.nEl_;
  fd_         = reg.fd_;
  format_     = reg.format_;
  block_      = reg.block_;
  nBuffer_    = reg.nBuffer_;
  iBuffer_    = reg.iBuffer_;
  bytes_      = reg.bytes_;
}

/**.......................................................................
 * Open a file for this register in the requested directory
 */
int ArchiverWriterDirfile::Register::open(char* dir)
{
  close();

  std::stringstream os;
  os << dir << "/" << name_;

  // Open the file in exclusive create mode (will return error if the
  // file already exists), and with rwx permissions for user, read
  // permissions for others

  if((fd_ = ::open(os.str().c_str(), O_RDWR|O_CREAT|O_EXCL, S_IRWXU|S_IRGRP|S_IROTH))< 0) {
    ReportSysError("open(), while attempting to open file: " << os.str());
    return 1;
  }

  return 0;
}

/**.......................................................................
 * Close any file currently associated with this register
 */
void ArchiverWriterDirfile::Register::close()
{
  if(fd_ > 0)
    ::close(fd_);
  fd_ = -1;
}

/**.......................................................................
 * Flush any file currently associated with this register
 */
void ArchiverWriterDirfile::Register::flush() 
{
  if(iBuffer_ != 0 && fd_ > 0) {
    if(::write(fd_, (void*)&bytes_[0], nByte_ * iBuffer_) != nByte_ * iBuffer_)
      ThrowError("Write failed for register: " << name_);
    iBuffer_ = 0;
  }
}

/**.......................................................................
 * Write the latest value to the file
 */
int ArchiverWriterDirfile::Register::write()
{
  unsigned short us=0;
  unsigned char* uPtr=0;
  signed short ss=0;
  signed char* sPtr=0;
  unsigned nByte=0;
  unsigned* uIntPtr=0;
  double mjd=0;
  static size_t dSize=sizeof(double);
  static size_t sSize=sizeof(short);
  unsigned char* uIter;

  // Store a pointer to the current location in the buffer

  unsigned char* destBase = &bytes_[nByte_ * iBuffer_];

  // If this is an unsigned 8-bit type, convert to an unsigned 16-bit
  // type for the limited dirfile format

  if(block_->flags_ & (REG_BOOL|REG_UCHAR)) {

    uPtr = (unsigned char*)ptr_;
    for(unsigned iEl=0; iEl < nEl_; iEl++) {
      us = (unsigned short)uPtr[iEl];
      memcpy((void*)(destBase+iEl*nByte_), (void*)&us, sSize);
    }
	  
  // Else convert to a signed 16-bit type if this is a signed 8-bit
  // type

  } else if(block_->flags_ & REG_CHAR) {

    sPtr = (signed char*)ptr_;
    for(unsigned iEl=0; iEl < nEl_; iEl++) {
      ss = (signed short)sPtr[iEl];
      memcpy((void*)(destBase+iEl*nByte_), (void*)&ss, sSize);
    }

    // Else if this is a RegDate format convert it to a double mjd

  } else if(block_->flags_ & REG_UTC) {

    // Convert pairs of unsigned ints to double mjd 
    //
    // (day + milliseconds/millisecondsPerDay)

    uIntPtr = (unsigned*)ptr_;
    for(unsigned iEl=0; iEl < nEl_; iEl++) {
      mjd = (double)uIntPtr[2*iEl] + (double)(uIntPtr[2*iEl+1])/(86400*1000);
      memcpy((void*)(destBase+iEl*nByte_), (void*)&mjd, dSize);
    }

    // Else just block-copy the byte array for this register

  } else {
    memcpy((void*)destBase, ptr_, nByte_);
  }

  // If we have reached the buffer count, write to disk

  if(++iBuffer_ == nBuffer_) {
    //    nByte = ::write(fd_, (void*)destBase, nByte_ * nBuffer_);
    nByte = nByte_ * nBuffer_;
    iBuffer_ = 0;

    // Check if the total write count equals the expected byte count
    
    if(nByte != nByte_ * nBuffer_) {
      COUT("Write failed for register: " << name_);
      return 1;
    }
  }

  return 0;
}

