#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/DirList.h"
#include "gcp/util/common/Exception.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "path",     "/data/sptdaq/arc",  "s", "the directory to search"},
  { "descend",     "f",              "b", "descend into subdirectories?"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  COUT("ALL: ");

  DirList dirList(Program::getParameter("path"), 
		  Program::getbParameter("descend"));
  //  dirList.listEntries();


  COUT(std::endl << "Files: ");

  std::list<DirList::DirEnt> files = dirList.getFiles();
  dirList.listEntries(files);

  COUT(std::endl << "Dirs: ");

  std::list<DirList::DirEnt> dirs = dirList.getDirs();
  dirList.listEntries(dirs);

  return 0;
}
