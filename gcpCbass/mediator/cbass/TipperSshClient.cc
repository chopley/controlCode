#include "gcp/mediator/specific/TipperSshClient.h"

#include<iostream>

using namespace std;

using namespace gcp::mediator;

/**.......................................................................
 * Constructors
 */
TipperSshClient::
TipperSshClient(gcp::util::RegMapDataFrameManager* share,
		std::string gateway, std::string host,
		unsigned short port) :
  gcp::util::TipperSshClient(true, gateway, host, port)
{
  share_     = 0;
  utcBlock_  = 0;
  tauBlock_  = 0;
  tAtmBlock_ = 0;

  // Now assign pointers

  if(share) {
    share_     = share;
    
    utcBlock_  = share_->findReg("tipper", "utc");
    tauBlock_  = share_->findReg("tipper", "tau");
    tAtmBlock_ = share_->findReg("tipper", "tatm");
    
    if(utcBlock_ == 0 || tauBlock_ == 0 || tAtmBlock_ == 0)
      ThrowError("Unable to find tipper registers");
  }
}

/**.......................................................................
 * Destructor.
 */
TipperSshClient::~TipperSshClient() {}

/**.......................................................................
 * Method called when data have been completely read from the server
 */
void TipperSshClient::readServerData(gcp::util::NetHandler& handler) 
{
  // Get the data from the server
  
  gcp::util::TipperSshClient::readServerData(handler);

  // Convert to default time representation

  gcp::util::RegDate date(tipperData_.mjdDays_, tipperData_.mjdMs_);

  if(share_) {
    share_->writeReg(utcBlock_,  date.data());
    share_->writeReg(tauBlock_,  tipperData_.tau_);
    share_->writeReg(tAtmBlock_, tipperData_.tAtm_);
  }
}

void TipperSshClient::reportError()
{
  static unsigned counter=0;
  if(++counter % 10 == 0)
    ReportSimpleError("Unable to connect to tipper server at: " << host_ 
		      << ":" << port_);
}
