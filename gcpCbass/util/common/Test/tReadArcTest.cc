#include <iomanip>
#include <iostream>
#include <fstream>

#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/ArchiveReader.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/String.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "start",    "18-jul-2011",                     "s", "Start UTC"},
  { "stop",     "20-jul-2011",                     "s", "Start UTC"},
  { "arcdir",   "/mnt/data/cbass/arc/", "s", "Archive directory"},
  { "calfile",  "/dev/null",                       "s", "Cal file to apply"},
  { "regfile",  "regs.txt",                        "s", "File describing which registers to read"},
  { "mp",       "",                                "s", "Register, or semicolon-separated list of registers, to read, instead of file"},
  { "carma",    "f",                               "b", "File describing which registers to read"},
  { END_OF_KEYWORDS}
};

void readRegistersFromList(std::string mpList, ArchiveReader& reader);
void readRegistersFromFile(std::string fileName, ArchiveReader& reader);

void readRegistersFromFile(std::string fileName, ArchiveReader& reader)
{
  std::ifstream ifStr;

  ifStr.open(fileName.c_str());

  ifStr.clear();
  
  if(!ifStr.is_open())
    ThrowSimpleError("Couldn't open file: " << fileName);

  std::string line;

  while(!ifStr.eof()) {
    getline(ifStr, line);

    // Check the line, and check for comments (lines beginning with '%')

    if(line.size() > 0 && line[0] != '%') {
      reader.addRegister(line);
    }
  }

  ifStr.close();
}

void readRegistersFromList(std::string mpList, ArchiveReader& reader)
{
  String mpListStr(mpList);
  String mpSpec;

  while(!mpListStr.atEnd()) {
      
    mpSpec = mpListStr.findNextInstanceOf("", false, ";", false, true);

    if(!mpSpec.isEmpty()) {
      reader.addRegister(mpSpec.str());
    }
	
  };
  
  reader.addRegister(mpSpec.str());
}

void printCarmaHeader(ArchiveReader& reader, unsigned nFrame)
{
  COUT("Estimated number of rows = " << nFrame);
  COUT("");
  COUT("Comparing the following Monitor Point value:");
  COUT(std::setw(80) << "Monitor Point Canonical Name" << std::setw(4) << "ID");

  std::vector<RegDescription>&  regs = reader.getRegDescs();

  std::ostringstream os;
  for(unsigned iReg=1; iReg < regs.size(); iReg++) {
    RegDescription& reg = regs[iReg];
    os.str("");
    os << reg.regMapName() << "." << reg.boardName() << "." << reg.blockName();
    COUT(std::setw(80) << os.str() << std::setw(4) << iReg);
  }
  
  COUT("");
  COUT("");

  os.str("");
  os << std::setw(25) << "Date/Time (UTC)";
  for(unsigned iReg=1; iReg < regs.size(); iReg++) {
    os << std::setw(12) << "integVal";
  }

  COUT(os.str());
}

int Program::main(void)
{
  sleep(10);

  try {

    //-----------------------------------------------------------------------
    // Intialize the archive reader
    //-----------------------------------------------------------------------

    std::string directory;  // The directory in which to look for files 
    std::string calfile;    // The name of the calibration file 
    std::string start_date; // Start time as date:time string 
    std::string end_date;   // End time as date:time string 

    // Get start/end date:time strings 
  
    start_date = Program::getParameter("start");
    end_date   = Program::getParameter("stop");
    directory  = Program::getParameter("arcdir");
    calfile    = Program::getParameter("calfile");

    bool carma = Program::getbParameter("carma");

    for(unsigned i=1; i < 100; i++) {

      COUT("Iteration " << i);

      ArchiveReader reader(directory, calfile, start_date, end_date, false, false, true);

      reader.getFileList();

      //-----------------------------------------------------------------------
      // Add default registers, plus any registers the user requested
      //-----------------------------------------------------------------------
  
      // And add the user-requested regs

      if(!Program::isDefault("mp")) {
	readRegistersFromList(Program::getParameter("mp"), reader);
      } else {
	readRegistersFromFile(Program::getParameter("regfile"), reader);
      }

      // Add other registers here...

      unsigned nFrame = reader.countFrames();

      COUT("nFrame = " << nFrame);

      reader.resetToBeginning();

      std::ostringstream os;

      while(reader.readNextFrame()) {
	//	os.str("");
	//	reader.printRegs(os);
	//	COUT(os.str());
      }
    }

    sleep(10);

  } catch(Exception& err) {
    COUT(err.what());
  } catch(...) {
    COUT("Caught an unknown error");
  }

}

