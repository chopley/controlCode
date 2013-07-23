#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#if MAC_OSX == 0
#include <stropts.h> 
#endif

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#include "tcpip.h"

#include "lprintf.h"
#include "genericcontrol.h"

#include "gcp/util/common/ModemPager.h"
#include "gcp/util/common/SpecificName.h"
#include "gcp/util/common/FdSet.h"

#define MAPO_REDLIGHT_IP_ADDR "204.89.132.177" /* IP address of the MAPO
						  red light */
#define DOME_PAGER_IP_ADDR "199.4.251.81" /* IP address of the dome pager */

#define ELDORM_PAGER_IP_ADDR "199.4.251.81" /*IP address of the eldorm pager */

#define ISERVER_PORT 1000 /* default TCP socket on the iserver */

#define TERM_BAUD_RATE B19200

/*
 * Useful enumerators for opening & closing socket connections 
 */
enum {
  CLOSE=0,
  OPEN=1
};
/*
 * An object of the following type is used to hold the state of
 * the terminal I/O thread.
 */
struct Term {
  ControlProg* cp;  /* The state-object of the control program */
  gcp::util::Pipe* pipe;       /* An externally allocated control-input pipe */

  std::vector<std::string>* emails_;

  bool pagingEnabled_;

  gcp::util::ModemPager* modemPager_;
};
/*
 * Local commands for manipulating pager device resources
 */
static int activate_reg_pager(Term *term, char* reg=NULL);
static int activate_msg_pager(Term *term, char* msg=NULL);
static int pagerAddEmailAddress(Term *term, bool add, char *email);
static int enable_pager(Term *term, bool enable, bool enableModem=true);

static void sendModemPage(Term *term, std::string message);
static int  sendEmailPage(Term *term, std::string message);

/*.......................................................................
 * Create the state object of a terminal I/O thread.
 *
 * Input:
 *
 *  cp     ControlProg *   The state object of the control program.
 *  pipe          Pipe *   The pipe on which to listen for control messages.
 *
 * Output:
 *
 *  return        void *   The new Term object, or NULL on error.
 */
CP_NEW_FN(new_Term)
{
  Term *term=NULL; /* The object to be returned */
  /*
   * Allocate the container.
   */
  if((term=(Term *)malloc(sizeof(Term)))==NULL) {
    lprintf(stderr, "new_Term: Insufficient memory.\n");
    return NULL;
  };

  term->cp = cp;
  term->pipe = pipe;

  term->pagingEnabled_ = true;

  term->emails_ = 0;
  term->emails_ = new std::vector<std::string>();

  term->modemPager_ = 0;
  if(cp_useModemPager(cp)) {
    term->modemPager_ = new gcp::util::ModemPager(stdout, lprintf_to_logger, cp,
						  stderr, lprintf_to_logger, cp);
    term->modemPager_->spawn();
  }

  return term;
}
/*.......................................................................
 * Delete the state-object of a term thread.
 *
 * Input:
 *  obj         void *  The Term object to be deleted.
 *
 * Output:
 *  return      void *  The deleted Term object (always NULL).
 */
CP_DEL_FN(del_Term)
{
  Term *term = (Term* )obj;

  if(term) {

    if(term->modemPager_) {
      delete term->modemPager_;
      term->modemPager_ = 0;
    }

    if(term->emails_) {
      delete term->emails_;
      term->emails_ = 0;
    }
    
    // Finally, free the container itself

    free(term);
    term = NULL;
  }

  return NULL;
}
/*.......................................................................
 * Return the term resource object.
 *
 * Input:
 *  cp       ControlProg *   The control program resource object.
 *
 * Output:
 *  return      Term *   The term resource object.
 */
static Term *cp_Term(ControlProg *cp)
{
  return (Term* )cp_ThreadData(cp, CP_TERM);
}

/*.......................................................................
 * Attempt to send a message to a term thread.
 *
 * Input:
 *
 *  cp       ControlProg *  The state-object of the control program.
 *  msg      TermMessage *  The message to be sent. This must have been
 *                          filled by one of the pack_term_<type>()
 *                          functions.
 *  timeout  long           The max number of milliseconds to wait for the
 *                          message to be sent, or -1 to wait indefinitely.
 *
 * Output:
 *
 *  return   PipeState      The status of the transaction:
 *
 *                            PIPE_OK    - The message was read successfully.
 *                            PIPE_BUSY  - The send couldn't be accomplished
 *                                         without blocking (only returned
 *                                         when timeout=PIPE_NOWAIT).
 *                            PIPE_ERROR - An error occurred.
 */
