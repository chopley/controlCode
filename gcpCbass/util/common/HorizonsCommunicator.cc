#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Connection.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/HorizonsCommunicator.h"
#include "gcp/util/common/Pathname.h"
#include "gcp/util/common/SpecificName.h"
#include "gcp/util/common/SshTunnel.h"
#include "gcp/util/common/TimeVal.h"

#include <string.h>
#include <sys/stat.h>

using namespace std;
using namespace gcp::util;

const std::string HorizonsCommunicator::horizonsHost_ = "horizons.jpl.nasa.gov";
const unsigned    HorizonsCommunicator::horizonsPort_ = 6775;

/**.......................................................................
 * Constructor.
 */
HorizonsCommunicator::HorizonsCommunicator(FdSet* fdSetPtr,
					   unsigned intervalInSeconds, 
					   bool useSshTunnel,
					   std::string gateway) :
  SpawnableTask<HorizonsCommunicatorMsg>(false)
{
  setupTimeOuts(intervalInSeconds);
  initialize(useSshTunnel, gateway);
  fdSetPtr_ = fdSetPtr;
}

/**.......................................................................
 * Constructor.
 */
HorizonsCommunicator::HorizonsCommunicator(unsigned intervalInSeconds, 
					   bool useSshTunnel,
					   std::string gateway) :
  SpawnableTask<HorizonsCommunicatorMsg>(true)
{
  setupTimeOuts(intervalInSeconds);
  initialize(useSshTunnel, gateway);
  fdSetPtr_ = &fdSet_;
}

void HorizonsCommunicator::initialize(bool useSshTunnel, std::string gateway)
{
  // Initialize the container to connect to the strip via the standard
  // telnet port

  client_        = 0;
  tunnel_        = 0;
  updateMapIter_ = updateMap_.end();
  useSshTunnel_  = useSshTunnel;
  gateway_       = gateway;

  if(useSshTunnel_) {
    client_ = new TcpClient("localhost",   horizonsPort_);
  } else { 
    client_ = new TcpClient(horizonsHost_, horizonsPort_);
  }

  // This is necessary or else the returned string may contain
  // characters like '\r' which will cause subsequent string
  // comparisons by the likes of strstr(), etc., to fail.

  client_->stripUnprintable(true);
  client_->dontStrip("\n\t");

  // And we will append newline to everything we send to the telnet
  // interface

  client_->append("\n");

  // Now initialize the map of sources we recognize

  horizonsSrcNames_["sun"]     = "sun";
  horizonsSrcNames_["mercury"] = "199";
  horizonsSrcNames_["venus"]   = "299";
  horizonsSrcNames_["moon"]    = "301";
  horizonsSrcNames_["mars"]    = "499";
  horizonsSrcNames_["jupiter"] = "599";
  horizonsSrcNames_["saturn"]  = "699";
  horizonsSrcNames_["uranus"]  = "799";
  horizonsSrcNames_["neptune"] = "899";
  horizonsSrcNames_["pluto"]   = "999";

  // Now initialize the map of intervals

  intervals_["sun"]     = "1d";
  intervals_["mercury"] = "12h";
  intervals_["venus"]   = "1d";
  intervals_["moon"]    = "6h";
  intervals_["mars"]    = "1d";
  intervals_["jupiter"] = "1d";
  intervals_["saturn"]  = "1d";
  intervals_["uranus"]  = "1d";
  intervals_["neptune"] = "1d";
  intervals_["pluto"]   = "1d";

  expiryThresholdInDays_ =  5;
  updateThresholdInDays_ = 60;

  stepState(STATE_INIT);
}

/**.......................................................................
 * Initialize the various timeouts used by this object
 */
void HorizonsCommunicator::setupTimeOuts(unsigned timeOutIntervalInSeconds)
{
  retryTimeOut_.setIntervalInSeconds(timeOutIntervalInSeconds);

  initialTimeOut_.setIntervalInSeconds(5);

  if(timeOutIntervalInSeconds > 0) {
    retryTimeOut_.activate(true);
    initialTimeOut_.activate(true);
  } else {
    retryTimeOut_.activate(false);
    initialTimeOut_.activate(false);
  }
  
  commTimeOut_.setIntervalInSeconds(30);
  commTimeOut_.activate(true);
}

/**.......................................................................
 * Destructor.
 */
