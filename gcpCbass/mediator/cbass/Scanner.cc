#define __FILEPATH__ "mediator/specific/Scanner.cc"

#include <sys/socket.h> // Needed for shutdown()
#include <setjmp.h>     // Needed for jmp_buf, etc.

#include "gcp/util/common/ArrayDataFrameManager.h"
#include "gcp/util/common/BoardDataFrameManager.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/FrameFlags.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"
#include "gcp/util/common/Profiler.h"
#include "gcp/util/common/RegDate.h"
#include "gcp/util/common/TimeOut.h"

#include "gcp/mediator/specific/AntennaConsumer.h"
#include "gcp/mediator/specific/AntennaConsumerNormal.h"
#include "gcp/mediator/specific/Control.h"
#include "gcp/mediator/specific/Master.h"
#include "gcp/mediator/specific/Scanner.h"

#include "gcp/control/code/unix/libunix_src/specific/rtcnetcoms.h"
#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

#define EML_DEBUG

using namespace std;
using namespace gcp::util;
using namespace gcp::mediator;

//=======================================================================
// Scanner class
//=======================================================================

/**.......................................................................
 * Scanner initialization
 * 
 * @throws Exception
 */
Scanner::Scanner(Master* parent) : 
  fb_(ArrayFrameBuffer(NUM_BUFFER_FRAMES))
{
  // Sanity check
  
  if(parent == 0)
    ThrowError("Received NULL parent argument");
  
  // Initialize the scanner encapsulation
  
  arraymap_               = 0;
  arrayStr_               = 0;
  parent_                 = parent;
  dispatchPending_        = false;
  
  features_.transient     = 0;
  features_.persistent    = 0;
  features_.seq           = 0;
  
  // Objects managed by spawned threads
  
  antennaConsumer_        = 0;

  // Initialize the array of threads controlled by this task.
  
  threads_.push_back(new Thread(&startAntennaConsumer, 
				&cleanAntennaConsumer, 
				0, "AntennaConsumer"));

#if DIR_HAVE_MUX
  if(!parent_->sim() && !parent_->simRx()) {

    threads_.push_back(new Thread(&startBolometerConsumer, 
				  &cleanBolometerConsumer, 
				  0, "BolometerConsumer"));

    threads_.push_back(new Thread(&startSquidConsumer, 
				  &cleanSquidConsumer, 
				  0, "SquidConsumer"));

    threads_.push_back(new Thread(&startReceiverConfigConsumer, 
				  &cleanReceiverConfigConsumer, 
				  0, "ReceiverConfigConsumer"));

  }
#endif

  // Get the array map.
  
  arraymap_ = new_ArrayMap();
  
  if(arraymap_ == 0)
    ThrowError("Insufficient memory to allocate new ArrayMap");
  
  // Intialize resources to do with antennas
  
  initAntennaResources();
  
  //-----------------------------------------------------------------------
  // Network communications resources.
  //-----------------------------------------------------------------------
  
  // Create a TCP/IP network input stream. The longest message will be
  // the array of register blocks passed as a 4-byte address plus
  // 2-byte block dimension per register.
  //
  // Pass the size of the send stream as 0, as it will be externally
  // allocated.
  
  arrayStr_ = new NetStr(-1, SCAN_MAX_CMD_SIZE, 0);
  
  // Install handlers
  
  arrayStr_->getReadStr()->installReadHandler(readHandler, this);
  arrayStr_->getSendStr()->installSendHandler(sendHandler, this);
  arrayStr_->getReadStr()->installErrorHandler(networkError, this);
  arrayStr_->getSendStr()->installErrorHandler(networkError, this);
  
  // Attempt to connect to the archiver port
  
  connectScanner(true);
  
  // And run threads managed by this task.
  
  startThreads(this);
}

/**.......................................................................
 * Initialize antenna resources.
 */
