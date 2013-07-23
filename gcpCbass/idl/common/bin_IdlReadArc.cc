#include <iostream>  

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Monitor.h"
#include "gcp/util/common/RegDescription.h"

#include "gcp/idl/common/IdlParser.h"
#include "gcp/idl/common/IdlHandler.h"

#include "idl_export.h"  

using namespace gcp::idl;
using namespace gcp::util;

// Macro to assign values from a real C array (double* dPtr) to a
// matlab array (MexHandler::MxArray* array).  Matlab arrays index
// backwards from C arrays; i.e., when encoding a multi-dimensional
// array into 1-dimension, the index which changes most rapidly in C
// is the slowest changing inde in matlab

#define ASSIGN_VALS(cast) \
  {\
    cast* ptr = (cast*)array;\
    switch(axes.nAxis()) {\
    case 2:\
      {\
        unsigned nEl0 = axes.nEl(0);\
        unsigned nEl1 = axes.nEl(1);\
        unsigned iM, iC;\
        for(unsigned iEl1=0; iEl1 < nEl1; iEl1++)\
          for(unsigned iEl0=0; iEl0 < nEl0; iEl0++) {\
            iC = iEl0*nEl1 + iEl1;\
            iM = iEl1*nFrame*nEl0 + iEl0*nFrame + iFrame;\
            *(ptr + iM) = (cast)*(dPtr + iC);\
          }\
      }\
      break;\
    case 1:\
      {\
        unsigned nEl0 = axes.nEl(0);\
        unsigned iM, iC;\
        for(unsigned iEl0=0; iEl0 < nEl0; iEl0++) {\
          iC = iEl0;\
          iM = iEl0 * nFrame + iFrame;\
          *(ptr + iM) = (cast)*(dPtr + iC);\
        }\
      }\
      break;\
    default:\
      break;\
   }\
  }

class MyClass {
public:
  void printSomething() {
    std::cout << "Hello from MyClass!" << std::endl;
  }
};

/**.......................................................................
 * Count the number of frames in the monitor stream
 */
unsigned countFrames(IDL_VPTR* argv,
		     std::vector<gcp::util::RegDescription>& regs,
		     std::vector<gcp::util::MonitorDataType::FormatType>& formats);

/**.......................................................................
 * Create the hierarchical board/register structure to be returned to
 * the IDL environment
 */
IDL_VPTR createOutputValueStructure(IDL_VPTR* argv,
				    std::vector<gcp::util::RegDescription>& regs,
				    std::vector<gcp::util::MonitorDataType::FormatType>& formats,
				    unsigned nFrame,
				    std::vector<char*>& data);

/**.......................................................................
 * Create empty matlab arrays to be filled in by the monitor stream
 */
void createRegValArray(IdlStructDef* ptr, 
		       RegDescription& reg,
		       MonitorDataType::FormatType formatType,
		       unsigned nFrame);

// Read frames from the monitor stream

void readFrames(IDL_VPTR* argv, std::vector<char*>& data, unsigned nFrame); 

// Each time the register changes during a read, we must call this
// function to update register information

void cacheRegInfo(Monitor* monitor, std::vector<RegDescription>& regs, 
		  std::vector<int>& startSlots, 
		  std::vector<gcp::util::MonitorDataType::FormatType>& nativeFormats, 
		  std::vector<gcp::util::MonitorDataType::FormatType>& selectedFormats, 
		  double** slotPtr);

// Assign values returned from the monitor stream into the
// corresponding matlab array

void assignVals(char* array,
		RegDescription& reg,
		MonitorDataType::FormatType nativeFormat,
		MonitorDataType::FormatType selectedFormat,
		double* dPtr,
		unsigned iFrame,
		unsigned nFrame);

// Set a string value in an IDL array

void assignStringVals(char* array, double* dPtr, unsigned len, unsigned iFrame);

// Convert a date to a string value in a matlab cell array

