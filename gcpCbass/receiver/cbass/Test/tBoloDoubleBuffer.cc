#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/receiver/specific/BoloDoubleBuffer.h"

#include "gcp/receiver/specific/BolometerDataFrameManager.h"

#include "gcp/util/common/Exception.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::receiver;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  {"dioHost", "stoli",  "s", "DIO host"},
  {"hwHost",  "stoli",  "s", "HW host"},
  {"dioPort",  "5205",  "i", "DIO port"},
  {"hwPort",   "5207",  "i", "HW port"},
  {"ac",          "t",  "b", "ac|dc"},
  {"nsamp",       "0",  "i", "Number of samples to acquire (0 = free running)"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  BolometerDataFrameManager* rfm=0;
  BolometerDataFrameManager* wfm=0;

  BoloDoubleBuffer db;

  rfm = (BolometerDataFrameManager*) db.grabReadBuffer();
  COUT("Read frame is: " << rfm);
  db.releaseReadBuffer();

  wfm = (BolometerDataFrameManager*) db.grabWriteBuffer();
  COUT("Write frame is: " << wfm);
  db.releaseWriteBuffer();
  
  db.switchBuffers();

  rfm = (BolometerDataFrameManager*) db.grabReadBuffer();
  COUT("Read frame is: " << rfm);
  db.releaseReadBuffer();

  wfm = (BolometerDataFrameManager*) db.grabWriteBuffer();
  COUT("Write frame is: " << wfm);

  COUT("About to gran write buffer again (should block)");
  wfm = (BolometerDataFrameManager*) db.grabWriteBuffer();


  return 0;
}
