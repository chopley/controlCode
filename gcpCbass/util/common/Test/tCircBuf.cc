#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/CircBuf.h"

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
  CircBuf<int> buf(5);

  buf.push(1);
  buf.push(2);
  buf.push(3);
  buf.push(4);
  buf.push(5);

  COUT("new: " << buf.newest());
  COUT("old: " << buf.oldest());

  std::valarray<int> arr = buf.copy();

  buf.push(6);

  COUT("new: " << buf.newest());
  COUT("old: " << buf.oldest());
  
  arr = buf.copy();

  COUT(arr[0] << " " << arr[arr.size()-1]);

  buf.push(7);

  COUT("new: " << buf.newest());
  COUT("old: " << buf.oldest());
  
  arr = buf.copy();

  COUT(arr[0] << " " << arr[arr.size()-1]);

  return 0;
}
