#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/receiver/specific/ReceiverConfigConsumer.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::receiver;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "host", "stoli2",  "s", "HWM host"},
  { "port",   "5207",  "i", "HWM Port"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

static THREAD_START(startReceiverConsumer)
{
  // Block in receiverconsumer

  ReceiverConfigConsumer* rc =
    (ReceiverConfigConsumer*) arg; 

  rc->serviceMsgQ();
}

int Program::main()
{
  ReceiverConfigConsumer* rc = 
    new ReceiverConfigConsumer(0, 
			       Program::getParameter("host"),
			       Program::getiParameter("port"));
  
  pthread_t id;
  pthread_create(&id, NULL, &startReceiverConsumer, (void*)(rc));

  rc->sendDispatchDataFrameMsg();

  for(unsigned i=0; i < 10; i++) {
    sleep(1);
    rc->sendDispatchDataFrameMsg();
  }

  return 0;
}

