#include <curses.h>
#include <term.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "gcp/util/common/Control.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/SpecificName.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/control/code/unix/libunix_src/common/control.h"

using namespace gcp::util;
using namespace std;

Control* Control::control_ = 0;

/**.......................................................................
 * Constructor.
 */
Control::Control(string host, bool log) :
  stop_(false), client_(host, CP_CONTROL_PORT), signalTask_(true)
{
  xterm_.setTextMode("bold");
  xterm_.setFg("yellow");
  
  control_ = this;
  
  // Register the exit handler with signalTask.
  
  signalTask_.sendInstallSignalMsg(SIGINT,  exitHandler);
  signalTask_.sendInstallSignalMsg(SIGQUIT, exitHandler);
  signalTask_.sendInstallSignalMsg(SIGABRT, exitHandler);

  // Allocate our thread objects

  readThread_ = new Thread(startRead, NULL, NULL, "read");
  sendThread_ = new Thread(startSend, NULL, NULL, "send");

  // Allocate the buffers used to communicate with the control program
  
  netStr_.setReadBuffer(0, NET_PREFIX_LEN + net_max_obj_size(&cc_msg_table));
  netStr_.setSendBuffer(0, NET_PREFIX_LEN + net_max_obj_size(&cc_cmd_table));
  
  // Install handlers for the network streams
  
  netStr_.getSendStr()->installSendHandler(&sendHandler, this);
  netStr_.getReadStr()->installReadHandler(&readHandler, this);

  netStr_.getSendStr()->installErrorHandler(&errorHandler, this);
  netStr_.getReadStr()->installErrorHandler(&errorHandler, this);
  
  // Attempt to connect 

  if(client_.connectToServer(true) < 0) {
    ThrowError("Unable to connect to the control host");
  }
  
  // And attach our streams to the file descriptor
  
  netStr_.attach(client_.getFd());

  // Print a greeting message

  cout << "Type 'exit' or cntl-C to exit" << endl;

  // If everything checks out, run our threads
  
  if(log)
    readThread_->start(this);

  // The calling thread will block in startup, since broadcastReady()
  // is not called in the startup function for this thread.

  sendThread_->start(this);
}

/**.......................................................................
 * Destructor.
 */
Control::~Control() 
{
  // Disconnect from the ACC

  control_->client_.disconnect();

  // And delete our threads

  delete readThread_;
  delete sendThread_;
}

/**.......................................................................
 * Run method which uses readline
 */
void Control::processCommands()
{
  LogStream logStr;
  char* buff=NULL;
  gcp::control::CcNetCmd netCmd;

  while(!stop_ && (buff=readline("gcpCommand> ")) != NULL) {
    
    add_history(buff);
    
    if(strlen(buff) > gcp::control::CC_CMD_MAX) {
      logStr.appendMessageSimple(true, "Line is too long");
      logStr.report();
    } else {
      
      string buffStr(buff);

      // Definitions of "standardized" C++ methods appear to be in
      // flux.  In particular, the string class compare method changed
      // the order of its arguments somewhere between gcc 2.96 and gcc
      // 3.2.2
      
#if (__GNUC__ > 2)
      if(buffStr.compare(0, 4, "exit")==0)
#else
      if(buffStr.compare("exit", 0, 4)==0)
#endif
	exitHandler(1, NULL);

      else {

	// Else pack it into our send buffer
	
	strcpy(netCmd.input.cmd, buff);
	
	// Pack the message into the output network buffer.
	
	NetSendStr* nss = netStr_.getSendStr();
	
	nss->startPut(gcp::control::CC_INPUT_CMD);
	nss->putObj(&cc_cmd_table, gcp::control::CC_INPUT_CMD, &netCmd);
	nss->endPut();
	
	// And send it
	
	netStr_.send();
      }
    }
    free(buff);
    buff = NULL;
  }

  // Call the exit handler on EOF

  if(!stop_)
    exitHandler(1, NULL);
}

/**.......................................................................
 * Run method which uses readline
 */
