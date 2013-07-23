#define __FILEPATH__ "util/common/Test/tCoordAxes.cc"

#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/CoordAxes.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  gcp::util::CoordAxes* ax1 = new gcp::util::CoordAxes(16,0,0);
  gcp::util::CoordAxes* ax  = new gcp::util::CoordAxes(ax1);
  
  delete ax1;
  
  COUT("Here");
  delete ax;
  
#if 0
  Debug::setLevel(Program::getiParameter("debuglevel"));

  CoordAxes axes, axes2;

  COUT("axes.nEl() = " << axes.nEl());
#if 0
  axes.setAxis(0, 10);
  axes.setAxis(1, 2);
  axes.setAxis(2, 10);

  axes2.setAxis(0, 10);
  axes2.setAxis(1, 2);
  axes2.setAxis(2, 10);

  std::cout << "Axes are: " << (axes==axes2 ? "equivalent" : "not equivalent") << std::endl;

  // Compute non-contiguous ranges

  std::vector<Range<unsigned> > ranges;

  // Compute ranges

  Coord start(0, 0, 0), stop(3, 1, 8);
  CoordRange range(start, stop);

  ranges = axes.getRanges(range);

  for(unsigned i=0; i < ranges.size(); i++)
    cout << ranges[i] << endl;

  range.setContiguous(true);

  ranges = axes.getRanges(range);

  for(unsigned i=0; i < ranges.size(); i++)
    cout << ranges[i] << endl;

  // Now test coordinate range checking

  range.setStartIndex(0, 0);
  range.setStopIndex(0, 9);

  cout << "Range: " << range << " is " 
       << (axes.rangeIsValid(range) ? "" : "not") 
       << " consistent with axes: " << axes << endl;

  range.setStopIndex(0, 10);

  cout << "Range: " << range << " is " 
       << (axes.rangeIsValid(range) ? "" : "not") 
       << " consistent with axes: " << axes << endl;

  range.setStartIndex(0, 4);
  range.setStopIndex(0, 3);

  cout << "Range: " << range << " is " 
       << (range.isValid() ? "" : "not") 
       << " valid " << endl;

  cout << "Range: " << range << " is " 
       << (axes.rangeIsValid(range) ? "" : "not") 
       << " consistent with axes: " << axes << endl;

  // Now test coordinate <--> element conversion

  axes.reset();
  axes.setAxis(0, 4);
  axes.setAxis(1, 3);
  axes.setAxis(2, 2);

  Coord coord(3, 2, 1);

  DBPRINT(true, Debug::DEBUG2, "element offset of " << coord << " in " << axes 
	  << " is: " << axes.elementOffsetOf(coord));

  Coord newCoord = axes.coordOf(axes.elementOffsetOf(coord));

  DBPRINT(true, Debug::DEBUG2, "Coordinate of offset " 
	  << axes.elementOffsetOf(coord) << " in: "
	  << axes << " is: " << newCoord);

#endif
#endif
  return 0;
}