HorizonsCommunicator::~HorizonsCommunicator() 
{
  terminateCommSequence(false);

  if(client_) {
    delete client_;
    client_ = 0;
  }

  if(tunnel_) {
    delete tunnel_;
    tunnel_ = 0;
  }
}

/**.......................................................................
 * Register a source to be watched for ephemeris updates
 */
void HorizonsCommunicator::registerEphemeris(std::string sourceName, std::string fileName)
{
  mapGuard_.lock();
  sourceMap_[sourceName] = Pathname::expand("", fileName);
  COUT("Registered ephemeris: " << sourceName << " " << sourceMap_[sourceName]);
  mapGuard_.unlock();
}

/**.......................................................................
 * Deregister a source to be watched for ephemeris updates
 */
void HorizonsCommunicator::deregisterEphemeris(std::string sourceName)
{
  mapGuard_.lock();
  sourceMap_.erase(sourceName);
  mapGuard_.unlock();
}

/**.......................................................................
 * Clear all ephemerides to be watched
 */
void HorizonsCommunicator::clearEphemeris()
{
  mapGuard_.lock();
  sourceMap_.clear();
  mapGuard_.unlock();
}

/**.......................................................................
 * Return true if any registered ephemeris file needs updating
 */
bool HorizonsCommunicator::ephemerisNeedsUpdating()
{
  // Clear the update map to begin with

  updateMap_.clear();

  // Now perform a locking copy of the source map

  mapGuard_.lock();
  std::map<std::string, std::string> tmpMap = sourceMap_;
  mapGuard_.unlock();

  // Now iterate through the copied source map, seeing if any file needs updating
  
  for(std::map<std::string, std::string>::iterator iter=tmpMap.begin(); iter != tmpMap.end(); iter++) {
    std::string fileName = iter->second;
    if(!ephemExists(fileName) || ephemIsAboutToRunOut(fileName) || ephemFileIsOutOfDate(fileName)) {
      updateMap_[iter->first] = iter->second;
      COUT("Ephemeris for: " << iter->first << " needs updating");
    }
  }

  // If the update map isn't empty on exit, then one or more of the
  // ephemeris files needs updating

  setEphemerisDates();
  updateMapIter_ = updateMap_.begin();
  return !updateMap_.empty();
}

void HorizonsCommunicator::setEphemerisDates()
{
  TimeVal time;
  time.setToCurrentTime();

  // Get the ephemeris from one day ago to a year from the start date

  double mjdStart = (double)((unsigned) time.getMjd() - 1);
  double mjdStop  = mjdStart + 365;

  startUtc_ = Date::mjdToHorizonsCal(mjdStart);
  stopUtc_  = Date::mjdToHorizonsCal(mjdStop);
}

/**.......................................................................
 * Return true if the ephemeris file exists
 */
bool HorizonsCommunicator::ephemExists(std::string ephemFileName)
{
  std::ifstream fin(ephemFileName.c_str(), ios::in);

  if(fin) {
    fin.close();
    return true;
  } else {
    return false;
  }
}

/**.......................................................................
 * Return true if the ephemeris is with expiryThresholdInDays_ of
 * running out
 */
bool HorizonsCommunicator::ephemIsAboutToRunOut(std::string ephemFileName) 
{
  TimeVal curr;
  curr.setToCurrentTime();

  double mjdLast = getLastEphemMjd(ephemFileName);

  return (mjdLast - curr.getMjd()) < expiryThresholdInDays_;
}

/**.......................................................................
 * Return the MJD of the last entry in the ephemeris file
 */
double HorizonsCommunicator::getLastEphemMjd(std::string ephemFileName)
{
  std::ifstream fin(ephemFileName.c_str(), ios::in);
  std::string line, lastLine;

  if(fin) {
    while(getline(fin, line)) {
      lastLine = line;
    }
    fin.close();
  } else {
    ThrowError("Unable to open file: " << ephemFileName);
  }

  String str(lastLine);
  return str.findNextStringSeparatedByChars(" ", true).toDouble();
}

/**.......................................................................
 * Return true if the ephemeris hasn't been updated in more than
 * updateThresholdInDays_
 */
bool HorizonsCommunicator::ephemFileIsOutOfDate(std::string ephemFileName)
{
  struct stat statbuf;    /* The file-statistics return buffer */

  if(lstat(ephemFileName.c_str(), &statbuf) < 0) {
    ThrowSysError("lstat()");
  }

#if MAC_OSX == 0
  TimeVal modTime(statbuf.st_mtime);
#else
  TimeVal modTime(statbuf.st_mtimespec);
#endif

  TimeVal currTime;
  currTime.setToCurrentTime();

  // If file is older than updateThresholdInDays_, call it out of date

  return (currTime.getMjd() - modTime.getMjd()) > updateThresholdInDays_;
}

