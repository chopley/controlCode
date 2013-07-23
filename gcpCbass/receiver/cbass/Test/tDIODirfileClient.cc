#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/TimeVal.h"

#include "gcp/receiver/specific/BolometerConsumer.h"

#include "Utilities/DIOClient.h"
#include "Utilities/DirfileWriter.h"
#include "Utilities/TCPConnection.h"
#include "Utilities/DIOClient_BaseClass.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::receiver;
using namespace gcp::util;

using namespace MuxReadout;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  {"host",    "localhost", "s", "Server host"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  DIOClient_BaseClass testclient(Program::getParameter("host"), 5305);

  DIOColumnTokenFactory factory;

  vector<DIODatum> data(1);

  //  testclient.addColumn(factory.calenderTimeStampColumn());
  //  testclient.addColumn(factory.dataColumn("21:sa5", "localhost"));
  testclient.addColumn(factory.dataColumn("41:rb7", Program::getParameter("host")));

  testclient.setFilterFunction(vector<double>(1000, 0.001));
  testclient.setSubsampling(999);
  testclient.setSleepTime(1.0);

  while(!testclient.start())
    sleep(1);

  float retVal;
  double mjd;

  // Instantiate a DirfileWriter object

  DirfileWriter writer;

  // Add two data items this object will write whenever its
  // writeIntegration() method is called

  //  writer.addRegister("mjd",    (unsigned char*)&mjd,    DirfileWriter::Register::DOUBLE);
  writer.addRegister("21:sa5", (unsigned char*)&retVal, DirfileWriter::Register::FLOAT);

  // Finally, open the dirfile

  writer.openArcfile("dirfileTest");

  // Now acquire data from the server, writing to disk

  TimeVal last, curr, diff;
  bool first = true;

  unsigned count=0;
  while(count++ < 100) {

    // Get the next sample from the client

    (void) testclient.getNextSample(&data[0]);
  
    // Assign the values into our external variables

    //    mjd    = data[0].timeStamp->mjd();
    //    retVal = data[1].data;
    retVal = data[0].data;

    // And write the current integration

    curr.setToCurrentTime();

    if(first) {
      last.setToCurrentTime();
      first = false;
    }

    diff = curr - last;
    CTOUT("Writing integration: " << count << retVal << " delta T = " << diff.getTimeInSeconds());
    last = curr;
    //    writer.writeIntegration();
  }

  return 0;
}