PipeState send_TermMessage(ControlProg *cp, TermMessage *msg,
			       int timeout)
{
  return cp_Term(cp)->pipe->write(msg, sizeof(*msg), timeout);
}

/*.......................................................................
 * Send a shutdown message to the term thread using non-blocking I/O.
 * Return 0 if the message was sent, 0 otherwise.
 *
 * Input:
 *  cp      ControlProg *  The control-program resource object.
 *
 * Output:
 *  return          int    0 - Message sent ok.
 *                         1 - Unable to send message.
 */
CP_STOP_FN(stop_Term)
{
  TermMessage msg;   /* The message to be sent */
  return pack_term_shutdown(&msg) ||
         send_TermMessage(cp, &msg, PIPE_NOWAIT) != PIPE_OK;
}

/*.......................................................................
 * Prepare a shutdown message for subsequent transmission to the
 * term thread.
 *
 * Input:
 *  msg     TermMessage *  The message object to be prepared.
 *
 * Output:
 * return   int            0 - OK.
 *                         1 - Error.
 */
int pack_term_shutdown(TermMessage *msg)
{
/*
 * Check arguments.
 */
  if(!msg) {
    lprintf(stderr, "pack_term_shutdown: NULL argument.\n");
    return 1;
  };
  msg->type = TERM_SHUTDOWN;
  return 0;
}

/**.......................................................................
 * Return a list of pager email addresses.
 */
std::vector<std::string>* getPagerEmailList(ControlProg *cp)
{
  return cp_Term(cp)->emails_;
}

/*.......................................................................
 * Prepare a port message for subsequent transmission to the
 * term thread.
 *
 * Input:
 *
 *    msg    TermMessage *  The message object to be prepared.
 *    dev    PagerDev       The device whose IP address we are changing
 *    ip     char *         The new IP address
 *
 * Output:
 *
 *   return  int            0 - OK.
 *                          1 - Error.
 */
int pack_pager_ip(TermMessage *msg, PagerDev dev, char *ip)
{
  size_t len;
  /*
   * Check arguments.
   */
  if(!msg) {
    lprintf(stderr, "pack_term_port: NULL argument.\n");
    return 1;
  };
  msg->type = TERM_IP;

  if(ip) {
    len = strlen(ip);

    if(len > CP_FILENAME_MAX) {
      lprintf(stderr,"IP address too long\n");
      return 1;
    }
    strncpy(msg->body.ip, ip, len);
    msg->body.ip[len] = '\0';
  } else {
    msg->body.ip[0] = '\0';
  }
  msg->dev = dev;

  return 0;
}

/*.......................................................................
 * Prepare a pager enable message for subsequent transmission to the
 * term thread.  
 */
int pack_pager_enable(TermMessage *msg, bool enable)
{
  // Check arguments.

  if(!msg) {
    lprintf(stderr, "pack_pager_enable: NULL argument.\n");
    return 1;
  };

  msg->type = TERM_ENABLE;
  msg->body.enable.enable = enable;

  return 0;
}

/*.......................................................................
 * Prepare an email message for subsequent transmission to the
 * term thread.
 *
 * Input:
 *
 *    msg    TermMessage *  The message object to be prepared.
 *    dev    PagerDev       The device whose IP address we are changing
 *    ip     char *         The new IP address
 *
 * Output:
 *
 *   return  int            0 - OK.
 *                          1 - Error.
 */
int pack_pager_email(TermMessage *msg, bool add, char *email)
{
  size_t len;
  
  // Check arguments.

  if(!msg) {
    lprintf(stderr, "pack_pager_email: NULL argument.\n");
    return 1;
  };

  msg->type = TERM_EMAIL;
  msg->body.email.add = add;

  if(email) {
    len = strlen(email);

    if(len > CP_FILENAME_MAX) {
      lprintf(stderr,"Email address too long\n");
      return 1;
    }
    strncpy(msg->body.email.address, email, len);
    msg->body.email.address[len] = '\0';
  } else {

    // Don't allow addition of null addresses

    if(add) 
      ThrowError("No Email address specified");

    msg->body.email.address[0] = '\0';
  }
    
  return 0;
}
/*.......................................................................
 * Prepare a pager message for subsequent transmission to the term
 * thread.
 */
