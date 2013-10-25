#include "gcp/antenna/control/specific/RoachBackendMsg.h"
#include "gcp/util/common/DataArray.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/String.h"
#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"


/**
 * @file RoachBackendMsg.c
 * 
 * 
 * @author Stephen Muchovej
 */



using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor initializes request type to invalid and constructs the command map.
 */
RoachBackendMsg::RoachBackendMsg() 
{

  request_ = INVALID;
  packetPtr_ = NULL;
  //  Assign3DVectorMemory();
  Assign2DVectorMemory();

}

/**.......................................................................
 * Destructor
 */
RoachBackendMsg::~RoachBackendMsg() {};


/**
 *  Function to packetize the message
 */
int RoachBackendMsg::packetizeNetworkMsg() 
{
  
  // all this needs to do is take responseReceived_ and work back to the structure
  int lptr;
  int headIndex;
  int i,j,index;

  headIndex = 1;
  lptr = 0;

     // COUT("HEERRRE ");
  // by the wonders of memory allocation, we should be ok.
  version_    = ntohl(packet_.version);
  packetSize_ = ntohl(packet_.data_size);  // size of packet in bytes
  numFrames_  = ntohl(packet_.dataCount);  // number of frames in this packet
  intCount_   = ntohl(packet_.int_count);  // total count of data on fpga
  tstop_      = ntohl(packet_.tend)*CC_TO_SEC;
  intLength_  = ntohl(packet_.int_len)*CC_TO_SEC;
  mode_       = ntohl(packet_.reserved1);
  res2_       = ntohl(packet_.reserved2);

  //  COUT("DATA"<< ntohl(packet_.buffBacklog));
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER;i++){
    bufferBacklog_[i]   = ntohl(packet_.buffBacklog[i]);
    seconds_[i] = ntohl(packet_.tsecond[i]);
    useconds_[i] = ntohl(packet_.tusecond[i]);
//    COUT("DATA"<<i << "---"  << seconds_[i]);
  }
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER;i++){
    tstart_[i] = ntohl(packet_.tstart[i]);
    //COUT("starts" <<i << "---"  << tstart_[i]);
  }
  
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
     // COUT("HERE ");
      switchstatus_[i] = ntohl(packet_.data_switchstatus[i]);
    };

  // next we deal with ch0 (LL)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      LL_[i][2*j]   = ntohl(packet_.data_ch0even[index]);
      LL_[i][2*j+1] = ntohl(packet_.data_ch0odd[index]);
      index++;
    };
  };

  // next we deal with ch1 (Q)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      Q_[i][2*j]   = ntohl(packet_.data_ch1even[index]);
      Q_[i][2*j+1] = ntohl(packet_.data_ch1odd[index]);
      index++;
    };
  };

  // next we deal with ch2 (U)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      U_[i][2*j]   = ntohl(packet_.data_ch2even[index]);
      U_[i][2*j+1] = ntohl(packet_.data_ch2odd[index]);
      index++;
    };
  };

  // next we deal with ch3 (RR)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      RR_[i][2*j]   = ntohl(packet_.data_ch3even[index]);
      RR_[i][2*j+1] = ntohl(packet_.data_ch3odd[index]);
      index++;
    };
  };

  // next we deal with ch4 (TL1)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      TL1_[i][2*j]   = ntohl(packet_.data_ch4even[index]);
      TL1_[i][2*j+1] = ntohl(packet_.data_ch4odd[index]);
      index++;
    };
  };

  // next we deal with ch5 (TL2)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      TL2_[i][2*j]   = ntohl(packet_.data_ch5even[index]);
      TL2_[i][2*j+1] = ntohl(packet_.data_ch5odd[index]);
      index++;
    };
  };
  


