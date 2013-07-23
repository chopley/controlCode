#include "InitScript.h"
#include <sstream>
#include "gcp/util/common/Directives.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/SpecificName.h"

using namespace std;

using namespace gcp::control;


/**.......................................................................
 * Constructor.
 */
InitScript::InitScript() {}

/**.......................................................................
 * Destructor.
 */
InitScript::~InitScript() {}

std::string InitScript::initScript()
{
  std::ostringstream os;

  os << "$" << gcp::util::SpecificName::envVarName()  << "/control/conf/"
     << gcp::util::SpecificName::experimentName();

#if DIR_IS_STABLE
  os << "/control.init";
#else
  os << "/controlU.init";
#endif

  COUT("Loading init script: " << os.str());

  return os.str();
}
