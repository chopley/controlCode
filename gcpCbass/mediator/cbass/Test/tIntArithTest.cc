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

void doFloatSum(unsigned nsamp)
{
  unsigned short val=1;
  double sum=0.0;
  for(unsigned i=0; i < nsamp; i++)
    sum += 1.0*val;
}

void doIntegerSum(unsigned nsamp)
{
  unsigned short val=1;
  unsigned int sum=0;
  for(unsigned i=0; i < nsamp; i++)
    sum += 1*val;
}

void doIntegerVecSum(unsigned nsamp)
{
  std::vector<unsigned short> sVec(100);

  unsigned int sum=0;
  unsigned int index=0;
  unsigned short* sptr=&sVec[0];

  for(unsigned i=0; i < nsamp; i++) {
    index = i%100;
    //sum += 1*sVec[index];
    sum += 1*(*(sptr+index));
  }
}

void doDoubleVecSum(unsigned nsamp)
{
  std::vector<double> dVec(100);

  double sum=0.0;
  unsigned int index=0;
  double* dptr=&dVec[0];

  for(unsigned i=0; i < nsamp; i++) {
    index = i%100;
    //sum += 1*dVec[index];
    sum += 1*(*(dptr+index));
  }

}

int Program::main(void)
{
  gcp::util::Profiler prof1, prof2, prof3;

  unsigned nsamp = Program::getiParameter("nsamp");

  prof1.start();
  doDoubleVecSum(nsamp);
  prof1.stop();

  prof2.start();
  doIntegerVecSum(nsamp);
  prof2.stop();

  COUT(endl << 
       "Elapsed times: " << endl << endl
       << " Double vec: " << prof1.diff().getMilliSeconds() << " mseconds" << endl
       << " Int Ptr:    " << prof2.diff().getMilliSeconds() << " mseconds" << endl
       << std::endl
       << " ratio (1/2): " << prof1.diff().getTimeInSeconds()/prof2.diff().getTimeInSeconds() << endl);

  return 0;
}

