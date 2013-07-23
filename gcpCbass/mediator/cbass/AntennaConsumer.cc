#define __FILEPATH__ "mediator/specific/AntennaConsumer.cc"

#include "gcp/mediator/specific/AntennaConsumer.h"

using namespace gcp::mediator;

/**.......................................................................
 * Constructor.
 */
AntennaConsumer::AntennaConsumer(Scanner* parent) :
  parent_(parent) {}

/**.......................................................................
 * Destructor.
 */
AntennaConsumer::~AntennaConsumer() {}
