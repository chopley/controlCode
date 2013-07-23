#define __FILEPATH__ "util/specific/Test/tArcWriter.cc"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <iostream>

#include "gcp/util/specific/Directives.h"

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Coordinates.h"
#include "gcp/util/common/Vector.h"
#include "gcp/util/common/Debug.h"

#include "gcp/util/common/TerminalServer.h"
#include "gcp/util/common/ArchiverWriterFrame.h"

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "nsamp",  "100", "i", "The number of frames to write"},
  { "dir",      ".", "s", "The directory in which to create data files"},
  { END_OF_KEYWORDS}
};

void Program::initializeUsage() {};

int Program::main()
{
  unsigned nsamp = Program::getiParameter("nsamp");

  ArchiverWriterFrame aw;
  ArrayMapDataFrameManager* frame = aw.frame();

  // Open the data files in the specified directory

  aw.openArcfile(Program::getParameter("dir").c_str());

  // Loop over integrations

  for(unsigned iFrame=0; iFrame < nsamp; iFrame++) {

    // Increment the frame record

    frame->writeReg("array",      "frame",    "record",   &iFrame);

    // Set the current date

    {
      RegDate date;
      date.setToCurrentTime();
      frame->writeReg("array",      "frame",    "utc",    date.data());
    }

    // Write an empty array of bolometer data

    {
      float ac[NUM_BOLOMETERS][DATA_SAMPLES_PER_FRAME];
      frame->writeReg("receiver", "bolometers", "ac",     &ac[0][0]);
    }

    // Finally, record the integration to the current file

    aw.saveIntegration();
  }
  
  return 0;
}
