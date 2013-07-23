#include "gcp/util/common/CoProc.h"
#include "gcp/util/common/Port.h"
#include "gcp/util/common/Vector.h"

#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/ReceiverControl.h"

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

using namespace gcp::mediator;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
ReceiverControl::ReceiverControl(Control* parent) 
{
  outputDir_ = ".";
  parent_ = parent;

  logFile_.setDirectory(outputDir_);
  logFile_.setDatePrefix();
}

/**.......................................................................
 * Process control messages.
 */
void ReceiverControl::processMsg(ReceiverControlMsg* msg)
{
  switch(msg->type) {
    
    // A network message to be forwarded to the frame receiver
    
  case ReceiverControlMsg::COMMAND:
    installNewScript(msg->body.command.script, msg->body.command.seq);
    break;
  case ReceiverControlMsg::DIRECTORY:
    logFile_.setDirectory(msg->body.directory.dir);
    break;
  default:
    ThrowError("Unrecognized message type");
    break;
  }
}

/**.......................................................................
 * Destructor.
 */
ReceiverControl::~ReceiverControl() {}

/**.......................................................................
 * Service our message queue.
 */
void ReceiverControl::serviceMsgQ()
{
  bool stop   = false;
  int  nready = 0;
  int  msgqFd = msgq_.fd();
  
  // Initially our select loop will check the msgq file descriptor for
  // readability, and the server socket for connection requests.  Once
  // connections are establihed, the select loop will block until
  // either a message is received on the message queue, on the control
  // port, or on a port to an established antenna.
  
  fdSet_.registerReadFd(msgqFd);
  
  while(!stop) {
    nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
		  NULL, NULL);
    
    // A message on our message queue?
    
    if(fdSet_.isSetInRead(msgqFd))
      processTaskMsg(&stop);

    // Receipt of data on one of the script channels?

    for(std::list<Script*>::iterator iScript=scripts_.begin(); 
	iScript != scripts_.end(); iScript++)
    {
      Script* script = *iScript;

      if(fdSet_.isSetInRead(script->stdOutFd()))
	processStdOut(script);
      
      if(fdSet_.isSetInRead(script->stdErrFd()))
	processStdErr(script);
    }

    // Remove any scripts that finished on this last iteration

    removeFinishedScriptsFromList();
  }
}

/**.......................................................................
 * Register receipt of a command to execute a new script
 */
void ReceiverControl::installNewScript(char* command, unsigned seq)
{
  try {

    Script* script = new Script(command, seq, logFile_.newFileName());

    // And install the stdout and stderr fds in the list to watch for
    // input
    
    fdSet_.registerReadFd(script->stdOutFd());
    fdSet_.registerReadFd(script->stdErrFd());

    // Finally, add this script to our list

    scripts_.push_back(script);

  } catch(Exception& err) {
    registerError(err.what(), seq);
  }
}

/**.......................................................................
 * Respond to data on the stdout pipe for this script
 */
void ReceiverControl::processStdOut(Script* script)
{
  if(script->copyFromStdoutToFile()==0) 
    registerStdOutClosed(script);
}

/**.......................................................................
 * Respond to data on the stdout pipe for this script
 */
void ReceiverControl::registerStdOutClosed(Script* script)
{
  // Remove the descriptor from the set to be watched

  fdSet_.clearFromReadFdSet(script->stdOutFd());
  script->stdOutWasClosed_ = true;

  // Close the file descriptor

  script->close();

  // And check if the script completed

  checkCompletionStatus(script);
}

/**.......................................................................
 * Respond to data on the stdout pipe for this script
 */
void ReceiverControl::registerStdErrClosed(Script* script)
{
  // Remove the descriptor from the set to be watched

  fdSet_.clearFromReadFdSet(script->stdErrFd());
  script->stdErrWasClosed_ = true;

  // And check if the script completed

  checkCompletionStatus(script);
}

/**.......................................................................
 * Register an error installing a new script
 */
void ReceiverControl::registerError(std::string message, unsigned seq)
{
  // Report the error

  ReportSimpleError("(exec) " << message);

  // And let any schedules waiting on completion status reports know
  // that this script is finished
  
  sendScriptCompletedMsg(seq);
}

