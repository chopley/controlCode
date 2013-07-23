#define __FILEPATH__ "antenna/control/Test/tTimeVal.cc"

#include <iostream>

#include "carma/util/Program.h"

#include "gcp/antenna/util/TimeVal.h"

using namespace std;
using namespace gcp::antenna::util;

int carma::util::Program::main()
{
  TimeVal curr, last, diff;

  last.setToCurrentTime(CLOCK_REALTIME);

  for(unsigned i=0;i < 10; i++) {

    curr.setToCurrentTime(CLOCK_REALTIME);
    
    diff = curr-last;

    cout << endl;
    cout << curr.getMjdDays() << endl;
    cout << curr.getMjdSeconds() << endl;
    cout << curr.getMjdNanoSeconds() << endl;
    
    cout << "\nElapsed seconds: " << diff.getMjdSeconds() << endl;
    cout << "\nElapsed nanoseconds: " << diff.getMjdNanoSeconds() << endl;

    last = curr;
  }
    
}