void assignDateVals(char* array, double* dPtr, 
		    MonitorDataType::FormatType format, unsigned iFrame);

/* Define message codes and their corresponding printf(3) format  
 * strings. Note that message codes start at zero and each one is  
 * one less that the previous one. Codes must be monotonic and  
 * contiguous. */  
static IDL_MSG_DEF msg_arr[] = {  
#define M_TM_INPRO                       0  
  {  "M_TM_INPRO",   "%NThis is from a loadable module procedure." },  
#define M_TM_INFUN                       -1    
  {  "M_TM_INFUN",   "%NThis is from a loadable module function." },  
};  

/* The load function fills in this message block handle with the  
 * opaque handle to the message block used for this module. The other  
 * routines can then use it to throw errors from this block. */  
static IDL_MSG_BLOCK msg_block;  

/**.......................................................................
 * Count the number of frames in the monitor stream
 */
unsigned countFrames(IDL_VPTR* argv,
		     std::vector<gcp::util::RegDescription>& regs,
		     std::vector<gcp::util::MonitorDataType::FormatType>& formats)
  
{
  std::string directory;    // The directory in which to look for files 
  std::string calfile;      // The name of the calibration file 
  std::string start_date;   // Start time as date:time string 
  std::string end_date;     // End time as date:time string 
  
  // How many registers requested?
  
  IdlParser regNames(argv[0]);
  unsigned nreg = regNames.getNumberOfElements();
  
  // Get start/end date:time strings 
  
  IdlParser startDate(argv[1]);
  start_date = startDate.getString();
  
  IdlParser endDate(argv[2]);
  end_date = endDate.getString();
  
  // Get arcdir and calfile locations 
  
  IdlParser dir(argv[3]);
  directory  = dir.getString();
  
  IdlParser cal(argv[4]);
  calfile    = cal.getString();
  
  Monitor monitor((char*)directory.c_str(),  (char*)calfile.c_str(), 
		  (char*)start_date.c_str(), (char*)end_date.c_str());
  
  // Always add the UTC register
  
  //  monitor.addRegister("array.frame.utc double");
  
  // And add the user-requested regs
  
  for(unsigned iReg=0; iReg < nreg; iReg++) {
    std::string reg = regNames.getString(iReg);
    if(reg[0] != '\0')
      monitor.addRegister(reg);
  }
  
  // Return the vector of register descriptions
  
  unsigned nFrame = monitor.countFrames();
  
  regs    = monitor.selectedRegs();
  formats = monitor.selectedFormats();
  
  return nFrame;
}

/**.......................................................................
 * Create the hierarchical board/register structure to be returned to
 * the IDL environment
 */
IDL_VPTR createOutputValueStructure(IDL_VPTR* argv,
				    std::vector<gcp::util::RegDescription>& regs,
				    std::vector<gcp::util::MonitorDataType::FormatType>& formats,
				    unsigned nFrame,
				    std::vector<char*>& data)
{
  // Create an empty arraymap structure 
  
  IdlStructDef arrMap;
  
  // Get the number of registers requested?
  
  unsigned nreg = regs.size();
  
  // For each register, add a register field
  
  for(unsigned iReg=0; iReg < regs.size(); iReg++) {
    IdlStructDef* regMap = arrMap.getStructMember(regs[iReg].regMapName(), true);
    IdlStructDef* board  = regMap->getStructMember(regs[iReg].boardName(), true);
    createRegValArray(board, regs[iReg], formats[iReg], nFrame);
  }
  
  IDL_VPTR v = IdlHandler::createStructure(arrMap);
  
  // Now that the structure has been created, we can assign pointers
  // to the data
  
  IdlParser parser(v);
  
  data.resize(regs.size());
  for(unsigned iReg=0; iReg < regs.size(); iReg++) {
    IdlParser regMap = parser.getTag(regs[iReg].regMapName());
    IdlParser board  = regMap.getTag(regs[iReg].boardName());
    data[iReg] = board.getPtrToDataForTag(regs[iReg].blockName());
  }
  
  return v;
}