void Scanner::initAntennaResources()
{
  ArrRegMapReg arreg;
  
  DBPRINT(true, Debug::DEBUG10, "1");
  
  //------------------------------------------------------------
  // Resources for dealing with antennas.
  //------------------------------------------------------------
  
  // Record the number of antennas we know about
  
  AntNum antnum;
  nAntenna_ = 1;
  
  if(nAntenna_ < 1)
    ThrowError("nAntenna_ < 1");
  
  // Reserve space for relevant vectors.
  
  antStartSlots_.reserve(nAntenna_);
  antRecSlots_.reserve(nAntenna_);
  nArchiveAntenna_.reserve(nAntenna_);
  
  // Initialize the bitmask of received data to 0
  
  antReceivedLast_    = 0;
  antReceivedCurrent_ = 0;
  
  // Initialize these vectors to something impossible
  
  for(unsigned iant=0; iant < nAntenna_; iant++) {
    antStartSlots_.push_back(-1);
    antRecSlots_.push_back(-1);
    nArchiveAntenna_.push_back(0);
  }
  
  DBPRINT(true, Debug::DEBUG10, "2");
  
  // Look up pertinent register slots
  //
  // We assume here that each antenna has a frame.status register as
  // the first register in its register map.  If this changes, as
  // undoubtedly it will, this should be changed accordingly.
  
  for(unsigned iant=0; iant < nAntenna_; iant++) {
    
    DBPRINT(true, Debug::DEBUG10, "3");
    
    AntNum antenna(iant);
    
    // Get the register map for this antenna
    
    ArrRegMap* arregmap = find_ArrRegMap(arraymap_, antenna.getAntennaName());
    
    // For now, just ignore antenna register maps we can't find.
    
    if(arregmap != 0) {
      
      // Record the number of archived registers per antenna
      
      nArchiveAntenna_[iant] = arregmap->regmap->narchive_;
      
      // Get the descriptor for the first register in this register
      // map.
      
      DBPRINT(true, Debug::DEBUG10, "4");
      
      if(find_ArrRegMapReg(arraymap_, antenna.getAntennaName(), 
			   "frame", "status", 
			   REG_PLAIN, 0, 1, &arreg)==1) {
	
	ThrowError("Error finding register: "
		   << antenna.getAntennaName()
		   << ".frame.status"
		   << " in the array map");
      }
      
      // And record the starting point of this register
      
      antStartSlots_[iant] = arregmap->iByte_;
      
      // Get the descriptor for the received register of this register
      // map
      
      if(find_ArrRegMapReg(arraymap_, antenna.getAntennaName(), 
			   "frame", "received", 
			   REG_PLAIN, 0, 1, &arreg)==1) {

	ThrowError("Error finding register: "
		   << antenna.getAntennaName()
		   << ".frame.received"
		   << " in the array map");
      }
      
      // And record the starting point of this register
      
      antRecSlots_[iant] = arreg.reg.iByte_;
    }
  }
}

/**.......................................................................
 * Destructor
 */
Scanner::~Scanner()
{
  disconnect();
  
  if(arrayStr_ != 0) {
    delete arrayStr_;
    arrayStr_ = 0;
  }
  
  if(arraymap_ != 0)
    arraymap_ = del_ArrayMap(arraymap_);
}

/**.......................................................................
 * Private method to set the received status for all antennas in a
 * register map
 */
void Scanner::setAntReceived(bool received, ArrayDataFrameManager* frame)
{
  for(unsigned iant=0; iant < nAntenna_; iant++)
    setAntReceived(iant, received, frame);
}

/**.......................................................................
 * Private method to set the received status for a register map.
 */
void Scanner::setAntReceived(unsigned iant, bool received, 
			     ArrayDataFrameManager* frame)
{
  static Mutex mutex;
  static AntNum antNum;
  
  mutex.lock();
  
  // Set our internal record-keeping bitmask
  
  if(received)
    antReceivedCurrent_ |= 1U << iant;
  else
    antReceivedCurrent_ &= ~(1U << iant);
  
  // And write it in the frame
  
  if(frame != 0) {
    antNum.setId((unsigned int)iant);
    
    unsigned char rcvd = received ? (unsigned char)FrameFlags::RECEIVED : 
      (unsigned char)FrameFlags::NOT_RECEIVED;
    
    frame->writeReg(antNum.getAntennaName(), "frame", "received", &rcvd);
  }
  
  mutex.unlock();
}

/**.......................................................................
 * Poll slow consumers for their data
 */
void Scanner::pollConsumers()
{
#if DIR_HAVE_MUX
  if(receiverConfigConsumer_) {
    //COUT("Polling receiver");
    receiverConfigConsumer_->sendDispatchDataFrameMsg();
    //COUT("Polling receiver done");
  }
#endif
}

/**.......................................................................
 * Pack an antenna frame into the next available slot of our frame
 * buffer
 */
void Scanner::packAntennaFrame(AntennaDataFrameManager* antFrame)
{
  RegDate antDate;

  antFrame->readReg("frame", "utc", antDate.data());

  // Find the frame in our buffer corresponding to the passed frame.

  ArrayDataFrameManager* frame =
    dynamic_cast<ArrayDataFrameManager*>
    (fb_.getFrame(antFrame->getId(DATAFRAME_NSEC), false));

  // If no corresponding frame was found, drop the frame.

  if(frame == 0) {
    return;
  }

  // Unpack this frame into the location of the first slot of the
  // antenna register map

  unsigned iant = antFrame->getAntIntId();

  frame->writeAntennaRegMap(*antFrame, false);

  // And mark it as received in our bitmask

  setAntReceived(iant, true, frame);
}

#if DIR_HAVE_MUX
/**.......................................................................
 * Pack bolometer data into the array database
 */
void Scanner::packBolometerFrame()
{
  static Profiler prof;
  static unsigned record=0;

  prof.start();

  // Pack the bolometer frame into the array frame buffer

  try {

    ++record;
    RegMapDataFrameManager* boloFrame = bolometerConsumer_->grabReadFrame();

    // Find the frame in our buffer corresponding to the passed frame.

    ArrayDataFrameManager* frame =
      dynamic_cast<ArrayDataFrameManager*>
      (fb_.getFrame(boloFrame->getId(DATAFRAME_NSEC), false));
    
    // If a valid frame was found, mark it as received and pack into
    // the data frame

    if(frame != 0) {

      // Mark this frame as received

      boloFrame->setReceived(true);
      boloFrame->setRecord(record);

      // Copy the squid board data into the frame

      if(squidConsumer_)
	boloFrame->writeBoard(squidConsumer_->getFrame(), true);

      // Finally, copy the bolometer frame into the array data frame

      frame->writeGenericRegMap(*boloFrame, false, "receiver");

      // And copy persistent register values into the frame

      bolometerConsumer_->copyPersistentRegs(frame);
    }

  } catch(...) {
  }

  bolometerConsumer_->releaseReadFrame();

  prof.stop();
}
#endif

