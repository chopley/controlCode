#include <iostream>
#include <sstream>
#include <cmath>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/WxReaderSA.h"
#include "gcp/util/common/String.h"
#include "gcp/program/common/Program.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  {END_OF_KEYWORDS}
};

void Program::initializeUsage() {}

int Program::main()
{
  WxReaderSA reader;

  reader.getData();

  //  COUT(reader.data_);
  
  std::string date;
  date = (const char*) reader.data_.mtSampleTimeUchar_;
  String str;
  str = String(date);

  std::string weekDay  = str.findNextStringSeparatedByChars(" ").str();
  std::string monthStr = str.findNextStringSeparatedByChars(" ", " ").str();
  unsigned int day     = str.findNextStringSeparatedByChars(" ", " ").toInt();
  unsigned int hour    = str.findNextStringSeparatedByChars(":").toInt();
  unsigned int min     = str.findNextStringSeparatedByChars(":").toInt();
  double sec           = str.findNextStringSeparatedByChars("UTC").toDouble();
  unsigned int year    = str.findNextStringSeparatedByChars("P").toInt();

  //  unsigned int month   = validateMonth(monthStr);
  //  COUT("day = " << day << " month = " << month << " year = " << year);
  COUT("day = " << day << " year = " << year);
  COUT("TIME: " << hour << ":" << min << ":" << sec);

  return 0;
}

