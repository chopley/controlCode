#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/RegDate.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/receiver/specific/BolometerConsumer.h"

#include "Utilities/DIOClient.h"
#include "Utilities/TCPConnection.h"
#include "Utilities/DIOClient_BaseClass.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::receiver;
using namespace gcp::util;

using namespace MuxReadout;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  {"chan",    "2e:ra8", "s", "Channel"},
  {"host",    "stoli2", "s", "Server host"},
  {"port",    "5205",   "i", "Server socket (5305 for devel, 5205 for real)"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  DIOClient_BaseClass testclient(Program::getParameter("host"), 
				 Program::getiParameter("port"));

  DIOColumnTokenFactory factory;

  vector<DIODatum> data;
  data.resize(2);

  testclient.addColumn(factory.calendarTimeStampColumn());
  //  testclient.addColumn(factory.dataColumn("21:sa5", "localhost"));

#if 0
  testclient.addColumn(factory.dataColumn(Program::getParameter("chan"),
					  Program::getParameter("host")));
#else

  std::vector<std::string> channels;

  channels.push_back(Program::getParameter("chan"));

  testclient.addColumn(DIOColumnTokenFactory::
		       batchDataColumn(channels, 
				       Program::getParameter("host"),
				       Program::getiParameter("port")));
#endif

  std::vector<double> filterFunction;
  unsigned nFilter = 100;
  filterFunction.resize(nFilter);

  for(unsigned i=0; i < nFilter; i++)
    filterFunction[i] = 0.01;

  //  testclient.setFilterFunction(filterFunction);
  testclient.setSincFilter(37.0, 129);
  testclient.setSubsampling(999);
  testclient.setSleepTime(1.0);

  while(!testclient.start())
    sleep(1);

  float retVal;
  double mjd;

  // Now acquire data from the server, writing to disk

  TimeVal last, curr, diff;
  bool first = true;

  unsigned count=0;
  while(count++ < 100) {

    // Get the next sample from the client

    COUT("About to call getNextSample()");

    (void)testclient.getNextSample(&data[0]);
  
    static RegDate date;

    // Buffer the fast time-data
    
    date.setMjd(data[0].timeStamp->mjd());

    COUT(date);

#if 0
    retVal = data[1].data;
#else
    retVal = data[1].batchData[0];
#endif

    // And write the current integration

    curr.setToCurrentTime();

    if(first) {
      last.setToCurrentTime();
      first = false;
    }

    diff = curr - last;
    CTOUT("Writing integration: " << count << " " << retVal << " delta T = " 
	  << diff.getTimeInSeconds());
    last = curr;
  }

  return 0;
}