/**.......................................................................
 * Pack bolometer data into the array database
 */
void Scanner::packDataFrame(RegMapDataFrameManager* dataFrame, 
			    string regMapName)
{
  // Find the frame in our buffer corresponding to the passed frame.

  ArrayDataFrameManager* frame =
    dynamic_cast<ArrayDataFrameManager*>
    (fb_.getFrame(dataFrame->getId(DATAFRAME_NSEC), false));

  if(frame == 0) {
    //    COUT("Frame is NULL for id: " << dataFrame->getId(DATAFRAME_NSEC));
    return;
  } else {
    //    COUT("Valid Frame found for id:" << dataFrame->getId(DATAFRAME_NSEC));
  }

  // Pack the frame into the array frame buffer

  frame->writeGenericRegMap(*dataFrame, false, regMapName);
}

/**.......................................................................
 * Pack a board into the array register map
 */
void Scanner::packBoard(BoardDataFrameManager* dataFrame)
{
  ArrayRegMapDataFrameManager& share = parent_->getArrayShare();
  share.writeBoard(*dataFrame, false);
}

/**.......................................................................
 * Pack a board into the data frame corresponding to the passed
 * timestamp
 */
void Scanner::packBoardSynch(BoardDataFrameManager* dataFrame, unsigned int id)
{
  // Find the frame in our buffer corresponding to the passed frame.

  ArrayDataFrameManager* frame =
    dynamic_cast<ArrayDataFrameManager*>(fb_.getFrame(id, false));

  if(frame == 0) {
    return;
  } else {
    frame->writeBoard(*dataFrame, false);
  }
}

/**.......................................................................
 * Start a new frame, and dispatch any frames waiting to be sent.
 *
 * @throws Exception
 */
ArrayDataFrameManager* 
Scanner::startNewArrayFrame()
{
  static TimeVal timeVal;
  static ArrayDataFrameManager* frame = 0;

  ArrayRegMapDataFrameManager& share = parent_->getArrayShare();
  
  // Record the bit-mask of feature markers received since the last
  // record was successfully dispatched to the archiver, plus any
  // persistent features that haven't been cancelled yet.
  
  unsigned features = features_.transient | features_.persistent;
  
  share.writeReg("frame", "features", &features);
  share.writeReg("frame", "markSeq",  &features_.seq);
  
  features_.transient = 0x0;
  
  // Copy the shared memory segment into the last frame we created

  if(frame != 0) {

    share.lock();
    
    try {
      
      frame->writeRegMap("array", share, false);
	
    } catch(Exception& err) {
      share.unlock();
      ThrowError(err.what());
    } catch(...) {
      share.unlock();
      ThrowError("Caught unknown error");
    }
  }

  // If we always create a slot with the current MJD, because of clock
  // skew between computers, the antenna/receiver might report their
  // frames before we have created a slot in the frame buffer for
  // them.  To avoid this, on the current tick we will always create a
  // slot with the id of the _next_ MJD second.  This will ensure that
  // a frame already exists for the next second well before we try to
  // pack any antenna data into it.

  // And get the current time, truncated to the absolute second
  // boundary, incremented by the dataframe interval

  timeVal.setToCurrentTime();
  timeVal.setNanoSeconds(0); // truncate
  timeVal.incrementNanoSeconds(DATAFRAME_NSEC);

  // Set the static frame pointer pointing to the new frame
 
  frame = dynamic_cast<ArrayDataFrameManager*> 
    (fb_.getFrame(timeVal.getMjdId(DATAFRAME_NSEC),true));
  
  // Flag this array map as received

  share.setReceived(true);

  // Update the record counter in shared memory

  share.incrementRecord();

  // Write the date into shared memory.  This date will be the START
  // date of this new frame

  share.setMjd(timeVal);

  // Fill in default values for registers 

  initializeShare(timeVal);

  // Fill in default values for registers 

  initializeDefaultRegisters(frame, timeVal);

  // If access to any antennas changed state during the last half
  // second, report it to the main thread.

  if(antennasChangedState())
    reportStateChange();

  // Get data from slow consumers

  pollConsumers();

#if 1
  frame->writeReg("antenna0", "thermal", "lsTemperatureSensors", (float)103.0);
#endif

  // If there are multiple frames waiting in the queue, keep sending
  // until the buffer is drained.  But don't send the next frame until
  // at least a full 1-second period after its timestamp.  If we
  // were creating frames for the current second, we would have
  // to wait until the count is at least 2, since the number of frames
  // in the queue is not decremented until the next call to
  // FrameBuffer::dispatchNextFrame().  But since we are always
  // creating frames for the next second, we must now wait until
  // the count is at least 3.  This means that we have the frame we
  // just created, for the next second, a frame for the current
  // second, and a frame for the last second, which should
  // be the next one sent.

  checkDataFrames();
}

/**.......................................................................
 * Fill register time axes with default values
 */
