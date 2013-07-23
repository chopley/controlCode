#include "gcp/util/common/CryoCon.h"

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructors
 */
CryoCon::CryoCon(bool doSpawn) : GpibUsbDevice(doSpawn) {}
CryoCon::CryoCon(std::string port, bool doSpawn) : GpibUsbDevice(port, doSpawn) {}
CryoCon::CryoCon(GpibUsbController& controller) : GpibUsbDevice(controller) {}

/**.......................................................................
 * Destructor.
 */
CryoCon::~CryoCon() {}

/**.......................................................................
 * Destructor.
 */
void CryoCon::setUpLoop(int loopNum)
{
  /* when there are no inputs, we just use the defaults */
  std::vector<float> values(8);
  values[0] = 1;
  values[1] = 12.0;  // sky temperature
  values[2] = 1;
  values[3] = 0.4;
  values[4] = 3.2;
  values[5] = 0;
  values[6] = 50;
  values[7] = 50;

  setUpLoop(loopNum, values);
  return;
};

void CryoCon::setUpLoop(int loopNum, std::vector<float>& values)
{

  COUT("HERE 0");
  /* first we set the output units to Kelvin */
  setInputUnits();

  /* Next we set up the PID loops */
  stopControlLoop();
  setControlLoopType(loopNum, 1);
  setSourceChannel(loopNum, (int) values[0]);
  setSkyTemp(loopNum, values[1]);
  setLoopRange(loopNum, (int) values[2]);
  setPGain(loopNum, values[3]);
  setIGain(loopNum, values[4]);
  setDGain(loopNum, values[5]);
  setPowerOutput(loopNum, values[6]);
  setHeaterLoad(loopNum, values[7]);

  /* Now we start the loop */
  engageControlLoop();

  return;
};

/**.......................................................................
 * Heat Up the sensor
 */
void CryoCon::heatUpSensor(int loopNum)
{
  /* turn off the current loop */
  stopControlLoop();

  /* set control Loop to Manual */
  setControlLoopType(loopNum, 2);
  
  /* turn loop back on */
  engageControlLoop();
};


/**.......................................................................
 * Resume cooling after warming up the sensor
 */
void CryoCon::resumeCooling(int loopNum)
{
  /* turn off the current loop */
  stopControlLoop();

  /* set control Loop to PID */
  setControlLoopType(loopNum, 1);
  
  /* turn loop back on */
  engageControlLoop();
};

/**.......................................................................
 * Set Input Units
 */
void CryoCon::setInputUnits()
{
  std::ostringstream os;
  
  os << "INPUT A:UNITS K";
  sendDeviceCommand(os.str());
  // does not expect response;

  return;
};

/**.......................................................................
 * Clear status
 */
void CryoCon::clearStatus()
{
  std::ostringstream os;
  
  os << "*CLS";
  sendDeviceCommand(os.str());
  // does not expect response;

  return;
};

/**.......................................................................
 * Reset the module
 */
void CryoCon::resetModule()
{
  std::ostringstream os;

  os << "*RST";
  sendDeviceCommand(os.str());
  return;
};

/**.......................................................................
 * Stop the control Loop
 */
void CryoCon::stopControlLoop()
{
  std::ostringstream os;

  os << "STOP";
  sendDeviceCommand(os.str());
  return;
};

/**.......................................................................
 * Engage Control Loop
 */
void CryoCon::engageControlLoop()
{
  std::ostringstream os;

  os << "CONTROL";
  sendDeviceCommand(os.str());
  return;
};

/**.......................................................................
 * Set Sky Temperature
 */
void CryoCon::setSkyTemp(int loopNum, float val)
{
  std::ostringstream os;

  os << "LOOP " << loopNum << ":SETPT " << val;
  sendDeviceCommand(os.str());
  return;
};

/**.......................................................................
 * Set Source Channel
 */
void CryoCon::setSourceChannel(int loopNum, int val)
{
  std::ostringstream os;

  /* val should be 2 for channel B, otherwise it defaults to channel A */

  switch(val){
  case 2:
    {
      os << "LOOP " << loopNum << ":SOUR CHB";
    };
    break;

  default:
    {
      os << "LOOP " << loopNum << ":SOUR CHA";
    };
    break;
  };
  sendDeviceCommand(os.str());
  return;
};

/**.......................................................................
 * Set Loop Range
 */
