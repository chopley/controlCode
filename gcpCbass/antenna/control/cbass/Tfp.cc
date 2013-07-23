#include "gcp/antenna/control/specific/Tfp.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

const std::string Tfp::devName_("/dev/tfp0");

#define ACK_NDELAY 1000

/**.......................................................................
 * Constructor.
 */
Tfp::Tfp(bool simulate) : simulate_(simulate)
{
  initialize();

  if(!simulate_) {
    map();
    specificSetUp();
  }
}

/**.......................................................................
 * Destructor.
 */
Tfp::~Tfp() 
{
  if(!simulate_) {
    close();
  }
}

void Tfp::initialize()
{
  fd_ = -1;
  resetCtlPointers();
  resetDpramPointers();
}

void Tfp::resetCtlPointers()
{
  ctlPtr_         =  0;
  ctlPtrLength_   =  0;
  
  ctlAckPtr_         =  0;
  ctlCtlPtr_         =  0;
  ctlTime0Ptr_       =  0;
  ctlTime1Ptr_       =  0;
  ctlTimeReqPtr_     =  0;
}

void Tfp::resetDpramPointers()
{
  dpramPtr_        =  0;
  dpramPtrLength_  =  0;
  
  dpramInPtr_      =  0;
  dpramOutPtr_     =  0;
  dpramGpsPtr_     =  0;
  dpramYearPtr_    =  0;
  
  dpramInOffset_   =  0;
  dpramOutOffset_  =  0;
  dpramGpsOffset_  =  0;
  dpramYearOffset_ =  0;
}

/**.......................................................................
 * Open the device 
 */
void Tfp::open()
{
  // Make sure the file is closed before we try to reopen it.
  
  close();
  
  // Now attempt to open the device
  
  fd_ = ::open(devName_.c_str(), O_RDWR);
  
  if(fd_ < 0)
    ThrowSysError("open()");
}

/**.......................................................................
 * Close the device
 */
