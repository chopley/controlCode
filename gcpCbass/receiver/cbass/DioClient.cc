#include "gcp/receiver/specific/DioClient.h"

#include "gcp/util/common/RegMapDataFrameManager.h"

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"

using namespace std;
using namespace gcp::receiver;
using namespace gcp::util;

using namespace MuxReadout;

const Frequency DioClient::defaultSincFilterCutoffFreq_ = 
Frequency(Frequency::Hertz(), 37.0);

/**.......................................................................
 * Constructor.
 */
DioClient::DioClient(std::string dioHost, unsigned short dioPort,
		     std::string hwHost,  unsigned short hwPort,
		     unsigned nSamplesPerSecond, bool batch) :
  DIOClient_BaseClass(dioHost, dioPort, 0), data_(0)
{
  dioHost_           = dioHost; 
  dioPort_           = dioPort; 

  hwHost_            = hwHost; 
  hwPort_            = hwPort; 

  isConnected_       = false;

  firstSample_       = true;
  needFirstValidSample_  = true;

  sampleCount_       = 0;
  nSamplesPerSecond_ = nSamplesPerSecond;

  batch_ = batch;
  first_ = true;

  // Inheritors are responsible for allocating

  frameManager_      = 0;
}

void DioClient::initialize()
{
  // setSubsampling() expects the number of samples to discard before
  // sending back data.  Thus, if we want 1 sample every 1000 samples,
  // this should be set to 1000-1 = 999

  unsigned nSubSample = 1000/nSamplesPerSecond_-1;

  setSampling(nSubSample);

  // Set up a sinc filter on the server side

  setUpFilter();

  // Add a timestamp column

  addColumn(DIOColumnTokenFactory::calendarTimeStampColumn());

  // Must make this call to initialize the timezone, or we
  // occasionally see the time stamps returned by TimeStamp class
  // change by an hour, presumably because the system timezone is
  // changing under our feet!

  TimeStamp::init();

  // And tell the server to align its samples with the 10ms absolute
  // time boundary

  setAlignTo10ms(true);
}

/**.......................................................................
 * Destructor.
 */
DioClient::~DioClient() {}

/**.......................................................................
 * Add a channel to the list of channels we want to monitor
 */
void DioClient::addChannel(std::string channelSpec)
{
  if(batch_) {
    channels_.push_back(channelSpec);
  } else {
    addColumn(DIOColumnTokenFactory::dataColumn(channelSpec, hwHost_, hwPort_));
  }
}

/**.................................................................................
 * Overload of the base-class so that we can increment the size of our
 * data vector whenever we add a new column
 */
bool DioClient::addColumn(DIOColumnToken column)
{
  data_.resize(data_.size()+1);
  return DIOClient_BaseClass::addColumn(column);
}

void DioClient::configureFilterFunction()
{
  // By default, we will set the filter function to boxcar average
  // over 100 samples

  std::vector<double> filterFunction;

  filterFunction.resize(101);

  for(unsigned i=0; i < 101; i++)
    filterFunction[i] = 0.01;

  // And call the base-class method to set the filter function

  setFilterFunction(filterFunction);
}

/**.......................................................................
 * Overload the base-class method to start.  This is so that we can
 * add a single batch-filtered data column just before we tell the
 * server to start.  This will cause the configuration to be updated
 * and sent to the server.
 */
bool DioClient::start(int sampleCount)
{
  if(batch_) {
    
    // Add the stored a single batch-filtered data column, but only if
    // this is the first time we have been called.
    
    if(first_) {

      addColumn(DIOColumnTokenFactory::
		batchDataColumn(channels_, hwHost_, hwPort_));

      first_ = false;
    }
  }

  // Now call the base-class method to start sending data


  return DIOClient_BaseClass::start(sampleCount);
}

/**.......................................................................
 * Return true if we are connected
 */
bool DioClient::isConnected()
{
  return isConnected_;
}

/**.......................................................................
 * Connect
 */
bool DioClient::connect()
{
  // Initialize the channels we will read

  try {
    initializeChannels();
  } catch(...) {
#if 0
    ReportSimpleError("Unable to connect to the HardwareManager daemon at " 
		      << hwHost_ << ":" << hwPort_ << "\n"
		      << "(" << strerror(errno) << ")");
#endif
    isConnected_ = false;
    return isConnected_;
  }
  
  // If we successfully initialized from the hardware manager, try
  // to connect to the dio server
  
  try {
    if(start())
      isConnected_ = true;
  } catch(...) {
    ReportSimpleError("Unable to connect to the DIOServer daemon at " 
		      << dioHost_ << ":" << dioPort_);
    isConnected_ = false;
    return isConnected_;
  }

  return isConnected_;
}

/**.......................................................................
 * Read the next sample from the server and buffer it
 */
