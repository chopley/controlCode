#include <iostream>
#include <unistd.h>

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/antenna/control/specific/Tfp.h"

#include "gcp/program/common/Program.h"

#include <math.h>
#include <time.h>

using namespace std;
using namespace gcp::program;
using namespace gcp::util;
using namespace gcp::antenna::control;

KeyTabEntry Program::keywords[] = {
  { "year",      "2006",                        "i", "Year"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

void Program::initializeUsage() {}

int Program::main()
{
  Tfp tfp;
  TimeVal time;

  tfp.requestModelId();
  tfp.requestAssemblyPartNo();
  tfp.readYear();

  tfp.setTimeFormat(Tfp::FORMAT_UNIX);

  for(unsigned i=0; i < 10; i++) {

    tfp.readUnixTime(time);

    sleep(1);

    COUT(time);
  }
  return 0;

#if 0
  //tfp.setYear(Program::getiParameter("year"));

  //tfp.setOutputClockFreq(Tfp::FREQ_10MHZ);
  //tfp.setOutputClockFreq(Tfp::FREQ_5MHZ);
  //tfp.setOutputClockFreq(Tfp::FREQ_1MHZ);

  //  tfp.setFrequencyOutput(true, 75x.0, 8000.0);
  //  tfp.setFrequencyOutput(true, 50.0, 250000.0);


  tfp.readYear();

  // Now set the major time

  //  time.setToCurrentTime();

  //  tfp.setMajorTime((unsigned int)pow(2.0, 31));

  unsigned uval = 0x00ffffff;

  //  tfp.requestAssemblyPartNo();
  //  tfp.requestModelId();

  //  tfp.softReset();

  //tfp.setTimeMode(Tfp::MODE_FREERUN);

#ifdef BCD 
  tfp.setTimeFormat(Tfp::FORMAT_BCD);
#else
  tfp.setTimeFormat(Tfp::FORMAT_UNIX);
#endif

  struct timespec ts;
  ts.tv_sec  = 1;
  ts.tv_nsec = 0;

  //  tfp.setMajorTime(0);

  for(unsigned i=0; i < 10; i++) {

    tfp.readYear();
#ifdef BCD
    tfp.readBcdTime(time);
#else
    tfp.readUnixTime(time);
#endif

    COUT("Time was: " << time);
    sleep(1);

  }

    nanosleep(&ts, NULL);

return 0;
#endif
}