void Scanner::
initializeDefaultRegisters(ArrayDataFrameManager* frame, TimeVal& mjd)
{
  static std::vector<RegDate::Data> fastDataUtc(DATA_SAMPLES_PER_FRAME);
  static std::vector<RegDate::Data> fastPosUtc(SERVO_POSITION_SAMPLES_PER_FRAME);
  static std::vector<RegDate::Data> fastPosTickUtc(POSITION_SAMPLES_PER_FRAME);
  static std::vector<RegDate::Data> fastGpibUtc(THERMAL_SAMPLES_PER_FRAME);
  static std::vector<RegDate::Data> fastServoUtc(SERVO_POSITION_SAMPLES_PER_FRAME);
  static std::vector<RegDate::Data> fastReceiverUtc(RECEIVER_SAMPLES_PER_FRAME);

  static bool first=true;
  static unsigned char recv = gcp::util::FrameFlags::NOT_RECEIVED;

  if(first) {

    first = false;

    // Servo positions correspond to the previous 1-second tick, so
    // subtract 1 second from the time corresponding to the start of
    // this frame

    TimeVal tickMjd = mjd;
    TimeVal prevSecMjd = mjd;
    //    prevSecMjd.incrementSeconds(-1.0);
    //    tickMjd.incrementSeconds(-1.0);
    
    for(unsigned i=0; i < fastDataUtc.size(); i++) {
      // data samples per frame is 100;
      fastDataUtc[i]  = mjd;
      fastDataUtc[i] += i * MS_PER_DATA_SAMPLE;
    }
    for(unsigned i=0; i < fastReceiverUtc.size(); i++) {
      fastReceiverUtc[i]  = prevSecMjd;
      fastReceiverUtc[i] += i * MS_PER_RECEIVER_SAMPLE;
    }

    for(unsigned i=0; i < fastPosUtc.size(); i++) {
      // servo positions from the previous second
      fastPosUtc[i]  = prevSecMjd;
      fastPosUtc[i] += i * MS_PER_SERVO_POSITION_SAMPLE;
    }

    for(unsigned i=0; i < fastServoUtc.size(); i++) {
      // servo positions from the previous second
      fastServoUtc[i]  = prevSecMjd;
      fastServoUtc[i] += i * MS_PER_SERVO_POSITION_SAMPLE;
    }

    for(unsigned i=0; i < fastGpibUtc.size(); i++) {
      // gpib positions are in real-time
      fastGpibUtc[i]  = prevSecMjd;
      fastGpibUtc[i] += i * MS_PER_THERMAL_SAMPLE;
    }

    for(unsigned i=0; i < fastPosTickUtc.size(); i++) {
      // not really from the previous second. 
      fastPosTickUtc[i]  = mjd;
      fastPosTickUtc[i] += i * MS_PER_POSITION_SAMPLE;
    }

  } else {

    for(unsigned i=0; i < fastDataUtc.size(); i++) {
      fastDataUtc[i] += 1000;
    }

    for(unsigned i=0; i < fastReceiverUtc.size(); i++) {
      fastReceiverUtc[i] += 1000;
    }

    for(unsigned i=0; i < fastPosUtc.size(); i++) {
      fastPosUtc[i]     += 1000;
    }

    for(unsigned i=0; i < fastServoUtc.size(); i++) {
      fastServoUtc[i] += 1000;
    }

    for(unsigned i=0; i < fastGpibUtc.size(); i++) {
      fastGpibUtc[i] += 1000;
    }

    for(unsigned i=0; i < fastPosTickUtc.size(); i++) {
      fastPosTickUtc[i] += 1000;
    }

  }

  // Initialize antenna data as unreceived, but set the time axes to
  // something appropriate for this frame

  frame->writeReg("antenna0", "frame",      "received", &recv);
  frame->writeReg("antenna0", "frame",      "utc", &fastDataUtc[0]);
  frame->writeReg("antenna0", "roach1",  "utc", &fastReceiverUtc[0]);
  frame->writeReg("antenna0", "roach2",  "utc", &fastReceiverUtc[0]);
  frame->writeReg("antenna0", "servo",    "utc", &fastServoUtc[0]);
  frame->writeReg("antenna0", "tracker",  "utc", &fastPosTickUtc[0]);
  frame->writeReg("antenna0", "thermal",  "utc", &fastGpibUtc[0]);

  // Copy the persistent bolometer registers into the array frame

#if NOT_CBASS
  if(bolometerConsumer_)
    bolometerConsumer_->copyPersistentRegs(frame);
#endif
}

/**.......................................................................
 * Reinitialize the shared register map
 */
void Scanner::initializeShare(TimeVal& timeVal)
{
  static RegDate date;
  static unsigned char recv = gcp::util::FrameFlags::NOT_RECEIVED;
  date = timeVal;

  ArrayRegMapDataFrameManager& share = parent_->getArrayShare();

  // Initialize slow consumer registers to default values

#if NOT_CBASS
  share.writeReg("deicing",         "received", &recv);
  share.writeReg("fridge",          "received", &recv);
  share.writeReg("fpga",            "received", &recv);
  share.writeReg("optics",          "received", &recv);
  share.writeReg("pointingTel",     "received", &recv);
  share.writeReg("squidController", "received", &recv);
  share.writeReg("holo",            "received", &recv);
#endif
}

