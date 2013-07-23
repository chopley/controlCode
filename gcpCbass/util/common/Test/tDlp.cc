#include <iostream>

#include "program/common/Program.h"

#include "util/common/Debug.h"
#include "util/common/TimeVal.h"
#include "util/common/DlpUsbThermal.h"
#include "util/common/DlpUsbThermalMsg.h"


using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { END_OF_KEYWORDS}
};


int Program::main()
{
  DlpUsbThermal dlp;
  DlpUsbThermalMsg msg;
  std::vector<float> values(8);
  int i, j;
  TimeVal start, stop, diff;

  struct timespec delay;
  delay.tv_sec  =         0;
  delay.tv_nsec = 300000000;


  dlp.connect();

  

  dlp.setupDefault();
  COUT("default set up");

  sleep(1);

  for (j=0;j<10;j++) {
  start.setToCurrentTime();
  values = dlp.queryAllTemps();
  COUT("temps queried");
  stop.setToCurrentTime();
  diff = stop - start;
  COUT("diff: " << diff.getTimeInMicroSeconds());
  COUT("TEMPVALS: " << values[0] << "," << values[1] << "," << values[2] << "," << values[3] << "," << values[4] << "," << values[5] << "," << values[6] << "," << values[7]);

  nanosleep(&delay, 0);


  }

  for (i=1;i<9;i++){
  start.setToCurrentTime();
  float value = dlp.queryTemperature(i);
  COUT("temps queried");
  stop.setToCurrentTime();
  diff = stop - start;
  COUT("diff: " << diff.getTimeInMicroSeconds());
  COUT("TEMPVALS: " << value);
  nanosleep(&delay, 0);
  }


  for (j=0;j<10;j++) {
  start.setToCurrentTime();
  values = dlp.queryAllVoltages();
  COUT("volts queried");
  stop.setToCurrentTime();
  diff = stop - start;
  COUT("diff: " << diff.getTimeInMicroSeconds());
  COUT("VOLTVALS: " << values[0] << "," << values[1] << "," << values[2] << "," << values[3] << "," << values[4] << "," << values[5] << "," << values[6] << "," << values[7]);

  nanosleep(&delay, 0);


  }



#if(0)
  start.setToCurrentTime();
  values = dlp.queryAllTemps();
  COUT("temps queried");
  stop.setToCurrentTime();
  diff = stop - start;
  COUT("diff: " << diff.getTimeInMicroSeconds());
  COUT("TEMPVALS: " << values[0] << "," << values[1] << "," << values[2] << "," << values[3] << "," << values[4] << "," << values[5] << "," << values[6] << "," << values[7]);

  start.setToCurrentTime();
  values = dlp.queryAllTemps();
  COUT("temps queried");
  stop.setToCurrentTime();
  diff = stop - start;
  COUT("diff: " << diff.getTimeInMicroSeconds());
  COUT("TEMPVALS: " << values[0] << "," << values[1] << "," << values[2] << "," << values[3] << "," << values[4] << "," << values[5] << "," << values[6] << "," << values[7]);

  start.setToCurrentTime();
  values = dlp.queryAllTemps();
  COUT("temps queried");
  stop.setToCurrentTime();
  diff = stop - start;
  COUT("diff: " << diff.getTimeInMicroSeconds());
  COUT("TEMPVALS: " << values[0] << "," << values[1] << "," << values[2] << "," << values[3] << "," << values[4] << "," << values[5] << "," << values[6] << "," << values[7]);

  start.setToCurrentTime();
  values = dlp.queryAllTemps();
  COUT("temps queried");
  stop.setToCurrentTime();
  diff = stop - start;
  COUT("diff: " << diff.getTimeInMicroSeconds());
  COUT("TEMPVALS: " << values[0] << "," << values[1] << "," << values[2] << "," << values[3] << "," << values[4] << "," << values[5] << "," << values[6] << "," << values[7]);


  COUT("how long to query all at once?")
    std::string query("90-=OP[]'");
  start.setToCurrentTime();
  dlp.writeString(query);
  dlp.readPort(msg);
  stop.setToCurrentTime();
  diff = stop - start;
  COUT("diff: " << diff.getTimeInMicroSeconds());  
#endif


  dlp.disconnect();
  
  return 0;


};
