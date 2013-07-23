#include "gcp/receiver/specific/DioSquidClient.h"
#include "gcp/receiver/specific/SquidDataFrameManager.h"

#include "gcp/util/common/RegDate.h"

#include "Utilities/HardwareManagerClient.h"

using namespace std;
using namespace gcp::receiver;
using namespace gcp::util;
using namespace MuxReadout;

/**.......................................................................
 * Constructor.
 */
DioSquidClient::DioSquidClient(std::string dioHost, unsigned short dioPort,
		     std::string hwHost,  unsigned short hwPort,
		     unsigned nSamplesPerSecond) :
  DioClient(dioHost, dioPort, hwHost, hwPort, nSamplesPerSecond, true)
{
  initialize();
}

/**.......................................................................
 * Destructor.
 */
DioSquidClient::~DioSquidClient() {}


/**.......................................................................
 * Read the next sample from the server and buffer it
 */
void DioSquidClient::bufferData(vector<MuxReadout::DIODatum>& data) 
{ 
  static RegDate date;

  date.setMjd(data[0].timeStamp->mjd(), true);

  addSampleToBuffer(date, data, true);
}

/**.......................................................................
 * Add a single sample of DIO data to the buffer
 */
void DioSquidClient::addSampleToBuffer(RegDate& date,
				       vector<MuxReadout::DIODatum>& data,
				       bool isValid)
{
  frame_.setMjd(date);
  frame_.incrementRecord();

  for(unsigned i=0; i < data[1].batchData.size(); i++)
    frame_.bufferVolts(isValid ? data[1].batchData[i] : NAN, i);
}

/**.......................................................................
 * Reset the buffer pointers
 */
void DioSquidClient::resetBufferPointers() {}

/**.......................................................................
 * Define what happens when the subsampling is set for this client
 */
void DioSquidClient::archiveSubSampling(unsigned sampling)
{
  frame_.setSubSampling(sampling);
}

/**.......................................................................
 * Initialize the list of channels from the HardwareManager
 */
void DioSquidClient::initializeChannels()
{
  // Initialize the pixel map from the hardware manager
  
  MuxReadout::HardwareManagerClient hwClient(hwHost_, hwPort_, true);

  if(hwClient.connect(false)) {
    squids_ = hwClient.getSQChannels();
    haveSquidChannels_ = true;

  } else {
    ThrowError("Couldn't connect to the hardware manager daemon");
  }

  // And assign a channel for each squid
  
  ReportMessage("Hardware manager reports there are: " 
		<< squids_.size() << " squid channels");
  
  for(unsigned iSquid=0; iSquid < squids_.size(); iSquid++) {
    addChannel(squids_[iSquid]);
    frame_.bufferId(squids_[iSquid], iSquid);
  }

}

/**.......................................................................
 * Overload the base-class method to set up the filter function for
 * this client
 */
void DioSquidClient::setUpFilter()
{
  // Set up a unity-gain filter with 1 tap
  
  std::vector<double> filterFunction;

  unsigned ntap=11;

  filterFunction.resize(ntap);

  for(unsigned i=0; i < ntap; i++)
    filterFunction[i] = 1.0/ntap;

  setFilterFunction(filterFunction);

  frame_.setFilterNtap(ntap);
}

BoardDataFrameManager& DioSquidClient::getFrame()
{
  return frame_;
}