/**.......................................................................
 * Check if there are data frames waiting to be sent.
 */
void Scanner::checkDataFrames()
{
  // If there are multiple frames waiting in the queue, keep sending
  // until the buffer is drained.  But don't send the next frame until
  // at least a full half-second period after its timestamp.  If we
  // were creating frames for the current half-second, we would have
  // to wait until the count is at least 2, since the number of frames
  // in the queue is not decremented until the next call to
  // FrameBuffer::dispatchNextFrame().  But since we are always
  // creating frames for the next half-second, we must now wait until
  // the count is at least 3.  This means that we have the frame we
  // just created, for the next half-second, a frame for the current
  // half-second, and a frame for the last half-second, which should
  // be the next one sent.
  
  if(fb_.getNframesInQueue() > MIN_FRAMES_IN_QUEUE) 
    sendDispatchDataFrameMsg();
}

/**.......................................................................
 * Return true if any antennas changed state (ie, sent a data frame,
 * or stopped sending data frames) during this last snapshot.
 */
bool Scanner::antennasChangedState()
{
  return (antReceivedCurrent_ ^ antReceivedLast_);
}

/**.......................................................................
 * Return true if any antennas changed state (ie, sent a data frame,
 * or stopped sending data frames) during this last snapshot.
 */
void Scanner::reportStateChange() 
{
  DBPRINT(true, Debug::DEBUG8, "Reporting a state change");
  
  unsigned changed = antReceivedCurrent_ ^ antReceivedLast_;
  
  unsigned recLast, recCurr;
  
  for(unsigned iant=0; iant < nAntenna_; iant++) {
    if(changed & (1U << iant)) {
      recLast = antReceivedLast_ & (1U << iant);
      recCurr = antReceivedCurrent_ & (1U << iant);
      
      // Flag this antenna if we previously received data from it, but
      // did not on the last integration.
      
      if(recLast && !recCurr)
	sendFlagAntennaMsg(iant, true);
      
      // Else unflag an antenna for which we have newly started to
      // receive data.
      
      else
	sendFlagAntennaMsg(iant, false);
    }
  }
  
  // Store the current state
  
  antReceivedLast_ = antReceivedCurrent_;
}

//-----------------------------------------------------------------------
// Thread management functions
//-----------------------------------------------------------------------

/**.......................................................................
 * AntennaConsumer startup function
 */
THREAD_START(Scanner::startAntennaConsumer)
{
  Scanner* parent = (Scanner*) arg;
  Thread* thread = 0;
  
  thread = parent->getThread("AntennaConsumer");
  
  // Instantiate the subsystem object
  
  Master* master = parent->parent_;
  
  parent->antennaConsumer_ = 
    new gcp::mediator::AntennaConsumerNormal(parent);
  
  // Set our internal thread pointer pointing to the AntennaConsumer
  // thread
  
  parent->antennaConsumer_->thread_ = thread;
  
  // Let other threads know we are ready
  
  thread->broadcastReady();
  
  // Finally, block, running our select loop
  
  DBPRINT(true, Debug::DEBUG10, "About to call run");
  
  parent->antennaConsumer_->run();
  
  DBPRINT(true, Debug::DEBUG10, "Leaving");
  
  return 0;
};

/**.......................................................................
 * AntennaConsumer thread cleanup function
 */
THREAD_CLEAN(Scanner::cleanAntennaConsumer)
{
  Scanner* parent = (Scanner*) arg;
  
  DBPRINT(true, Debug::DEBUG10, "Inside");
  
  if(parent->antennaConsumer_ != 0) {
    delete parent->antennaConsumer_;
    parent->antennaConsumer_ = 0;
  }
  
  DBPRINT(true, Debug::DEBUG10, "Leaving");
}

#if DIR_HAVE_MUX
/**.......................................................................
 * BolometerConsumer thread startup function
 */
THREAD_START(Scanner::startBolometerConsumer)
{
  Scanner* parent = (Scanner*) arg;
  Thread* thread = 0;

  try {
    thread = parent->getThread("BolometerConsumer");

    // Instantiate the subsystem object

    //DBPRINT(true, Debug::DEBUGANY, "About to instantiate bolometerConsumer_");

    parent->bolometerConsumer_ =
      new gcp::receiver::
      BolometerConsumer(parent, 
			parent->dioHost(), parent->dioPort(), 
			parent->hwHost(),  parent->hwPort(), 0);

    // Set our internal thread pointer pointing to the
    // BolometerConsumer thread

    parent->bolometerConsumer_->thread_ = thread;

    // Let other threads know we are ready

    thread->broadcastReady();

    // Finally, block, listening for data from the bolometer.

    DBPRINT(true, Debug::DEBUG10, "About to call run");

    parent->bolometerConsumer_->run();

  } catch(Exception& err) {
    std::cout << err.what() << std::endl;
  }

  return 0;
};

/**.......................................................................
 * BolometerConsumer thread cleanup function
 */
THREAD_CLEAN(Scanner::cleanBolometerConsumer)
{
  Scanner* parent = (Scanner*) arg;

  if(parent->bolometerConsumer_ != 0)
    delete parent->bolometerConsumer_;
}

/**.......................................................................
 * BolometerConsumer thread startup function
 */
