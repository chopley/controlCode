#include "gcp/mediator/specific/Scanner.h"

#include "gcp/receiver/specific/BolometerConsumer.h"
#include "gcp/receiver/specific/DioBoloClient.h"

#include "gcp/control/code/unix/libunix_src/spt/specificregs.h"

#include <sstream>
#include <fstream>
#include <string>
#include <popt.h>

using namespace gcp::mediator;
using namespace gcp::receiver;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
BolometerConsumer::
BolometerConsumer(Scanner* parent, 
		  std::string dioHost, unsigned short dioPort,
		  std::string hwHost,  unsigned short hwPort,
		  unsigned npix) :
  DioConsumer(parent, dioHost, dioPort, hwHost, hwPort)
{
  // Instantiate a DIO Client

  dioClient_ = new DioBoloClient(dioHost, dioPort, hwHost, hwPort, 
				 SAMPLES_PER_FRAME, npix);
}

/**.......................................................................
 * Destructor.
 */
BolometerConsumer::~BolometerConsumer() {}

/**.......................................................................
 * Pack a frame
 */
void BolometerConsumer::packFrame()
{
  if(parent_) 
    parent_->sendPackBoloDataFrameMsg();
}

/**.......................................................................
 * Process a message specific to this task.
 */
void BolometerConsumer::processMsg(gcp::mediator::DioMsg* msg) 
{
  switch (msg->type) {

    // Our frame timer has expired

  case gcp::mediator::DioMsg::SETFILTER:
    dioClient_->setFilter(msg->body.setFilter.mask,
			  msg->body.setFilter.freqHz,
			  msg->body.setFilter.ntaps);
			  
    break;
  default:
    ThrowError("Unrecognized message type: " << msg->type);
    break;
  }
}

