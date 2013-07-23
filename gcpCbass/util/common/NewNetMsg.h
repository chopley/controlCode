// $Id: NewNetMsg.h,v 1.1.1.1 2009/07/06 23:57:26 eml Exp $

#ifndef GCP_UTIL_NEWNETMSG_H
#define GCP_UTIL_NEWNETMSG_H

/**
 * @file NewNetMsg.h
 * 
 * Tagged: Tue Jun 28 12:18:41 PDT 2005
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:26 $
 * 
 * @author Erik Leitch
 */
#include <map>

#include "gcp/util/common/NetStruct.h"
#include "gcp/util/common/NetUnion.h"

namespace gcp {
  namespace util {

    // Define an allocator for an experiment-specific network message.
    // This must be defined by inheritors

    class NewNetMsg;
    NewNetMsg* newSpecificNewNetMsg();

    // Send a greeting message

    class NewNetGreetingMsg : public gcp::util::NetStruct {
    public:
      unsigned int revision;
      unsigned int nReg;
      unsigned int nByte;

      NewNetGreetingMsg() {
	NETSTRUCT_UINT(revision);
	NETSTRUCT_UINT(nReg);
	NETSTRUCT_UINT(nByte);
      }
    };

    // The following object is used to forward log messages from the
    // real-time control system to the control program. Note that
    // cnt_send_log_message() in controller.c relies on the text[]
    // member of NetLogMsg being the last member (in order not to have
    // to copy all NET_LOG_MAX+1 bytes of text[] if the string in
    // text[] is shorter).

    #define NET_LOG_MAX 127

    class NewNetLogMsg : public gcp::util::NetStruct {
    public:
      short bad;                // True if the text describes an error
				// condition
      unsigned seq;
      bool end;
      char text[NET_LOG_MAX+1]; // The text of the log message 

      NewNetLogMsg() {
	NETSTRUCT_SHORT(bad);     // True if the text describes an
				  // error condition
	NETSTRUCT_UINT(seq);
	NETSTRUCT_BOOL(end);
	NETSTRUCT_CHAR_ARR(text, NET_LOG_MAX+1); // The text of the log message 
      }
    };
    
    // The following object is used by the receiver-control task to
    // report the completion of a setreg transactions.

    class NewNetSetregDoneMsg : public gcp::util::NetStruct {
    public:
      unsigned int seq;      // The sequence number of the
				 // transaction that completed

      NewNetSetregDoneMsg() {
	NETSTRUCT_UINT(seq);      // The sequence number of the transaction
			   // that completed
      }
    };
    
    // The following object is used by the scanner task to report the
    // completion of a tv_offset transaction.

    class NewNetTvOffsetDoneMsg : public gcp::util::NetStruct {
    public:
      unsigned int seq;      // The sequence number of the
				 // transaction that completed

      NewNetTvOffsetDoneMsg() {
	NETSTRUCT_UINT(seq);      // The sequence number of the transaction
			   // that completed
      }
    };
    
    
    // The following object is used by the tracker task to report
    // target acquisition.

    class NewNetFrameDoneMsg : public gcp::util::NetStruct {
    public:
      unsigned int seq;      // The sequence number of the
				 // transaction that completed

      NewNetFrameDoneMsg() {
	NETSTRUCT_UINT(seq);      // The sequence number of the transaction
			   // that completed
      }
    };

    // The following object is used by the tracker task to report
    // target acquisition.

    class NewNetDriveDoneMsg : public gcp::util::NetStruct {
    public:
      unsigned int seq;      // The sequence number of the
				 // transaction that completed

      NewNetDriveDoneMsg() {
	NETSTRUCT_UINT(seq);      // The sequence number of the transaction
			   // that completed
      }
    };
    
    // The following object is used by the tracker task to report
    // target acquisition.

    class NewNetBenchDoneMsg : public gcp::util::NetStruct {
    public:
      unsigned int seq;      // The sequence number of the
				 // transaction that completed

