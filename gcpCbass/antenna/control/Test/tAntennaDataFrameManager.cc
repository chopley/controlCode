#define __FILEPATH__ "antenna/control/Test/tAntennaDataFrameManager.cc"

#include <iostream>
#include <iomanip>

#include <cmath>

#include "gcp/util/common/Test/Program.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/AntennaDataFrameManager.h"
#include "gcp/util/common/ArrayDataFrameManager.h"
#include "gcp/util/common/CorrelatorDataFrameManager.h"

#include "gcp/antenna/control/SzaShare.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

KeyTabEntry Program::keywords[] = {
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  try {
  Debug::setLevel(Program::getiParameter("debuglevel"));

  cout << "Here" << endl;


  SzaShare antman("");

  CoordRange range(0,0);
  range.setStartIndex(0,1);
  range.setStopIndex(0,1);

  // Now test writing to a weird board

  bool bW[2], bR[2];
  double dW[2], dR[2];
  unsigned iW[2], iR[2];

  bW[0] = true;  bW[1] = false;
  antman.writeReg("test", "bReg", bW);
  dW[0] = 1.234; dW[1] = 5.678;
  antman.writeReg("test", "dReg", dW);

  Coord startCoord(1, 4); 
  Coord stopCoord(1, 5); 

  range.setStartCoord(startCoord);
  range.setStopCoord(stopCoord);

  iW[0] = 111; iW[1] = 222;
  antman.writeReg("test", "iReg", iW, &range);
  iR[0] = 0; iR[1] = 0;
  antman.readReg("test", "iReg", iR, &range);
  cout << iR[0] << ", " << iR[1] << endl;

  range.initialize();

  bR[0] = false;  bR[1] = true;
  antman.readReg("test", "bReg", bR, &range);
  cout << bR[0] << ", " << bR[1] << endl;

  cout << " Range is Valid? " << range.isValid() << endl;

  dR[0] = 0.0; dR[1] = 0.0;
  antman.readReg("test", "dReg", dR, &range);
  cout << dR[0] << ", " << dR[1] << endl;

  cout << " Range is Valid? " << range.isValid() << endl;

  range.setStartCoord(startCoord);
  range.setStopCoord(stopCoord);

  iR[0] = 0; iR[1] = 0;
  antman.readReg("test", "iReg", iR, &range);
  cout << iR[0] << ", " << iR[1] << endl;

  cout << " Range is Valid? " << range.isValid() << endl;

  range.initialize();

  range.setStartIndex(0,1);
  range.setStopIndex(0,1);

  dR[0] = 0.0; dR[1] = 0.0;
  antman.readReg("test", "dReg", dR, &range);
  cout << dR[0] << ", " << dR[1] << endl;

  AntennaDataFrameManager antmanCopy(antman), antmanCopy2, antmanCopy3(true);

  dR[0] = 0.0; dR[1] = 0.0;
  antmanCopy.readReg("test", "dReg", dR);
  cout << dR[0] << ", " << dR[1] << endl;

  dR[0] = 0.0; dR[1] = 0.0;
  antmanCopy2.readReg("test", "dReg", dR);
  cout << dR[0] << ", " << dR[1] << endl;

  antmanCopy2 = antman;

  dR[0] = 0.0; dR[1] = 0.0;
  antmanCopy2.readReg("test", "dReg", dR);
  cout << dR[0] << ", " << dR[1] << endl;

  bR[0] = false; bR[1] = false;
  antmanCopy2.readReg("test", "bReg", bR);
  cout << bR[0] << ", " << bR[1] << endl;

  antmanCopy3 = antman;

  dR[0] = 0.0; dR[1] = 0.0;
  antmanCopy3.readReg("test", "dReg", dR);
  cout << dR[0] << ", " << dR[1] << endl;

  bR[0] = false; bR[1] = false;
  antmanCopy3.readReg("test", "bReg", bR);
  cout << bR[0] << ", " << bR[1] << endl;

  // Now test out array data frame manager

  ArrayDataFrameManager arrayman(true);
  ArrayDataFrameManager arrayman2(true);
  DataFrameManager*      dman = dynamic_cast<DataFrameManager*>(&arrayman);
  ArrayDataFrameManager* aman = dynamic_cast<ArrayDataFrameManager*>(dman);

  RegDate date;

  cout << "Writing date: " << date << endl;
  arrayman.writeReg("array", "frame", "utc", date.data());
  cout << "Id is: " << arrayman.getId() << endl;
  cout << "Id is: " << dman->getId() << endl;
  cout << "Id is: " << aman->getId() << endl;

  cout << endl << "Testing ArrayMap: " << endl << endl;

  bW[0] = true;  bW[1] = false;
  arrayman.writeReg("antenna0", "test", "bReg", bW);

  dW[0] = 1.234; dW[1] = 5.678;
  arrayman.writeReg("antenna0", "test", "dReg", dW);

  dW[0] = 51.234; dW[1] = 55.678;
  arrayman.writeReg("antenna5", "test", "dReg", dW);

  range.setStartCoord(startCoord);
  range.setStopCoord(stopCoord);

  iW[0] = 111; iW[1] = 222;
  arrayman.writeReg("antenna0", "test", "iReg", iW, &range);

  bR[0] = false;  bR[1] = true;
  arrayman.readReg("antenna0", "test", "bReg", bR);
  cout << bR[0] << ", " << bR[1] << endl;

  dR[0] = 0.0; dR[1] = 0.0;
  arrayman.readReg("antenna0", "test", "dReg", dR);
  cout << dR[0] << ", " << dR[1] << endl;

  iR[0] = 0; iR[1] = 0;
  arrayman.readReg("antenna0", "test", "iReg", iR, &range);
  cout << iR[0] << ", " << iR[1] << endl;

  range.initialize();
  range.setStartIndex(0,0);
  range.setStopIndex(0,0);

  dR[0] = 0.0; dR[1] = 0.0;
  arrayman.readReg("antenna0", "test", "dReg", dR, &range);
  cout << dR[0] << ", " << dR[1] << endl;

  arrayman.readReg("antenna5", "test", "dReg", dR, &range);
  cout << dR[0] << ", " << dR[1] << endl;

  range.setStopIndex(0,1);
  arrayman.readReg("antenna5", "test", "dReg", dR, &range);
  cout << dR[0] << ", " << dR[1] << endl;

  arrayman.readReg("antenna5", "test", "dReg", dR);
  cout << dR[0] << ", " << dR[1] << endl;

  // Test stuffing an antenna frame into my array manager

  cout << endl << "Testing stuffing" << endl << endl;

  try {
    dW[0] = 1.0; dW[1] = 5.0;
    arrayman.writeReg("antenna0", "test", "dReg", dW);
    
    dW[0] = 1.1; dW[1] = 5.1;
    antman.writeReg("test", "dReg", dW);
    
    dR[0] = dR[1] = 0.0;
    antman.readReg("test", "dReg", dR);
    cout << dR[0] << ", " << dR[1] << endl;
  } catch(...) {}

  antman.setAnt(AntNum::ANT0);
  arrayman.writeAntennaRegMap(antman, true);

  try {
    dW[0] = 1.3; dW[1] = 5.3;
    antman.writeReg("test", "dReg", dW);
  } catch(...) {}

  int siteActual[3], readActual[3];
  CoordRange actualRange(0,2);
  siteActual[0] = -1;
  siteActual[1] =  2;
  siteActual[2] = -3;

  antman.writeReg("tracker", "siteActual", siteActual, &actualRange);

  // Write the antenna map to the array map

  antman.setAnt(AntNum::ANT3);
  arrayman.writeAntennaRegMap(antman, true);

  try {
    dR[0] = dR[1] = 0.0;
    antman.readReg("test", "dReg", dR);
    cout << "Antenna3 test.dReg from antman: " << dR[0] << ", " << dR[1] << endl;
    
    dR[0] = dR[1] = 0.0;
    arrayman.readReg("antenna3", "test", "dReg", dR);
    cout << "antenna3.test.dReg from arrayman: " << dR[0] << ", " << dR[1] << endl;
    arrayman.readReg("antenna0", "test", "dReg", dR);
    cout << "antenna0.test.dReg from arrayman: " << dR[0] << ", " << dR[1] << endl;
  } catch(...) {}

  arrayman.readReg("antenna3", "tracker", "siteActual", readActual, &actualRange);

  cout << "antenna3.tracker.siteActual is: " << readActual[0] << " "
       << readActual[1] << " "
       << readActual[2] << endl;


  // And this should throw an exception

  try {
    dR[0] = dR[1] = 0.0;
    arrayman2.readReg("antenna3", "test", "dReg", dR);
    cout << dR[0] << ", " << dR[1] << endl;
  } catch(...) {
  }

  //------------------------------------------------------------
  // Now test adding two maps together
  //------------------------------------------------------------

  ArrayDataFrameManager fm1, fm2;

  // These registers should be added together on integration

  dW[0] = 1.2; dW[1] = 5.6;
  fm1.writeReg("antenna0", "test", "dReg", dW);
  fm2.writeReg("antenna0", "test", "dReg", dW);

  // These registers should take the last value on integration

  bW[0] = true; bW[1] = false;
  fm1.writeReg("antenna0", "test", "bReg", bW);
  bW[0] = false; bW[1] = true;
  fm2.writeReg("antenna0", "test", "bReg", bW);

  range.initialize();
  range.setStartIndex(0,0);
  range.setStopIndex(0,1);
  range.setStartIndex(1,0);
  range.setStopIndex(1,1);

  // These should be OR'd together on integration

  iW[0] = 2; iW[1] = 4;
  fm1.writeReg("antenna0", "test", "iReg", iW, &range);
  iW[0] = 3; iW[1] = 2;
  fm2.writeReg("antenna0", "test", "iReg", iW, &range);

  // Add the two maps

  fm1 += fm2;

  // And extract and print the results

  dR[0] = dR[1] = 0.0;
  fm1.readReg("antenna0", "test", "dReg", dR);
  cout << dR[0] << ", " << dR[1] << endl;

  iR[0] = iR[1] = 0;
  fm1.readReg("antenna0", "test", "iReg", iR, &range);
  cout << iR[0] << ", " << iR[1] << endl;

  bR[0] = bR[1] = false;
  fm1.readReg("antenna0", "test", "bReg", bR);
  cout << bR[0] << ", " << bR[1] << endl;
  } catch(gcp::util::Exception& err) {
    cout << err.what() << endl;
    return 1;
  } catch(...) {
    cout << "Caught an error" << endl;
    return 1;
  }

  return 0;
}
