#ifndef GCP_RECEIVER_DIOBOLOCLIENT_H
#define GCP_RECEIVER_DIOBOLOCLIENT_H

/**
 * @file DioBoloClient.h
 * 
 * Tagged: Fri Dec  8 22:47:29 PST 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/RegDate.h"

#include "gcp/receiver/specific/BoloDoubleBuffer.h"
#include "gcp/receiver/specific/DioClient.h"
#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

#define BOLO_DEBUG

namespace gcp {
  namespace receiver {
    
    class DioBoloClient : public DioClient {
    public:
      
      // Constructor.

      DioBoloClient(std::string dioHost, unsigned short dioPort,
		    std::string hwHost,  unsigned short hwPort,
		    unsigned int nSamplesPerSecond,
		    unsigned npix);
      
      // Destructor.

      virtual ~DioBoloClient();
      
      // Add a single sample of DIO data to the buffer

      void addSampleToBuffer(gcp::util::RegDate& date,
			     vector<MuxReadout::DIODatum>& data,
			     bool isValid);

      gcp::util::RegMapDataFrameManager* grabReadFrame();

      void releaseReadFrame();

      // Over load the base-class method to initialize the channels we
      // will read from

      void initializeChannels();

      // Overload the base-class method to set up the filter function
      // for this client

      void setUpFilter();

      // Overload the baseclass method to archive the filter values

      void archiveFilterValues(gcp::util::Frequency& freq, unsigned ntaps);

      // Overload the baseclass method to archive the subsampling rate

      void archiveSubSampling(unsigned sampling);

      // Copy stored values of persistent items into the register
      // frame
      
      void copyPersistentRegs(gcp::util::ArrayMapDataFrameManager* frame);

    private:

      // Switch buffers in our double-buffering scheme and reset
      // pointers

      void switchBuffers();

      void bufferId(std::string id, unsigned iBolo);
      void bufferPixelXY(std::vector<double> xy, unsigned iBolo);

      // The double buffer to which we will record data

      BoloDoubleBuffer buffer_;

      // Pointers to specific memory locations in the frame to which
      // we will record data

      gcp::util::RegDate::Data* frameUtcPtr_;
      unsigned int* frameRecordPtr_;
      gcp::util::RegDate::Data* boloUtcPtr_;
      float* acPtr_;
      float* dtPtr_;
      double* xyPtr_;

      unsigned nPixel_;

      float filterFreqHz_;
      unsigned filterNtaps_;
      unsigned subSampling_;
      unsigned char ids_[NUM_BOLOMETERS * DIO_ID_LEN];
      double xy_[NUM_BOLOMETERS * 2];

    }; // End class DioBoloClient
    
  } // End namespace receiver
} // End namespace gcp



#endif // End #ifndef GCP_RECEIVER_DIOBOLOCLIENT_H