      NewNetBenchDoneMsg() {
	NETSTRUCT_UINT(seq);      // The sequence number of the transaction
			   // that completed
      }
    };

    // The following object is used by the tracker task to report
    // target acquisition.

    class NewNetScanDoneMsg : public gcp::util::NetStruct {
    public:
      unsigned int seq;      // The sequence number of the
				 // transaction that completed

      NewNetScanDoneMsg() {
	NETSTRUCT_UINT(seq);      // The sequence number of the transaction
			   // that completed
      }
    };

    
    // The following object is used by the tracker task to report if
    // the telescope can't reach the current source due to it being
    // too low in the sky.

    class NewNetSourceSetMsg : public NetStruct {
    public:
      unsigned int seq;      // The sequence number of the
                              // transaction that resulted in the
                              // telescope needing to point too
                              // low.

      NewNetSourceSetMsg() {
	NETSTRUCT_UINT(seq); // The sequence number of the
			      // transaction that resulted in the
			      // telescope needing to point too low.
      }
    };

    // The following object is used by the tracker task to report if
    // the telescope can't reach the current source due to it being
    // too low in the sky.

    class NewNetScriptDoneMsg : public NetStruct {
    public:
      unsigned int seq;      // The sequence number of the
                              // transaction that resulted in the
                              // telescope needing to point too
                              // low.

      NewNetScriptDoneMsg() {
	NETSTRUCT_UINT(seq); // The sequence number of the
			      // transaction that resulted in the
			      // telescope needing to point too low.
      }
    };

    /**
     * A class for collecting together various messages that will be
     * sent across the network
     */
    class NewNetMsg : public NetUnion {
    public:

      // Enumerate structs that are part of this message

      enum {
	NET_UNKNOWN_MSG,          // No message has been specified
	NET_FRAME_DONE_MSG,       // A frame completion message
	NET_GREETING_MSG,         // A handshake message
	NET_ID_MSG,               // A message from an antenna to
				  // identify itself 
	NET_LOG_MSG,              // A message to be logged
	NET_DRIVE_DONE_MSG,        // A pmac transaction-completion message 
	NET_BENCH_DONE_MSG,        // A BENCH transaction-completion message 
	NET_SCAN_DONE_MSG,        // A scan-completion message 
	NET_SOURCE_SET_MSG,       // A warning message that the source has set 
	NET_SETREG_DONE_MSG,      // A setreg transaction completion message 
	NET_TV_OFFSET_DONE_MSG,   // A tv_offset transaction completion message 
	NET_NAV_UPDATE_MSG,       // A request from the controller for
	                          // a re-initialization of ephemeris
	                          // positions from the navigator
	                          // thread

	NET_SCRIPT_DONE_MSG,
	NET_LAST_MSG,             // This should always come last
      };
     
      // This class will contain a representative of each type of
      // message

      NewNetFrameDoneMsg    frame_done;  // A greeting message
      NewNetGreetingMsg     greeting;    // A greeting message
      NewNetLogMsg          log;         // A message to be logged
      NewNetDriveDoneMsg    drive_done;   // A tracker
				      // transaction-completion message
      NewNetBenchDoneMsg    bench_done;   // A bench
				      // transaction-completion message
      NewNetScanDoneMsg     scan_done;   // A tracker
				      // transaction-completion message
      NewNetSetregDoneMsg   setreg_done; // A setreg transaction
				      // completion message
				      // NetSourceSetMsg source_set;
      NewNetSourceSetMsg    source_set;  // A source-has-set advisory
				      // message
      NewNetTvOffsetDoneMsg tv_offset_done;// A tv_offset transaction

      NewNetScriptDoneMsg   scriptDone;

      // Constructor/Destructors

      NewNetMsg();
      virtual ~NewNetMsg();

    }; // End class NewNetMsg

  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_NEWNETMSG_H
