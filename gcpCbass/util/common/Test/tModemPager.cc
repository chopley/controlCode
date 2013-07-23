#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/ModemPager.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "command", "alive",           "s", "Command to send to the pager"},
  { "msg",     "This is a page",  "s", "Message to send on error"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

void doTest1();
void doTest2();
void doTest3();

int Program::main()
{
  doTest3();
  return 0;
}

void doTest1()
{
  ModemPager mp;
  
  mp.activate("this is a test");
  mp.blockForever();
}

void doTest2()
{
  ModemPager mp;
  
#if 0
  std::string command = Program::getParameter("command");
  if(strstr(command.c_str(), "ack") != 0)
    mp.acknowledge();
  if(strstr(command.c_str(), "alive") != 0)
    mp.sendAlive();
  if(strstr(command.c_str(), "enable") != 0)
    mp.enable(true);
  if(strstr(command.c_str(), "disable") != 0)
    mp.enable(false);
  if(strstr(command.c_str(), "on") != 0)
    mp.activate(Program::getParameter("msg"));
  if(strstr(command.c_str(), "start") != 0)
    mp.start();
  if(strstr(command.c_str(), "status") != 0)
    mp.requestStatus();
  if(strstr(command.c_str(), "stop") != 0)
    mp.stop();
  if(strstr(command.c_str(), "reset") != 0)
    mp.reset();
#endif
}

void doTest3()
{
  ModemPager mp;
  mp.spawn();
  
  while(true) {
    mp.requestStatus();
    sleep(5);
  }
}
