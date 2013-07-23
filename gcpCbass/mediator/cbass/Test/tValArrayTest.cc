#include <iostream>

#include <utility>

#include <vector>
#include <deque>
#include <valarray>

using namespace std;

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/Profiler.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/program/common/Program.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  {"nsamp",    "100000", "i", "Number of samples to iterate over"},
  {END_OF_KEYWORDS}
};

void Program::initializeUsage() {};

struct board_data_raw 
{
  unsigned short int data[16];
  unsigned char sample_index;
  unsigned char sample_type;
  unsigned char board_id;
  unsigned char checksum;
};

valarray<unsigned short> rawDataNoSizeOf(struct board_data_raw& data)
{
  return valarray<unsigned short>(data.data, 16);
}

valarray<unsigned short> rawData(struct board_data_raw& data)
{
  unsigned int byteCount=sizeof(data.data)/sizeof(unsigned short);
  return valarray<unsigned short>(data.data, byteCount);
}

unsigned short* rawDataPtr(struct board_data_raw& data)
{
  return &data.data[0];
}


void executeValArrayLoop(unsigned nsamp)
{
  struct board_data_raw data;

  for(unsigned i=0; i < nsamp; i++) {
    valarray<unsigned short> vArr = rawData(data);
  }
}

void executeRawPtrLoop(unsigned nsamp)
{
  struct board_data_raw data;

  for(unsigned i=0; i < nsamp; i++) {
    unsigned short* ptr = rawDataPtr(data);
  }
}

void executeVecPtrMultLoop(unsigned nsamp)
{
  unsigned short val=1;
  std::vector<float> fVec(100);
  std::vector<float> sum(100);
  float* fVecPtr = &fVec[0];
  float* sumPtr  = &sum[0];

  for(unsigned iSamp=0; iSamp < nsamp; iSamp++) {
    for(unsigned i=0; i < 100; i++) {
      *(sumPtr+i) += *(fVecPtr+i) * val;
    }
  }
}

void executeVecMultLoop(unsigned nsamp)
{
  unsigned short val=1;
  std::vector<float> fVec(100);
  std::vector<float> sum(100);

  for(unsigned iSamp=0; iSamp < nsamp; iSamp++) {
    for(unsigned i=0; i < 100; i++) {
      sum[i] += fVec[i] * val;
    }
  }
}

void executeValArrMultLoop(unsigned nsamp)
{
  unsigned short val=1;
  std::valarray<float> fVec(100);
  std::valarray<float> sum(100);

  for(unsigned iSamp=0; iSamp < nsamp; iSamp++) {
    fVec *= val;
    sum += fVec;
  }
}

int Program::main(void)
{
  gcp::util::Profiler prof1, prof2, prof3;
  std::valarray<float> vals(100);

  vals = 1.0;

  vals *= 2;

  COUT(vals[0]);
  COUT(vals[99]);


  unsigned nsamp = Program::getiParameter("nsamp");

  prof1.start();
  executeVecMultLoop(nsamp);
  prof1.stop();

  prof2.start();
  executeVecPtrMultLoop(nsamp);
  prof2.stop();

  prof3.start();
  executeValArrMultLoop(nsamp);
  prof3.stop();
  
  COUT(endl << 
       "Elapsed times: " << endl << endl
       << " vec ind: " << prof1.diff().getTimeInSeconds() << " seconds" << endl
       << " vec ptr: " << prof1.diff().getTimeInSeconds() << " seconds" << endl
       << " val arr: " << prof3.diff().getTimeInSeconds() << " seconds" << endl
       << std::endl
       << " ratio (1/3): " << prof1.diff().getTimeInSeconds()/prof3.diff().getTimeInSeconds() << endl);

  for(unsigned i=0; i < 100; i++) {
    vals[i] = i;
    COUT(vals[i]);
  }

  float* ptr = &vals[0];
  for(unsigned i=0; i < 100; i++) {
    COUT(*(ptr+i));
  }

  
  return 0;
}

