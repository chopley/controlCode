#ifndef control_h
#define control_h

#include "gcp/control/code/unix/libunix_src/common/netobj.h"
#include "gcp/util/common/Ports.h"

namespace gcp {
  namespace control {

    /*-----------------------------------------------------------------------
     * Define the ids of client -> control-program commands along with
     * the corresponding local command containers.
     */
    typedef enum {
      CC_INPUT_CMD              /* A command line typed by the user */
    } CcNetCmdId;
    
    /*
     * The CC_TEXT_LINE command conveys a single text command line to the
     * control program.
     */
    
    enum {CC_CMD_MAX=255}; /* The max size of a command string (excluding '\0') */
    
    typedef struct {
      char cmd[CC_CMD_MAX+1];  /* The ascii command string (including '\0') */
    } CcInputCmd;
    
    /*
     * Create a union of the above message containers.
     */
    typedef union {
      CcInputCmd input;        /* A command line to be compiled and executed */
    } CcNetCmd;
  }
}
/*
 * The following network-object description table, defined in control.c,
 * describes the above commands.
 */
extern const NetObjTable cc_cmd_table;

namespace gcp {
  namespace control {


    typedef enum {
      UPRIGHT,      // A positive increment on the image is a positive
		    // increment on the sky
      INVERTED      // A positivce increment on the image is a
		    // negative increment on the sky
    } ImDir;
    
    typedef enum {
      CW,          // Angle increases in the clockwise direction
      CCW          // Angle increases in the counter-clockwise direction
    } RotationSense;


    /*-----------------------------------------------------------------------
     * Define the types of messages that are sent to control clients by the
     * control program.
     */
    typedef enum {
      CC_LOG_MSG,          /* A log message to be displayed */
      CC_REPLY_MSG,        /* A reply to a CC_INPUT_CMD command line */
      CC_SCHED_MSG,        /* A message regarding the state of the scheduler */
      CC_ARC_MSG,          /* A message regarding the state of the archiver */
      CC_PAGE_MSG,         /* A message regarding the pager status */
      CC_ANT_MSG,          /* A message regarding the state of the default 
			      antenna selection */
      CC_GRABBER_MSG,      /* A message regarding the grabber status */
      CC_PAGECOND_MSG,     /* A message regarding the pager status */
      CC_CMD_TIMEOUT_MSG
    } CcNetMsgId;
    
    /*
     * The following interface is for use in sending and receiving messages
     * sent from the control program to control clients.
     */
    enum {CC_MSG_MAX=131}; /* The max length of a message string (excluding '\0') */
    
    /*
     * Define a log message object.
     */
    typedef struct {
      unsigned seq;
      NetBool end;
      NetBool error;            /* True if the text is an error message */
      NetBool interactive;      /* True if the text was the result of an interactive message */
      char text[CC_MSG_MAX+1];  /* The ascii message string (including '\0') */
    } CcLogMsg;
    
    /*
     * Define a reply message object.
     */
    typedef struct {
      NetBool error;            /* True if the text is an error message */
      char text[CC_MSG_MAX+1];  /* The ascii message string (including '\0') */
    } CcReplyMsg;
    
    /*
     * Define a message for reporting changes in the state of the
     * schedule queue.
     */
    typedef struct {
      char text[CC_MSG_MAX+1];  /* The status message */
    } CcSchedMsg;
    
    /*
     * Define a message for reporting changes in the state of the
     * archiver.
     */
    typedef struct {
      char text[CC_MSG_MAX+1];  /* The status message */
    } CcArcMsg;
    
    enum {
      PAGE_ENABLE = 0x1, // Set if this is an ordinary page
			 // enable/disable message
      PAGE_REG    = 0x2, // Set if this is page enable/disable message
                         // was sent because the pager was activated
      PAGE_MSG    = 0x4, // Set if this is page enable/disable message
                         // was sent because of some other reason
    };

    /*
     * Define a message for reporting changes in the state of the
     * pager
     */
    typedef struct {
      char text[CC_MSG_MAX+1];  /* The status message */
      NetEnum mask;
      NetBool allow;
    } CcPageMsg;
    
