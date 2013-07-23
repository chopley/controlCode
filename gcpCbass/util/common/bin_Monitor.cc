#define __FILEPATH__ "util/common/monitor.cc"

#include <iostream>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Directives.h"
#include "gcp/util/common/Monitor.h"

using namespace std;
using namespace gcp::util;
#include "gcp/program/common/Program.h"

using namespace gcp::program;

void Program::initializeUsage()
{
  Program::usage = 
    "Usage: gcpMonitor [host=gcpnet.ovro.caltech.edu] [arcdir=/data/gcpdaq/arc] [calfile=cal] [file=myregs.txt] [start=01-dec-2004:01] [stop=01-dec-2004:02]";

  Program::description = 
    "\ngcpMonitor is a program for reading data from either the real-time\n"
    "monitor stream or the archive, and outputting it as ASCII text.\n"
    "\n"
    "After typing 'gcpMonitor', you must tell gcpMonitor which registers\n"
    "you wish to monitor using the following syntax:\n"
    "\n"
    "\tadd regMap.board.block[indices] type formatString\n"
    "\n"
    "where:\n"
    "\n"
    "\tregMap.board.block  -- is a valid register name\n"
    "\n\ttype                -- (optional) is the data type you wish to use\n"
    "\t                       to display the data, from:\n"
    "\n"
    "\t                         string\n"
    "\t                         bool\n"
    "\t                         char\n"
    "\t                         float\n"
    "\t                         double\n"
    "\t                         int\n"
    "\t                         uint\n"
    "\t                         long\n"
    "\t                         ulong\n"
    "\t                         date\n"
    "\t                         complex\n"
    "\n\tformatString         -- (optional) is a C-style format string to use in\n"
    "\t                        printing the converted value\n"
    "\n"
    "When you are done specifying which registers to monitor, just type 'read'.\n"
    "\n"
    "For each frame of data read, gcpMonitor will print the registers you requested\n"
    "as successive columns in a single row.\n\n"
    "Alternatively, these commands can be placed in a file, and used like:\n"
    "\n"
    "\tgcpMonitor < filename\n"
    "\n or\n"
    "\tgcpMonitor file=filename\n"
    "\n\n"
    "Example: To print the source for all antennas as a string:\n\n"
    "\tadd antenna*.tracker.source string\n\n"
    "Example: To print all LSB visibility amplitudes for band0, baseline 0-1, \n"
    "         formatted in a field 12 characters wide, with 6 digit precision:\n\n"
    "\tadd corr.band0.lsb.amp[0] double \"%12.6f\"\n\n"
    "Example: To print the same as complex numbers with default formatting:\n\n"
    "\tadd corr.band0.lsb[0]\n\n"
    "Example: To print the UTC as a date string:\n\n"
    "\tadd array.frame.utc date\n\n"
    "Example: To print the UTC as an MJD:\n\n"
    "\tadd array.frame.utc double\n\n"
    "The program can attach to either the real-time data stream, a disk archive, \n"
    "or both. The behavior is as follows:\n\n"
    "        When no start and no stop date are specified using the 'start' and\n"
    "        'stop' keywords, respectively, the program will attach to the real-time\n"
    "        data stream from the host specified using the 'host' keyword.\n"
    "\n"
    "        If a start but no stop date is specified, the program will instead \n"
    "        attach to the archive specified with the 'arcdir' keyword, then\n"
    "        attach to the real-time data stream.\n"
    "\n"
    "        If a stop but no start date is specified, the program will read all\n"
    "        data from the beginning of the archive to the specified stop date.\n"
    "\n"
    "        If both a start and stop date are given, the program will read archived\n"
    "        data between the start and stop dates.\n"
    "\n";
};

KeyTabEntry Program::keywords[] = {
  { "arcdir",     "/data/gcpdaq/arc",    "s", "Archive directory"},
  { "calfile",    CALFILE, "s", "Cal file"},
  { "file",       "",      "s", "Register file"},
  { "host",       HOST,    "s", "ACC control host"},
  { "date",       "t",     "b", "If true, print the default date as a string"},
  { "start",      "",      "s", "The start UTC (dd-mmm-yyyy:hh:mm:ss)"},
  { "stop",       "",      "s", "The stop  UTC (dd-mmm-yyyy:hh:mm:ss)"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

/**.......................................................................
 * There are several possible sets of inputs.  We will treat them as
 * follows:
 *
 * 1) If a stop mjd is given, we will read from the archive until that
 * stop date.  If no start is given, we will read from the beginning
 * of the archive to the stop date.
 *
 * 2) If no -` stop is given, we will read from the start date, if any,
 * then connect to the real-time controller.  If no start date is
 * given, we will simply connect to the real-time controller.
 */
int Program::main()
{
  Monitor monitor(Program::getParameter("arcdir"),
		  Program::getParameter("calfile"),
		  Program::getParameter("host"),
		  Program::getParameter("start"),
		  !isDefault("start"),
		  Program::getParameter("stop"),
		  !isDefault("stop"),
		  Program::getParameter("file"));
    
  // We will always record the date
    
  // See if the user requested dates to be printed as readable
  // strings, or as integer MJD + milliseconds
    
  if(Program::getbParameter("date"))
    monitor.addRegister("array.frame.utc date");
  else
    monitor.addRegister("array.frame.utc double");
    
  // Finally, run this object
    
  monitor.run();

  return 0;
}
