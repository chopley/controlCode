#include "gcp/control/code/unix/control_src/common/ArchiverWriterFrame.h"

#include "gcp/control/code/unix/control_src/common/archiver.h"

#include<iostream>

using namespace std;
using namespace gcp::control;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
ArchiverWriterFrame::ArchiverWriterFrame(Archiver* arc) : 
ArchiverWriter(arc) {}

/**.......................................................................
 * Const Copy Constructor.
 */
ArchiverWriterFrame::ArchiverWriterFrame(const ArchiverWriterFrame& objToBeCopied)
{
  *this = (ArchiverWriterFrame&)objToBeCopied;
};

/**.......................................................................
 * Copy Constructor.
 */
ArchiverWriterFrame::ArchiverWriterFrame(ArchiverWriterFrame& objToBeCopied)
{
  *this = objToBeCopied;
};

/**.......................................................................
 * Const Assignment Operator.
 */
void ArchiverWriterFrame::operator=(const ArchiverWriterFrame& objToBeAssigned)
{
  *this = (ArchiverWriterFrame&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void ArchiverWriterFrame::operator=(ArchiverWriterFrame& objToBeAssigned)
{
  std::cout << "Calling default assignment operator for class: ArchiverWriterFrame" << std::endl;
};

/**.......................................................................
 * Destructor.
 */
ArchiverWriterFrame::~ArchiverWriterFrame() {}

int ArchiverWriterFrame::chdir(char* dir)
{
  return chdir_archiver(arc_, dir);
}

int ArchiverWriterFrame::openArcfile(char* dir)
{
  return open_arcfile(arc_, dir);
}

void ArchiverWriterFrame::closeArcfile() 
{
  close_arcfile(arc_);
}

void ArchiverWriterFrame::flushArcfile()
{
  flush_arcfile(arc_);
}

int ArchiverWriterFrame::writeIntegration() 
{
  NetBuf *net = arc_->net;
  return (int)(fwrite(net->buf, sizeof(net->buf[0]), net->nput, arc_->fp) != net->nput);
}

bool ArchiverWriterFrame::isOpen()
{
  return arc_->fp != 0;
}
