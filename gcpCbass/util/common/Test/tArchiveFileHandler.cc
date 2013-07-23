#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/ArchiveFileHandler.h"
#include "gcp/util/common/RegDate.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "file", "/data/sptdaq/arc",    "s", "Archive file"},
  { "time", "24-jan-2009:00:00:00","s", "Target time"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  ArchiveFileHandler file;

  file.setTo(Program::getParameter("file"));
  file.openForRead();

  unsigned iFrame = 1;
  
  RegDate date;

  date.setMjd(file.getMjd(iFrame));
  COUT("Date (" << iFrame << ") is: " << date);

  date.setMjd(file.getMjd(iFrame-1));
  COUT("Date (" << iFrame-1 << ") is: " << date);

  date.setMjd(file.getMjd(iFrame+1));
  COUT("Date (" << iFrame+1 << ") is: " << date);

  file.close();

  return 0;
}
