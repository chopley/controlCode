#include "gcp/receiver/specific/DioBoloClient.h"
#include "gcp/receiver/specific/BoloPixelManager.h"

#include "gcp/receiver/specific/BolometerDataFrameManager.h"

#include "gcp/util/common/ArrayMapDataFrameManager.h"


#include <iomanip>

using namespace std;
using namespace gcp::receiver;
using namespace gcp::util;

using namespace MuxReadout;

/**.......................................................................
 * Constructor.
 */
DioBoloClient::DioBoloClient(std::string dioHost, unsigned short dioPort,
			     std::string hwHost,  unsigned short hwPort,
			     unsigned nSamplesPerSecond, unsigned npix) :
  DioClient(dioHost, dioPort, hwHost, hwPort, nSamplesPerSecond, true) 
{
  nPixel_         = npix;
  frameUtcPtr_    = 0;
  frameRecordPtr_ = 0;
  boloUtcPtr_     = 0;
  acPtr_          = 0;
  xyPtr_          = 0;

  nRecord_ = 0;

  // Initialize this client

  initialize();

  // Set up data acquisition pointers 

  switchBuffers();
}

/**.......................................................................
 * Initialize the list of channels from the HardwareManager
 */
void DioBoloClient::initializeChannels()
{
  // Initialize the pixel map from the hardware manager
  
  BoloPixelManager man(hwHost_, hwPort_);

  std::vector<BoloPixel> pixels = man.getPixels();
  
  // Store the number of pixels
  
  nPixel_ = nPixel_ > 0 ? nPixel_ : pixels.size();
  
  // And assign a column for each pixel 

  ReportMessage("Hardware manager reports there are: " 
		<< pixels.size() << " readout channels");
  
  for(unsigned iPix=0; iPix < nPixel_; iPix++) {
    addChannel(pixels[iPix].getBoloReadoutChannel());
    bufferId(pixels[iPix].getBoloReadoutChannel(), iPix);
    bufferPixelXY(pixels[iPix].getPixelXY(), iPix);
  }
}

void DioBoloClient::bufferId(std::string id, unsigned iBolo)
{
  strncpy((char*)(ids_+iBolo*DIO_ID_LEN), (char*)id.c_str(), DIO_ID_LEN);
}

void DioBoloClient::bufferPixelXY(std::vector<double> xy, unsigned iBolo)
{
  xy_[2*iBolo]   = xy[0];
  xy_[2*iBolo+1] = xy[1];
}

/**.......................................................................
 * Get the frame, and set up pointers to memory locations in the frame
 * to which we will buffer data
 */
void DioBoloClient::switchBuffers()
{
  // Release the buffer we have been writing to

  buffer_.releaseWriteBuffer();

  // Swap the double buffers

  buffer_.switchBuffers();

  // Get a reference to the frame to which we will write data next

  frameManager_ = (BolometerDataFrameManager*) buffer_.grabWriteBuffer();

  // Store pointers to relevant memory locations in the frame

  frameUtcPtr_    = frameManager_->getDatePtr(  "frame",      "utc");
  frameRecordPtr_ = frameManager_->getUintPtr(  "frame",      "record");
  boloUtcPtr_     = frameManager_->getDatePtr(  "bolometers", "utc");
  acPtr_          = frameManager_->getFloatPtr( "bolometers", "adc");
  xyPtr_          = frameManager_->getDoublePtr("bolometers", "xy");
}

/**.......................................................................
 * Destructor.
 */
DioBoloClient::~DioBoloClient() {}

/**.......................................................................
 * Add a single sample of DIO data to the buffer
 */
void DioBoloClient::addSampleToBuffer(RegDate& date,
				      vector<MuxReadout::DIODatum>& data,
				      bool isValid)
{
  // Buffer the time

  *(boloUtcPtr_ + sampleCount_) = *date.data();

  // Buffer the fast bolo data

  if(batch_) {
    
    for(unsigned i=0; i < data[1].batchData.size(); i++) {
      *(acPtr_+ i * nSamplesPerSecond_ + sampleCount_) = 
	isValid ? (float)data[1].batchData[i] : NAN;
    }

  } else {
    
    for(unsigned i=1; i < data.size(); i++) 
      *(acPtr_+ (i-1) * nSamplesPerSecond_ + sampleCount_) = 
	isValid ? (float)data[i].data : NAN;
    
  }
    
  // If this is the last sample for this second, write the timestamp
  // and the record count into the frame as well

  if(sampleCount_ == nSamplesPerSecond_-1) {

    *(frameUtcPtr_)    =   *date.data();
    *(frameRecordPtr_) = ++nRecord_;

    //    COUT("Finished bolo frame: " << data[0].timeStamp->ISO_8601() 
    //	 << " :" << *acPtr_ << endl);

    // Switch buffers and reset pointers

    switchBuffers();

  } else if(sampleCount_ == 0) {
    //COUT("Starting bolo frame: " << data[0].timeStamp->ISO_8601());
  }
}

/**.......................................................................
 * Lock and return the read buffer
 */
RegMapDataFrameManager* DioBoloClient::grabReadFrame()
{
  return (RegMapDataFrameManager*)buffer_.grabReadBuffer();
}

/**.......................................................................
 * Unlock the read buffer
 */
void DioBoloClient::releaseReadFrame()
{
  buffer_.releaseReadBuffer();
}

/**.......................................................................
 * Overload the baseclass method to archive the filter values
 */
void DioBoloClient::archiveFilterValues(gcp::util::Frequency& freq, 
					unsigned ntaps)
{
  filterFreqHz_ = (float)freq.Hz();
  filterNtaps_  = ntaps;
}

/**.......................................................................
 * Overload the baseclass method to archive the subsampling rate
 */
void DioBoloClient::archiveSubSampling(unsigned sampling)
{
  subSampling_ = sampling;
}

/**.......................................................................
 * Copy stored values of persistent items into the register
 * frame
 */
void DioBoloClient::
copyPersistentRegs(gcp::util::ArrayMapDataFrameManager* frame)
{
  frame->writeReg("receiver", "bolometers", "id",          ids_);
  frame->writeReg("receiver", "bolometers", "cutOffFreq",  &filterFreqHz_);
  frame->writeReg("receiver", "bolometers", "ntap",        &filterNtaps_);
  frame->writeReg("receiver", "bolometers", "subSampling", &subSampling_);
  frame->writeReg("receiver", "bolometers", "xy",          xy_);
}

/**.......................................................................
 * Overload the base-class method to set up the filter function for
 * this client
 */
void DioBoloClient::setUpFilter()
{
  // Set up a sinc filter on the server side
  
  setFilter(defaultSincFilterCutoffFreq_, defaultNTaps_);
}
