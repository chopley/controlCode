#include <iostream>

#include "program/common/Program.h"

#include "antenna/control/cbass/ServoComms.h"
#include "antenna/control/cbass/ServoCommand.h"
#include "util/common/Debug.h"
#include "util/common/TimeVal.h"

using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "init",  "f", "b", "shall i initialize?"},
  { "az",  "0", "f", "az"},
  { "el",  "0", "f", "el"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  ServoComms comms;
  ServoCommand command;

  std::vector<float> vals(37);
  const float pars[37] = {0.4,0.4,2.0,2.0,0.01,0.01,1.5,1.0,5.0,1.2,1.0,5.0,2.0,1.2,1.0,1.0,0.8,1.5,0.0,0.0,1.0,1.0,800,1200,-800,-1600,1.0,1.0,150,144000,144000,20000,20000,-251.488,102.301,1522.5,19749};
  for (unsigned i=0;i<37;i++){
    vals[i] = pars[i];
  };

  //  comms.issueCommand(ServoCommand::LOAD_LOOP_PARAMS, vals);


#if(0)  


  comms.connect();


  COUT("INITIALIZING ANTENNA");

  comms.initializeAntenna(1);
  
  
  if(Program::getbParameter("init")){
    comms.initializeAntenna(1);
    COUT("ANTENNA INITIALIZED");
  };

  COUT("SETTING AZ/EL");
  gcp::util::Angle az;
  gcp::util::Angle el;
  az.setDegrees(Program::getdParameter("az"));
  el.setDegrees(Program::getdParameter("el"));
  comms.setAzEl(az, el);

  //  COUT("AZVAL:  " << az);
  // COUT("ElVAL:  " << el);



  COUT("QUERYING STATUS");
  comms.queryStatus();

  COUT("ANTNENA POSITIONS");
  TimeVal start, stop, diff;
  start.setToCurrentTime();
  CTOUT("TIME 1");
  comms.queryAntPositions();
  stop.setToCurrentTime();
  diff = stop - start;
  CTOUT("TIME 2");
  COUT("TIME DIFF: " << diff.getFractionalTimeInSeconds());


  COUT("ANTNENA POSITIONS2");
  start.setToCurrentTime();
  CTOUT("TIME 1");
  comms.queryAntPositions();
  stop.setToCurrentTime();
  diff = stop - start;
  CTOUT("TIME 2");
  COUT("TIME DIFF: " << diff.getFractionalTimeInSeconds());

  

  sleep(2);


  COUT("HALTING");  
  comms.haltAntenna();
  sleep(2);




  COUT("DISCONNECT");  
  comms.disconnect();
#endif


  return 0;
}