/**.......................................................................
 * Execute a request to add a handler
 */
void HorizonsCommunicator::executeAddHandler(HORIZONS_HANDLER(*fn), void* args)
{
  Handler handler;
  handler.fn_   = fn;
  handler.args_ = args;

  handlers_.push_back(handler);
}

/**.......................................................................
 * Execute a request to remove a handler
 */
void HorizonsCommunicator::executeRemoveHandler(HORIZONS_HANDLER(*fn))
{
  std::vector<Handler>::iterator iter=handlers_.begin();

  for(; iter != handlers_.end(); iter++) {
    if(iter->fn_ == fn)
      break;
  }
    
  if(iter != handlers_.end())
    handlers_.erase(iter);
}

/**.......................................................................
 * Call any handler that was registered
 */
void HorizonsCommunicator::callHandlers(bool error)
{
  for(unsigned iHandler=0; iHandler < handlers_.size(); iHandler++) {
    Handler& handler = handlers_[iHandler];
    (*handler.fn_)(handler.args_, getSource(), getFilename(), error);
  }
}

/**.......................................................................
 * Initiate getting the ephemeris for the next source
 */
void HorizonsCommunicator::checkNextSource()
{
  // If there are any sources left to update, continue

  if(updateMapIter_ != updateMap_.end()) {

    initiateGetEphemerisCommSequence((std::string&)updateMapIter_->first, 
				     (std::string&)updateMapIter_->second,
				     startUtc_, stopUtc_);
    ++updateMapIter_;

    // Else we just finished 

  } else {
    terminateSshConnection();
    stepState(STATE_WAIT);
  }
}

bool HorizonsCommunicator::initiateSshConnection()
{
  Connection conn;

  // If using ssh tunnel, see if we have a connection to the gateway machine

  if(useSshTunnel_) {

    // If the machine is reachable, try to establish the tunnel

    COUT("Trying " << gateway_);
    if(conn.isReachable(gateway_)) {

      tunnel_ = new SshTunnel(gateway_, horizonsHost_, (unsigned short) horizonsPort_, 
			      (unsigned) 30);

      if(tunnel_->succeeded()) {
	return true;
      } else {
	ReportError(tunnel_->error());
	terminateSshConnection();
	return false;
      }

    } else {
      ReportError("Host: " << gateway_ << " is not currently reachable");
      return false;
    } 

    // Else see if we have a connection to the horizons server
    // directly

  } else {
    if(!conn.isReachable(horizonsHost_)) {
      ReportError("Host: " << horizonsHost_ << " is not currently reachable");
      return false;
    }
  }

  return true;
}

/**.......................................................................
 * Method to terminate any existing ssh tunnel
 */
void HorizonsCommunicator::terminateSshConnection()
{
  if(useSshTunnel_) {
    if(tunnel_) {
      delete tunnel_;
      tunnel_ = 0;
    }
  }
  return;
}

/**.......................................................................
 * Initiate sending commands to the server.
 */
void HorizonsCommunicator::
initiateGetEphemerisCommSequence(std::string body, 
				 std::string fileName,
				 std::string startUtc, 
				 std::string stopUtc)
{
  os_.str("");

  setFilename(fileName);

  // Compile the command/response stack associated with this command

  compileGetEphemerisStateMachine(body, startUtc, stopUtc);
  
  // Initiate a connection to the client
  
  try {
    if(client_->connectToServer(true) < 0) {
      ThrowError("Unable to connect to horizons server");
    }
  } catch(Exception& err) {
    ReportError("Unable to connect to horizons server: " << err.what());
    stepState(STATE_WAIT);
  }

  // Clear the msg fd from the set. We don't want to process another
  // message until the current one has been processed

  fdSetPtr_->clearFromReadFdSet(msgq_.fd());

  // Register the fd to be watched for readability

  fdSetPtr_->registerReadFd(client_->getFd());

  // And set a timeout for communication with the server

  stepState(STATE_COMM);
}

/**.......................................................................
 * Terminate a command sequence to the server.
 */