THREAD_START(Scanner::startSquidConsumer)
{
  Scanner* parent = (Scanner*) arg;
  Thread* thread = 0;

  try {
    thread = parent->getThread("SquidConsumer");

    // Instantiate the subsystem object

    parent->squidConsumer_ =
      new gcp::receiver::
      SquidConsumer(parent, 
		    parent->dioHost(), parent->dioPort(), 
		    parent->hwHost(),  parent->hwPort());

    // Set our internal thread pointer pointing to the
    // BolometerConsumer thread

    parent->squidConsumer_->thread_ = thread;

    // Let other threads know we are ready

    thread->broadcastReady();

    // Finally, block, listening for data from the bolometer.

    DBPRINT(true, Debug::DEBUG10, "About to call run");

    parent->squidConsumer_->run();

  } catch(Exception& err) {
    std::cout << err.what() << std::endl;
  }

  return 0;
};

/**.......................................................................
 * BolometerConsumer thread cleanup function
 */
THREAD_CLEAN(Scanner::cleanSquidConsumer)
{
  Scanner* parent = (Scanner*) arg;

  if(parent->squidConsumer_ != 0)
    delete parent->squidConsumer_;
}
#endif

#if DIR_HAVE_MUX
/**.......................................................................
 * ReceiverConfigConsumer thread startup function
 */
THREAD_START(Scanner::startReceiverConfigConsumer)
{
  Scanner* parent = (Scanner*) arg;
  Thread* thread = 0;

  try {
    thread = parent->getThread("ReceiverConfigConsumer");

    parent->receiverConfigConsumer_ = 
      new gcp::receiver::ReceiverConfigConsumer(parent, 
						parent->hwHost(), 
						parent->hwPort());

    // Set our internal thread pointer pointing to the
    // BolometerConsumer thread

    parent->receiverConfigConsumer_->thread_ = thread;

    // Let other threads know we are ready

    thread->broadcastReady();

    // Finally, block, listening for data from the bolometer.

    DBPRINT(true, Debug::DEBUG10, "About to call run");

    parent->receiverConfigConsumer_->run();

  } catch(Exception& err) {
    std::cout << err.what() << std::endl;
  }

  return 0;
};

/**.......................................................................
 * BolometerConsumer thread cleanup function
 */
THREAD_CLEAN(Scanner::cleanReceiverConfigConsumer)
{
  Scanner* parent = (Scanner*) arg;

  if(parent->receiverConfigConsumer_ != 0)
    delete parent->receiverConfigConsumer_;
}
#endif

//-----------------------------------------------------------------------
// Messaging functions
//-----------------------------------------------------------------------

/**.......................................................................
 * Service our message queue.
 */
void Scanner::serviceMsgQSave()
{
  bool stop   = false;
  int  nready = 0;
  int  msgqFd = msgq_.fd();
  
  // Register the msgq to be watched for readability  
  
  fdSet_.registerReadFd(msgqFd);
  
  // Initially our select loop will only check the msgq file
  // descriptor for readability, but attempt to connect to the control
  // port every two seconds.  Once a connection is achieved, the
  // select loop will block until either a message is received on the
  // message queue, or on the control port.
  
  while(!stop) {
    
    nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
		  NULL, NULL);
    
    // A message on our message queue?
    
    if(fdSet_.isSetInRead(msgqFd)) {
      processTaskMsg(&stop);
    }

    // If we are connected the archiver, check the archiver fd
    
    if(client_.isConnected()) {
      
      // A greeting message from the archiver?
      
      if(fdSet_.isSetInRead(client_.getFd())) {
	arrayStr_->read();
      }
      
      // Send a pending data frame to the control program
      
      if(fdSet_.isSetInWrite(client_.getFd())) {
	arrayStr_->send();
      }

    }
  }
}

/**.......................................................................
 * Service our message queue.
 */
void Scanner::serviceMsgQ()
{
  bool stop   = false;
  int  nready = 0;
  int  msgqFd = msgq_.fd();
  TimeOut timeOut_;

  timeOut_.setIntervalInSeconds(1);

  // Timeout shouldn't get used for the running system.  Frames are
  // driven by the Master thread sending a message on the 1-second
  // timer

  timeOut_.activate(false);

  // Register the msgq to be watched for readability  
  
  fdSet_.registerReadFd(msgqFd);
  
  // Initially our select loop will only check the msgq file
  // descriptor for readability, but attempt to connect to the control
  // port every two seconds.  Once a connection is achieved, the
  // select loop will block until either a message is received on the
  // message queue, or on the control port.
  
  while(!stop) {
    
    nready=select(fdSet_.size(), fdSet_.readFdSet(), fdSet_.writeFdSet(), 
		  NULL, timeOut_.tVal());
    
    // A message on our message queue?
    
    if(fdSet_.isSetInRead(msgqFd)) {
      processTaskMsg(&stop);
    }

    // If we are connected the archiver, check the archiver fd
    
    if(client_.isConnected()) {
      
      // A greeting message from the archiver?
      
      if(fdSet_.isSetInRead(client_.getFd())) {
	arrayStr_->read();
      }
      
      // Send a pending data frame to the control program
      
      if(fdSet_.isSetInWrite(client_.getFd())) {
	arrayStr_->send();
      }

    }

    if(nready == 0) {
      startNewArrayFrame();
    }

  }
}

