#ifndef GCP_RECEIVER_DIOCLIENT_H
#define GCP_RECEIVER_DIOCLIENT_H

/**
 * @file DioClient.h
 * 
 * Tagged: Fri Dec  8 22:47:29 PST 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author Erik Leitch
 */
#include "Utilities/DIOClient.h"

#include "gcp/util/common/Frequency.h"
#include "gcp/util/common/RegDate.h"

namespace gcp {

  namespace util {
    class BoardDataFrameManager;
    class RegMapDataFrameManager;
    class ArrayMapDataFrameManager;
  }

  namespace receiver {

      class DioClient : public MuxReadout::DIOClient_BaseClass {
      public:

	// Constructor.

	DioClient(std::string dioHost, unsigned short dioPort,
		  std::string hwHost,  unsigned short hwPort,
		  unsigned int nSamplesPerSecond, bool batch=false);
	
	// Destructor.

	virtual ~DioClient();

	void initialize();

	bool connect();
	bool isConnected();
	bool dataAreReady();

	bool bufferNextSample();

	virtual gcp::util::RegMapDataFrameManager* grabReadFrame();
	virtual void releaseReadFrame();

	virtual gcp::util::BoardDataFrameManager& getFrame() {};
	
	// The default cutoff frequency and ntap for the sinc filter

	static const gcp::util::Frequency defaultSincFilterCutoffFreq_;
	static const unsigned defaultNTaps_ = 129;

	// Methods to set the subsampling

	void setSampling(unsigned sampling);
	virtual void archiveSubSampling(unsigned sampling);

	// Methods to set the filtering function

	void setFilter(gcp::util::Frequency freq, unsigned ntaps);
	void setFilterFrequency(gcp::util::Frequency freq);
	void setFilterNtap(unsigned ntaps);
	void setFilter(unsigned mask, double freqHz, unsigned ntaps);

	// Called in initialize to set up the filter for this client

	virtual void setUpFilter() {};

	// Called to archive filter values

	virtual void archiveFilterValues(gcp::util::Frequency& freq, 
					 unsigned ntaps);

	// Copy stored values of persistent items into the register
	// frame
      
	virtual void 
	  copyPersistentRegs(gcp::util::ArrayMapDataFrameManager* frame) {};

      protected:

	gcp::util::Frequency sincCutoffFreq_;
	unsigned nTaps_;

	void addBatchChannel(std::string channelSpec);

	void addChannel(std::string channelSpec);

	// Overload the base class method to add a data column

	bool addColumn(MuxReadout::DIOColumnToken column);

	// Initialize the channels we will acquire

	virtual void initializeChannels() {};

	// Overload the base-class method to start

	bool start(int sampleCount=-1);

	virtual void bufferData(vector<MuxReadout::DIODatum>& data);

	virtual void addSampleToBuffer(gcp::util::RegDate& date,
				       std::vector<MuxReadout::DIODatum>& data,
				       bool isValid) {};

	std::string hwHost_;
	unsigned short hwPort_;

	std::string dioHost_;
	unsigned short dioPort_;

	unsigned sampleCount_;
	unsigned nSamplesPerSecond_;
	unsigned nRecord_;

	bool firstSample_;
	bool needFirstValidSample_;

	gcp::util::RegMapDataFrameManager* frameManager_;

	bool batch_;
	bool first_;

      private:

	void registerError();
	void configureFilterFunction();
	bool incrementSampleCount();

	bool isConnected_;

      public:

	std::vector<std::string> channels_;

	std::vector<MuxReadout::DIODatum> data_;
	
      }; // End class DioClient

  } // End namespace receiver
} // End namespace gcp



#endif // End #ifndef GCP_RECEIVER_DIOCLIENT_H
