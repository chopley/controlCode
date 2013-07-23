#define __FILEPATH__ "control/code/unix/control_src/common/NewRtcNetMsg.c"

#include "gcp/util/common/Debug.h"

#include "NewRtcNetMsg.h"
#include "NewNetMsg.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
NewRtcNetMsg::NewRtcNetMsg() 
{
  NETSTRUCT_UINT(antenna);
  msg = newSpecificNetMsg();

  addMember(msg, true);
}

/**.......................................................................
 * Copy constructor.
 */
NewRtcNetMsg::NewRtcNetMsg(const NewRtcNetMsg& rtc)
{
#if 0
  antenna = rtc.antenna;
  *msg    = *rtc.msg;
#endif
}

/**.......................................................................
 * Copy constructor.
 */
NewRtcNetMsg::NewRtcNetMsg(NewRtcNetMsg& rtc)
{
  DBPRINT(false, Debug::DEBUGANY, "Inside NewRtcNetMsg copy");
#if 0
  antenna = rtc.antenna;
  *msg    = *rtc.msg;
#endif
}

NewRtcNetMsg& NewRtcNetMsg::operator=(const NewRtcNetMsg& rtc) 
{
  DBPRINT(false, Debug::DEBUGANY, "Inside NewRtcNetMsg const assignment operator");
  antenna = rtc.antenna;
  *msg    = *rtc.msg;
}

NewRtcNetMsg& NewRtcNetMsg::operator=(NewRtcNetMsg& rtc) 
{
  DBPRINT(false, Debug::DEBUGANY, "Inside NewRtcNetMsg assignment operator");
  antenna = rtc.antenna;
  *msg    = *rtc.msg;
}

/**.......................................................................
 * Destructor.
 */
NewRtcNetMsg::~NewRtcNetMsg() {}
