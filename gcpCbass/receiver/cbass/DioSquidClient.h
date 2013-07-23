#ifndef GCP_RECEIVER_DIOSQUIDCLIENT_H
#define GCP_RECEIVER_DIOSQUIDCLIENT_H

/**
 * @file DioSquidClient.h
 * 
 * Tagged: Fri Dec  8 22:47:29 PST 2006
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/BoardDataFrameManager.h"

#include "gcp/receiver/specific/DioClient.h"
#include "gcp/receiver/specific/SquidDataFrameManager.h"

namespace gcp {
  namespace receiver {
    
    class DioSquidClient : public DioClient {
    public:
      
      // Constructor.

      DioSquidClient(std::string dioHost, unsigned short dioPort,
		     std::string hwHost,  unsigned short hwPort,
		     unsigned int nSamplesPerSecond);
      
      // Destructor.

      virtual ~DioSquidClient();
      
      // Initialize the list of channels from the HardwareManager

      void initializeChannels();

      // Overload the base-class method to set up the filter function
      // for this client

      void setUpFilter();

      gcp::util::BoardDataFrameManager& getFrame();

      // Copy stored values of persistent items into the register
      // frame
      
    private:

      std::vector<std::string> squids_;
      bool haveSquidChannels_;

      SquidDataFrameManager frame_;
      float* dcPtr_;
      unsigned filterNtaps_;
      unsigned subSampling_;

      void bufferData(std::vector<MuxReadout::DIODatum>& data);

      // Add a single sample of DIO data to the buffer
      
      void addSampleToBuffer(gcp::util::RegDate& date,
			     std::vector<MuxReadout::DIODatum>& data,
			     bool isValid);

      // Reset pointers to memory where data are being buffered

      void resetBufferPointers();

      // Define what happens when the subsampling is set for this
      // client

      void archiveSubSampling(unsigned sampling);
      
    }; // End class DioSquidClient
    
  } // End namespace receiver
} // End namespace gcp



#endif // End #ifndef GCP_RECEIVER_DIOSQUIDCLIENT_H