/**.......................................................................
 * Create empty matlab arrays to be filled in by the monitor stream
 */
void createRegValArray(IdlStructDef* ptr, 
		       RegDescription& reg,
		       MonitorDataType::FormatType formatType,
		       unsigned nFrame)
{
  std::vector<unsigned> dims;
  CoordAxes axes = reg.axes();
  
  // Create and return the array in which values will be stored. We
  // want to format the array to have the same number of dimensions as
  // the C array.
  
  if(nFrame > 0)
    dims.push_back(nFrame);
  
  if(formatType != MonitorDataType::FM_STRING) {
    unsigned nDim = axes.nAxis();
    for(unsigned iDim=0; iDim < nDim; iDim++)
      dims.push_back(axes.nEl(iDim));
    
  } else {
    dims.push_back(1);
  }
  
  ptr->addDataMember(reg.blockName(), IdlHandler::idlTypeOf(formatType), dims);
}

/**.......................................................................
 * Read frames from the monitor stream
 */
void readFrames(IDL_VPTR* argv, std::vector<char*>& data, unsigned nFrame) 
{
  std::string directory;    // The directory in which to look for files 
  std::string calfile;      // The name of the calibration file 
  std::string start_date;   // Start time as date:time string 
  std::string end_date;     // End time as date:time string 
  
  // How many registers requested?
  
  IdlParser regNames(argv[0]);
  unsigned nreg = regNames.getNumberOfElements();
  
  // Get start/end date:time strings 
  
  IdlParser startDate(argv[1]);
  start_date = startDate.getString();
  
  IdlParser endDate(argv[2]);
  end_date = endDate.getString();
  
  // Get arcdir and calfile locations 
  
  IdlParser dir(argv[3]);
  directory  = dir.getString();
  
  IdlParser cal(argv[4]);
  calfile    = cal.getString();
  
  Monitor monitor((char*)directory.c_str(),  (char*)calfile.c_str(), 
		  (char*)start_date.c_str(), (char*)end_date.c_str());
  
  // Always add the UTC register
  
  //  monitor.addRegister("array.frame.utc double");
  
  // And add the user-requested regs
  
  for(unsigned iReg=0; iReg < nreg; iReg++) 
    monitor.addRegister(regNames.getString(iReg));
  
  monitor.reinitialize();
  
  std::vector<gcp::util::RegDescription> regs;
  std::vector<gcp::util::MonitorDataType::FormatType> nativeFormats;
  std::vector<gcp::util::MonitorDataType::FormatType> selectedFormats;
  std::vector<int> startSlots;
  double* slotPtr=0;
  unsigned iFrame=0;
  
  gcp::control::MsReadState state;
  
  cacheRegInfo(&monitor, regs, startSlots, nativeFormats, selectedFormats, 
	       &slotPtr);
  
  // Loop, reading frames
  
  while((state=monitor.readNextFrame()) != gcp::control::MS_READ_ENDED) {
    
    // If the register map changed, re-cache the register information
    
    if(state==gcp::control::MS_READ_REGMAP) {
      cacheRegInfo(&monitor, regs, startSlots, nativeFormats, selectedFormats, &slotPtr);
    }
    
    // Loop over all requested registers from this frame
    
    if(state == gcp::control::MS_READ_DONE) {
      for(unsigned iReg=0; iReg < regs.size(); iReg++) {
	assignVals(data[iReg], regs[iReg], nativeFormats[iReg], selectedFormats[iReg],
		   (slotPtr+startSlots[iReg]), iFrame, nFrame);
      }

      //      COUT("Read frame: " << iFrame);

      ++iFrame;
    }
  }
}

/**.......................................................................
 * Read the array map from the monitor stream
 */
