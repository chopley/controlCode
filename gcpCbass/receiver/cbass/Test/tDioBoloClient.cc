#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/receiver/specific/DioBoloClient.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::receiver;
using namespace gcp::util;

using namespace MuxReadout;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  {"dioHost", "stoli2",  "s", "DIO host"},
  {"hwHost",  "stoli2",  "s", "HW host"},
  {"dioPort",  "5305",  "i", "DIO port"},
  {"hwPort",   "5207",  "i", "HW port"},
  {"ac",          "t",  "b", "ac|dc"},
  {"nsamp",       "0",  "i", "Number of samples to acquire (0 = free running)"},
  {"npix",        "0",  "i", "Number of pixels to acquire (0 = all)"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  unsigned nsamp = Program::getiParameter("nsamp");

  DioBoloClient client(Program::getParameter("dioHost"), 
		       Program::getiParameter("dioPort"), 
		       Program::getParameter("hwHost"), 
		       Program::getiParameter("hwPort"),
		       Program::getiParameter("hwPort"),
		       Program::getiParameter("npix")); 



  // Now acquire data from the server, writing to disk

  TimeVal last, curr, diff;
  bool first = true;

  vector<DIODatum> data;
  data.resize(1);

  client.connect();

  COUT("4 data_.size() = " << client.data_.size());

  unsigned count=0;
  while(count++ < 100) {

    // Get the next sample from the client

    COUT("About to call getNextSample()");
    COUT("5 with data_.size() = " << client.data_.size());
  
    (void)client.bufferNextSample();
  
    // And write the current integration

    curr.setToCurrentTime();

    if(first) {
      last.setToCurrentTime();
      first = false;
    }

    diff = curr - last;
    CTOUT("Writing integration: " << count << " " << client.data_[0].data << " delta T = " 
	  << diff.getTimeInSeconds());
    last = curr;
  }

  return 0;
}
