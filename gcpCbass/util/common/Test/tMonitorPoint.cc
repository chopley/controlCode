#define __FILEPATH__ "util/common/Test/tMonitorPoint.cc"

#include <iostream>
#include <iomanip>

#include <cmath>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/AntennaDataFrameManager.h"
#include "gcp/util/common/ArrayRegMapDataFrameManager.h"
#include "gcp/util/common/MonitorPointManager.h"
#include "gcp/util/common/MonitorPoint.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};


static MONITOR_CONDITION_HANDLER(handler) {
  cout << "Handler was called.  Condition was " << (satisfied ? "" : "not") << " satisfied." 
       << "Message was: " << message << endl;
}

static void testNoCondition(bool persistent);
static void testRangeCondition();
static void testArrayRegMap();
static void testConditionOnMultipleElements();

int Program::main()
{
  testNoCondition(false);
  testNoCondition(true);
  testRangeCondition(); 
  testArrayRegMap();
  testConditionOnMultipleElements();
  return 0;
}

static void testNoCondition(bool persistent)
{
  AntennaDataFrameManager antman;
  MonitorPointManager manager(&antman);
  
  MonitorPoint* monitorPoint = manager.addMonitorPoint("caltert", "apiNo");
  
  MonitorCondition condition;
  condition.setTo();
  
  monitorPoint->registerConditionHandler(handler, (void*)0, 
					 "this is a no condition test", 
					 "",
					 condition, 
					 persistent);
  
  // Now write to the monitor point
  
  cout << "About to write through the manager" << endl;
  antman.writeReg("caltert", "apiNo", (unsigned short)105);
  
  cout << "About to write through the monitor point (1)" << endl;
  monitorPoint->writeReg(false, (unsigned short)104);
  
  cout << "About to write through the monitor point (2)" << endl;
  monitorPoint->writeReg(false, (unsigned short)104);
  
  cout << "About to write through the monitor point (3)" << endl;
  monitorPoint->writeReg(false, (unsigned short)104);
  
  cout << "About to write through the monitor point (4)" << endl;
  monitorPoint->writeReg(false, (unsigned short)104);
}

static void testRangeCondition()
{
  AntennaDataFrameManager antman;
  MonitorPointManager manager(&antman);
  
  MonitorPoint* monitorPoint = manager.addMonitorPoint("caltert", "apiNo");
  
  MonitorCondition condition;
  DataType min((unsigned short)100);
  DataType max((unsigned short)106);

  condition.setTo(DataTypeTruthFn::greaterThanAndLessThan, min, max);
  
  monitorPoint->registerConditionHandler(handler, (void*)0, "this is a range test", "",
					 condition, false);

  monitorPoint->registerConditionHandler(handler, (void*)0, "this is another range test", "",
					 condition, true);
  
  // Now write to the monitor point
  
  cout << "About to write through the monitor point (1)" << endl;
  monitorPoint->writeReg(false, (unsigned short)99);
  
  cout << "About to write through the monitor point (2)" << endl;
  monitorPoint->writeReg(false, (unsigned short)100);
  
  cout << "About to write through the monitor point (3)" << endl;
  monitorPoint->writeReg(false, (unsigned short)101);
  
  cout << "About to write through the monitor point (4)" << endl;
  monitorPoint->writeReg(false, (unsigned short)104);

  cout << "About to write through the monitor point (5)" << endl;
  monitorPoint->writeReg(false, (unsigned short)101);
}

static void testArrayRegMap()
{
  ArrayRegMapDataFrameManager arrman;
  MonitorPointManager manager(&arrman);
  
  MonitorPoint* monitorPoint = manager.addMonitorPoint("delay", "adjustableDelay", 0);

  monitorPoint->writeReg(false, 100.1);

  CoordRange range;
  range.setStartIndex(0, 0);
  range.setStopIndex(0, 0);
  double dVal;
  arrman.readReg("delay", "adjustableDelay", &dVal, &range);

  std::cout << "Value is: " << dVal << std::endl;
}

static void testConditionOnMultipleElements()
{
  ArrayRegMapDataFrameManager arrman;
  MonitorPointManager manager(&arrman);
  
  MonitorPoint* monitorPoint = manager.addMonitorPoint("delay", 
						       "location", 1);
  
  MonitorCondition condition;
  condition.setTo();
  
  monitorPoint->registerConditionHandler(handler, (void*)0, 
					 "Delay location was set", "",
					 condition, false);

  double locations[8][3], readLocations[8][3];
  for(unsigned i=0; i < 8; i++) {
    for(unsigned j=0; j < 3; j++) {
      locations[i][j]  = (double)(i + j);
      readLocations[i][j] = 0.0;
    }
  }

  arrman.writeReg("delay", "location", locations[0]);

  arrman.readReg("delay", "location", readLocations[0]);
  for(unsigned i=0; i < 8; i++) {
    for(unsigned j=0; j < 3; j++) 
      std::cout << readLocations[i][j] << " ";
    std::cout << " ";
  }
  std::cout << std::endl;

  monitorPoint->writeReg(false, (double)1.2);

  arrman.readReg("delay", "location", readLocations[0]);
  for(unsigned i=0; i < 8; i++) {
    for(unsigned j=0; j < 3; j++) 
      std::cout << readLocations[i][j] << " ";
    std::cout << " ";
  }
  std::cout << std::endl;

  static double test[3]= {0.1, 0.2, 0.3};
  monitorPoint->writeReg(false, test);

  arrman.readReg("delay", "location", readLocations[0]);
  for(unsigned i=0; i < 8; i++) {
    for(unsigned j=0; j < 3; j++) 
      std::cout << readLocations[i][j] << " ";
    std::cout << " ";
  }
  std::cout << std::endl;
}
