#include "gcp/util/common/DirfileWriter.h"

#include "gcp/control/code/unix/libunix_src/common/arraymap.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include<iostream>
#include<fstream>

// Needed for mkdir()

#include <sys/stat.h>
#include <sys/types.h>

// Needed for open()/close()/write()

#include <fcntl.h>
#include <unistd.h>
#include <iomanip>

#include <string.h>

using namespace std;
using namespace gcp::control;
using namespace gcp::util;

#ifdef ThrowError
#undef ThrowError
#endif

#define ThrowError(text) \
{\
  std::ostringstream _macroOs; \
  _macroOs << text;\
  throw DirfileException(_macroOs.str());\
}

#ifdef COUT
#undef COUT
#endif

#define COUT(text) \
{\
  std::cout << text << std::endl;\
}

/**.......................................................................
 * Constructor with no archiver parent
 */
DirfileWriter::DirfileWriter(unsigned nBuffer, bool writeIndex)
{
  initialize(nBuffer, writeIndex);
}

/**.......................................................................
 * Generic initialize method
 */
void DirfileWriter::initialize(unsigned nBuffer, bool writeIndex) 
{
  nBuffer_    = nBuffer;
  writeIndex_ = writeIndex;
  isOpen_     = false;
  maxlen_     = 0;
}

/**.......................................................................
 * Destructor
 */
DirfileWriter::~DirfileWriter() 
{
  flushArcfile();
  closeArcfile();
}

/**.......................................................................
 * Const Copy Constructor.
 */
DirfileWriter::DirfileWriter(const DirfileWriter& objToBeCopied)
{
  *this = (DirfileWriter&)objToBeCopied;
};

/**.......................................................................
 * Copy Constructor.
 */
DirfileWriter::DirfileWriter(DirfileWriter& objToBeCopied)
{
  *this = objToBeCopied;
};

/**.......................................................................
 * Const Assignment Operator.
 */
