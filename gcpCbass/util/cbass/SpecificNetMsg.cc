#include "SpecificNetMsg.h"

using namespace std;

using namespace gcp::util;

namespace gcp {
  namespace util {
    /**.......................................................................
     * Define an allocator for experiment-specific network messages
     */
    NewNetMsg* newSpecificNetMsg() 
    {
      NewNetMsg* msg = new SpecificNetMsg();
	
      return msg;
    }
  }
}

/**.......................................................................
 * Constructor.
 */
SpecificNetMsg::SpecificNetMsg() : NewNetMsg()
{
}

/**.......................................................................
 * Destructor.
 */
SpecificNetMsg::~SpecificNetMsg() {}
