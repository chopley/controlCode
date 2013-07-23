#define __FILEPATH__ "util/common/Test/tAntNum.cc"

#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/IoLock.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "antenna",     "0",                        "i", "Antenna number"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  AntNum antNum;

  cout << "Hello" << endl;

  // If no value was specified for the antenna number on the
  // command, initialize it from the host name.
  
  if(isDefault("antenna")) {
    antNum.setIdFromHost();
    cout << "Setting id from host" << endl;
  } else {
    antNum.setId(Program::getiParameter("antenna"));
    cout << "Setting id from command line" << endl;
  }

  cout << "Hello(1)" << endl;

  cout << antNum.getId() << endl;
  cout << antNum.getIntId() << endl;
  cout << antNum.getObjectName() << endl;

  cout << "Hello(2)" << endl;

  for(unsigned i=0; i < antNum.getAntMax(); i++) {
    antNum.setId((unsigned int) i);
    cout << antNum << " " << antNum.getAntennaName() << endl;
  }

  return 0;
}