void HorizonsCommunicator::terminateCommSequence(bool error)
{
  // Clear the client fd from the set to be watched

  fdSetPtr_->clearFromReadFdSet(client_->getFd());

  // And disconect from the client

  client_->disconnect();

  // Add the msgq fd back into the set to be watched

  fdSetPtr_->registerReadFd(msgq_.fd());

  // Call any handlers that were registered

  callHandlers(error);

  if(error) {
    terminateSshConnection();
    stepState(STATE_WAIT);
  } else if(state_ == STATE_COMM) {

    // And proceed to the next source

    checkNextSource();
  }
}

/**.......................................................................
 * Compile the state machine we will use to command the strip
 */
void HorizonsCommunicator::
compileGetEphemerisStateMachine(std::string& body, 
				std::string& startUtc, 
				std::string& stopUtc)
{
  setSource(body);

  // Clear the stack of sent commands and expected responses

  sentStrings_.clear();
  rcvdStrings_.clear();

  // And re-build the communications stack

  //  rcvdStrings_.push_back(RcvdStr("Press return to continue", sendNextString, this));
  //  sentStrings_.push_back("");

  rcvdStrings_.push_back(RcvdStr("Horizons>", sendNextString, this));
  sentStrings_.push_back("page");

  rcvdStrings_.push_back(RcvdStr("Horizons>", sendNextString, this));
  sentStrings_.push_back(getHorizonsSource());

  rcvdStrings_.push_back(RcvdStr("<cr>:", sendNextString, this));
  sentStrings_.push_back("E");

  rcvdStrings_.push_back(RcvdStr("[o,e,v,?]", sendNextString, this));
  sentStrings_.push_back("o");

  rcvdStrings_.push_back(RcvdStr("Coordinate center", sendNextString, this));
  sentStrings_.push_back("geo");

  rcvdStrings_.push_back(RcvdStr("Starting", sendNextString, this));
  sentStrings_.push_back(startUtc+"TT");

  rcvdStrings_.push_back(RcvdStr("Ending", sendNextString, this));
  sentStrings_.push_back(stopUtc);

  rcvdStrings_.push_back(RcvdStr("Output interval", sendNextString, this));
  sentStrings_.push_back(getInterval());

  rcvdStrings_.push_back(RcvdStr("Accept default output", sendNextString, this));
  sentStrings_.push_back("n");

  rcvdStrings_.push_back(RcvdStr("Select table quantities", sendNextString, this));
  sentStrings_.push_back("2,20");

  rcvdStrings_.push_back(RcvdStr("Output reference frame", sendNextString, this));
  sentStrings_.push_back("J2000");

  rcvdStrings_.push_back(RcvdStr("Output TT time format", sendNextString, this));
  sentStrings_.push_back("BOTH");

  rcvdStrings_.push_back(RcvdStr("Output time digits", sendNextString, this));
  sentStrings_.push_back("FRACSEC");

  rcvdStrings_.push_back(RcvdStr("Output R.A. format", sendNextString, this));
  sentStrings_.push_back("HMS");

  rcvdStrings_.push_back(RcvdStr("Output high precision", sendNextString, this));
  sentStrings_.push_back("YES");

  rcvdStrings_.push_back(RcvdStr("Output APPARENT", sendNextString, this));
  sentStrings_.push_back("Airless");

  rcvdStrings_.push_back(RcvdStr("Set units for RANGE output", sendNextString, this));
  sentStrings_.push_back("AU");

  rcvdStrings_.push_back(RcvdStr("Suppress RANGE_RATE output", sendNextString, this));
  sentStrings_.push_back("NO");

  rcvdStrings_.push_back(RcvdStr("Minimum elevation", sendNextString, this));
  sentStrings_.push_back("");

  rcvdStrings_.push_back(RcvdStr("Maximum air-mass", sendNextString, this));
  sentStrings_.push_back("");

  rcvdStrings_.push_back(RcvdStr("Print rise-transit-set only", sendNextString, this));
  sentStrings_.push_back("N");

  rcvdStrings_.push_back(RcvdStr("Skip printout during daylight", sendNextString, this));
  sentStrings_.push_back("N");

  rcvdStrings_.push_back(RcvdStr("Solar elongation cut-off", sendNextString, this));
  sentStrings_.push_back("");

  rcvdStrings_.push_back(RcvdStr("Spreadsheet CSV format", sendNextString, this));
  sentStrings_.push_back("N");

  rcvdStrings_.push_back(RcvdStr("*****", ">>> Select", readEphemeris, this, endEphemeris, this));
  sentStrings_.push_back("N");

  rcvdStrings_.push_back(RcvdStr("Horizons>", quitFromServer, this));

  // And set the iterators pointing to the head of the stack
  
  sentStringIter_ = sentStrings_.begin();
  rcvdStringIter_ = rcvdStrings_.begin();

  // And set the current time

  timer_.setToCurrentTime();
}

