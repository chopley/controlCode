#include "gcp/util/common/WxServer.h"
#include "gcp/util/common/WxData.h"
#include "gcp/util/common/Ports.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
WxServer::WxServer(bool spawnThread, std::string port, int baudRate, 
		   int listenPort) : 
  Server(spawnThread, listenPort), port_(port, baudRate, true, true)
{
  setReadBufSize(data_.size() + 8);
  setSendBufSize(data_.size() + 8);

  // Set up for immediate timeout, to attempt to connect to the serial
  // port

  setTimeOutSeconds(0);
}

/**.......................................................................
 * Set up our port connection to the weather stations
 */
void WxServer::initPort()
{
  // This is necessary or else the returned string may contain
  // characters like '\r' which will cause subsequent string
  // comparisons by the likes of strstr(), etc., to fail.
  
  port_.stripUnprintable(true);
   
  // The weather station should terminate lines with CRLF
  
  port_.terminateAt("\n");
}

/**.......................................................................
 * On timeout, we will try to connect to the serial port
 */
void WxServer::timeOutAction() 
{
  // Attempt to connect to the serial port

  if(port_.connect() > 0) {

    initPort();

    fdSet_.registerReadFd(port_.getFd());
    timeOut_.activate(false);

  } else {
    setTimeOutSeconds(60);
  }
}

/**.......................................................................
 * Destructor.
 */
WxServer::~WxServer() {}

/**.......................................................................
 * Send clients data when they connect
 */
void WxServer::acceptClientAction()
{
  sendClientData(data_);
}

/**.......................................................................
 * Check the serial port for data
 */
void WxServer::checkOtherFds() 
{
  // See if new data are available on the serial port

  if(port_.getFd() > 0 && fdSet_.isSetInRead(port_.getFd())) {

    // Concatenate data until carriage return read (newline will terminate read)

    os_ << port_.readString();

    // If we have not yet read a complete record, discard until we
    // reach the beginning of the next one

    if(os_.str().find("01+") != 0) {
      os_.str("");
      return;
    }
  
    // Keep appending util we have all parameters 1 - 12
      
    if(os_.str().find("01+") == 0 && 
       os_.str().find(" 12+") != std::string::npos) {
      
      // Parse the record and send the data to parent
      
      readRecord();

      // Send any connected clients the new data

      sendClientData(data_);

      // And reset the string

      os_.str("");
    }
  }
}  

/**.......................................................................
 * Parse a record from the weather station
 */
void WxServer::readRecord()
{
  os_ << " "; // Add delimiting space after last parameter

  String str(os_.str());
  String val;

  val = str.findNextInstanceOf("02", " ");
  data_.year_ = atoi((char*)val.str().c_str());
  
  val = str.findNextInstanceOf("03", " ");
  data_.day_ = atoi((char*)val.str().c_str());
  
  val = str.findNextInstanceOf("04", " ");
  data_.hour_ = atoi((char*)val.str().substr(0,3).c_str());
  data_.min_  = atoi((char*)val.str().substr(3,2).c_str());
  
  val = str.findNextInstanceOf("05", " ");
  data_.internalTemperatureC_ = atof((char*)val.str().c_str());
  
  val = str.findNextInstanceOf("06", " ");
  data_.batteryVoltage_ = atof((char*)val.str().c_str());

  val = str.findNextInstanceOf("07", " "); 
  data_.airTemperatureC_ = atof((char*)val.str().c_str());
  
  val = str.findNextInstanceOf("08", " ");
  data_.relativeHumidity_ = atof((char*)val.str().c_str())/100;
  
  val = str.findNextInstanceOf("09", " ");
  data_.windSpeed_ = atof((char*)val.str().c_str());
  
  val = str.findNextInstanceOf("10", " ");
  data_.windDirectionDegrees_ = atof((char*)val.str().c_str());
  
  val = str.findNextInstanceOf("11", " ");
  data_.pressure_ = atof((char*)val.str().c_str()); 

  val = str.findNextInstanceOf("12", " ");
  data_.power_ = static_cast<unsigned short>(atof((char*)val.str().c_str())); 

  data_.received_ = true;
}
