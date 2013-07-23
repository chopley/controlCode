#include <iostream>
#include <cerrno>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/SshTunnel.h"
#include "gcp/util/common/CoProc.h"
#include "gcp/util/common/FdSet.h"

#include "gcp/util/common/Source.h"
#include "gcp/util/common/ModelReader.h"
#include "gcp/control/code/unix/libunix_src/common/astrom.c"
#include "gcp/antenna/control/specific/Site.cc"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;
using namespace gcp::antenna::control;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "src",    "jupiter",  "s", "Source"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
#if(0)
  Source source;

  source.reset();
  source.setName("moon");
  source.setType(gcp::control::SRC_EPHEM);

  double tt = gcp::control::mjd_utc_to_mjd_tt(55484.3);

  COUT("TT: " << tt);

  COUT("CAN BRACKET: " << source.canBracket(55484.3));

  
  double final = source.getDist(tt);

  COUT("final dist: " << final);

#endif

  Site site;
  Angle lon, lat;
  lon.setDegrees(112);
  lat.setDegrees(37);
  double altitude = 1222;
  site.setFiducial(lon, lat, altitude);
  
  COUT("RCENT: " << site.actual_.rcent);


  double final;
  string dir("/home/cbass/gcpCbass/control/ephem");
  string filename("moon_3yr.ephem");

  ModelReader model(dir, filename);
  
  TimeVal time;
  time.setMjd(56285.49);
  unsigned int error;
  
  final = model.getDistance(time, error);
  COUT("error: " << error);
  
  COUT("final dist: " << final);

  sleep(1);
  COUT("TRY AGAIN");
  final = model.getDistance(56285.49, error);
  COUT("error: " << error);  
  COUT("final dist: " << final);

}