    /*
     * Define a message for reporting changes in the state of the
     * default antenna selection
     */
    typedef struct {
      char text[CC_MSG_MAX+1];  /* The status message */
    } CcAntMsg;

    /**
     * Enumerate frame grabber configuration parameters
     */
    typedef enum {
      CFG_NONE               =    0x0,
      CFG_CHANNEL            =    0x1,
      CFG_COMBINE            =    0x2,
      CFG_FLATFIELD          =    0x4,
      CFG_FOV                =    0x8,
      CFG_ASPECT             =   0x10,
      CFG_COLLIMATION        =   0x20,
      CFG_XIMDIR             =   0x40,
      CFG_YIMDIR             =   0x80,
      CFG_DKROTSENSE         =  0x100,
      CFG_PEAK_OFFSETS       =  0x200,
      CFG_CHAN_ASSIGN        =  0x400,
      CFG_ADD_SEARCH_BOX     =  0x800,
      CFG_REM_SEARCH_BOX     = 0x1000,
      CFG_REM_ALL_SEARCH_BOX = 0x2000,

      CFG_ALL = CFG_CHANNEL | CFG_COMBINE | CFG_FLATFIELD | CFG_FOV | CFG_ASPECT | CFG_COLLIMATION | CFG_XIMDIR | CFG_YIMDIR | CFG_DKROTSENSE | CFG_PEAK_OFFSETS | CFG_CHAN_ASSIGN | CFG_ADD_SEARCH_BOX
    } CcFrameGrabberOpt;
    
    typedef struct {
      NetEnum mask;         // The mask of parameters to configure 
      unsigned channelMask; // The channel number to selec 
      unsigned nCombine;    // The number of frames to combine 
      unsigned flatfield;   // The current flatfield option 
      double fov;           // The current field of view 
      double aspect;        // The current aspect ratio 
      double collimation;   // The current aspect ratio 
      ImDir ximdir;              // The orientation of the x-axis
      ImDir yimdir;              // The orientation of the y-axis
      RotationSense dkRotSense;  // The sense of the deck rotation
      unsigned ipeak;
      unsigned jpeak;
      unsigned ptelMask;
      unsigned ixmin;
      unsigned ixmax;
      unsigned iymin;
      unsigned iymax;
      bool inc;
    } CcFrameGrabberMsg;
    
    /*
     * Define a message for reporting changes in the state of the
     * archiver.
     */
    typedef struct {
      unsigned mode;  
      double min;
      double max;
      bool isDelta;
      bool isOutOfRange;
      unsigned nFrame;
      char text[CC_MSG_MAX+1];  /* The status message */
    } CcPageCondMsg;

    enum PageCondMode {
      PAGECOND_ADD,
      PAGECOND_REMOVE,
      PAGECOND_CLEAR,
      PAGECOND_UPDATE,
    };

    /*
     * Define a message for reporting changes in the state of the
     * timeout pager
     */
    typedef struct {
      unsigned mode;
      unsigned seconds;
      bool active;
    } CcCmdTimeoutMsg;

    enum CmdTimeoutMode {
      CT_CMD_ACTIVE,
      CT_CMD_TIMEOUT,
      CT_DATA_ACTIVE,
      CT_DATA_TIMEOUT,
    };

    /*
     * Define a union of all control-program -> control-client message types.
     */
    typedef union {
      CcLogMsg log;             /* A CC_LOG_MSG message */
      CcReplyMsg reply;         /* A CC_REPLY_MSG message */
      CcSchedMsg sched;         /* A CC_SCHED_MSG message */
      CcArcMsg arc;             /* A CC_ARC_MSG message */
      CcPageMsg page;           /* A CC_PAGE_MSG message */
      CcAntMsg ant;             /* A CC_ANT_MSG message */
      CcFrameGrabberMsg grabber;/* A CC_GRABBER_MSG message */
      CcPageCondMsg pageCond;   /* A CC_PAGECOND_MSG message */
      CcCmdTimeoutMsg cmdTimeout; // A command timeout message
    } CcNetMsg;
  }
}  
/*
 * The following network-object description table, defined in control.c,
 * describes the messages that are sent from the control program to
 * control clients.
 */
extern const NetObjTable cc_msg_table;

#endif
