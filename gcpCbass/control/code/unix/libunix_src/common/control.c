#include <string.h>

#include "control.h"
#include "netbuf.h"
#include "netobj.h"
#include "lprintf.h"


#define ARRAY_DIM(array) (sizeof(array)/sizeof(array[0]))

namespace gcp {
  namespace control{
    /*-----------------------------------------------------------------------
     * Separately describe each of the CcNetCmd objects in control.h.
     */
    static const NetObjMember input_cmd_members[] = {
      {"cmd", offsetof(CcInputCmd,cmd), NET_ASCII, CC_CMD_MAX+1}
    };
    
    /*
     * Collect all of the object definitions in an array.
     */
    static const NetObjInfo cmd_objects[] = {
      {"input",          input_cmd_members,         ARRAY_DIM(input_cmd_members),
       sizeof(CcInputCmd)},
    };
    
  }
}
/*
 * Form the global table of objects that is used by the functions
 * in netobj.h.
 */
const NetObjTable cc_cmd_table = {
  "cmd", gcp::control::cmd_objects, sizeof(gcp::control::cmd_objects)/sizeof(NetObjInfo), sizeof(gcp::control::CcNetCmd)
};

namespace gcp {
  namespace control {
    /*-----------------------------------------------------------------------
     * Separately describe each of the CcNetMsg objects in control.h.
     */
    static const NetObjMember log_msg_members[] = {
      {"seq",      offsetof(CcLogMsg,seq),      NET_INT,    1},
      {"end",      offsetof(CcLogMsg,end),      NET_BOOL,   1},
      {"error",    offsetof(CcLogMsg,error),    NET_BOOL,   1},
      {"interactive", offsetof(CcLogMsg,interactive), NET_BOOL,   1},
      {"text",     offsetof(CcLogMsg,text),     NET_ASCII,  CC_MSG_MAX+1}
    };
    static const NetObjMember reply_msg_members[] = {
      {"error",    offsetof(CcReplyMsg,error),  NET_BOOL,   1},
      {"text",     offsetof(CcReplyMsg,text),   NET_ASCII,  CC_MSG_MAX+1}
    };
    static const NetObjMember sched_msg_members[] = {
      {"text",     offsetof(CcSchedMsg,text),   NET_ASCII,  CC_MSG_MAX+1}
    };
    static const NetObjMember arc_msg_members[] = {
      {"text",     offsetof(CcArcMsg,text),     NET_ASCII,  CC_MSG_MAX+1}
    };
    static const NetObjMember page_msg_members[] = {
      {"page",     offsetof(CcPageMsg,allow),   NET_BOOL,   1}
    };
    static const NetObjMember ant_msg_members[] = {
      {"text",     offsetof(CcAntMsg,text),     NET_ASCII,  CC_MSG_MAX+1}
    };
    
    static const NetObjMember frameGrabber_msg_members[] = {
      {"mask",        offsetof(CcFrameGrabberMsg, mask),        NET_MASK,   1},
      {"channelMask", offsetof(CcFrameGrabberMsg, channelMask), NET_MASK,   1},
      {"ncombine",    offsetof(CcFrameGrabberMsg, nCombine),    NET_INT,    1},
      {"flatfield",   offsetof(CcFrameGrabberMsg, flatfield),   NET_INT,    1},
      {"fov",         offsetof(CcFrameGrabberMsg, fov),         NET_DOUBLE, 1},
      {"aspect",      offsetof(CcFrameGrabberMsg, aspect),      NET_DOUBLE, 1},
      {"collimation", offsetof(CcFrameGrabberMsg, collimation), NET_DOUBLE, 1},
      {"ximdir",      offsetof(CcFrameGrabberMsg, ximdir),      NET_INT,    1},
      {"yimdir",      offsetof(CcFrameGrabberMsg, yimdir),      NET_INT,    1},
      {"dkRotSense",  offsetof(CcFrameGrabberMsg, dkRotSense),  NET_INT,    1},
      {"ipeak",       offsetof(CcFrameGrabberMsg, ipeak),       NET_INT,    1},
      {"jpeak",       offsetof(CcFrameGrabberMsg, jpeak),       NET_INT,    1},
      {"ptelMask",    offsetof(CcFrameGrabberMsg, ptelMask),    NET_MASK,   1},
      {"ixmin",        offsetof(CcFrameGrabberMsg, ixmin),      NET_INT,    1},
      {"ixmax",        offsetof(CcFrameGrabberMsg, ixmax),      NET_INT,    1},
      {"iymin",        offsetof(CcFrameGrabberMsg, iymin),      NET_INT,    1},
      {"iymax",        offsetof(CcFrameGrabberMsg, iymax),      NET_INT,    1},
      {"inc",         offsetof(CcFrameGrabberMsg, inc),         NET_BOOL,   1},
    };

    static const NetObjMember pageCond_msg_members[] = {
      {"mode",         offsetof(CcPageCondMsg, mode),         NET_INT,    1},
      {"min",          offsetof(CcPageCondMsg, min),          NET_DOUBLE, 1},
      {"max",          offsetof(CcPageCondMsg, max),          NET_DOUBLE, 1},
      {"isDelta",      offsetof(CcPageCondMsg, isDelta),      NET_BOOL,   1},
      {"isOutOfRange", offsetof(CcPageCondMsg, isOutOfRange), NET_BOOL,   1},
      {"nFrame",       offsetof(CcPageCondMsg, nFrame),       NET_INT,    1},
      {"text",         offsetof(CcPageCondMsg, text),         NET_ASCII,  CC_MSG_MAX+1},
    };

    static const NetObjMember cmdTimeout_msg_members[] = {
      {"mode",         offsetof(CcCmdTimeoutMsg, mode),       NET_INT,    1},
      {"seconds",      offsetof(CcCmdTimeoutMsg, seconds),    NET_INT,    1},
      {"active",       offsetof(CcCmdTimeoutMsg, active),     NET_BOOL,   1},
    };

    /*
     * Collect all of the message object definitions in an array.
     */
    static const NetObjInfo msg_objects[] = {
      {"log",          log_msg_members,         ARRAY_DIM(log_msg_members),
       sizeof(CcLogMsg)},
      {"reply",        reply_msg_members,       ARRAY_DIM(reply_msg_members),
       sizeof(CcReplyMsg)},
      {"sched",        sched_msg_members,       ARRAY_DIM(sched_msg_members),
       sizeof(CcSchedMsg)},
      {"arc",          arc_msg_members,         ARRAY_DIM(arc_msg_members),
       sizeof(CcArcMsg)},
      {"page",         page_msg_members,        ARRAY_DIM(page_msg_members),
       sizeof(CcPageMsg)},
      {"ant",         ant_msg_members,          ARRAY_DIM(ant_msg_members),
       sizeof(CcAntMsg)},
      {"grabber",      frameGrabber_msg_members, ARRAY_DIM(frameGrabber_msg_members),
       sizeof(CcFrameGrabberMsg)},
      {"pageCond",     pageCond_msg_members,    ARRAY_DIM(pageCond_msg_members),
       sizeof(CcPageCondMsg)},
      {"cmdTimeout",   cmdTimeout_msg_members,  ARRAY_DIM(cmdTimeout_msg_members),
       sizeof(CcCmdTimeoutMsg)},
    };
  }
}
/*
 * Form the global table of message objects that is required by
 * the functions in netobj.h.
 */
const NetObjTable cc_msg_table = {
  "msg", gcp::control::msg_objects, sizeof(gcp::control::msg_objects)/sizeof(NetObjInfo), sizeof(gcp::control::CcNetMsg)
};

