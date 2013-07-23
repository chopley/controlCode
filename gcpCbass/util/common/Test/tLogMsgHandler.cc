#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogMsgHandler.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "index", "0",  "i", "Index"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  LogMsgHandler lgm;
  unsigned seq1 = lgm.nextSeq();
  unsigned seq2 = lgm.nextSeq();

  lgm.append(seq1, "This is the start of message 1");
  lgm.append(seq2, "This is the start of message 2");

  lgm.append(seq1, " and this is the end of message 1");

  COUT(lgm.getMessage(seq1));

  bool isLast=false;

  COUT(lgm.getNextMessageSubstr(seq2, 10, isLast));
  COUT(isLast);
  COUT(lgm.getNextMessageSubstr(seq2, 10, isLast));
  COUT(isLast);
  COUT(lgm.getNextMessageSubstr(seq2, 10, isLast));
  COUT(isLast);
  COUT(lgm.getNextMessageSubstr(seq2, 10, isLast));
  COUT(isLast);

  return 0;
}
