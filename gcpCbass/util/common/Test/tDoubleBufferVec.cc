#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/DoubleBufferVec.h"

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
  DoubleBufferVec<float> db(10);

  float* ptr = db.getReadBuffer();
  ptr[0] = 1.234;
  db.releaseReadBuffer();

  ptr = db.getWriteBuffer();
  ptr[0] = 4.321;
  db.releaseWriteBuffer();

  db.switchBuffers();

  COUT(db.getReadBuffer()[0]);
  return 0;
}
