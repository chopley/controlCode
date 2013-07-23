#define __FILEPATH__ "util/common/Test/tVector.cc"

#include <iostream>
#include <vector>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Vector.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "antenna",     "0",                        "i", "Antenna number"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

Vector<unsigned char> command();

int Program::main()
{
  Vector<unsigned char> vec1, vec2;

  vec1.push_back('A');
  vec1.push_back('t');
  vec1.push_back('e');
  vec1.push_back('s');
  vec1.push_back('t');

  vec2.push_back('B');
  vec2.push_back('t');
  vec2.push_back('e');
  vec2.push_back('s');
  vec2.push_back('t');

  std::cout << vec1 << "size=" << vec1.size() << std::endl;
  std::cout << vec2 << "size=" << vec2.size() << std::endl;

  vec1.add(vec2);

  std::cout << vec1 << "size=" << vec1.size() << std::endl;

  vec1.add(command());
  std::cout << vec1 << "size=" << vec1.size() << std::endl;

  vec1 = command();
  std::cout << vec1 << "size=" << vec1.size() << std::endl;

  return 0;
}

Vector<unsigned char> command()
{
  Vector<unsigned char> vec;

  vec.push_back('C');
  vec.push_back('o');
  vec.push_back('m');
  vec.push_back('m');
  vec.push_back('a');
  vec.push_back('n');
  vec.push_back('d');

  return vec;

}
