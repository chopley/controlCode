#define __FILEPATH__ "util/common/regDoc.cc"

#include <iostream>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Directives.h"
#include "gcp/program/common/Program.h"

#include "gcp/control/code/unix/libunix_src/common/genericregs.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::control;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { END_OF_KEYWORDS},
};

void Program::initializeUsage() {};

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
  documentArrayMap();
  return 0;
}
