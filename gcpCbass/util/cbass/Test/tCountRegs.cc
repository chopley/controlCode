#define __FILEPATH__ "util/common/Test/tCountRegs.cc"

#include <iostream>
#include <iomanip>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Coord.h"
#include "gcp/util/common/CoordAxes.h"
#include "gcp/util/common/Debug.h"

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "debuglevel",  "0", "i", "Debug level"},
  { "nsec",        "1", "i", "Frame rate, in seconds"},
  { "nadd",        "0", "i", "Additional per-frame bytes from slow registers"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  unsigned nsec = Program::getiParameter("nsec");
  unsigned nadd = Program::getiParameter("nadd");

  ArrayMap* arrMap = 0;

  arrMap = new_ArrayMap();

  double nByte=0, nFastByte=0, nSlowByte=0;

  for(unsigned iRegMap=0; iRegMap != arrMap->regmaps.size(); iRegMap++) {
    RegMap* regMap = arrMap->regmaps[iRegMap]->regmap;

    for(unsigned iBoard=0; iBoard != regMap->boards_.size(); iBoard++) {
      RegMapBoard* board = regMap->boards_[iBoard];

      for(unsigned iBlock=0; iBlock != board->blocks.size(); iBlock++) {
	RegMapBlock* block = board->blocks[iBlock];

	// If this block isn't archived, skip it
	
	if(block->flags_ & REG_EXC) 
	  continue;

	// Else increment the number of bytes

	nByte += block->nByte();

	// Check the fastest-changing index. 

	unsigned nAxis = block->axes_->nAxis();
	if(block->axes_->nEl(nAxis-1) == DATA_SAMPLES_PER_FRAME)
	  nFastByte += block->nByte();
	else
	  nSlowByte += block->nByte();

      }
    }
  }

  COUT(endl << "Array map consists of: " << nByte/1000 << " kB, of which " 
       << nFastByte/1000 << " are fast, and "
       << nSlowByte/1000 << " are slow");

  double nSecPerDay = 86400;

  double nBytePerFrame = nFastByte * nsec + nSlowByte;

  double nFramePerDay = nSecPerDay/nsec;

  COUT(endl << "At a rate of 1 frame every " << nsec << " seconds, this amounts to: " 
       << std::setprecision(3) << (nBytePerFrame * nFramePerDay)/1e9 << " GB per day");

  COUT(endl << "With " << nadd << " additional bytes per frame from slow registers: " 
       << std::setprecision(3) << ((nBytePerFrame+nadd) * nFramePerDay)/1e9 << " GB per day" << endl);

  if(arrMap != 0)
    arrMap = del_ArrayMap(arrMap);

  return 0;
}
