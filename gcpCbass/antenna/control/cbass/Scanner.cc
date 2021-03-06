#define __FILEPATH__ "antenna/control/specific/Scanner.cc"

#include "gcp/util/common/Debug.h"
#include "gcp/antenna/control/specific/Scanner.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor with Antenna number
 */
Scanner::Scanner(SpecificShare* share, AntNum* ant) : 
  fb_(ant), share_(share)
{
  unsigned short iant;

  // Initialize the board pointers to NULL

  frame_     = 0;

  // Now allocate the new descriptors

  frame_     = new FrameBoard(share, "frame");

  // Finally, perform various initialization tasks

  initialize();
}

/**.......................................................................
 * Destructor
 */
Scanner::~Scanner()
{
  if(frame_ != 0)
    delete frame_;
}

/**.......................................................................
 * Reset non-pointer members of the Scanner object
 */
void Scanner::initialize()
{
  skipOne_            = false;
  recordNumber_       = 0;

  // Reset members of bookkeeping structs

  features_.reset();

  // And record the walsh state of the FrameBoard register

  recordWalshState();


  DBPRINT(true, Debug::DEBUG14, "Exiting initialize()");
}

/**.......................................................................
 * Reset members of the features struct
 */
void Scanner::Features::reset()
{
  seq_        = 0;
  transient_  = 0;
  persistent_ = 0;
}

/**.......................................................................
 * Reset members of the walsh struct
 */
void Scanner::Walsh::reset()
{
  // Default to no slow walshing and set the current and last walsh
  // states to 0.

  current_on_    = false;
  request_on_    = false;
  counter_       = 0;
  current_state_ = 0;
  last_state_    = 0;
}

/**.......................................................................
 * Update the record number internally, and in the register database
 */
void Scanner::recordRecordNumber(unsigned record)
{
  frame_->archiveRecordNumber(record);
  recordNumber_ = record;
}

/**.......................................................................
 * Record the features mask in the database
 */
void Scanner::recordFeatures()
{
  unsigned features = features_.transient_ | features_.persistent_;
  frame_->archiveFeatures(features, features_.seq_);
}

/**.......................................................................
 * Record the walshstate bitmask in the database
 */
void Scanner::recordWalshState()
{
}

/**.......................................................................
 * Record the features mask in the database
 */
void Scanner::recordTime()
{
  frame_->archiveTime();
}

/**.......................................................................
 * Record the features mask in the database
 */
void Scanner::setTime()
{
  frame_->setTime();
}

/**.......................................................................
 * If there is room in the circular frame buffer, record another
 * data frame and push it onto the event channel.
 *
 * Input:
 *  sender  FrameSender  An object used to push data onto 
 *                       a TCP/IP socket or CORBA event channel.
 */
void Scanner::packNextFrame()
{
  // Set the current time in the database.

  setTime();

  // Increment the record number.

  recordRecordNumber(recordNumber_+1);

  // Record the time.

  recordTime();

  // Record the bit-mask of feature markers received since the last
  // record was successfully dispatched to the archiver, plus any
  // persistent features that haven't been cancelled yet.

  recordFeatures();

  // Copy any buffered registers into the frame

  share_->copyBufferedBlocksToFrame();

  // Finally, create a new frame, and add it to the queue of frames to
  // be sent off
  
  share_->packFrame(fb_.getNextFrame());

  // Finally, clear the transient feature mask.

  features_.transient_ = 0;
}

/**.......................................................................
 * If there is room in the circular frame buffer, record another
 * data frame and push it onto the event channel.
 *
 * Input:
 *  sender  FrameSender  An object used to push data onto 
 *                       a TCP/IP socket or CORBA event channel.
 */
DataFrameManager* Scanner::dispatchNextFrame()
{
  // Get the next frame (if any) to be dispatched

  return fb_.dispatchNextFrame();
}

/**.......................................................................
 * If there is room in the circular frame buffer, record another
 * data frame and push it onto the event channel.
 *
 * Input:
 *  sender  FrameSender  An object used to push data onto 
 *                       a TCP/IP socket or CORBA event channel.
 */
void Scanner::dispatchNextFrame(FrameSender* sender)
{
  // Send the frame off to the outside world

  sender->sendFrame(fb_.dispatchNextFrame());
}

/**.......................................................................
 * Return the number of frames waiting to be dispatched.
 */
unsigned int Scanner::getNframesInQueue()
{
  return fb_.getNframesInQueue();
}

