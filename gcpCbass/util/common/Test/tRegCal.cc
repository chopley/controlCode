#define __FILEPATH__ "util/common/Test/tRegCal.cc"

#include <iostream>
#include <vector>
#include <iomanip>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/RegCal.h"
#include "gcp/util/common/RegDate.h"
#include "gcp/util/common/AxisRange.h"
#include "gcp/util/common/RegDescription.h"
#include "gcp/util/common/RegParser.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "reg",         "",  "s", "Register to check"},
  { "dir",         "",  "s", "Cal file directory"},
  { "name",        "",  "s", "Cal file name"},
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel(Program::getiParameter("debuglevel"));
  
  RegCal cal;

  cal.loadCalFile(Program::getParameter("dir"), Program::getParameter("name"));

  // Read a register, and print its cal factors

  RegParser parser;
  
  std::vector<RegDescription> regs = parser.inputRegs(Program::getParameter("reg"));

  // Print out the bytes ranges:

  std::vector<Range<unsigned> > ranges = regs[0].getByteRanges();
  for(unsigned iRange=0; iRange < ranges.size(); iRange++)
    cout << "[" << ranges[iRange].start() << "-"
	 << ranges[iRange].stop() << "]" << endl;

  ranges = regs[0].getSlotRanges();
  for(unsigned iRange=0; iRange < ranges.size(); iRange++)
    cout << "[" << ranges[iRange].start() << "-"
	 << ranges[iRange].stop() << "]" << endl;
      
  cal.printCalFactors(regs);

  // Now create a data frame

  ArrayDataFrameManager arrayman(true);
  double dW[2], dR[2];
  float  fW[2], fR[2];
  dW[0] = 1.234; dW[1] = 5.678;
  arrayman.writeReg("antenna0", "test", "d1Reg", dW);

  arrayman.readReg("antenna0", "test", "d1Reg", dR);
  cout << dR[0] << ", " << dR[1] << endl;

  fW[0] = 4.4; fW[1] = 6.6;

  CoordRange range0(0, 0), range1(1,1), range01(0,1);

  cout << range0 << endl;
  cout << range1 << endl;

  Complex<float> wVal(4.4, 6.6), rVal;

  arrayman.writeReg("band1", "corr", "usbAvg", wVal.data(), &range1);
  arrayman.writeReg("band1", "corr", "usbAvg", wVal.data(), &range0);
  arrayman.readReg("band1", "corr", "usbAvg", rVal.data(), &range0);
  cout << rVal << endl;

  arrayman.readReg("band1", "corr", "usbAvg", rVal.data(), &range1);
  cout << rVal << endl;
  cout << range1 << endl;

  RegisterSet regSet;
  regSet.addRegisters(regs);

  // Test writing a Date to the data frame

  gcp::util::RegDate date(56007, 24356), newDate(0,0);
  cout << endl << "Writing date: " << endl << endl;
  cout << date << endl;
  arrayman.writeReg("array", "frame", "utc", date.data(), &range0);

  // And read it back

  arrayman.readReg("array", "frame", "utc", newDate.data(), &range0);
  cout << endl << "Reading date: " << endl << endl;
  cout << newDate << endl;

  //------------------------------------------------------------
  // Test calibrating data now
  //------------------------------------------------------------

  // Create regs for a date and a complex float, and add them to the
  // set to be calibrated

  RegDescription dReg;
  dReg.setTo("array", "frame", "utc", REG_PLAIN, REG_INT_PLAIN);
  cout << "Date descriptor range is: " << dReg.range() << endl;
  regSet.addRegister(dReg);

  RegDescription cReg;
  cReg.setTo("band1", "corr", "usbAvg", REG_PLAIN, REG_INT_PLAIN, range01);
  cout << range0 << endl;
  cout << "Complex descriptor range is: " << cReg.range() << endl;
  regSet.addRegister(cReg);

  // Now calibrate the data frame

  cal.calibrateRegData(regSet, arrayman);

  // Test reading the date back

  gcp::util::RegDate tDate(0,0);
  cout << endl << "Date before reading calibrated data: " << endl << endl;
  cout << tDate << endl;

  cal.calData()->getCalDate(&dReg, tDate.data());

  cout << endl << "Date after reading calibrated data: " << endl << endl;
  cout << tDate << endl;

  // Test reading a complex number back

  Complex<float> newCfVal;
  cout << endl << "Complex before reading calibrated data: " << endl << endl;
  cout << newCfVal << endl;
  cout << range1 << endl;
  cal.calData()->getCalComplexFloat(&cReg, newCfVal.data(), &range1);

  cout << endl << "Complex[1] after reading calibrated data: " << endl << endl;
  cout << newCfVal << endl;

  cal.calData()->getCalComplexFloat(&cReg, newCfVal.data(), &range0);

  cout << endl << "Complex[0] after reading calibrated data: " << endl << endl;
  cout << newCfVal << endl;

  return 0;
}