static IDL_VPTR readArrayMap(int argc, IDL_VPTR* argv)
{
  IDL_VPTR v=0;

  try {

    std::string directory;    // The directory in which to look for files 
    std::string calfile;      // The name of the calibration file 
    std::string start_date;   // Start time as date:time string 
    std::string end_date;     // End time as date:time string 
    
    // Get start/end date:time strings 
    
    IdlParser startDate(argv[0]);
    start_date = startDate.getString();
    
    IdlParser endDate(argv[1]);
    end_date = endDate.getString();
    
    // Get arcdir and calfile locations 
    
    IdlParser dir(argv[2]);
    directory  = dir.getString();
    
    // Default the cal file to none, since we aren't actually reading
    // data here
    
    calfile    = "/dev/null";
    
    Monitor monitor((char*)directory.c_str(),  (char*)calfile.c_str(), 
		    (char*)start_date.c_str(), (char*)end_date.c_str());
    
    // Always add the UTC register
    
    monitor.reinitialize();
    gcp::control::MsReadState state;
    
    // After the first successful read, we should have the array map
    
    if((state=monitor.readNextFrame()) != gcp::control::MS_READ_ENDED) {
      
      // Now return a vector of all registers in the array map
      
      std::vector<std::string> allRegs = monitor.getArrayMapRegisters(true);

      // Add them to the monitor object as if we were going to read them
      // -- this is so we can extract the formats below

      for(unsigned i=0; i < allRegs.size(); i++) {
	monitor.addRegister(allRegs[i]);
      }
      
      // Now get the selected reg names and formats
      
      monitor.reinitialize();

      std::vector<gcp::util::RegDescription> regs = monitor.selectedRegs();
      std::vector<gcp::util::MonitorDataType::FormatType> formats = monitor.selectedFormats();
      
      // Create an empty arraymap structure 
      
      IdlStructDef arrMap;
      
      for(unsigned iReg=0; iReg < regs.size(); iReg++) {
	IdlStructDef* regMap = arrMap.getStructMember(regs[iReg].regMapName(), true);
	IdlStructDef* board  = regMap->getStructMember(regs[iReg].boardName(), true);
	createRegValArray(board, regs[iReg], formats[iReg], 0);
      }
      
      v = IdlHandler::createStructure(arrMap);
      
    } else {
      ThrowError("Unable to read array map from the archive file");
    }
    
  } catch(Exception& err) {
    std::ostringstream os;

    os << err.what() << std::endl << std::endl;
    os << "Usage: d = readarraymap(startDate('dd-mmm-yyyy:hh:mm:ss'), stopDate('dd-mmm-yyyy:hh:mm:ss'), arcdir(string))";
    os << std::endl << std::endl;

    IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_LONGJMP, os.str().c_str());

  } catch(...) {
    IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_LONGJMP, "Caught an unknown error");
  }

  return v;
}

/**.......................................................................
 * Each time the register changes during a read, we must call this
 * function to update register information
 */
void cacheRegInfo(Monitor* monitor, std::vector<RegDescription>& regs, 
		  std::vector<int>& startSlots, 
		  std::vector<gcp::util::MonitorDataType::FormatType>& nativeFormats, 
		  std::vector<gcp::util::MonitorDataType::FormatType>& selectedFormats, 
		  double** slotPtr)
{
  // Cache the register information
  
  regs            = monitor->selectedRegs();
  nativeFormats   = monitor->nativeFormats();
  selectedFormats = monitor->selectedFormats();
  
  startSlots.resize(regs.size());
  
  for(unsigned iReg=0; iReg < regs.size(); iReg++) 
    startSlots[iReg] = regs[iReg].startSlot();
  
  *slotPtr = monitor->getCalSlotPtr();
}

/**.......................................................................
 * Assign values returned from the monitor stream into the
 * corresponding matlab array
 */