/**.......................................................................
 * Send a script completion message to the control program
 */
void ReceiverControl::sendScriptCompletedMsg(unsigned seq)
{
  if(parent_)
    parent_->sendScriptDoneMsg(seq);
}

/**.......................................................................
 * Respond to data on the stdout pipe for this script
 */
void ReceiverControl::checkCompletionStatus(Script* script)
{

  if(script->stdErrWasClosed_ && script->stdOutWasClosed_) {

    switch (script->status_) {
    case Script::NONE:
      ReportSimpleError("(exec) Script command: \"" << script->script_ 
			<< "\" exited without a status");
      break;
    case Script::FAILED:
      ReportSimpleError("(exec) Script command: \"" << script->script_ 
			<< "\" reported failure");
      break;
    default:
      ReportMessage("(exec) Script command: \"" << script->script_ 
		    << "\" reported success");
      break;
    }

    // And let any schedules waiting on completion status reports know
    // that this script is finished

    sendScriptCompletedMsg(script->seq_);

    // Finally, queue this script for deletion.  But don't delete it
    // yet, in case we are in the middle of iterating on the list of
    // scripts!

    finishedScripts_.push_back(script);
  }
}

/**.......................................................................
 * Respond to data on the stderr pipe for this script
 */
void ReceiverControl::processStdErr(Script* script) 
{
  if(script->readFromStdErr()==0)
    registerStdErrClosed(script);
}

void ReceiverControl::removeFinishedScriptsFromList()
{
  for(std::list<Script*>::iterator iScript=finishedScripts_.begin(); 
      iScript != finishedScripts_.end(); iScript++) 
  {
    Script* script = *iScript;
    scripts_.remove(script);
    delete script;
  }

  finishedScripts_.clear();
}

/**.......................................................................
 * Forward a script message to the receiver control task.
 */
void ReceiverControl::sendScript(std::string script, unsigned seq)
{
  ReceiverControlMsg msg;
  msg.packCommandMsg((char*)script.c_str(), 0);
  sendTaskMsg(&msg);
}

//-----------------------------------------------------------------------
// Methods of Script
//-----------------------------------------------------------------------

ReceiverControl::Script::
Script(std::string script, unsigned seq, std::string fileName)
{
  // Initialize variables

  proc_   = 0;
  stdOut_ = 0;
  stdErr_ = 0;

  seq_    = seq;
  status_ = NONE;
  script_ = script;

  tagCount_ = 0;
  messageState_ = DONE;

  startTag_.str("");
  message_.str("");
  endTag_.str("");

  // First attempt to open the file to which stdout will be written.
  // If this fails, it will throw an error, and we won't bother
  // running the script

  std::ostringstream os;
  os << fileName << ".log";
  open(os.str());

  // Start a new process in which the script will be run

  proc_   = new gcp::util::CoProc(script);
  stdOut_ = new gcp::util::Port(stdOutFd());
  stdErr_ = new gcp::util::Port(stdErrFd());

  stdOutWasClosed_ = false;
  stdErrWasClosed_ = false;
}

ReceiverControl::Script::~Script() 
{
  // Close any file associated with this process

  close();

  // Delete the port objects

  if(stdOut_) {
    delete stdOut_;
    stdOut_ = 0;
  }

  if(stdErr_) {
    delete stdErr_;
    stdErr_ = 0;
  }

  // And delete the process itself

  if(proc_ != 0) {
    delete proc_;
    proc_ = 0;
  }
}

/**.......................................................................
 * Open a file for this register in the requested directory
 */
void ReceiverControl::Script::close()
{
  if(outFile_) {
    outFile_.close();
    outFile_.clear();
  }
}

/**.......................................................................
 * Open a file for this register in the requested directory
 */
void ReceiverControl::Script::open(std::string fileName)
{
  close();

  // Open the file in exclusive create mode (will return error if the
  // file already exists), and with rwx permissions for user, read
  // permissions for others

  outFile_.open(fileName.c_str(), ios::out);

  if(!outFile_) {
    ThrowSysError("open(), while attempting to open file: " << fileName);
  }

  // If we successfully opened, write the command that generated this
  // file into it

  outFile_ << "=======================================================================" << std::endl
	   << "File contains output generated by the command: " << std::endl
	   << std::endl << std::endl
	   << "\"" << script_ << "\""
	   << std::endl << std::endl
	   << "=======================================================================" << std::endl << std::endl;

  // Report the successful opening of the output file

  ReportMessage("Opened output file: " << fileName);
}

