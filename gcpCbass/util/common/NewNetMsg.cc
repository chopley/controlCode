#include "NewNetMsg.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
NewNetMsg::NewNetMsg() 
{
  addMember(NET_GREETING_MSG,     &greeting);    // A greeting
						      // message

  addMember(NET_LOG_MSG,          &log);         // A message to be
						      // logged

  addMember(NET_DRIVE_DONE_MSG,    &drive_done);   // A tracker
						      // transaction-completion
						      // message
  addMember(NET_BENCH_DONE_MSG,    &bench_done);   // A bench
						      // transaction-completion
						      // message
  addMember(NET_SCAN_DONE_MSG,    &scan_done);   // A scan-completion
						 // message

  addMember(NET_SOURCE_SET_MSG,   &source_set);  // A
						      // source-has-set
						      // advisory
						      // message

  addMember(NET_SETREG_DONE_MSG,  &setreg_done); // A setreg
						      // transaction
						      // completion
						      // message

  addMember(NET_TV_OFFSET_DONE_MSG, &tv_offset_done); // A
							   // tv_offset
							   // transaction


  addMember(NET_SCRIPT_DONE_MSG,   &scriptDone);  // script has finished message


  addMember(NET_NAV_UPDATE_MSG);
}

/**.......................................................................
 * Destructor.
 */
NewNetMsg::~NewNetMsg() {}