bool DioClient::bufferNextSample()
{
  try {

    if(getNextSample(&data_[0])) {
      bufferData(data_);
      return true;

    } else {
      registerError();
      return false;
    }

  } catch(...) {
    registerError();
    return false;
  }
}

/**.......................................................................
 * Respond to an error condition while contacting the server
 */
void DioClient::registerError()
{
  sampleCount_          = 0;
  firstSample_          = true;
  needFirstValidSample_ = true;
  isConnected_          = false;
}

/**.......................................................................
 * Return the readable frame
 */
RegMapDataFrameManager* DioClient::grabReadFrame() {}

/**.......................................................................
 * Release the readable frame
 */
void DioClient::releaseReadFrame() {}

/**.......................................................................
 * Check if buffered data are ready to be read out
 */
bool DioClient::dataAreReady()
{
  return (sampleCount_ == 0 && !firstSample_);
}

/**.......................................................................
 * Check if buffered data are ready to be read out
 */
bool DioClient::incrementSampleCount()
{
  // increment the sample count

  sampleCount_++;

  // If the sample count has reached the number of samples per second,
  // reset it

  sampleCount_ %= nSamplesPerSecond_;

  // Register that the count has turned over for the first time

  if(firstSample_) 
    firstSample_ = false;
}

/**.......................................................................
 * Most general way of setting up the filter
 */
void DioClient::setFilter(Frequency freq, unsigned ntaps)
{
  sincCutoffFreq_ = freq;
  nTaps_          = ntaps;

  // Set up the sinc filter on the server side

  setSincFilter(freq.Hz(), ntaps);

  // Restart the session with the DIO server if need be

  reconfigureIfNecesary();

  // Archive the new values

  archiveFilterValues(freq, ntaps);
}

void DioClient::setFilterFrequency(Frequency freq)
{
  setFilter(freq, nTaps_);
}

void DioClient::setFilterNtap(unsigned ntaps)
{
  setFilter(sincCutoffFreq_, ntaps);
}

/**.......................................................................
 * Set one or both of the filter frequency/ntap
 */
void DioClient::setFilter(unsigned mask, double freqHz, unsigned ntaps)
{

  if(mask & gcp::control::FILTER_FREQ) {
    Frequency freq(Frequency::Hertz(), freqHz);

    if(mask & gcp::control::FILTER_NTAP) {
      setFilter(freq, ntaps);
    } else {
      setFilterFrequency(freq);
    }
  } else if(mask & gcp::control::FILTER_NTAP) {
      setFilterNtap(ntaps);
  }

}

void DioClient::archiveFilterValues(gcp::util::Frequency& freq, unsigned ntaps)
{}

void DioClient::archiveSubSampling(unsigned sampling) {}

void DioClient::setSampling(unsigned sampling)
{
  setSubsampling(sampling);
  archiveSubSampling(sampling);
}

/**.......................................................................
 * Read the next sample from the server and buffer it
 */
void DioClient::bufferData(vector<MuxReadout::DIODatum>& data) 
{ 
  static RegDate date;
  static TimeVal curr, last, diff;

  // Store the timestamp on this sample

  curr.setMjd(data[0].timeStamp->mjd());

  // Check the timestamp until we hit the first 10ms after an integral
  // second boundary

  if(needFirstValidSample_) {

    // If we are waiting for the first sample, and this one has
    // arrived on the first 10-ms boundary after the integral second,
    // then call it a valid first sample

    if(curr.getMilliSeconds(true)%1000 == 10) {
      needFirstValidSample_ = false;
      sampleCount_ = 0;
    } else {
      return;
    }

    // Else we are currently time-synchronized with the DIO server.
    // Check that this sample arrived 10 ms after the last one

  } else {

    diff = curr - last;

    // If the delta between this and the last sample is greater than
    // 10 ms, it's probably because the DIO server has choked and has
    // restarted our connection.  In this case, we need to
    // re-synchronize

    if(diff.getMilliSeconds(true) > 10) {
      needFirstValidSample_ = true;
      return;
    }
  }

  // Check if we are at 10-ms after the integral second.  If the
  // receiver clock has slewed, this can occur before the end of a
  // frame.  In this case, we buffer the frame with NANs and start a
  // new one.

  date = curr;
  bool first = (curr.getMilliSeconds(true)%1000 == 10);

  // If this should be the first sample in a frame, but it's not,
  // buffer the remainder of the frame with NANs, and start a new
  // frame

  if(first && sampleCount_ != 0) {

    // Pad the frame with invalid flags (NANs)

    do {    
  
      addSampleToBuffer(date, data, false);
      incrementSampleCount();

    } while(sampleCount_ != 0);

  }

  // And buffer this sample.

  addSampleToBuffer(date, data, true);
  incrementSampleCount();

  // Store the current time stamp for comparison next time 'round

  last = curr;
}

