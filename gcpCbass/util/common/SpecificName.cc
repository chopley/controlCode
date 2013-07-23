#include "gcp/util/common/SpecificName.h"
#include "gcp/util/common/Directives.h"

#include <sstream>

#include <stdlib.h>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
SpecificName::SpecificName() {}

/**.......................................................................
 * Destructor.
 */
SpecificName::~SpecificName() {}

std::string SpecificName::experimentName()
{
  std::ostringstream os;
  os << SPECIFIC;
  return os.str();
}

std::string SpecificName::experimentNameCaps()
{
  std::string name(SPECIFIC);

  for(unsigned i=0; i < name.size(); i++)
    name[i] = toupper(name[i]);

  return name;
}

std::string SpecificName::mediatorName()
{
  std::ostringstream os;

#if DIR_IS_STABLE
  os << experimentName() << "Mediator";
#else
  os << experimentName() << "UMediator";
#endif

  return os.str();
}

std::string SpecificName::controlName()
{
  std::ostringstream os;

#if DIR_IS_STABLE
  os << experimentName() << "Control";
#else
  os << experimentName() << "UControl";
#endif

  return os.str();
}

std::string SpecificName::viewerName()
{
  std::ostringstream os;

#if DIR_IS_STABLE
  os << experimentName() << "Viewer";
#else
  os << experimentName() << "UViewer";
#endif

  return os.str();
}

std::string SpecificName::envVarName()
{
  std::ostringstream os;

#if DIR_IS_STABLE
  os << experimentNameCaps() << "_DIR";
#else
  os << experimentNameCaps() << "UDIR";
#endif

  return os.str();
}

bool SpecificName::envVarIsDefined()
{
  return getenv(envVarName().c_str());
}