/**.......................................................................
 * Copy data from the stdout pipe of the process to the output file
 */
unsigned ReceiverControl::Script::copyFromStdoutToFile()
{
  Vector<unsigned char> buffer;
  unsigned nBytes = stdOut_->readBytes(buffer);

  if(nBytes > 0) {
    for(unsigned i=0; i < nBytes; i++)
      outFile_ << buffer[i];
  }

  return nBytes;
}

// Methods to return the stdout/stderr fds for this process

int ReceiverControl::Script::stdInFd()
{
  return proc_->stdIn()->writeFd();
}

int ReceiverControl::Script::stdOutFd()
{
  return proc_->stdOut()->readFd();
}

int ReceiverControl::Script::stdErrFd()
{
  return proc_->stdErr()->readFd();
}

/**.......................................................................
 * Read the next available data from stderr
 *
 * Look for message strings of the form 
 * 
 *  %%%BEGIN_ERROR
 *  %%%BEGIN_MESSAGE
 *  %%%BEGIN_STATUS
 */
unsigned ReceiverControl::Script::readFromStdErr()
{
  unsigned nByte = stdErr_->getNbyte();

  for(unsigned i=0; i < nByte; i++) {

    unsigned char c = stdErr_->getNextByte();

    switch (messageState_) {

      // If we are not currently reading a message, check if the next
      // char is a special char.  If it is, count how many consecutive
      // times this character has occurred.  If we have reached the
      // tag count, then this is a start tag

    case DONE:
      if(c=='%') {
	++tagCount_;
      } else {
	tagCount_ = 0;
      }

      if(tagCount_==3) {
	tagCount_ = 0;
	messageState_ = START;
	startTag_.str("");
	message_.str("");
	endTag_.str("");
      } 

      break;

    case START:

      // If we have encountered a tag, the next string of characters
      // should specify what sort of tag this is.  If we hit a space,
      // the tag is finished, and we can process what sort of message
      // to expect

      if(isspace(c)) {
	setMessageType();
	messageState_ = BODY;
      } else {
	startTag_ << c;
      } 

      break;

      // If we are currently reading a message, continuing cat'ing it
      // into the stream.  If we hit a special character, start a tag
      // count to see if this is an end tag

    case BODY:

      if(c=='%') {
	++tagCount_;
      } else {
	tagCount_ = 0;
	message_ << c;
      }

      if(tagCount_ == 3) {
	tagCount_ = 0;
	messageState_ = END;
      }

      break;

      // If this is the end, then start accumulating the end tag.  If
      // we hit a space, we are done and can process the message

    case END:

      if(isspace(c)) {
	processMessage();
	messageState_ = DONE;
      } else {
	endTag_ << c;
      }

      break;

    default:
      break;
    }
  }

  return nByte;
}


void ReceiverControl::Script::setMessageType()
{
  if(startTag_.str()      == "BEGIN_ERROR")
    messageType_ = ERROR;
  else if(startTag_.str() == "BEGIN_MESSAGE")
    messageType_ = MESSAGE;
  else if(startTag_.str() == "BEGIN_STATUS")
    messageType_ = STATUS;

  // Else this is an invalid tag.  Set the state to done and prepare
  // to disregard chars until the next valid tag is encountered

  else
    messageState_ = DONE;
}

/**.......................................................................
 * Process a message received on stderr
 */
void ReceiverControl::Script::processMessage()
{
  std::string str = message_.str();

  // Strip off any terminal newline character.  We'll append one when
  // the message is sent.

  if(str[str.size()-1] == '\n')
    str[str.size()-1] = '\0';

  switch (messageType_) {
  case Script::ERROR:
    ReportSimpleError("(exec) " << str);
    break;
  case Script::MESSAGE:
    ReportMessage("(exec) " << str);
    break;
  case Script::STATUS:
    if(strstr(str.c_str(), "SUCCESS 1") != 0)
      status_ = Script::OK;
    else 
      status_ = Script::FAILED;
    break;
  }
}