void DirfileWriter::operator=(const DirfileWriter& objToBeAssigned)
{
  *this = (DirfileWriter&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void DirfileWriter::operator=(DirfileWriter& objToBeAssigned)
{
  std::cout << "Warning: calling default assignment operator for class: "
    "DirfileWriter" << std::endl;
};

/**.......................................................................
 * Open dir files for all archived registers
 */
int DirfileWriter::openArcfile(std::string dirname)
{
  int waserr=0;

  COUT("Inside openArcfile");

  // Close the current archive file if open.

  closeArcfile();

  // For dir files, the path is a new directory that must be created:

  COUT("About to create dir");

  if(createDir((char*)dirname.c_str()))
    return 1;

  COUT("Done creating dir");

  // Write out the format file

  if(outputFormatFile((char*)dirname.c_str()))
    return 1;

  // Attempt to open the new data files.

  for(std::list<Register>::iterator iReg=registers_.begin();
      iReg != registers_.end(); iReg++)
    waserr |= iReg->open((char*)dirname.c_str());
  
  // Return error if an error occurred opening any of the files

  if(waserr) 
    return 1;

  isOpen_ = true;

  COUT("Leaving arcfile");

  return 0;
}

/**.......................................................................
 * Create the named directory
 */
int DirfileWriter::createDir(char* path)
{
  return (mkdir(path, 0755) < 0);
}

/**.......................................................................
 * Close all open files
 */
void DirfileWriter::closeArcfile() 
{
  for(std::list<Register>::iterator iReg=registers_.begin();
      iReg != registers_.end(); iReg++)
    iReg->close();

  isOpen_ = false;
}

/**.......................................................................
 * Flush all open files
 */
void DirfileWriter::flushArcfile()
{
  for(std::list<Register>::iterator iReg=registers_.begin();
      iReg != registers_.end(); iReg++)
    iReg->flush();
}

/**.......................................................................
 * Write the latest integration
 */
int DirfileWriter::writeIntegration() 
{
  int waserr=0;
  static int counter=0;

  COUT("Writing integration: " << counter);
  for(std::list<Register>::iterator iReg=registers_.begin();
      iReg != registers_.end(); iReg++) {
    waserr |= iReg->write();
  }

  ++counter;
  return waserr;
}

/**.......................................................................
 * Return true if an archive "file" is currently open
 */
bool DirfileWriter::isOpen()
{
  return isOpen_;
}

/**.......................................................................
 * Output the dirfile format file needed by things like KST
 */
int DirfileWriter::outputFormatFile(char* dir)
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

/**.......................................................................
 * Insert a block of register elements into the list
 */
void DirfileWriter::insertReg(std::string name, unsigned char* base,
			      unsigned startEl, unsigned nEl, 
			      Register::Type type)
{
  Register reg(name, base, startEl, nEl, type, writeIndex_, nBuffer_);
  
  // Insert the new element in the list
  
  registers_.insert(registers_.end(), reg);
  
  // Increment the maximum name length
  
  maxlen_ = (reg.name_.size() > maxlen_) ? reg.name_.size() : maxlen_;
}

/**.......................................................................
 * Add an arbitrary type of register to the list of registers
 * maintained by this object
 */
void DirfileWriter::addRegister(std::string name, unsigned char* base, 
				Register::Type type,
				unsigned startEl, unsigned nEl)
{
  insertReg(name, base, startEl, nEl, type);
}

/**.......................................................................
 * Add a floating-point register to the list of registers this
 * object will maintain
 */
void DirfileWriter::addFloatRegister(std::string name, unsigned char* base, 
				     unsigned startEl, unsigned nEl)
{
  insertReg(name, base, startEl, nEl, Register::FLOAT);
}

//-----------------------------------------------------------------------
// Methods of Register class
//-----------------------------------------------------------------------

/**.......................................................................
 * Constructor for Register
 */
DirfileWriter::Register::Register(std::string name,
				  unsigned char* base,
				  unsigned startEl,
				  unsigned nEl,
				  Type type,
				  bool writeIndex,
				  unsigned nBuffer)
{
  // Initialize the file descriptor

  fd_ = -1;
  
  // Calculate the byte offset from the head of the data frame
  
  unsigned nBytePerEl = sizeOf(type);
  unsigned byteOffset = startEl * nEl * nBytePerEl;

  ptr_ = (void*)(base + byteOffset);
  
  // Store the total number of bytes in this register
  
  unsigned nBytePrefac = (type & (BOOL|CHAR|UCHAR)) ? 2 : 1;

  nByte_      = nBytePrefac * nEl * nBytePerEl;
  nBytePerEl_ = nBytePerEl;
  type_       = type; 

  // nEl_ will only be used if we have to write elements separately
  // (rather than block copy bytes from the data frame).  This will be
  // done only for data types unsupported by the dirfile data format
  // (8-bit types).  
  //
  //  Furthermore, if the data type being written is a date or complex
  //  type, then as far as dirfile format is concerned, there will be
  //  two elements written per frame

  unsigned nElPrefac = (type_ & COMPLEX) ? 2 : 1;
  nEl_ = nElPrefac * nEl;
  
  // A format string for this register
  
  format_ = format();
  
  // And the name of this register
  
  std::ostringstream os;
  os.str("");
  os << name;

  // If writing indices separately, format the index as part of the
  // name too

  if(writeIndex) {
    os << startEl;
  }
  
  name_    = os.str();

  nBuffer_ = nBuffer;
  iBuffer_ = 0;

  // And resize the byte array

  bytes_.resize(nByte_ * nBuffer);
}

/**.......................................................................
 * Output a dirfile-style format flag for this register
 */
std::string DirfileWriter::
Register::format()
{
  std::string format;
  
  // Dirfiles don't support 8-bit types.  Convert these to 16-bit types

  if(type_ & (BOOL|UCHAR|USHORT))
    format = "u";
  else if(type_ & (CHAR|SHORT))
    format = "s";

  // Write supported types as is

  else if(type_ & (UINT))
    format = "U";
  else if(type_ & (INT))
    format = "S";
  else if(type_ & (FLOAT))
    format = "f";
  else if(type_ & (DOUBLE))
    format = "d";

  return format;
}

/**.......................................................................
 * Copy constructor for Register
 */
DirfileWriter::Register::
Register(const Register& reg)
{
  name_       = reg.name_;
  ptr_        = reg.ptr_;
  nByte_      = reg.nByte_;
  nBytePerEl_ = reg.nBytePerEl_;
  nEl_        = reg.nEl_;
  fd_         = reg.fd_;
  nBuffer_    = reg.nBuffer_;
  iBuffer_    = reg.iBuffer_;
  type_       = reg.type_;
  format_     = reg.format_;
  bytes_      = reg.bytes_;
}

/**.......................................................................
 * Destructor for Register
 */
DirfileWriter::Register::
~Register()
{
  flush();
  close();
}

/**.......................................................................
 * Open a file for this register in the requested directory
 */
int DirfileWriter::Register::
open(char* dir)
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
void DirfileWriter::Register::
close()
{
  if(fd_ > 0)
    ::close(fd_);
  fd_ = -1;
}

/**.......................................................................
 * Flush any file currently associated with this register
 */
void DirfileWriter::Register::
flush() 
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
int DirfileWriter::Register::
write()
{
  unsigned short us=0;
  unsigned char* uPtr=0;
  signed short ss=0;
  signed char* sPtr=0;
  int nByte=0;
  unsigned* uIntPtr=0;
  double mjd=0;
  static size_t dSize=sizeof(double);
  static size_t sSize=sizeof(short);
  unsigned char* uIter;

  // Store a pointer to the current location in the buffer

  unsigned char* destBase = &bytes_[nByte_ * iBuffer_];

  // If this is an unsigned 8-bit type, convert to an unsigned 16-bit
  // type for the limited dirfile format

  if(type_ & (BOOL|UCHAR)) {
    uPtr = (unsigned char*)ptr_;
    for(unsigned iEl=0; iEl < nEl_; iEl++) {
      us = (unsigned short)uPtr[iEl];
      memcpy((void*)(destBase+iEl*nBytePerEl_), (void*)&us, sSize);
    }
	  
  // Else convert to a signed 16-bit type if this is a signed 8-bit
  // type

  } else if(type_ & CHAR) {
    sPtr = (signed char*)ptr_;
    for(unsigned iEl=0; iEl < nEl_; iEl++) {
      ss = (signed short)sPtr[iEl];
      memcpy((void*)(destBase+iEl*nBytePerEl_), (void*)&ss, sSize);
    }

    // Else just block-copy the byte array for this register

  } else {
    memcpy((void*)destBase, ptr_, nByte_);
  }

  // If we have reached the buffer count, write to disk

  if(++iBuffer_ == nBuffer_) {

#if 1
    if((nByte = ::write(fd_, (void*)&bytes_[0], nByte_ * nBuffer_)) < 0)
      ReportSysError("write(), while attempting to write to file: " << name_
		     << " fd = " << fd_);
#else
    nByte = nByte_ * nBuffer_;
#endif

    iBuffer_ = 0;

    // Check if the total write count equals the expected byte count
    
    if(nByte != nByte_ * nBuffer_) {
      COUT("Write failed for register: " << name_ << "nByte = " << nByte);
      return 1;
    }

  }

  return 0;
}

/**.......................................................................
 * Return the size, in bytes, of the requested type
 */
unsigned DirfileWriter::Register::
sizeOf(DirfileWriter::Register::Type type)
{
  switch(type) {
  case BOOL:
    return sizeof(bool);
    break;
  case UCHAR:
    return sizeof(unsigned char);
    break;
  case CHAR:
    return sizeof(char);
    break;
  case USHORT:
    return sizeof(unsigned short);
    break;
  case SHORT:
    return sizeof(short);
    break;
  case UINT:
    return sizeof(unsigned int);
    break;
  case INT:
    return sizeof(int);
    break;
  case ULONG:
    return sizeof(unsigned long);
    break;
  case LONG:
    return sizeof(long);
    break;
  case FLOAT:
    return sizeof(float);
    break;
  case DOUBLE:
    return sizeof(double);
    break;
  default:
    return 0;
    break;
  }
}