int pack_term_reg_page(TermMessage *msg, char* reg)
{
  // Check arguments.

  if(!msg) {
    lprintf(stderr, "pack_term_page: NULL argument.\n");
    return 1;
  };

  if(reg) {
    unsigned len = strlen(reg);

    if(len > CP_FILENAME_MAX) {
      lprintf(stderr,"Register name too long\n");
      return 1;
    }
    strncpy(msg->body.page.msg, reg, len);
    msg->body.page.msg[len] = '\0';
  } else {
    msg->body.page.msg[0] = '\0';
  }

  msg->type         = TERM_REG_PAGE;
  msg->body.page.on = true;

  return 0;
}

/**.......................................................................
 * A handler to be called when a register goes out of range
 */
MONITOR_CONDITION_HANDLER(sendRegPage)
{
  ControlProg* cp = (ControlProg*) arg;
  send_reg_page_msg(cp, (char*)message.c_str());
}

/*.......................................................................
 * Prepare a pager message for subsequent transmission to the term
 * thread.
 */
int send_reg_page_msg(ControlProg* cp, char* reg)
{
  unsigned waserr = 0;

  // Only send the page if paging is enabled

  if(cp_Term(cp)->pagingEnabled_) {
    TermMessage msg;
    waserr |= pack_term_reg_page(&msg, reg);
    waserr |= send_TermMessage(cp, &msg, PIPE_WAIT)==PIPE_ERROR;
    enable_pager(cp_Term(cp), false, false);
  }

  return waserr;
}

/*.......................................................................
 * Prepare a pager message for subsequent transmission to the term
 * thread.
 */
int pack_term_msg_page(TermMessage *msg, std::string txt)
{
  // Check arguments.

  if(!msg) {
    lprintf(stderr, "pack_term_msg_page: NULL argument.\n");
    return 1;
  };

  unsigned len = txt.size();

  if(len > CP_FILENAME_MAX) {
    lprintf(stderr,"Register name too long\n");
    return 1;
  }

  strncpy(msg->body.page.msg, (char*)txt.c_str(), len);
  msg->body.page.msg[len] = '\0';

  msg->type         = TERM_MSG_PAGE;
  msg->body.page.on = true;

  return 0;
}

/*.......................................................................
 * This is the entry-point of the term thread.
 *
 * Input:
 *  arg          void *  A pointer to the Term state object pointer,
 *                       cast to (void *).
 * Output:
 *  return       void *  NULL.
 */
CP_THREAD_FN(term_thread)
{
  Term *term = (Term* )arg; /* The state-object of the current thread */
  TermMessage msg; /* An message reception container */
  
  // Enable logging of the scheduler's stdout and stderr streams.

  if(log_thread_stream(term->cp, stdout) ||
     log_thread_stream(term->cp, stderr)) {
    cp_report_exit(term->cp);
    return NULL;
  };
  
  // Wait for commands from other threads.

  gcp::util::FdSet fdSet;
  fdSet.registerReadFd(term->pipe->readFd());
  
  while(select(fdSet.size(), fdSet.readFdSet(), NULL, NULL, NULL) > 0) {
    
    if(term->pipe->read(&msg, sizeof(msg), PIPE_WAIT) != PIPE_OK)
      break;
    
    // Interpret the message.
    
    switch(msg.type) {
    case TERM_SHUTDOWN: /* A shutdown message to the term thread */
      cp_report_exit(term->cp); /* Report the exit to the control thread */
      return NULL;
      break;
    case TERM_EMAIL:
      pagerAddEmailAddress(term, msg.body.email.add, msg.body.email.address);
      break;
    case TERM_ENABLE:
      enable_pager(term, msg.body.enable.enable);
      break;
    case TERM_REG_PAGE:
      activate_reg_pager(term, msg.body.page.msg);
      break;
    case TERM_MSG_PAGE:
      activate_msg_pager(term, msg.body.page.msg);
      break;
    default:
      lprintf(stderr, "term_thread: Unknown command-type received.\n");
      break;
    };
  };
  lprintf(stderr, "Term thread exiting after pipe read error.\n");
  cp_report_exit(term->cp);

  return NULL;
}