COMM_PARSER_FN(HorizonsCommunicator::readEphemeris)
{
  HorizonsCommunicator* comm = (HorizonsCommunicator*) arg;
  comm->readEphemeris(str);
}

void HorizonsCommunicator::readEphemeris(gcp::util::String& str)
{
  rcvdStringIter_->matchAny_ = true;
}

COMM_END_PARSER_FN(HorizonsCommunicator::endEphemeris)
{
  HorizonsCommunicator* comm = (HorizonsCommunicator*) arg;
  comm->endEphemeris();
}

void HorizonsCommunicator::endEphemeris()
{
  writeEphem();
  sendNextString();
}

/**.......................................................................
 * Static parser method to respond to disconnect from the FTP server
 */
COMM_PARSER_FN(HorizonsCommunicator::quitFromServer)
{
  HorizonsCommunicator* comm = (HorizonsCommunicator*) arg;
  comm->quitFromServer();
}

/**.......................................................................
 * Method to disconnect from the FTP server
 */
void HorizonsCommunicator::quitFromServer()
{
  std::string str("exit");
  fdSetPtr_->clearFromReadFdSet(client_->getFd());
  client_->writeString(str);
}

/**.......................................................................
 * Public method to add a handler to be called when the ephemeris file
 * is updated
 */
void HorizonsCommunicator::addHandler(HORIZONS_HANDLER(*fn), void* args)
{
  HorizonsCommunicatorMsg msg;
  msg.genericMsgType_ = GenericTaskMsg::TASK_SPECIFIC;
  msg.type = HorizonsCommunicatorMsg::ADD_HANDLER;
  msg.body.addHandler.fn = fn;
  msg.body.addHandler.args = args;
  sendTaskMsg(&msg);
}

void HorizonsCommunicator::removeHandler(HORIZONS_HANDLER(*fn))
{
  HorizonsCommunicatorMsg msg;
  msg.genericMsgType_ = GenericTaskMsg::TASK_SPECIFIC;
  msg.type = HorizonsCommunicatorMsg::REM_HANDLER;
  msg.body.remHandler.fn = fn;
  sendTaskMsg(&msg);
}

/**.......................................................................
 * Send a message to this object to fetch an ephemeris
 */
void HorizonsCommunicator::getEphem(std::string source,
				    std::string fileName,
                                    Date start, Date stop)
{
  HorizonsCommunicatorMsg msg;
  msg.genericMsgType_ = GenericTaskMsg::TASK_SPECIFIC;
  msg.type = HorizonsCommunicatorMsg::GET_EPHEM;

  if(source.size() > SRC_LEN)
    ThrowError("source name is too long.\n");
  strncpy(msg.body.getEphem.sourceId, source.c_str(), SRC_LEN);

  if(fileName.size() > HORIZONS_FILENAME_MAX)
    ThrowError("fileName is too long.\n");
  strncpy(msg.body.getEphem.fileName, fileName.c_str(), HORIZONS_FILENAME_MAX);

  msg.body.getEphem.mjdStart = start.getMjd();
  msg.body.getEphem.mjdStop  = stop.getMjd();

  sendTaskMsg(&msg);
}

/**.......................................................................
 * Process a message received on our message queue
 */
void HorizonsCommunicator::processMsg(HorizonsCommunicatorMsg* msg)
{
  switch (msg->type) {

  case HorizonsCommunicatorMsg::GET_EPHEM:
    {
      initiateGetEphemerisCommSequence(msg->body.getEphem.sourceId,
				       msg->body.getEphem.fileName,			
				       Date::mjdToHorizonsCal(msg->body.getEphem.mjdStart),
				       Date::mjdToHorizonsCal(msg->body.getEphem.mjdStop));
    }
    break;

  case HorizonsCommunicatorMsg::ADD_HANDLER:
    executeAddHandler(msg->body.addHandler.fn, msg->body.addHandler.args);
    break;

  case HorizonsCommunicatorMsg::REM_HANDLER:
    executeRemoveHandler(msg->body.remHandler.fn);
    break;

  default:
    ThrowError("Unrecognized message type: " << msg->type);
    break;
  }
}