/**.......................................................................
 * Process a message specific to this task.
 */
void Scanner::processMsg(ScannerMsg* msg) 
{
  //COUT("Inside Scanner::processMsg: id = " << msg->type);

  switch (msg->type) {

    // Our frame timer has expired

  case ScannerMsg::START_DATAFRAME:
    startNewArrayFrame();
    break;
    
    // Dispatch the next data frame in the queue

  case ScannerMsg::DISPATCH_DATAFRAME:
    dispatchNextFrame();
    break;

  case ScannerMsg::FEATURE:
    changeFeatures(msg->body.feature.seq, msg->body.feature.mode, 
		   msg->body.feature.mask);
    break;
  case ScannerMsg::CONNECT:
    connectScanner(false);
    break;
#if DIR_HAVE_MUX
  case ScannerMsg::PACK_BOLO_DATAFRAME:
    packBolometerFrame();
    break;
  case ScannerMsg::DIO_MSG:
    forwardDioMsg(msg);
    break;
#endif
  default:
    ThrowError("Unrecognized message type: " << msg->type);
    break;
  }

  //COUT("Leaving Scanner::processMsg: id = " << msg->type);

}

/**.......................................................................
 * Method to tell this task to dispatch the next frame
 *
 * @throws Exception (via MsgQ::sendMsg)
 */
void Scanner::sendDispatchDataFrameMsg()
{
  ScannerMsg msg;
  
  msg.packDispatchDataFrameMsg();
  
  sendTaskMsg(&msg);
}

/**.......................................................................
 * Method by which we can tell the control task to reconnect to an
 * antenna.
 *
 * @throws Exception (via MsgQ::sendMsg)
 */
void Scanner::sendFlagAntennaMsg(unsigned antenna, bool flag)
{
  MasterMsg masterMsg;
  ControlMsg* controlMsg = masterMsg.getControlMsg();
  AntennaControlMsg* msg = controlMsg->getAntennaControlMsg();
  
  msg->packFlagAntennaMsg(antenna, flag);
  
  parent_->forwardMasterMsg(&masterMsg);
}

/**.......................................................................
 * A method to send a connection status message to the
 * Master object.
 */
void Scanner::sendScannerConnectedMsg(bool connected)
{
  MasterMsg msg;
  
  msg.packScannerConnectedMsg(connected);
  
  parent_->forwardMasterMsg(&msg);
}

/**.......................................................................
 * A method to send a connection status message to the
 * Master object.
 */
void Scanner::sendControlConnectedMsg(bool connected)
{
  MasterMsg msg;
  
  msg.packControlConnectedMsg(connected);
  
  parent_->forwardMasterMsg(&msg);
}

/**.......................................................................
 * Method to tell this task to pack the current bolometer frame
 *
 * @throws Exception (via MsgQ::sendMsg)
 */
void Scanner::sendPackBoloDataFrameMsg()
{
  ScannerMsg msg;
  
  msg.packPackBoloDataFrameMsg();
  
  sendTaskMsg(&msg);
}

/**.......................................................................
 * We will piggyback on the heartbeat signal received from the master
 * to send a heartbeat to our own tasks.
 */
void Scanner::respondToHeartBeat()
{
  // Respond to the master heartbeat request
  
  if(thread_ != 0)
    thread_->setRunState(true);
  
  // And ping any threads we are running.
  
  if(threadsAreRunning()) 
    pingThreads(this);
  else {
    sendRestartMsg();
  }
}

/**.......................................................................
 * Attempt to connect to the host
 */
void Scanner::connectScanner(bool reEnable)
{
  DBPRINT(true, Debug::DEBUG7, "reEnable = " << reEnable);
  
  // Ignore any further connection requests if we are in the process
  // of handshaking with the control program, or if we are already
  // connected (there may be a delay between the message getting
  // through to the master process that we have connected, and receipt
  // of another prod to connect.
  
  if(client_.isConnected())
    return;
  
  // Else attempt to connect
  
  if(connect())
    sendScannerConnectedMsg(true);
  
  // Only send a message to reenable the connect timer if it was
  // previously disabled.
  
  else if(reEnable)
    sendScannerConnectedMsg(false);
}

//-----------------------------------------------------------------------
// Network functions
//-----------------------------------------------------------------------

/**.......................................................................
 * Connect to the archiver port of the control program.
 *
 * Return:
 *  return     bool    true  - OK.
 *                     false - Error.
 */
bool Scanner::connect()
{
  // Terminate any existing connection.
  
  disconnect();
  
  if(client_.connectToServer(parent_->ctlHost(), CP_RTS_PORT, true) < 0) {
    ErrorDef(err, "Scanner::connect: Error in tcp_connect().\n");
    return false;
  }
  
  // Once we've successfully achieved a connection, reconfigure the
  // socket for non-blocking I/O
  
  client_.setBlocking(false);
  
  // Attach the network I/O streams to the new client socket.
  
  arrayStr_->attach(client_.getFd());
  
  // Register the socket to be watched for input.  We should get a
  // greeting message after the connection is made.
  
  fdSet_.registerReadFd(client_.getFd());
  
  return true;
}

