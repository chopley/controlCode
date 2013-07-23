#include "gcp/util/common/WxServerSA.h"
#include "gcp/util/common/WxDataSA.h"
#include "gcp/util/common/Ports.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
WxServerSA::WxServerSA(bool spawnThread, int listenPort) : 
  Server(spawnThread, listenPort)
{
  setReadBufSize(data_.size() + 8);
  setSendBufSize(data_.size() + 8);

  // Set up for immediate timeout, to attempt to read from the weather station

  setTimeOutSeconds(0);
}

/**.......................................................................
 * On timeout, we will try to read weather station data from the web
 * server
 */
void WxServerSA::timeOutAction() 
{
  // Attempt to connect to the serial port

  getData();

  COUT("Just read data: " << data_);

  // Send data to connected clients

  sendClientData(data_);

  // And set the timeout to fire again in 10 seconds

  setTimeOutSeconds(10);
}

/**.......................................................................
 * Destructor.
 */
WxServerSA::~WxServerSA() {}

/**.......................................................................
 * Send clients data when they connect
 */
void WxServerSA::acceptClientAction()
{
  sendClientData(data_);
}