/**.......................................................................                                                       
 * Main Task event loop: when this is called, the task blocks forever                                                            
 * in select(), or until a stop message is received.                                                                             
 */
void HorizonsCommunicator::serviceMsgQ()
{
  try {

    bool stop=false;
    int nready; // number of file descriptors ready for reading                                                                    

    if(msgq_.fd() < 0) {
      ThrowError("Received NULL file descriptor");
    }

    while(!stop) {

      nready = select(fdSet_.size(), fdSet_.readFdSet(),
		      NULL, NULL, timeOutPtr_);

      switch (nready) {

      case 0:

	registerTimeOut();

	break;

      case -1:

	stop = true;
	ThrowSysError("select()");
	break;

      default:

	// If a message is waiting to be read, process it now                                                                      

	if(fdSet_.isSetInRead(msgq_.fd())) {
	  processTaskMsg(&stop);
	}

	// If this communicator's read fd was set, read the next line                                                              
	// from the FTP server                                                                                                     

	if(fdSet_.isSetInRead(getFd())) {
	  processClientMessage();
	  commTimeOut_.reset();
	}

	break;
      }
    }
  } catch(Exception& err) {
    ReportError(err.what());
  }
}

/**.......................................................................
 * Write the ephemeris file
 */
void HorizonsCommunicator::writeEphem()
{
  ostringstream os;
  os << fileName_;
  std::ofstream fout(os.str().c_str(), ios::out);
  
  printHeader(fout);
  printEphem(fout);
  
  fout.close();
}

/**.......................................................................
 * Print the ephemeris file header
 */
void HorizonsCommunicator::printHeader(std::ofstream& fout)
{
  fout << "#                     " << SpecificName::experimentNameCaps() << " Ephemeris for " << getSource() << std::endl;
  fout << "#" << std::endl;
  fout << "# Note that TT ~= TAI + 32.184." << std::endl;
  fout << "# See slaDtt.c for how to convert TT to UTC." << std::endl;
  fout << "#" << std::endl;
  fout << "# Also note that MJD = JD - 2400000.5" << std::endl;
  fout << "#" << std::endl;
  fout << "# MJD (TT)   Right Ascen    Declination     Distance (au)       Calendar (TT)          " << std::endl;
  fout << "#---------  -------------  -------------   ----------------    ------------------------" << std::endl;
}

/**.......................................................................
 * Print the ephemeris file data
 */
void HorizonsCommunicator::printEphem(std::ofstream& fout)
{
  // Encapsulate the ephemeris in a String object, for parsing

  String str(os_.str());
  String body = str.findNextInstanceOf("$$SOE\n", true, "\n$$EOE", true);

  String calDate, calTime, jdDate;
  String rahr,    ramin,   rasec;
  String decdeg,  decmin,  decsec;
  String delta,   deldot;
  String strline;

  double mjd;

  do {

    // Extract the next line

    strline =    body.findNextInstanceOf("",      false, "\n",   false, true);

    // Get rid of the initial space

    strline.findNextStringSeparatedByChars(" ", true);

    // Now find individual fields we are interested in

    calDate = strline.findNextStringSeparatedByChars(" ", true);
    calTime = strline.findNextStringSeparatedByChars(" ", true);
    jdDate  = strline.findNextStringSeparatedByChars(" ", true);

    rahr    = strline.findNextStringSeparatedByChars(" ", true);
    ramin   = strline.findNextStringSeparatedByChars(" ", true);
    rasec   = strline.findNextStringSeparatedByChars(" ", true);

    decdeg  = strline.findNextStringSeparatedByChars(" ", true);
    decmin  = strline.findNextStringSeparatedByChars(" ", true);
    decsec  = strline.findNextStringSeparatedByChars(" ", true);

    delta   = strline.findNextStringSeparatedByChars(" ", true);
    deldot  = strline.findNextStringSeparatedByChars(" ", true);

    mjd = jdDate.toDouble() - 2400000.5;

    fout << setw(10)  << setprecision(4) << std::fixed << mjd << "  " 
	 << rahr   << ":"    << ramin   << ":" << rasec   << "  "
	 << decdeg << ":"    << decmin  << ":" << decsec  << "   "
	 << delta  << " #  " << calDate << " " << calTime << std::endl; 

  } while(!body.atEnd());
}

/**.......................................................................
 * Set the desired source
 */