/**.......................................................................
 * Disconnect the connection to the control-program archiver.
 *
 * Input:
 *  sup    Supplier *  The resource object of the program.
 */
void Scanner::disconnect()
{
  CTOUT("Inside disconnect...");
  // Clear the client fd from the set to be watched in select()
  
  if(client_.isConnected())
    fdSet_.clear(client_.getFd());
  
  // Disconnect from the server socket.
  
  client_.disconnect();
  
  // Detach network handlers
  
  arrayStr_->attach(-1);
}


/**.......................................................................
 * Disconnect from the host.
 */
void Scanner::disconnectScanner()
{
  // Disconnect and let the parent know it should re-enable the
  // connect timer.
  
  disconnect();
  
  sendScannerConnectedMsg(false);
  
  // We'll assume for now that the control connection has gone bad too
  
  sendControlConnectedMsg(false);
}

/**.......................................................................
 * Dispatch the next unsent frame  
 */
void Scanner::dispatchNextFrame() 
{
  static unsigned a3Counter=0;
  
  // If we are currently in the process of sending a frame, or have no
  // connection to the control program, do nothing.
  
  if(dispatchPending_ || !client_.isConnected())
    return;
  
  // Get the next frame (if any) to be dispatched
  
  DataFrameManager* frame = fb_.dispatchNextFrame();
  
  if(frame == 0)
    return;
  
  // Pack the frame into our network buffer.
 
  packFrame(frame->frame());
  
  // And register the archiver file descriptor to be watched for
  // writability
  
  fdSet_.registerWriteFd(client_.getFd());
  
  // And mark the transaction as in progress.
  
  dispatchPending_ = true;
}

/**.......................................................................
 * Install the frame buffer as the network buffer and pre-format the
 * register frame output message.
 */
void Scanner::packFrame(DataFrame* frame)
{
  NetSendStr* nss = arrayStr_->getSendStr();
  
  // Install the frame manager's data buffer as the network buffer.
  
  //  COUT("Inside packFrame with nbyte = " << frame->nByte());

  nss->setBuffer(frame->data(), frame->nByte());
  nss->startPut(0);
  nss->incNput(frame->nByte() - NET_PREFIX_LEN);
  nss->endPut();
}

/**.......................................................................
 * A handler to be called when a frame has been completely sent.
 */
NET_SEND_HANDLER(Scanner::sendHandler)
{
  Scanner* scanner =  (Scanner*)arg;
  
  // Mark the transaction as finished
  
  scanner->dispatchPending_ = false;
  
  // And stop watching the file descriptor
  
  scanner->fdSet_.clearFromWriteFdSet(scanner->client_.getFd());
  
  // Delay frames enough for receiver to catch up as it may run
  // behind due to digital filtering
   
  if(scanner->fb_.getNframesInQueue() > MIN_FRAMES_IN_QUEUE)
    scanner->sendDispatchDataFrameMsg();
}

/**.......................................................................
 * A handler to be called when a message has been completely read
 */
NET_READ_HANDLER(Scanner::readHandler)
{
  Scanner* scanner =  (Scanner*)arg;
  
  // Mark the transaction as finished
  
  scanner->parseGreeting();
}

/**.......................................................................
 * Parse a greeting message.
 */
void Scanner::parseGreeting()
{
  int opcode;                     // Message-type opcode 
  unsigned int arraymap_revision; // Register map structure revision 
  unsigned int arraymap_narchive; // The number of archive registers 

  NetReadStr* nrs = arrayStr_->getReadStr();
  
  nrs->startGet(&opcode);
  
  if(opcode != SCAN_GREETING) {
    ThrowError("Corrupt greeting message.");
  };
  
  nrs->getInt(1, &arraymap_revision);
  nrs->getInt(1, &arraymap_narchive);
  nrs->endGet();
  
  // Check that the register map being used by this program and the
  // remote host are in sync.
  
  if(arraymap_revision != ARRAYMAP_REVISION ||
     arraymap_narchive != arraymap_->narchive) 
  {
    ThrowError("Register-map mismatch wrt host.");
  }
}

/**.......................................................................
 * A handler to be called when an error occurs in communication
 */
NET_ERROR_HANDLER(Scanner::networkError)
{
  Scanner* scanner =  (Scanner*)arg;
  
  DBPRINT(true, Debug::DEBUG7, "Inside network error ");
  
  scanner->disconnectScanner();
}

/**.......................................................................
 * Change the features
 */
void Scanner::changeFeatures(unsigned seq, unsigned mode, unsigned mask)
{
  switch((gcp::control::FeatureMode)mode) {
    
    // When adding a persistent feature marker, the marker is also
    // added to the transient marker set to ensure that even if the
    // persistent marker is cancelled before the next frame is
    // recorded, it will be seen in at least one frame.
    
  case gcp::control::FEATURE_ADD:
    features_.persistent |= mask;
    features_.transient  |= mask;
    break;
    
    // Remove a feature marker from the persistent set.
    
  case gcp::control::FEATURE_REMOVE:
    features_.persistent &= ~mask;
    break;
    
    // Arrange for the marker to be set in just the next frame to be
    // archived.
    
  case gcp::control::FEATURE_ONE:
    features_.transient |= mask;
    break;
  };
  
  // Record the sequence number of the transaction.
  
  features_.seq = seq;
}
