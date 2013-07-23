#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/receiver/specific/BolometerConsumer.h"
#include "gcp/receiver/specific/SquidConsumer.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::receiver;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  {"dioHost",  "stoli2",  "s", "DIO host"},
  {"hwHost",   "stoli2",  "s", "HW host"},
  {"dioPort",   "5205",  "i", "DIO port"},
  {"hwPort",    "5207",  "i", "HW port"},
  {"ac",           "t",  "b", "ac|dc"},
  {"batch",        "t",  "b", "use batch filtering?"},
  {"nsamp",        "0",  "i", "Number of samples to acquire (0 = free running)"},
  {"npix",         "0",  "i", "Number of pixels to acquire (0 = all)"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  unsigned nsamp = Program::getiParameter("nsamp");

  BolometerConsumer bolo(0, 
			 Program::getParameter("dioHost"), 
			 Program::getiParameter("dioPort"), 
			 Program::getParameter("hwHost"), 
			 Program::getiParameter("hwPort"),
			 Program::getiParameter("npix"));

  SquidConsumer squid(0, 
		      Program::getParameter("dioHost"), 
		      Program::getiParameter("dioPort"), 
		      Program::getParameter("hwHost"), 
		      Program::getiParameter("hwPort"));
  
  
  // Acquire AC or DC data

  if(Program::getbParameter("ac")) {

    if(nsamp == 0) {
      bolo.serviceMsgQ();
    } else {
      for(unsigned iSamp=0; iSamp < nsamp; iSamp++)
	bolo.checkForDioData();
    }

  } else {

    COUT("About to servicemsq for squids");

    if(nsamp == 0) {
      squid.serviceMsgQ();
    } else {
      for(unsigned iSamp=0; iSamp < nsamp; iSamp++)
	squid.checkForDioData();
    }

  }
  
  return 0;
}