void assignVals(char* array,
		RegDescription& reg,
		MonitorDataType::FormatType nativeFormat,
		MonitorDataType::FormatType selectedFormat,
		double* dPtr,
		unsigned iFrame,
		unsigned nFrame)
{
  CoordAxes& axes = reg.axes_;
  
  if(selectedFormat == MonitorDataType::FM_STRING) {
    assignStringVals(array, dPtr, reg.nEl(), iFrame);
  } else if(nativeFormat == MonitorDataType::FM_DATE) {
    assignDateVals(array, dPtr, selectedFormat, iFrame);
  } else {
    
    switch (selectedFormat) {
      
    case MonitorDataType::FM_BOOL:
    case MonitorDataType::FM_UCHAR:
      ASSIGN_VALS(unsigned char);
      break;
    case MonitorDataType::FM_CHAR:
      ASSIGN_VALS(char);
      break;
    case MonitorDataType::FM_USHORT:
      ASSIGN_VALS(unsigned short);
      break;
    case MonitorDataType::FM_SHORT:
      ASSIGN_VALS(short);
      break;
    case MonitorDataType::FM_UINT:
      ASSIGN_VALS(unsigned int);
      break;
    case MonitorDataType::FM_INT:
      ASSIGN_VALS(int);
      break;
    case MonitorDataType::FM_ULONG:
      ASSIGN_VALS(unsigned long);
      break;
    case MonitorDataType::FM_LONG:
      ASSIGN_VALS(long);
      break;
    case MonitorDataType::FM_FLOAT:
      ASSIGN_VALS(float);
      break;
    case MonitorDataType::FM_DOUBLE:
      ASSIGN_VALS(double);
      break;
    }
  }
}

/**.......................................................................
 * Set a string value in an IDL array
 */
void assignStringVals(char* array, double* dPtr, unsigned len, unsigned iFrame)
{
  static std::ostringstream os;
  
  os.str("");
  for(unsigned i=0; i < len; i++)
    os << (char)*(dPtr+i);
  
  IDL_STRING* s = (IDL_STRING*)array + iFrame;
  IDL_StrStore(s, (char*)os.str().c_str());
}

/**.......................................................................
 * Convert a date to a string value in a matlab cell array
 */
void assignDateVals(char* array, double* dPtr, 
		    MonitorDataType::FormatType format, unsigned iFrame)
{
  RegDate date(*((RegDate::Data*)dPtr));
  static int count=0;
  
  ++count;
  if(count % 100 == 0)
    COUT("Reading: " << date);
  
  if(format == MonitorDataType::FM_DATE) {
    IDL_StrStore((IDL_STRING*)array+iFrame, (char*)date.str().c_str());
  } else {
    *((double*)array + iFrame) = date.mjd();
  }
}

static IDL_VPTR readArc(int argc, IDL_VPTR *argv)  
{
  // Count how many frames are in the requested monitor stream
  
  IDL_VPTR v=0;
  
  try {
    
    // Read register values now
    
    std::vector<gcp::util::RegDescription> regs;
    std::vector<gcp::util::MonitorDataType::FormatType> formats;
    std::vector<char*> data;
    
    unsigned nFrame = countFrames(argv, regs, formats);
    
    // Create the hierarchical IDL structure corresponding to the arraymap
    
    v = createOutputValueStructure(argv, regs, formats, nFrame, data);

    // Read frames
    
    readFrames(argv, data, nFrame);
    
  } catch(Exception& err) {
    std::ostringstream os;

    os << err.what() << std::endl << std::endl;
    os << "Usage: d = readarc(registers(string array), startDate('dd-mmm-yyyy:hh:mm:ss'), stopDate('dd-mmm-yyyy:hh:mm:ss'), arcdir(string)', calfile(string))";
    os << std::endl << std::endl;

    IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_LONGJMP, os.str().c_str());

  } catch(...) {
    IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_LONGJMP, "Caught an unknown error");
  }
  
  return v;
}   


