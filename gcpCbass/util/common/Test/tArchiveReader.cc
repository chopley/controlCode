#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/ArchiveReader.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/TimeVal.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "arcdir", "/data/sptdaq/arc",    "s", "Archive directory"},
  { "start",  "24-jan-2009:00:00:00","s", "Start time"},
  { "stop",   "25-jan-2009:00:00:00","s", "Stop time"},
  { "cal",    "",                    "s", "Cal file"},
  { "map",    "f",                   "b", "True to memory map the file"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  ArchiveReader reader(Program::getbParameter("map"));

  reader.setArchiveDirectory(Program::getParameter("arcdir"));
  reader.setDates(Program::getParameter("start"), Program::getParameter("stop"));
  reader.getFileList();
  unsigned nFrame1 = reader.countFrames();

  reader.addRegister("antenna0.frame.record double");
  reader.addRegister("array.frame.record");
  reader.addRegister("receiver.bolometers.adc[0]");

  if(!Program::isDefault("cal")) {
    reader.setCalFile(Program::getParameter("cal"));
  }

  TimeVal start, stop, diff;

  CTOUT("Abotu to read data");

#if 1
  start.setToCurrentTime();
  reader.resetToBeginning();

  unsigned nFrame2=0;
  while(reader.readNextFrame()) {
    reader.readRegs();
    nFrame2++;
  }
  stop.setToCurrentTime();
#endif

  reader.iterateFiles();

  diff = stop - start;

  COUT("diff = " << diff.getFractionalTimeInSeconds() << " nFrame1 = " << nFrame1 << " nFrame2 = " << nFrame2);

  return 0;
}
