#include "gcp/receiver/specific/SquidConsumer.h"
#include "gcp/receiver/specific/DioSquidClient.h"
#include "gcp/mediator/specific/Scanner.h"

#include "gcp/util/common/ArrayDataFrameManager.h"

#include <sstream>
#include <fstream>
#include <string>
#include <popt.h>

using namespace gcp::receiver;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
SquidConsumer::
SquidConsumer(gcp::mediator::Scanner* parent, 
		    std::string dioHost, unsigned short dioPort,
		    std::string hwHost,  unsigned short hwPort) :
  DioConsumer(parent, dioHost, dioPort, hwHost, hwPort)
{
  // Instantiate an DC DIO Client

  dioClient_ = new DioSquidClient(dioHost, dioPort, hwHost, hwPort, 1);
}

/**.......................................................................
 * Destructor.
 */
SquidConsumer::~SquidConsumer() {}

/**.......................................................................
 * Pack a frame
 */
BoardDataFrameManager& SquidConsumer::getFrame()
{
  return dioClient_->getFrame();
}
