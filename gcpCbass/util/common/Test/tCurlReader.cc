#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/CurlUtils.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/String.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "url",     "",                        "s", "URL to fetch"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  CurlUtils reader;

  String firstForm(reader.getUrl(Program::getParameter("url")));

  firstForm.findFirstInstanceOf("name=\"Challenge\"");
  String challenge = firstForm.findNextInstanceOf("value=\"", true, "\"", true);

  COUT("Challenge is: " << challenge.str());
  COUT("Here 0");
  COUT(reader.postUserPass(Program::getParameter("url"), "admin", "power", challenge.str()));
  COUT("Here 1");
  COUT(reader.getUrl(Program::getParameter("url")));
  COUT("Here 2");

  return 0;
}
