#include "gcp/util/common/ArcFileConverter.h"

#include<iostream>

using namespace std;
using namespace gcp::util;
using namespace gcp::control;

/**.......................................................................
 * Constructor.
 */
ArcFileConverter::ArcFileConverter(std::string datfileName, 
				   std::string dirfileName) 
{
  ms_ = new_FileMonitorStream((char*)datfileName.c_str());

  if(ms_ == 0) 
    ThrowSimpleError("Unable to open the archive stream");
  
  ms_->interval = 1;

  DataFrameManager* manager = ms_->raw->fm;
  DataFrame*        frame   = ms_->raw->fm->frame();

  // Map registers

  writer_.mapRegisters(ms_->arraymap, 
		       frame->getUcharPtr(manager->byteOffsetInFrameOfData()),
		       1);

  // And open the requested dirfile

  if(writer_.openArcfile(dirfileName))
    ThrowError("Unable to open files");
}

/**.......................................................................
 * Destructor.
 */
ArcFileConverter::~ArcFileConverter() 
{
  writer_.closeArcfile();

  if(ms_ != 0) {
    ms_ = del_MonitorStream(ms_);
    ms_ = 0;
  }
}

/**.......................................................................
 * Read the next frame
 */
MsReadState ArcFileConverter::readNextFrame()
{
  LogStream errStr;
  MsReadState state;
  
  // Read the next frame of registers
  
  switch((state=ms_read_frame(ms_, 1))) {
  case MS_READ_ENDED:  // We have reached the end of the file
    break;
  case MS_READ_DONE:   // A new frame of register values has been read 
    break;
  default:
    ReportError("An error occurred reading file");
    return MS_READ_ENDED;
  };
  return state;
}

/**.......................................................................
 * Read all records from an archive file, and convert to dirfile format
 */
void ArcFileConverter::convert()
{
  COUT("About to convert");
  while(readNextFrame() != MS_READ_ENDED) {
    writer_.writeIntegration();
  }
}