/*.......................................................................
 * De/Activate the pager.
 *
 * term      *  Term  The parent container
 * activate     int   True (1) to turn the pager on
 *                    False (0) to turn it off
 *
 * 
 *
 */
static int activate_reg_pager(Term *term, char* reg)
{
  int waserr=0;
  std::ostringstream os;

  os << "Pager was activated ";

  if(reg==0 || reg[0]=='\0')
    os << "(no register was specified)";
  else
    os << "(by register " << reg << ")";
  
  // Now send the message to various devices
  
  waserr  = sendEmailPage(term, os.str());
  
  try {
    sendModemPage(term, os.str());
  } catch(...) {
    waserr = 1;
  }
  
  // And disallow further pages until the pager is re-enabled

  waserr |= enable_pager(term, false, false);

  return waserr;
}

static int sendEmailPage(Term *term, std::string message)
{
  int waserr=0;
  std::ostringstream os;

  for(unsigned i=0; i < term->emails_->size(); i++) {

    os.str("");

    os << "echo \" " << message << "\" ";
    os << "| Mail -s \"" << gcp::util::SpecificName::experimentNameCaps() 
       << " Alert\" "
       << term->emails_->at(i);

    system(os.str().c_str());
  }

  return waserr;
}
static void sendModemPage(Term *term, std::string message)
{
  if(term->modemPager_)
    term->modemPager_->activate(message);
}

/*.......................................................................
 * Enable/disable the pager
 * 
 * term Term* The parent container enable bool True to enable the
 * pager False to disable it
 */
static int enable_pager(Term *term, bool enable,  bool enableModem)
{
  term->pagingEnabled_ = enable;

  try {

    if(term->modemPager_ && enableModem) {
      if(enable)
	term->modemPager_->enable(true);
      else
	term->modemPager_->enable(false);
    }

  } catch(...) {
    return 1;
  }
  
  // Tell remote monitor clients about our new status                                             
  if(sendPagingState(term->cp, enable))
    return 1;

  return 0;
}

/*.......................................................................
 * De/Activate the pager.
 *                          
 * term * Term The parent container activate int True (1) to turn the
 * pager on False (0) to turn it off
 */
static int activate_msg_pager(Term *term, char* msg)
{
  int waserr=0;
  std::ostringstream os;

  os.str("");
  os << "echo \"" << msg << "\" ";

  waserr |= sendEmailPage(term, os.str());

  try {
    sendModemPage(term, os.str());
  } catch(...) {
    waserr = 1;
  }

  return waserr;
}

/*.......................................................................
 * Add an email address to the list to be notified on a page
 *
 * Input:
 *  dev      char *   The device to use when creating subsequent
 *                    term files.
 * Output:
 *  return    int     0 - OK.
 *                    1 - Error.
 */
static int pagerAddEmailAddress(Term *term, bool add, char* email)
{
  if(add)
    term->emails_->push_back(email);
  else {

    if(email==NULL || email[0]=='\0') {
      term->emails_->clear();
      return 0;
    }

    std::vector<std::string>::iterator iString;

    for(iString=term->emails_->begin(); iString != term->emails_->end();
	iString++)
      if(*iString == email)
	break;
    if(iString != term->emails_->end())
      term->emails_->erase(iString);
  }

  return 0;
}

/*.......................................................................
 * Change the device we use to communicate with the pager
 *
 * Input:
 *  dev      char *   The device to use when creating subsequent
 *                    term files.
 * Output:
 *  return    int     0 - OK.
 *                    1 - Error.
 */
static int pager_addEmailAddress(Term *term, bool add, char* email)
{
  if(add)
    term->emails_->push_back(email);
  else {
    
  }

  std::cout << "Vector has nelements: " << term->emails_->size() << std::endl;
  return 0;
}

void requestPagerStatus(ControlProg* cp)
{
  Term* term = cp_Term(cp);

  if(term->modemPager_)
    term->modemPager_->requestStatus();
}