void CryoCon::setLoopRange(int loopNum, int val)
{
  std::ostringstream os;
  std::ostringstream os2;
  std::ostringstream os3;

  /* val should be 0 for low range, 
     1 for mid range,
     2 for hi range */

  switch(val){
  case 0:
    {
      os << "LOOP " << loopNum << ":RANGE LOW";
      os2 << "LOOP " << loopNum << ":MAXSET 100";
      os3 << "LOOP " << loopNum << ":MAXPWR 50";
    };
    break;

  case 1:
    {
      os << "LOOP " << loopNum << ":RANGE MID";
      os2 << "LOOP " << loopNum << ":MAXSET 100";
      os3 << "LOOP " << loopNum << ":MAXPWR 60";
    };
    break;

  case 2:
    {
      os << "LOOP " << loopNum << ":RANGE HI";
      os2 << "LOOP " << loopNum << ":MAXSET 100";
      os3 << "LOOP " << loopNum << ":MAXPWR 5";
    };
    break;

  default:
    {
      os << "LOOP " << loopNum << ":RANGE MID";
    };
    break;
  };
  sendDeviceCommand(os.str());
  sendDeviceCommand(os2.str());
  sendDeviceCommand(os3.str());
  return;
};

/**.......................................................................
 * Set P Gain
 */
void CryoCon::setPGain(int loopNum, float val)
{
  std::ostringstream os;

  os << "LOOP " << loopNum << ":PGAIN " << val;
  sendDeviceCommand(os.str());
  return;
};

/**.......................................................................
 * Set I Gain
 */
void CryoCon::setIGain(int loopNum, float val)
{
  std::ostringstream os;

  os << "LOOP " << loopNum << ":IGAIN " << val;
  sendDeviceCommand(os.str());
  return;
};

/**.......................................................................
 * Set D Gain
 */
void CryoCon::setDGain(int loopNum, float val)
{
  std::ostringstream os;

  os << "LOOP " << loopNum << ":DGAIN " << val;
  sendDeviceCommand(os.str());
  return;
};


/**.......................................................................
 * Set Control Loop Manual Power Output Setting
 */
void CryoCon::setPowerOutput(int loopNum, float val)
{
  std::ostringstream os;

  os << "LOOP " << loopNum << ":PMANUAL " << val;
  sendDeviceCommand(os.str());
  return;
};

/**.......................................................................
 * Set Heater Load
 */
void CryoCon::setHeaterLoad(int loopNum, float val)
{
  std::ostringstream os;

  os << "LOOP " << loopNum << ":LOAD " << val;
  sendDeviceCommand(os.str());
  return;
};

/**.......................................................................
 * Set Loop Type
 */
void CryoCon::setControlLoopType(int loopNum, int val)
{
  std::ostringstream os;
  /* val: 0 - off
          1 - PID
	  2 - Manual
	  3 - Table
	  4 - RampP  */

  switch(val){
  case 0:
    {
      os << "LOOP " << loopNum << ":TYPE Off";
    };
    break;

  case 1:
    {
      os << "LOOP " << loopNum << ":TYPE PID";
    };
    break;

  case 2:
    {
      os << "LOOP " << loopNum << ":TYPE Man";
    };
    break;

  case 3:
    {
      os << "LOOP " << loopNum << ":TYPE Table";
    };
    break;

  case 4:
    {
      os << "LOOP " << loopNum << ":TYPE RampP";
    };
    break;

  default:
    {
      os << "LOOP " << loopNum << ":TYPE Man";
    };
    break;
  };
  sendDeviceCommand(os.str());
  return;
};

/**.......................................................................
 * Query the temperature
 */
float CryoCon::queryChannelTemperature(int val)
{
  /* val - 0:  channel A
     val - 1:  channel B
     default:  channel A */
  std::string retVal;
  std::ostringstream os;
  float chanTemp;

  switch(val){
  case 1:
    {
      os << "INP B:TEMP?";
    };
    break;
    
  default:
    {
      os << "INP A:TEMP?";
    };
    break;
  };

  sendDeviceCommand(os.str(), true, GpibUsbController::checkString, true, (void*)&retVal);
//  COUT("Got cryocon retVal = " << retVal);
  String respString(retVal);
  chanTemp = respString.findNextStringSeparatedByChars(", ").toFloat();

  return chanTemp;
};

/**.......................................................................
 * Query the temperature
 */
float CryoCon::queryHeaterCurrent()
{
  std::string retVal;
  std::ostringstream os;
  float heaterCurrent;
  int position;
  os << "LOOP:HTRR?";

  sendDeviceCommand(os.str(), true, GpibUsbController::checkString, true, (void*)&retVal);
//    COUT("Got heater retVal = " << retVal);
   	 
  position=retVal.find("%");	
//  COUT("Got heater posn = " << position);
  retVal[position]='\0';	
  //  COUT("Got heater retVal = " << retVal);
  istringstream iss(retVal);
  iss >> heaterCurrent;
  //String respString(retVal);
  //heaterCurrent = respString.findNextStringSeparatedByChars(", ").toFloat();
//  heaterCurrent = atof(&retVal[position-1]);
//  COUT("Got heater retVal = " << heaterCurrent);
  return heaterCurrent;
};