void HorizonsCommunicator::setSource(std::string src)
{
  String name = src;
  String nameLower = name.toLower();

  srcName_ = nameLower.str();

  map<string,string>::iterator iter = horizonsSrcNames_.find(nameLower.str());
  if(iter != horizonsSrcNames_.end() ) {
    horizonsSrcName_ = iter->second;
    interval_        = intervals_[nameLower.str()];
  } else {
    horizonsSrcName_ = srcName_;
    interval_        = "1d";
  }
}

/**.......................................................................
 * Return a version of the source name uniquely recognizable by the
 * horizons system
 */
std::string HorizonsCommunicator::getHorizonsSource()
{
  return horizonsSrcName_;
}

/**.......................................................................
 * Return a human-readable version of the source name
 */
std::string HorizonsCommunicator::getSource()
{
  return srcName_;
}

/**.......................................................................
 * Set the output filename for the current ephemeris
 */
void HorizonsCommunicator::setFilename(std::string fileName)
{
  fileName_ = fileName;
}

/**.......................................................................
 * Get the output filename for the current ephemeris
 */
std::string HorizonsCommunicator::getFilename()
{
  return fileName_;
}

/**.......................................................................
 * Return an interval
 */
std::string HorizonsCommunicator::getInterval()
{
  return interval_;
}

/**.......................................................................
 * Advance the state machine to the requested state
 */
void HorizonsCommunicator::stepState(CommState state)
{
  switch (state) {
  case STATE_INIT:
    CTOUT("Advancing to state: INIT");
    initialTimeOut_.reset();
    timeOutPtr_ = initialTimeOut_.tVal();
    break;
  case STATE_WAIT:
    CTOUT("Advancing to state: WAIT");
    retryTimeOut_.reset();
    timeOutPtr_ = retryTimeOut_.tVal();
    break;
  case STATE_COMM:
    //CTOUT("Advancing to state: COMM");
    commTimeOut_.reset();
    timeOutPtr_ = commTimeOut_.tVal();
    break;
  case STATE_CHECK:
    CTOUT("Advancing to state: CHECK");
    commTimeOut_.reset();
    timeOutPtr_ = commTimeOut_.tVal();
    break;
  default:
    break;
  }

  state_ = state;
}

/**.......................................................................
 * React to a timeout in select
 */
void HorizonsCommunicator::registerTimeOut() 
{
  switch (state_) {

    // If we are waiting to check ephemerides, initiate the check now

  case STATE_INIT:
  case STATE_WAIT:

    // If any source needs updating, attempt to connect to the
    // horizons server

    if(ephemerisNeedsUpdating()) {

      // If we successfully establish a connection, proceed to
      // communicate with the server

      if(initiateSshConnection()) {
	checkNextSource();
	
	// But if we failed to establish a connection, go back to waiting

      } else {
	stepState(STATE_WAIT);
      }

      // Else no source needs updating.  Go back to waiting

    } else {
      COUT("No ephemeris needs updating");
      stepState(STATE_WAIT);
    }
    break;

    // Else if we timed out during comunication with the server, this is an error

  case STATE_COMM:
  case STATE_CHECK:
    registerCommTimeOut();
    break;
  default:
    break;
  }
}

/**.......................................................................,
 * React to a failure to reply
 */
void HorizonsCommunicator::registerCommTimeOut()
{
  // Register our failure to communicate

  LogMessage(true, "Timed out waiting for a response from the horizons server");
  terminateCommSequence(true);
}

/**.......................................................................
 * Test method to read a Horizons format file
 */
void HorizonsCommunicator::loadFile(std::string name)
{
  std::ifstream fin(name.c_str(), ios::in);

  os_.str("");

  std::string line;

  if(fin) {
    while(getline(fin, line)) {
      os_ << line << std::endl;
    }
    fin.close();
  } else {
    ThrowError("Unable to open file: " << name);
    return;
  }

  writeEphem();
}

void HorizonsCommunicator::testFn()
{
  initiateGetEphemerisCommSequence("jupiter", "jupiter.ephem", 
				   "01-jan-2010", "01-jan-2011");

  for(;rcvdStringIter_ != rcvdStrings_.end(); rcvdStringIter_++) {
    rcvdStringIter_->matchAny_ = true;
  }

  initiateGetEphemerisCommSequence("jupiter", "jupiter.ephem", 
				   "01-jan-2010", "01-jan-2011");
}
