#include <iostream>

#include <utility>

#include <vector>
#include <deque>

using namespace std;

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/program/common/Program.h"

#include "DIOUtils/DIOColumns.h"

using namespace gcp::util;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  {"nfilter",  "1000",   "i", "Length of the filter function"},
  {"nsamp",    "100000", "i", "Number of samples to iterate over"},
  {"nskip",    "0",      "i", "Number of samples to skip between integrations"},
  {END_OF_KEYWORDS}
};

void Program::initializeUsage() {};

void runCol(MuxReadout::FIRFilteredDataColumn* col, unsigned nSamples);

using namespace MuxReadout; 

int Program::main(void)
{
  unsigned nFilter  = Program::getiParameter("nfilter");
  unsigned nSamples = Program::getiParameter("nsamp");
  unsigned nSkip    = Program::getiParameter("nskip");

  std::vector<double> filterFunction;
  filterFunction.resize(nFilter);

  for(unsigned i=0; i < nFilter; i++)
    filterFunction[i] = 0.001;

  MuxReadout::FIRFilteredDataColumn*    oldCol = new    FIRFilteredDataColumn('2', "b3", true, filterFunction);
  MuxReadout::NewFIRFilteredDataColumn* newCol = new NewFIRFilteredDataColumn('2', "b3", true, filterFunction);

  TimeVal tValStart, tValStop, tDiff1, tDiff2;

  oldCol->setSamplesToSkip(nSkip);
  newCol->setSamplesToSkip(nSkip);

  COUT("============ About to run FIRFilteredDataColumn =============");

  tValStart.setToCurrentTime();
  runCol(oldCol, nSamples);
  tValStop.setToCurrentTime();
  tDiff1 = tValStop - tValStart;

  COUT("============ About to run NewFIRFilteredDataColumn =============");

  tValStart.setToCurrentTime();
  runCol(newCol, nSamples);
  tValStop.setToCurrentTime();
  tDiff2 = tValStop - tValStart;
  
  COUT(endl << 
       "Elapsed times: " << endl << endl
       << "  deque: " << tDiff1.getTimeInSeconds() << " seconds" << endl
       << "  lists: " << tDiff2.getTimeInSeconds() << " seconds" << endl
       << "  ratio: " << tDiff1.getTimeInSeconds()/tDiff2.getTimeInSeconds() << endl); 
  
  return 0;
}

void runCol(FIRFilteredDataColumn* col, unsigned nSamples) 
{
  //  col->setSamplesToSkip(0);

  for(unsigned iSamp=0; iSamp < nSamples; iSamp++) {

    col->testWriteBinaryToBuffer(1);

#if 0
    col->pushNewIntegration();
    col->addSampleToComputation(1);

    if(col->sampleIsReady()) {
      col->popSample();
    }
#endif
  }
}