#if(0)  // THIS IS FOR THE 3D STUFF
  // next we deal with ch0 (LL)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      LL_[i][j][0] = ntohl(packet_.data_ch0even[index]);
      LL_[i][j][1] = ntohl(packet_.data_ch0odd[index]);
      index++;
    };
  };

  // next we deal with ch1 (Q)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      Q_[i][j][0] = ntohl(packet_.data_ch1even[index]);
      Q_[i][j][1] = ntohl(packet_.data_ch1odd[index]);
      index++;
    };
  };

  // next we deal with ch2 (U)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      U_[i][j][0] = ntohl(packet_.data_ch2even[index]);
      U_[i][j][1] = ntohl(packet_.data_ch2odd[index]);
      index++;
    };
  };

  // next we deal with ch3 (RR)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      RR_[i][j][0] = ntohl(packet_.data_ch3even[index]);
      RR_[i][j][1] = ntohl(packet_.data_ch3odd[index]);
      index++;
    };
  };

  // next we deal with ch4 (TL1)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      TL1_[i][j][0] = ntohl(packet_.data_ch4even[index]);
      TL1_[i][j][1] = ntohl(packet_.data_ch4odd[index]);
      index++;
    };
  };

  // next we deal with ch5 (TL2)
  index = 0;
  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER; i++){
    for (j=0;j<NUM_CHANNELS_PER_BAND;j++){
      TL2_[i][j][0] = ntohl(packet_.data_ch5even[index]);
      TL2_[i][j][1] = ntohl(packet_.data_ch5odd[index]);
      index++;
    };
  };
  
#endif 

}

/**.......................................................................
 * Assigns our 3D vector memory so we don't have to do in the constructor definition
 */

void RoachBackendMsg::Assign3DVectorMemory()
{

#if(0)  
  tstart_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  switchstatus_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  seconds_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  // Set up sizes.
  LL_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    LL_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      LL_[i][j].resize(NUM_PARITY);
  }

  RR_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    RR_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      RR_[i][j].resize(NUM_PARITY);
  }

  Q_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    Q_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      Q_[i][j].resize(NUM_PARITY);
  }

  U_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    U_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      U_[i][j].resize(NUM_PARITY);
  }

  TL1_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    TL1_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      TL1_[i][j].resize(NUM_PARITY);
  }

  TL2_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    TL2_[i].resize(NUM_CHANNELS_PER_BAND);
    for (int j = 0; j < NUM_CHANNELS_PER_BAND; ++j)
      TL2_[i][j].resize(NUM_PARITY);
  }

#endif
};

/**.......................................................................
 * Assigns our 3D vector memory so we don't have to do in the constructor definition
 */

void RoachBackendMsg::Assign2DVectorMemory()
{
  
  tstart_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  bufferBacklog_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  switchstatus_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  seconds_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  useconds_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);

  // Set up sizes.
  LL_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    LL_[i].resize(CHANNELS_PER_ROACH);
  }

  RR_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    RR_[i].resize(CHANNELS_PER_ROACH);
  }

  Q_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    Q_[i].resize(CHANNELS_PER_ROACH);
  }

  U_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    U_[i].resize(CHANNELS_PER_ROACH);
  }

  TL1_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    TL1_[i].resize(CHANNELS_PER_ROACH);
  }

  TL2_.resize(NUM_ROACH_INTEGRATION_PER_TRANSFER);
  for (int i = 0; i < NUM_ROACH_INTEGRATION_PER_TRANSFER; ++i) {
    TL2_[i].resize(CHANNELS_PER_ROACH);
  }


};


/**.......................................................................
 * Prints out the data in the message
 */

void RoachBackendMsg::PrintData()
{

  int i,j,k;

  for(i=0;i<NUM_ROACH_INTEGRATION_PER_TRANSFER;i++){
    for(j=0;j<CHANNELS_PER_ROACH;j++){
        COUT("LL[i][j].  i: "  << i << ", j: " << j << " val: " << LL_[i][j]);
	//        COUT("RR[i][j].  i: "  << i << ", j: " << j << " k: " << k << " val: " << RR_[i][j][k]);
	//        COUT("U[i][j].  i: "   << i << ", j: " << j << " k: " << k << " val: " << U_[i][j][k]);
	//        COUT("Q[i][j].  i: "   << i << ", j: " << j << " k: " << k << " val: " << Q_[i][j][k]);
	//        COUT("TL1[i][j].  i: " << i << ", j: " << j << " k: " << k << " val: " << TL1_[i][j][k]);
	//        COUT("TL2[i][j].  i: " << i << ", j: " << j << " k: " << k << " val: " << TL2_[i][j][k]);
	//      }
    };
  };


  COUT("version: " << version_);
  COUT("numFrames: " << numFrames_);
  COUT("packetSize:" << packetSize_);

  return;
};
