#define __FILEPATH__ "util/common/Test/tArrayDataFrameManager.cc"

#include <iostream>
#include <iomanip>

#include <cmath>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/ArrayDataFrameManager.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  ArrayDataFrameManager arrman1, arrman2;
  short wTest;

  wTest = 24321;
  arrman1.writeReg("array", "test", "preAvgShortReg",  &wTest);
  arrman1.writeReg("array", "test", "postAvgShortReg", &wTest);
  arrman1.writeReg("array", "test", "sumShortReg",     &wTest);
  arrman1.writeReg("array", "test", "lastShortReg",    &wTest);

  wTest = 14321;
  arrman2.writeReg("array", "test", "preAvgShortReg",  &wTest);
  arrman2.writeReg("array", "test", "postAvgShortReg", &wTest);
  arrman2.writeReg("array", "test", "sumShortReg",     &wTest);
  arrman2.writeReg("array", "test", "lastShortReg",    &wTest);

  arrman1 += arrman2;

  short test;

  arrman1.readReg("array", "test", "preAvgShortReg",  &test);
  cout << test << endl;
  arrman1.readReg("array", "test", "postAvgShortReg", &test);
  cout << test << endl;
  arrman1.readReg("array", "test", "sumShortReg",     &test);
  cout << test << endl;
  arrman1.readReg("array", "test", "lastShortReg",    &test);
  cout << test << endl;

  static unsigned char source[13] = "AlpLyr";
  unsigned char testStr[13];

  try {
    cout << "About to test" << endl;
    arrman1.writeReg("antenna0", "tracker", "source", (unsigned char*)source);
    arrman1.readReg("antenna0", "tracker", "source", (unsigned char*)testStr);
    cout << "printing: " << testStr << endl;;
    cout << "Done testing" << endl;
  } catch(Exception& err) {
    cout << err.what() << endl;
  }
  return 0;
}