void Tfp::close()
{
  unmap();
  
  if(fd_ > 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

/**.......................................................................
 * Get a memory-mapped pointer to various control registers on the tfp
 * card
 */
void Tfp::mapCtlPtr(unsigned byteOffset)
{
  // Return immediately if we already have a reference
  
  if(ctlPtr_ != 0x0)
    return;
  
  // Make sure the descriptor is open
  
  if(fd_ < 0)
    open();
  
  // Length of the region to be mapped, in bytes
  
  unsigned lengthInBytes = ctlSizeInBytes_ - byteOffset;
  unsigned offsetInBytes = byteOffset + ctlOffsetInBytes_;
  
  errno = 0;
  ctlPtr_ = mmap(0x0, lengthInBytes, PROT_READ|PROT_WRITE, MAP_SHARED, 
		 fd_, offsetInBytes);
  
  if(errno != 0)
    ThrowSysError("mmap()");
  
  ctlPtrLength_ = lengthInBytes;
  
  // Store pointers to useful locations in the memory map
  
  ctlTimeReqPtr_ = (unsigned*) ctlPtr();
  ctlTime0Ptr_   = (unsigned*)(ctlPtr() + CTL_OFF_TIME0);
  ctlTime1Ptr_   = (unsigned*)(ctlPtr() + CTL_OFF_TIME1);
  ctlAckPtr_     = (unsigned*)(ctlPtr() + CTL_OFF_ACK);
  ctlCtlPtr_     = (unsigned*)(ctlPtr() + CTL_OFF_CONTROL);
}

/**.......................................................................
 * Get a memory-mapped pointer to the dual-ported RAM area on the tfp
 * card
 */
void Tfp::mapDpramPtr(unsigned byteOffset)
{
  // Return immediately if we already have a reference
  
  if(dpramPtr_ != 0x0)
    return;
  
  // Make sure the descriptor is open
  
  if(fd_ < 0)
    open();
  
  // Length of the region to be mapped, in bytes
  
  unsigned lengthInBytes = dpramSizeInBytes_ - byteOffset;
  
  // The offset we pass to mmap() is the offset relative to the first
  // bar.  The dpram is located after the ctl registers, so add in the
  // ctl length to get to the second bar.
  
  unsigned offsetInBytes = byteOffset + dpramOffsetInBytes_;
  
  errno = 0;
  dpramPtr_ = mmap(0x0, lengthInBytes, PROT_READ|PROT_WRITE, MAP_SHARED, 
		   fd_, offsetInBytes);
  
  if(errno != 0)
    ThrowSysError("mmap()");
  
  dpramPtrLength_ = lengthInBytes;
  
  // Now read the tail of the DPRAM area to determine the offsets of
  // the user-accessible ares
  
  unsigned char* cPtr = dpramPtr();
  dpramInOffset_ = constructShort(cPtr[dpramSizeInBytes_-2],
				  cPtr[dpramSizeInBytes_-1]);
  
  dpramOutOffset_ = constructShort(cPtr[dpramSizeInBytes_-4],
				   cPtr[dpramSizeInBytes_-3]);
  
  dpramGpsOffset_ = constructShort(cPtr[dpramSizeInBytes_-6],
				   cPtr[dpramSizeInBytes_-5]);
  
  dpramYearOffset_ = constructShort(cPtr[dpramSizeInBytes_-8],
				    cPtr[dpramSizeInBytes_-7]);
  
  // Store pointers to useful locations
  
  dpramInPtr_   = dpramPtr() + dpramInOffset_;
  dpramOutPtr_  = dpramPtr() + dpramOutOffset_;
  dpramGpsPtr_  = dpramPtr() + dpramGpsOffset_;
  dpramYearPtr_ = dpramPtr() + dpramYearOffset_;
}

/**.......................................................................
 * Map our pointers
 */
void Tfp::map()
{
  mapCtlPtr();
  mapDpramPtr();
}

/**.......................................................................
 * Unmap any pointers which were previously mapped
 */
void Tfp::unmap()
{
  if(ctlPtr_ != 0x0) {
    munmap(ctlPtr_, ctlPtrLength_);
    resetCtlPointers();
  }
  
  if(dpramPtr_ != 0x0) {
    munmap(dpramPtr_, dpramPtrLength_);
    resetDpramPointers();
  }
}

/**.......................................................................
 * Map our pointers
 */
unsigned char* Tfp::ctlPtr()
{
  if(ctlPtr_ == 0)
    mapCtlPtr();
  
  return (unsigned char*)ctlPtr_;
}

/**.......................................................................
 * Return a pointer to the DPRAM
 */
unsigned char* Tfp::dpramPtr()
{
  if(dpramPtr_ == 0)
    mapDpramPtr();
  
  return (unsigned char*)dpramPtr_;
}

//-----------------------------------------------------------------------
// Methods for packing a command destined for the DPRAM area
//-----------------------------------------------------------------------

Tfp::DpramCmd::DpramCmd(DpramCmdId cmd)
{
  data_.push_back((unsigned char)cmd);
}

void Tfp::DpramCmd::packVal(unsigned char val)
{
  data_.push_back(val);
}

void Tfp::DpramCmd::packVal(unsigned short val)
{
  unsigned char* cptr = (unsigned char*)&val;
  data_.push_back(cptr[1]);
  data_.push_back(cptr[0]);
}

void Tfp::DpramCmd::packVal(unsigned int val)
{
  unsigned char* cptr = (unsigned char*)&val;
  unsigned char buff[4];

  buff[0] = (val>>24&0xff);
  buff[1] = (val>>16&0xff);
  buff[2] = (val>>8&0xff);
  buff[3] = (val&0xff);

  std::cout << "Val = "; 
  printBits(val);

  std::cout << "buff[0]/cptr[3] = "; 
  printBits(buff[0]);
  printBits(cptr[3]);

  std::cout << "buff[1]/cptr[2] = "; 
  printBits(buff[1]);
  printBits(cptr[2]);

  std::cout << "buff[2]/cptr[1] = "; 
  printBits(buff[2]);
  printBits(cptr[1]);

  std::cout << "buff[3]/cptr[0] = "; 
  printBits(buff[3]);
  printBits(cptr[0]);

  data_.push_back(cptr[3]);
  data_.push_back(cptr[2]);
  data_.push_back(cptr[1]);
  data_.push_back(cptr[0]);
}

/*.......................................................................
 * Send a command via the DPRAM interface
 */ 
void Tfp::sendDpramCmd(DpramCmd& cmd, bool wait)
{
  memcpy((void*)dpramInPtr_, (void*)&cmd.data_[0], cmd.data_.size());
  
  if(wait)
    waitForAck();
}

/**.......................................................................
 * Delay until the ack bit is set.
 *
 * Don't ass up the CPU by polling; use select() instead.
 */
void Tfp::waitForAck()
{
  *ctlAckPtr_ =  READ_CMD;

  TimeVal timeOut;
  timeOut.setMicroSeconds(10000);

  unsigned count=0, maxCount=10;

  while(count < maxCount) {
    timeOut.reset();
    (void)select(0, NULL, NULL, NULL, timeOut.timeVal());
    if((*ctlAckPtr_ & 0x1))
      return;
    
    ++count;
  }
  
  ThrowError("Command wasn't acknowledged");
}

//-----------------------------------------------------------------------
// User API begins here
//-----------------------------------------------------------------------

/**.......................................................................
 * Read the time, in decimal BCD format
 */
void Tfp::readBcdTime(gcp::util::TimeVal& tVal)
{
  unsigned time0, time1;
  
  // Latch and read the time
  
  *ctlTimeReqPtr_ = 0x1;
  
  time0 = *ctlTime0Ptr_;
  time1 = *ctlTime1Ptr_;
  
  unsigned days, hours, min, sec;

  sec   = time1 & 0xff;
  min   = (time1 >> 8) & 0xff;
  hours = (time1 >> 16) & 0x0f;
  days  = (time1 >> 24) & 0xff;

  COUT("Time is: "
       << readYear()  << " (year)"
       << days  << " days"
       << hours << " hours"
       << min   << " min"
       << sec   << " sec");
}

/**.......................................................................
 * Read the time, in binary UNIX format
 */
void Tfp::readUnixTime(gcp::util::TimeVal& tVal)
{
  unsigned time0, time1;
  
  // Latch and read the time
  
  *ctlTimeReqPtr_ = 0x1;
  
  time0 = *ctlTime0Ptr_;
  time1 = *ctlTime1Ptr_;
  
  unsigned sec = time1;
  
  // Microseconds are the first 20 bits (0xfffff)
  
  unsigned uSec = time0 & 0xfffff;
  
  // 100s of ns are the next 4 (0xf)
  
  unsigned nSec = 100*(((time0) >> 20) & 0xf);
  
#if 0
  COUT("readUnixTime(): Time is: " << sec << " seconds, " << uSec << " microseconds, " << nSec << " nanoseconds");
#endif

  tVal.setTime(sec, uSec, nSec);
}

/**.......................................................................
 * Set the year
 */
void Tfp::setYear(unsigned short year)
{
  if(year < 1970 || year > 2050)
    ThrowError("Invalid year: " << year);
  
  DpramCmd cmd(DP_CMD_SET_YEAR);
  cmd.packVal(year);
  sendDpramCmd(cmd, true);
}

void Tfp::setTimeMode(TimeMode mode)
{
  DpramCmd cmd(DP_CMD_TIME_MODE);
  cmd.packVal((unsigned char)mode);
  sendDpramCmd(cmd, true);
}

/**.......................................................................
 * Set the time register format
 */
void Tfp::setTimeFormat(TimeFormat format)
{
  DpramCmd cmd(DP_CMD_TIME_FORMAT);
  cmd.packVal((unsigned char)format);
  sendDpramCmd(cmd, true);
}

/**.......................................................................
 * Set the format of the input timecode signal
 */
void Tfp::setInputTimeCodeFormat(TimeCodeFormat format)
{
  DpramCmd cmd(DP_CMD_SET_INP_FORMAT);
  cmd.packVal((unsigned char)format);
  sendDpramCmd(cmd, true);
}

/**.......................................................................
 * Set the format of the timecode signal output on the J1 connector
 */
void Tfp::setOutputTimeCodeFormat(TimeCodeFormat format)
{
  DpramCmd cmd(DP_CMD_SET_OUT_FORMAT);
  cmd.packVal((unsigned char)format);
  sendDpramCmd(cmd, true);
}

/**.......................................................................
 * Set the modulation type of the input timecode signal
 */
void Tfp::setInputTimeCodeModType(InputTimeCodeModType mod)
{
  DpramCmd cmd(DP_CMD_SET_INP_MOD);
  cmd.packVal((unsigned char)mod);
  sendDpramCmd(cmd, true);
}

/**.......................................................................
 * Set the major component of the time, in UNIX format
 */
void Tfp::setMajorTime(unsigned int seconds)
{
  setTimeFormat(FORMAT_UNIX);

  DpramCmd cmd(DP_CMD_SET_MAJOR);
  cmd.packVal(seconds);
  sendDpramCmd(cmd, true);
}

/**.......................................................................
 * Set the major component of the time, in BCD format
 */
void Tfp::setMajorTime(unsigned short year, unsigned short dayNo, 
		       unsigned char hours, unsigned char minutes, unsigned char seconds)
{
  setTimeFormat(FORMAT_BCD);

  DpramCmd cmd(DP_CMD_SET_MAJOR);
  cmd.packVal(year);
  cmd.packVal(dayNo);
  cmd.packVal(hours);
  cmd.packVal(minutes);
  cmd.packVal(seconds);

  sendDpramCmd(cmd, true);
}

/**.......................................................................
 * Software reset
 */
void Tfp::softReset()
{
  DpramCmd cmd(DP_CMD_SOFT_RESET);
  sendDpramCmd(cmd, false);
}

/**.......................................................................
 * Read the year
 */
unsigned short Tfp::readYear()
{
  return constructShort(dpramYearPtr_[0], dpramYearPtr_[1]);
}

/**.......................................................................
 * Request the assembly part number.  
 *
 * I don't understand yet how data commands are supposed to work.  The
 * manual says, "use this command in conjunction with 0x19" which is
 * the request data command, but it is not clear if the command for
 * which we want data is the data body of the 0x19 command, or if we
 * need to separately issue the data command before requesting the
 * data.  I would think the former, but I have failed to get anything
 * sensible back from the card when any of these commands are issued.
 */
void Tfp::requestAssemblyPartNo()
{
  COUT("Output was: " 
       << (unsigned int)dpramOutPtr_[0]
       << (unsigned int)dpramOutPtr_[1]);

  //  DpramCmd cmd1(DP_CMD_REQ_ASSY);
  //  sendDpramCmd(cmd1, true);

  DpramCmd cmd2(DP_CMD_REQ_DATA);
  cmd2.packVal((unsigned char)DP_CMD_REQ_ASSY);
  sendDpramCmd(cmd2, true);

  // Now read from the output area

  COUT("Output is: " 
       << (unsigned int)dpramOutPtr_[0]
       << (unsigned int)dpramOutPtr_[1]);
}

/**.......................................................................
 * Request the model ID number
 */
void Tfp::requestModelId()
{
  //  DpramCmd cmd1(DP_CMD_REQ_MODEL);
  //  sendDpramCmd(cmd1, true);

  DpramCmd cmd2(DP_CMD_REQ_DATA);
  cmd2.packVal((unsigned char)DP_CMD_REQ_MODEL);
  sendDpramCmd(cmd2, true);

  // Now read from the output area

  printBits(dpramOutPtr_[0]);
  printBits(dpramOutPtr_[1]);
  printBits(dpramOutPtr_[2]);
  printBits(dpramOutPtr_[3]);
  printBits(dpramOutPtr_[4]);
}

/**.......................................................................
 * Set periodic output.
 *
 * Frequency output is:
 *
 *     1,000,000/(n1 * n2) Hz, for 2 <= n1, n2 <= 65535
 *
 * Duty cycle is given by:
 *
 *     (1 - (1 / n2)) * 100%
 */
void Tfp::setFrequencyOutput(bool syncToPps, double dutyCyclePerc, double freqInHz)
{
  unsigned short n2 = (unsigned short)(100.0/(100.0 - dutyCyclePerc));
  unsigned short n1 = (unsigned short)(1000000.0/(freqInHz * n2));
  COUT("n1 = " << n1 << ", n2 = " << n2);
  setPeriodicOutput(syncToPps, n1, n2);
}

void Tfp::setPeriodicOutput(bool syncToPps, unsigned short n1, unsigned short n2)
{
  COUT("Calculated values will set the frequency to: " << 1000000.0/(n1*n2) << " Hz");
  COUT("With duty cycle: " << (1.0 - 1.0/n2) * 100 << "%");

  DpramCmd cmd(DP_CMD_SET_PERIODIC_OUTPUT);
  cmd.packVal((unsigned char)syncToPps);
  cmd.packVal(n1);
  cmd.packVal(n2);

  sendDpramCmd(cmd, true);
}

/**.......................................................................
 * Set the output clock frequency
 */
void Tfp::setOutputClockFreq(OutputFreq freq)
{
  unsigned ctlStatus = *ctlCtlPtr_;

  printBits(ctlStatus);

  // Clear the frequency bit locations

  ctlStatus &= ~(0x3 << 6);

  // Now set the requested bits

  ctlStatus |= (freq << 6);

  // And write the new configuration back to the control register

  *ctlCtlPtr_ = ctlStatus;
}

//-----------------------------------------------------------------------
// Debugging utility functions
//-----------------------------------------------------------------------

void Tfp::printBits(unsigned char c)
{
  for(unsigned i=0; i < 8; i++) 
    std::cout << ((c>>i) & 0x1);
  
  std::cout << std::endl;
}

void Tfp::printBits(unsigned short s)
{
  for(unsigned i=0; i < 16; i++) {
    if(i%8==0)
      std::cout << " ";
    std::cout << ((s>>i) & 0x1);
  }
  
  std::cout << std::endl;
}

void Tfp::testBits(unsigned int iVal)
{
  unsigned char* c = (unsigned char*)&iVal;
  for(unsigned i=0; i < 4; i++)
    printBits(c[i]);
}

void Tfp::printBits(unsigned int iVal)
{
  for(unsigned i=0; i < 32; i++) {
    if(i%8==0)
      std::cout << " ";
    std::cout << ((iVal>>i) & 0x1);
  }
  
  std::cout << std::endl;
}

/**.......................................................................
 * Method to construct a short from the 1-byte wide addressing space
 * of the DPRAM
 */
unsigned short Tfp::constructShort(unsigned char msb, unsigned char lsb)
{
  unsigned short s;
  unsigned char* cPtr = (unsigned char*)&s;
  
  cPtr[0] = lsb;
  cPtr[1] = msb;
  
  return s;
}

/**.......................................................................
 * Method to construct an unsigned int from the 1-byte wide addressing
 * space of the DPRAM
 */
unsigned Tfp::constructInt(unsigned char msb, unsigned char smsb, unsigned tmsb, unsigned char lsb)
{
  unsigned u;
  unsigned char* cPtr = (unsigned char*)&u;
  
  cPtr[0] = lsb;
  cPtr[1] = tmsb;
  cPtr[2] = smsb;
  cPtr[3] = msb;
  
  return u;
}

void Tfp::defaultSetUp()
{
  // Set the timing mode to Time Code Mode

  setTimeMode(MODE_TIMECODE);

  // Set the register format to output UNIX binary

  setTimeFormat(FORMAT_UNIX);

  // Set the time code input format

  setInputTimeCodeFormat(FORMAT_IRIGB);

  // Set up for an amplitude-modulated timecode signal

  setInputTimeCodeModType(INP_MOD_AMP);
}

void Tfp::specificSetUp()
{
  // Default to get time from the integrated GPS receiver

  setTimeMode(MODE_GPS);

  // No local time offset

  setLocalTimeOffset(0);

  // Default to no propagation delay

  setPropagationDelay(0);

  // Periodic Pulse synchronized to 1PPS with 99.9% duty cycle
  // i.e. 100 Hz 10-us pulse

  setPeriodicOutput(true, 2, 5000);

  // Get time in binary format

  setTimeFormat(FORMAT_UNIX);

  // Generate IRIG-B time code

  setOutputTimeCodeFormat(FORMAT_IRIGB);

  // Accept DC IRIG-B code input

  setInputTimeCodeFormat(FORMAT_IRIGB);

  // Use internal 10 Mhz clock

  setClockSource(CLK_INTERNAL);
    
  // Use UTC time, not GPS (leap seconds automatically included)

  setGpsFormat(FORMAT_UTC);
    
  // Optimize GPS timing mode for stationary operation

  enableGpsModeFlag(true);

  // Enable auto increment of year in GPS NVRAM on year rollover

  enableYearAutoIncrement(true);
  
#if 0
  // Disable Lockout
  // Trigger event on Periodic Output rising edge
  // Enable event capture
  // Disable strobe output
  // Output freqency 10MHz
  
  bcWriteReg(card_, PCI_CONTROL_OFFSET, 
	     (PROGRAM_PERIODIC << EVSOURCE_BIT) | 
	     (EVENT_ENABLE << EVENTEN_BIT) | 
	     (1 << FREQSEL1_BIT));
#endif
}

/**.......................................................................
 * Enable interrupts
 */
void Tfp::enableInterrupt(unsigned intMask, bool enable)
{
  // Get a pointer to the MASK control register

  unsigned* maskPtr = (unsigned*)(ctlPtr() + CTL_OFF_MASK);

  // Read the current value of the mask -- unspecified bits will be
  // left as is

  unsigned mask = *maskPtr;

  COUT("Mask was: ");
  printBits(mask);

  if(intMask & INT_EVENT)
    setBit(mask, MASK_EVENT_BIT,    enable);

  if(intMask & INT_PERIODIC)
    setBit(mask, MASK_PERIODIC_BIT, enable);

  if(intMask & INT_TIME)
    setBit(mask, MASK_TIME_BIT,     enable);

  if(intMask & INT_1PPS)
    setBit(mask, MASK_1PPS_BIT,     enable);

  COUT("Setting mask to: ");
  printBits(mask);
  printBits((unsigned)0x1);

  // Now set the mask in the control register

  *maskPtr = mask;
}

/**.......................................................................
 * Set or unset a bit in a 32-bit bitmask
 */
void Tfp::setBit(unsigned& mask, unsigned short bit, bool enable)
{
  if(enable)
    mask |= 1<<bit;
  else
    mask &= ~(1<<bit);
}

void Tfp::getDate(gcp::util::RegDate& date)
{
  if(simulate_) {
    date.setToCurrentTime();
    return;
  }

  readUnixTime(date.timeVal());
  date.updateFromTimeVal();
}

void Tfp::getDate(gcp::util::TimeVal& tVal)
{
  if(simulate_) {
    tVal.setToCurrentTime();
    return;
  }

  readUnixTime(tVal);
}

//-----------------------------------------------------------------------
// DPRAM commands
//-----------------------------------------------------------------------

void Tfp::setGeneratorTimeOffset(short offset, bool halfHour)
{
  DpramCmd cmd(DP_CMD_GEN_OFFSET);
  cmd.packVal((unsigned short)offset);
  cmd.packVal((unsigned char)halfHour);
  sendDpramCmd(cmd, true);
}

void Tfp::setLocalTimeOffset(short offset, bool halfHour)
{
  DpramCmd cmd(DP_CMD_LCL_OFFSET);
  cmd.packVal((unsigned short)offset);
  cmd.packVal((unsigned char)halfHour);
  sendDpramCmd(cmd, true);
}

void Tfp::setPropagationDelay(int delayIn100ns)
{
  DpramCmd cmd(DP_CMD_SET_PROP_DELAY);
  cmd.packVal((unsigned int)delayIn100ns);
  sendDpramCmd(cmd, true);
}

void Tfp::setClockSource(ClockSource src)
{
  DpramCmd cmd(DP_CMD_SET_CLK_SRC);
  cmd.packVal((unsigned char)src);
  sendDpramCmd(cmd, true);
}

void Tfp::setGpsFormat(GpsFormat format)
{
  DpramCmd cmd(DP_CMD_GPS_FMT);
  cmd.packVal((unsigned char)format);
  sendDpramCmd(cmd, true);
}

void Tfp::enableGpsModeFlag(bool enable)
{
  DpramCmd cmd(DP_CMD_SET_GPS_MODE_FLAG);
  cmd.packVal((unsigned char)enable);
  sendDpramCmd(cmd, true);
}

void Tfp::enableYearAutoIncrement(bool enable)
{
  DpramCmd cmd(DP_CMD_YEAR_AUTO_INC);
  cmd.packVal((unsigned char)enable);
  sendDpramCmd(cmd, true);
}
