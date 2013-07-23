#include "gcp/pgutil/common/MultipleImageReader.h"

#include<iostream>

using namespace std;

using namespace gcp::grabber;

/**.......................................................................
 * Constructor.
 */
MultipleImageReader::MultipleImageReader() : MultipleImagePlotter()
{
  ims_ = 0;
}

/**.......................................................................
 * Read an image from the stream
 */
ImsReadState MultipleImageReader::read()
{
  ImMonitorStream *ims; /* The image monitor data stream */
  ImsReadState state;   /* The image monitor-stream input-completion status */
  unsigned short* image;
  unsigned short* channel;
  int i;

  // Read the image.

  state   = ims_read_image(ims_, 0);
  image   = ims_get_image(ims_);
  channel = ims_get_channel(ims_);

  // And copy the new image into the display buffer.

  if(state==IMS_READ_DONE)
    installNewImage(*channel, image);

  return state;
}

/**.......................................................................
 * Destructor.
 */
MultipleImageReader::~MultipleImageReader() 
{
  // Delete the image monitor stream.

  ims_ = del_ImMonitorStream(ims_);
}

/**.......................................................................
 * Change the source of image monitor data. 
 */
void MultipleImageReader::changeStream(ImMonitorStream* ims)
{
  // Discard the previous data-source.

  ims_ = del_ImMonitorStream(ims_);
  ims_ = ims;
}

void MultipleImageReader::openStream(std::string host)
{
  ImMonitorStream* ims = new_NetImMonitorStream((char*)host.c_str());
  changeStream(ims);
}
