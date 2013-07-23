#include <iostream>

#include "program/common/Program.h"

#include "util/common/Debug.h"
#include "util/common/TimeVal.h"
#include "util/common/LsThermal.h"
#include "util/common/CryoCon.h"


using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { END_OF_KEYWORDS}
};


int Program::main()
{
  // gpib/usb has a set address

  COUT("ABOUT TO TALK TO THE CONVERTER");
  // GpibUsbController gpib("/dev/cu.usbserial-PXEIAFV5", 1);
  GpibUsbController gpib("/dev/ttyUSB0", 1);
  gpib.connectAndClear();
  gpib.setEoi(true);
  gpib.setAuto(false);
  gpib.setEos(GpibUsbController::EOS_CRLF);
  gpib.setEotChar('\n');
  gpib.enableEot(true);


  COUT("Version is: '" << gpib.getVersion()  << "'");
  //sleep(1);
  COUT("Version is: '" << gpib.getVersion()  << "'");
  COUT("Version is: '" << gpib.getVersion()  << "'");
  //sleep(1);

  LsThermal thermo(gpib);
  COUT("HERE 1h");
  CryoCon cryocon(gpib);
  COUT("HERE 1i");

  //  COUT(thermo.getEos());
  //  COUT(thermo.getEoi());

  COUT("Talking to Lakeshore");
  thermo.setAddress(11);

  COUT("Talking to Cryocon");
  cryocon.setAddress(12);

  std::vector<float> retVal;
  int i;

#if(1)
  
  int j;
  for (i=0;i<100;i++){
    COUT("QUERY NUMBER: " << i);
    //    COUT("QUERYING ALL TOGETHER");
    try { 
      retVal = thermo.requestMonitor();
    } catch(...) {
    };
    //sleep(1);
#if 1
    for(j=0;j<8;j++){
      COUT("Temp Monitor[" << j << "]: " << retVal[j]);    
    };
#endif
  }
#endif



  sleep(1);
  gpib.stop();
  //sleep(1);

  return 0;


};