void Control::processCommands2()
{
  char* buff=NULL;
  gcp::control::CcNetCmd netCmd;

  ostringstream os, esc;

  bool doEsc=false;

  setRawMode(STDIN_FILENO);

  Port port(STDIN_FILENO);
  std::string str;

  FdSet fdSet;
  fdSet.registerReadFd(STDIN_FILENO);

  int nready=0;
  unsigned char c;
  unsigned nByte;

  while(!stop_) {

    std::cout << "\eF" << SpecificName::experimentName() << "Command> " << os.str();

    fflush(stdout);

    nready = select(fdSet.size(), fdSet.readFdSet(), 0, 0, 0);

    if(nready > 0) {
  
      if(fdSet.isSetInRead(STDIN_FILENO)) {

	nByte = port.getNbyte();

	for(unsigned i=0; i < nByte; i++) {
	  
	  if(::read(0, &c, 1) < 0) {
	    ThrowSysError("read()");
	  }

	  //------------------------------------------------------------
	  // Start accumlating an escape sequence
	  //------------------------------------------------------------

	  if(doEsc) {

	    esc << c;

	    if(esc.str().compare(0, 3, "[A")==0) {
	      esc.str("");
	      doEsc = false;

	      HIST_ENTRY* hist = previous_history();

	      if(hist)
		COUT("hist data is: " << hist->data);

	    } else if(esc.str().compare(0, 3, "[B")==0) {
	      esc.str("");
	      doEsc = false;
	      HIST_ENTRY* hist = next_history();

	      if(hist)
		COUT("hist data is: " << hist->data);
	    }

	  }

	  if(c == '\e') {
	    doEsc = true;
	  }

	  os << c;
	  
	  if(c == '\n') {
	    
	    add_history(os.str().c_str());
	    
	    if(os.str().size() > gcp::control::CC_CMD_MAX) {
	      ReportMessage("Line is too long");
	    } else {
	      
	    // Definitions of "standardized" C++ methods appear to be in
	    // flux.  In particular, the string class compare method changed
	    // the order of its arguments somewhere between gcc 2.96 and gcc
	    // 3.2.2
	      
#if (__GNUC__ > 2)
	      if(os.str().compare(0, 4, "exit")==0)
#else
		if(os.str().compare("exit", 0, 4)==0)
#endif
		  exitHandler(1, NULL);
	      
		else {
		  
		  // Else pack it into our send buffer
		  
		  strcpy(netCmd.input.cmd, os.str().c_str());
		  
		  // Pack the message into the output network buffer.
		  
		  NetSendStr* nss = netStr_.getSendStr();
		  
		  nss->startPut(gcp::control::CC_INPUT_CMD);
		  nss->putObj(&cc_cmd_table, gcp::control::CC_INPUT_CMD, &netCmd);
		  nss->endPut();
		  
		  // And send it
		  
		  netStr_.send();
		}
	    }
	    
	    os.str("");
	  }
	}
      }
    }

    // Call the exit handler on EOF

    if(stop_)
      exitHandler(1, NULL);
  }
}

/**.......................................................................
 * Run method which uses readline
 */
void Control::processAll()
{
  char* buff=NULL;

  doEsc_ = false;

  setRawMode(STDIN_FILENO);
  setRawMode(STDOUT_FILENO);

  Port port(STDIN_FILENO);
  std::string str;

  FdSet fdSet;
  fdSet.registerReadFd(STDIN_FILENO);
  fdSet_.registerReadFd(client_.getFd());

  int nready=0;
  unsigned char c;
  unsigned nByte;

  while(!stop_) {

    std::cout << "\eF" << "\r" << SpecificName::experimentName() << "Command> " << os_.str();

    fflush(stdout);

    nready = select(fdSet.size(), fdSet.readFdSet(), 0, 0, 0);

    if(nready > 0) {
  
      if(fdSet.isSetInRead(STDIN_FILENO)) {
	readStdin();
      }

      if(fdSet_.isSetInRead(client_.getFd())) {
	readMessage();
      }
    }

    // Call the exit handler on EOF

    if(stop_)
      exitHandler(1, NULL);
  }
}

void Control::readStdin() 
{
  gcp::control::CcNetCmd netCmd;
  static Port port(STDIN_FILENO);

  unsigned nByte = port.getNbyte();
      
  unsigned char c;

  for(unsigned i=0; i < nByte; i++) {
	  
    if(::read(0, &c, 1) < 0) {
      ThrowSysError("read()");
    }

    //------------------------------------------------------------
    // Start accumlating an escape sequence
    //------------------------------------------------------------
    
    if(doEsc_) {
      
      esc_ << c;
      
      if(esc_.str().compare(0, 3, "[A")==0) {
	esc_.str("");
	doEsc_ = false;
	
	HIST_ENTRY* hist = previous_history();
	
	if(hist)
	  COUT("hist data is: " << hist->data);
	
      } else if(esc_.str().compare(0, 3, "[B")==0) {
	esc_.str("");
	doEsc_ = false;
	HIST_ENTRY* hist = next_history();
	
	if(hist)
	  COUT("hist data is: " << hist->data);
      }
      
    }
    
    if(c == '\e') {
      doEsc_ = true;
    }
    
    os_ << c;
    
    if(c == '\n') {
      
      add_history(os_.str().c_str());
      
      if(os_.str().size() > gcp::control::CC_CMD_MAX) {
	ReportMessage("Line is too long");
      } else {
	
	// Definitions of "standardized" C++ methods appear to be in
	// flux.  In particular, the string class compare method changed
	// the order of its arguments somewhere between gcc 2.96 and gcc
	// 3.2.2
	
#if (__GNUC__ > 2)
	if(os_.str().compare(0, 4, "exit")==0)
#else
	  if(os_.str().compare("exit", 0, 4)==0)
#endif
	    exitHandler(1, NULL);
	
	  else {
	    
	    // Else pack it into our send buffer
	    
	    strcpy(netCmd.input.cmd, os_.str().c_str());
	    
	    // Pack the message into the output network buffer.
	    
	    NetSendStr* nss = netStr_.getSendStr();
	    
	    nss->startPut(gcp::control::CC_INPUT_CMD);
	    nss->putObj(&cc_cmd_table, gcp::control::CC_INPUT_CMD, &netCmd);
	    nss->endPut();
	    
	    // And send it
	    
	    netStr_.send();
	  }
      }
      
      os_.str("");
    }
  }
}

