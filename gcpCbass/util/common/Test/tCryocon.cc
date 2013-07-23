#include <iostream>

#include "program/common/Program.h"

#include "util/common/Debug.h"
#include "util/common/Timer.h"
#include "util/common/TimeVal.h"
#include "util/common/LsThermal.h"
#include "util/common/CryoCon.h"


using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "dev",        "/dev/ttyUSB3", "s", "device"},
  { END_OF_KEYWORDS}
};


int Program::main()
{
  // gpib/usb has a set address
  int i;


  COUT("ABOUT TO TALK TO THE CONVERTER");
  // GpibUsbController gpib("/dev/cu.usbserial-PXEIAFV5", 1);
  GpibUsbController gpib(Program::getParameter("dev"), true);

  gpib.connectAndClear();
  gpib.setEoi(true);
  gpib.setAuto(false);
  gpib.setEos(GpibUsbController::EOS_CRLF);
  gpib.setEotChar('\n');
  gpib.enableEot(true);

  LsThermal thermo(gpib);
  CryoCon   cryocon(gpib);

  //  COUT(thermo.getEos());
  //  COUT(thermo.getEoi());

  thermo.setAddress(11);
  cryocon.setAddress(12);

  COUT("THERMO DEVICE NUMBER : " << thermo.getDevice());
  COUT("CRYOCON DEVICE NUMBER: " << cryocon.getDevice());

  sleep(1);

#if(1)
  float temp;
  std::vector<float> retVal;

  Timer timer;

  for(i=0; i < 10; i++) {
    timer.start();
    temp = cryocon.queryChannelTemperature(0);
    timer.stop();
    CTOUT("Cryocon temp = " << temp << " dT = " << timer.deltaInSeconds());
  }

  for(i=0; i < 10; i++) {
    timer.start();
    retVal = thermo.requestMonitor();
    timer.stop();
    CTOUT("Lakeshore temp = " << retVal[0] << " dT = " << timer.deltaInSeconds());
  }



  COUT("QUERYING ALL TOGETHER");
  retVal = thermo.requestMonitor();
  sleep(1);
  for(i=0;i<8;i++){
    COUT("Temp Monitor[" << i << "]: " << retVal[i]);    
  };





  COUT("THERMO DEVICE NUMBER : " << thermo.getDevice());
  COUT("CRYOCON DEVICE NUMBER: " << cryocon.getDevice());

  sleep(1);
  COUT("THERMO DEVICE NUMBER : " << thermo.getDevice());
  COUT("CRYOCON DEVICE NUMBER: " << cryocon.getDevice());

  sleep(1);
  COUT("THERMO DEVICE NUMBER : " << thermo.getDevice());
  COUT("CRYOCON DEVICE NUMBER: " << cryocon.getDevice());


  COUT("request analog output");
  retVal = thermo.requestAnalogOutput(1);
  COUT("analog output is: " << retVal[0]);

  COUT("QUERYING EACH INDIVIDUALLY");
  for (i=1;i<9;i++){
    retVal = thermo.requestMonitor(i);
    COUT("Temp Monitor[" << i << "]: " << retVal[0]);
  };

  sleep(1);
  COUT("QUERYING ALL TOGETHER");
  retVal = thermo.requestMonitor();
  sleep(1);
  for(i=0;i<8;i++){
    COUT("Temp Monitor[" << i << "]: " << retVal[i]);    
  };
#endif
  

#if(0)

  //  Debug::setLevel(Debug::DEBUG7);
  CTOUT("B4");
  COUT("CryoCon DEVICE NUMBER : " << cryocon.getDevice());
  CTOUT("AFTER");
  //  Debug::setLevel(Debug::DEBUGNONE);

  CTOUT("B4");
  COUT("CryoCon DEVICE NUMBER : " << cryocon.getDevice());
  CTOUT("AFTER");

  CTOUT("GET VERSION");
  COUT("CONTROLLER VERSION: " << gpib.getVersion());
  CTOUT("AFTER");

  values[0] = 1;
  values[1] = 16.5;
  values[2] = 11;
  values[3] = 20.5;
  values[4] = 31;
  values[5] = 20;
  values[6] = 50;
  values[7] = 50;

  CTOUT("LOADING LOOP 2 PARAMETERS");
  cryocon.setUpLoop(2, values);
  CTOUT("AFTER");

  CTOUT("LOADING LOOP 1 PARAMETERS");
  cryocon.setUpLoop(1);
  CTOUT("AFTER");

  COUT(gpib.getHelp());

#endif

#if(0)



  COUT("SETTING UNITS");
  cryocon.setInputUnits();


  COUT("HEATING UP");
  cryocon.heatUpSensor(1);
  sleep(2);


  for (j=0;j<100;j++){
    COUT("QUERYING THE TEMP SENSOR");
    temp = cryocon.queryChannelTemperature(0);
    COUT("TEMP: " << temp);
  }


  COUT("Engaging THE LOOP");
  cryocon.engageControlLoop();
  sleep(2);


  
  COUT("RESUME COOLING");
  cryocon.resumeCooling(1);

  for (j=0;j<100;j++){
    COUT("QUERYING THE TEMP SENSOR");
    temp = cryocon.queryChannelTemperature(0);
    COUT("TEMP: " << temp);
  }


  sleep(1);
  COUT("TURNING OFF LOOP");
  cryocon.stopControlLoop();

  for (j=0;j<100;j++){
    CTOUT("QUERYING THE TEMP SENSOR");
    temp = cryocon.queryChannelTemperature(0);
    CTOUT("TEMP: " << temp);
  }


  sleep(1);
  COUT("QUERYING THE HEATER CURRENT");
  float current = cryocon.queryHeaterCurrent();
  COUT("current: " << current);

#endif

  // let's swap between querying both.
  /*
    int jj;
  for (i=0;i<20;i++){
    retVal = thermo.requestMonitor();
    for(jj=0;jj<8;jj++){
      COUT("Temp Monitor[" << jj << "]: " << retVal[jj]);    
    };
    for (j=0;j<5;j++){
      temp = cryocon.queryChannelTemperature(0);
      COUT("cryoCon: " << temp);      
    }
  };
  */



  /*
  COUT("QUERY LAKESHORE TEMPS");
  retVal = thermo.requestMonitor();
  for(i=0;i<8;i++){
    COUT("Temp Monitor[" << i << "]: " << retVal[i]);    
  };
  sleep(1);
  */

#if(0)
  COUT("CHATTING WITH BOTH");

  
  COUT("QUERY LAKESHORE TEMPS");
  retVal = thermo.requestMonitor();
  for(i=0;i<8;i++){
    COUT("Temp Monitor[" << i << "]: " << retVal[i]);    
  };
  sleep(1);

  COUT("Engaging THE LOOP");
  cryocon.engageControlLoop();
  sleep(1);


  COUT("QUERYING THE CRYOCON TEMP SENSOR");
  temp = cryocon.queryChannelTemperature(0);
  COUT("TEMP: " << temp);

  sleep(1);
  COUT("QUERYING THE CRYOCON HEATER CURRENT");
  current = cryocon.queryHeaterCurrent();
  COUT("current: " << current);

  COUT("QUERY LAKESHORE TEMPS");
  retVal = thermo.requestMonitor();
  for(i=0;i<8;i++){
    COUT("Temp Monitor[" << i << "]: " << retVal[i]);    
  };

  sleep(1);
  COUT("TURNING OFF LOOP");
  cryocon.stopControlLoop();
  sleep(1);
  
#endif

  gpib.stop();
  sleep(1);

  return 0;


};
