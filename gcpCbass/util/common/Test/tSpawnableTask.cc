#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/SpawnableTask.h"
#include "gcp/util/common/GenericTaskMsg.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "index", "0",  "i", "Index"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

class TestMsg : public gcp::util::GenericTaskMsg {
public:

  
};

class TestClass : public gcp::util::SpawnableTask<TestMsg>
{
public:

  TestClass(bool spawn) :
    gcp::util::SpawnableTask<TestMsg>::SpawnableTask(spawn) {}

  virtual ~TestClass() {};

  void sendMsg() {
    COUT("Sending task msg");
    TestMsg msg;
    gcp::util::GenericTask<TestMsg>::sendTaskMsg(&msg);
  }

  //  void serviceMsgQ() {
  //    while(true) {
  //      COUT("Inside TestClass::serviceMsgQ()");
  //      sleep(1);
  //    }
  //  }

  void processMsg(TestMsg* msg) {
    COUT("Inside TestClass::processMsg()");
  }
};

int Program::main()
{
  Debug::setLevel(Debug::DEBUGANY);

  TestClass task(true);
  TestMsg msg;

  sleep(2);

  task.sendMsg();

  task.blockForever();

  return 0;
}