/**.......................................................................
 * Send a command to the control program
 */
void Control::sendCommand()
{
  netStr_.send();
}

/**.......................................................................
 * Method called when a command has been sent.
 */
NET_SEND_HANDLER(Control::sendHandler)
{
  Control* control = (Control*)arg;
  control->fdSet_.clearFromWriteFdSet(control->client_.getFd());
}

/**.......................................................................
 * Process messages received from the control program
 */
void Control::processMessages()
{
  int nready;
  fdSet_.registerReadFd(client_.getFd());

  while(!stop_) {

    nready = select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(),
		    NULL, NULL);

    if(fdSet_.isSetInRead(client_.getFd()))
      readMessage();
  }
}

void Control::readMessage() 
{
  netStr_.read();
};

/**.......................................................................
 * Method called when a message has been read.
 */
NET_READ_HANDLER(Control::readHandler)
{
  Control* control = (Control*)arg;

  gcp::control::CcNetMsg netMsg;
  gcp::control::CcNetMsgId opcode;

  NetReadStr* nrs = control->netStr_.getReadStr();

  nrs->startGet((int*)&opcode);
  nrs->getObj(&cc_msg_table, opcode, &netMsg);
  nrs->endGet();

  switch(opcode) {
  case gcp::control::CC_LOG_MSG:
    
    if(netMsg.log.error) {
      control->xterm_.setFg(XtermManip::C_RED);
    } else {
      control->xterm_.setFg(XtermManip::C_GREEN);
    }

    COUT("\r" << netMsg.log.text);

    break;
  case gcp::control::CC_REPLY_MSG:
    control->xterm_.setFg(XtermManip::C_BLUE);
    COUT("\r" << netMsg.reply.text);
    break;
  case gcp::control::CC_SCHED_MSG:
    COUT("\r" << netMsg.sched.text);
    break;
  case gcp::control::CC_ARC_MSG:
    COUT("\r" <<  netMsg.arc.text);
    break;
  case gcp::control::CC_ANT_MSG:
    COUT("\r" << netMsg.ant.text);
    break;
  default:
    break;
  }

  fflush(stdout);

  // Restore the default color scheme

  control->xterm_.setFg(XtermManip::C_YELLOW);
}

/**.......................................................................
 * Method called when an error occurs communicating with the control
 * program
 */
NET_SEND_HANDLER(Control::errorHandler)
{
  Control* control = (Control*)arg;
  control->exitHandler(1, NULL);
}

/**.......................................................................
 * Exit handler
 */
SIGNALTASK_HANDLER_FN(Control::exitHandler)
{
  LogStream logStr;

  logStr.appendMessageSimple(sigNo != 0, 
			     "Lost connection to the control program");
  logStr.report();

  // When the exit handler is called, the ready condition variable is
  // signalled, causing the main thread to exit its constructor.

  control_->xterm_.setTextMode("default");
  control_-> xterm_.setFg("default");

  control_->sendThread_->broadcastReady();

  control_->stop_ = true;

  //  exit(1);
}

THREAD_START(Control::startSend)
{
  Control* control = (Control*)arg;
  control->processCommands();

  return 0;
}

THREAD_START(Control::startRead)
{
  Control* control = (Control*)arg;

  control->readThread_->broadcastReady();
  control->processMessages();
  return 0;
}

void Control::setRawMode(int fd)
{
  struct termios t;

  if (tcgetattr(fd, &t) < 0) 
    ThrowSysError("tcgetattr");
  
  t.c_lflag &= ~ICANON;
  t.c_lflag &= ~ECHO;
  
  if (tcsetattr(fd, TCSANOW, &t) < 0)
    ThrowSysError("tcsetattr");

  setbuf(stdin, NULL);
}
