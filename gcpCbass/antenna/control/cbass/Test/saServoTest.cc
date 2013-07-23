#include <iostream>

#include "program/common/Program.h"

#include "antenna/control/cbass/ServoCommsSa.h"
#include "antenna/control/cbass/ServoCommandSa.h"
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
  ServoCommsSa comms;
  ServoCommandSa command;


  comms.connect();

  COUT("testing commands");

  int i;
  for(i=0;i<1;i++){

    #if(0)
    COUT("TESTING GAE");
    comms.issueCommand(ServoCommandSa::GET_AZEL);
    comms.issueCommand(ServoCommandSa::GET_PRIOR_LOC);


    COUT("brake commands");
    comms.issueCommand(ServoCommandSa::EL_BRAKE_ON);
    sleep(1);
    comms.issueCommand(ServoCommandSa::AZ_BRAKE_ON);
    sleep(1);
    comms.issueCommand(ServoCommandSa::QUERY_STATUS);


    sleep(5);
    COUT("turning brake back off");
    comms.issueCommand(ServoCommandSa::EL_BRAKE_OFF);
    sleep(1);
    comms.issueCommand(ServoCommandSa::AZ_BRAKE_OFF);

    comms.issueCommand(ServoCommandSa::QUERY_STATUS);


    sleep(1);

    COUT("clutches and contactors on");
    comms.issueCommand(ServoCommandSa::CLUTCHES_ON);
    comms.issueCommand(ServoCommandSa::AZ_CONTACTORS_ON);
    comms.issueCommand(ServoCommandSa::EL_CONTACTORS_ON);


    sleep(1);

    COUT("clutches and contactors off");
    comms.issueCommand(ServoCommandSa::CLUTCHES_OFF);
    comms.issueCommand(ServoCommandSa::AZ_CONTACTORS_OFF);

    comms.issueCommand(ServoCommandSa::EL_CONTACTORS_OFF);    

    COUT("TESTING AEL");
    std::vector<float> values(2);
    values[0] = 150.0;
    values[1] = 45.0;
    comms.issueCommand(ServoCommandSa::SEND_POS, values);
    

    std::vector<float> params(7);
    params[0] = 66000;
    params[1] = 100;
    params[2] = 100;
    params[3] = 1;
    params[4] = 1;
    params[5] = 0.5;
    params[6] = 0.002;
    
    COUT("sending A");
    comms.issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_A, params);

    params[3] = 10;
    sleep(1);
    COUT("sending B");
    comms.issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_B, params);

    params[3] = 3;
    sleep(1);
    COUT("sending C");
    comms.issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_C, params);

    params[3] = 4;
    sleep(1);
    COUT("sending D");
    comms.issueCommand(ServoCommandSa::LOAD_LOOP_PARAMS_D, params);

    COUT("TESTING STATUS");
    comms.issueCommand(ServoCommandSa::QUERY_STATUS);

    #endif
    std::vector<float> params(7);

    COUT("ENGAGING");
    params[0] = 1;
    comms.issueCommand(ServoCommandSa::SERVO_ENGAGE, params);
    sleep(1);

    COUT("(dis)ENGAGING");
    params[0] = 0;
    comms.issueCommand(ServoCommandSa::SERVO_ENGAGE, params);



    sleep(1);
  }
  COUT("DONE");
  sleep(2);

  COUT("DISCONNECT");  
  comms.disconnect();

  return 0;
}


