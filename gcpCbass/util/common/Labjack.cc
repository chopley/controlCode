#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string>

#include "gcp/control/code/unix/libunix_src/common/tcpip.h"

#include "gcp/util/common/Angle.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/util/common/Labjack.h"
#include "gcp/util/common/LabjackU3.h"
#include <fcntl.h>

# define USB_TIMEOUT_USEC 200000
# define SELECT_TIMEOUT_USEC 200000
//# define DEF_TERM_USB_PORT "/dev/tty.usbserial-DPRUKU0H"
//# define DEF_TERM_USB_PORT "/dev/ttyUSB1"
//# define DEF_TERM_USB_PORT "/dev/ttyLabjack"

// someting of note: the device works best if not queried
// continuously.  give it a 200 ms break between queries and it's
// great.

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor 
 */
Labjack::Labjack()
{
  fd_          = -1;
  connected_   = false;
}

/**.......................................................................
 * Destructor
 */
Labjack::~Labjack()
{
  // Disconnect from the port
  disconnect();
}

/**.......................................................................
 * Connect to the port
 */
bool Labjack::connect(int serialNumber)
{
   int localID ; //number to define the labjack instance

  // Return immediately if we are already connected.

   //  printf("Connecting to the labjack\n");
  if(connected_)
    return true;
  
  uint8 buffer[38];  //send size of 26, receive size of 38
  uint16 checksumTotal = 0;
  uint32 numDevices = 0;
  uint32 dev;
  int i, serial;
  bool retVal;
  connected_ = false;
  retVal = false;
  hDevice_ = 0;
  localID = serialNumber;
  numDevices = LJUSB_GetDevCount(U3_PRODUCT_ID);
  if( numDevices == 0 ){
    printf("Open error: No U3 devices could be found\n");
    return retVal;
  }
  
  for( dev = 1;  dev <= numDevices; dev++ ){
    hDevice_ = LJUSB_OpenDevice(dev, 0, U3_PRODUCT_ID);
    
    if( hDevice_ != NULL ){
      if( localID < 0 ){
	printf("hDevice = %d\n",hDevice_);
      } else {
	checksumTotal = 0;
	
	//Setting up a ConfigU3 command
	buffer[1] = (uint8)(0xF8);
	buffer[2] = (uint8)(0x0A);
	buffer[3] = (uint8)(0x08);
	
	for( i = 6; i < 38; i++ )
	  buffer[i] = (uint8)(0x00);
	
	extendedChecksum(buffer, 26);
	
	if( LJUSB_Write(hDevice_, buffer, 26) != 26 )
	  COUT("USB write not writing correct number of bytes");
	
	if( LJUSB_Read(hDevice_, buffer, 38) != 38 )
	  COUT("USB read not getting right number of bytes");
	
	checksumTotal = extendedChecksum16(buffer, 38);
	if( ( (uint8)((checksumTotal / 256) & 0xFF) != buffer[5] ) | ( (uint8)(checksumTotal & 0xFF) != buffer[4] ))
	  COUT("Checksum Error");
	
	if( buffer[1] != (uint8)(0xF8) || buffer[2] != (uint8)(0x10) ||
	    buffer[3] != (uint8)(0x08) )
	  COUT("Buffer check error");
	
	if( buffer[6] != 0 )
	  COUT("buffer check error 2");
	
	//Check serial number
	serial = (int)(buffer[15] + buffer[16]*256 + buffer[17]*65536 +
		       buffer[18]*16777216);
	//	COUT("THIS SERIAL: " << serial);
	if( serial == localID ){ 
	  hDevice_  = hDevice_;
	  COUT("Found our device");
	  //now we get the calibration information that we watn
	  getCalibrationInfo();
	  connected_ = true;
	  retVal = true;
	  return retVal;
	  break;
	}  else {
	  //No matches, not our device
	  try{
		LJUSB_CloseDevice(hDevice_);
	  }
	  catch(...){
	  COUT("Failed to close device");
		}
	  connected_ = false;
	  hDevice_ = 0;
	}
      } //else localID >= 0 end
    } //if hDevice != NULL end
  } //for end
  
  //  printf("Open error: could not find a U3 with a local ID or serial number of %d\n", localID);
  return connected_;
}

/**.......................................................................
 * Disconnect from port
 */
void Labjack::disconnect()
{
  // Before we are connected, the fd will be initialized to -1
  CTOUT("Disconnecting the labjack device");
  if(connected_ ==true){
    LJUSB_CloseDevice(hDevice_);
    connected_ = false;
    hDevice_ = 0;
  }
}

int Labjack::configAllIO(){
//declare a temporary handle to use in this
	HANDLE hDevice;
//assign the handle stored in the structure to this temporary handle
	hDevice = hDevice_;
    uint8 sendBuff[12], recBuff[12];
    uint16 checksumTotal, FIOEIOAnalog;
    int sendChars, recChars;
    numChannels_=12;
    quickSample_=1;
    longSettling_=0;
    uint8 numChannels=numChannels_;

    sendBuff[1] = (uint8)(0xF8);  //Command byte
    sendBuff[2] = (uint8)(0x03);  //Number of data words
    sendBuff[3] = (uint8)(0x0B);  //Extended command number

    sendBuff[6] = 13;  //Writemask : Setting writemask for TimerCounterConfig (bit 0),
                       //            FIOAnalog (bit 2) and EIOAnalog (bit 3)

    sendBuff[7] = 0;  //Reserved
    sendBuff[8] = 64;  //TimerCounterConfig: disable timer and counter, set
                       //                    TimerCounterPinOffset to 4 (bits 4-7)
    sendBuff[9] = 0;  //DAC1 enable : not enabling, though could already be enabled

    sendBuff[10] = 0;

    FIOEIOAnalog = pow(2.0, numChannels)-1;
    sendBuff[10] = (FIOEIOAnalog & 0xFF);  //FIOAnalog
    sendBuff[11] = FIOEIOAnalog / 256;  //EIOAnalog
    extendedChecksum(sendBuff, 12);

//Sending command to U3
    if( (sendChars = LJUSB_Write(hDevice, sendBuff, 12)) < 12 )
    {
        if( sendChars == 0 )
            printf("ConfigIO error : write failed\n");
        else
            printf("ConfigIO error : did not write all of the buffer\n");
        return -1;
    }

    //Reading response from U3
    if( (recChars = LJUSB_Read(hDevice, recBuff, 12)) < 12 )
    {
        if( recChars == 0 )
            printf("ConfigIO error : read failed\n");
        else
            printf("ConfigIO error : did not read all of the buffer\n");
        return -1;
    }

    checksumTotal = extendedChecksum16(recBuff, 12);
    if( (uint8)((checksumTotal / 256) & 0xFF) != recBuff[5] )
    {
        printf("ConfigIO error : read buffer has bad checksum16(MSB)\n");
        return -1;
    }

    if( (uint8)(checksumTotal & 0xFF) != recBuff[4] )
    {
        printf("ConfigIO error : read buffer has bad checksum16(LBS)\n");
        return -1;
    }
 if( extendedChecksum8(recBuff) != recBuff[0] )
    {
        printf("ConfigIO error : read buffer has bad checksum8\n");
        return -1;
    }

    if( recBuff[1] != (uint8)(0xF8) || recBuff[2] != (uint8)(0x03) || recBuff[3] != (uint8)(0x0B) )
    {
        printf("ConfigIO error : read buffer has wrong command bytes\n");
        return -1;
    }

    if( recBuff[6] != 0 )
    {
        printf("ConfigIO error : read buffer received errorcode %d\n", recBuff[6]);
        return -1;
    }

    if( recBuff[10] != (FIOEIOAnalog&(0xFF)) && recBuff[10] != ((FIOEIOAnalog&(0xFF))|(0x0F)) )
    {
        printf("ConfigIO error : FIOAnalog did not set correctly");
        return -1;
    }
 if( recBuff[11] != FIOEIOAnalog/256 )
    {
        printf("ConfigIO error : EIOAnalog did not set correctly");
        return -1;
    }

    //*isDAC1Enabled = (int)recBuff[9];

    return 0;


}

std::vector<float> Labjack::queryAllVoltages()
{
//function to read in the data from the labjack

    u3CalibrationInfo* caliInfo;
//declare a temporary handle to use in this
	HANDLE hDevice;
//assign the handle stored in the structure to this temporary handle
	hDevice = hDevice_;
   //set the pointer to the object in the current structure
    caliInfo = &caliInfo_;
   std::vector<float> voltages(12);
    uint8 *sendBuff, *recBuff;
    uint16 checksumTotal, ainBytes;
    int sendChars, recChars, sendSize, recSize;
    int valueDIPort, ret, i, j;
    double valueAIN[16];
    long time, numIterations;
    double hardwareVersion;
    uint8 numChannels=numChannels_;
    uint8 quickSample=quickSample_;
    uint8 longSettling=longSettling_;
   int dacEnabled;
    hardwareVersion = caliInfo->hardwareVersion;

    for( i = 0; i < 16; i++ ){
        valueAIN[i] = 9999;
}
    valueDIPort = 0;
    numIterations = 100;  //Number of iterations (how many times Feedback will
                           //be called)

    //Setting up a Feedback command that will set CIO0-3 as input
    sendBuff = (uint8 *)malloc(14*sizeof(uint8));  //Creating an array of size 14
    recBuff = (uint8 *)malloc(10*sizeof(uint8));  //Creating an array of size 10

    sendBuff[1] = (uint8)(0xF8);  //Command byte
    sendBuff[2] = 4;  //Number of data words (.5 word for echo, 3.5 words for
                      //                      IOTypes and data)
    sendBuff[3] = (uint8)(0x00);  //Extended command number

    sendBuff[6] = 0;  //Echo
    sendBuff[7] = 29;  //IOType is PortDirWrite
    sendBuff[8] = 0;  //Writemask (for FIO)
    sendBuff[9] = 0;  //Writemask (for EIO)
 sendBuff[10] = 15;  //Writemask (for CIO)
    sendBuff[11] = 0;  //Direction (for FIO)
    sendBuff[12] = 0;  //Direction (for EIO)
    sendBuff[13] = 0;  //Direction (for CIO)
    extendedChecksum(sendBuff, 14);

    //Sending command to U3
    if( (sendChars = LJUSB_Write(hDevice, sendBuff, 14)) < 14 )
    {
        if( sendChars == 0 )
            printf("Feedback (CIO input) error : write failed\n");
        else
            printf("Feedback (CIO input) error : did not write all of the buffer\n");
        ret = -1;
	printf("here %d",i);
        goto cleanmem;
    }

    //Reading response from U3
    if( (recChars = LJUSB_Read(hDevice, recBuff, 10)) < 10 )
    {
        if( recChars == 0 )
        {
            printf("Feedback (CIO input) error : read failed\n");
            ret = -1;
            goto cleanmem;
        }
        else
            printf("Feedback (CIO input) error : did not read all of the buffer\n");
    }

    checksumTotal = extendedChecksum16(recBuff, 10);
    if( (uint8)((checksumTotal / 256) & 0xFF) != recBuff[5] )
    {
        printf("Feedback (CIO input) error : read buffer has bad checksum16(MSB)\n");
        ret = -1;
        goto cleanmem;
    }
  if( (uint8)(checksumTotal & 0xFF) != recBuff[4] )
    {
        printf("Feedback (CIO input) error : read buffer has bad checksum16(LBS)\n");
        ret = -1;
        goto cleanmem;
    }

    if( extendedChecksum8(recBuff) != recBuff[0] )
    {
        printf("Feedback (CIO input) error : read buffer has bad checksum8\n");
        ret = -1;
        goto cleanmem;
    }

    if( recBuff[1] != (uint8)(0xF8) || recBuff[3] != (uint8)(0x00) )
    {
        printf("Feedback (CIO input) error : read buffer has wrong command bytes \n");
        ret = -1;
        goto cleanmem;
    }

    if( recBuff[6] != 0 )
    {
        printf("Feedback (CIO input) error : received errorcode %d for frame %d in Feedback response. \n", recBuff[6], recBuff[7]);
        ret = -1;
        goto cleanmem;
    }

    free(sendBuff);
    free(recBuff);
 //Setting up Feedback command that will run 1000 times
    if( ((sendSize = 7+2+1+numChannels*3) % 2) != 0 )
        sendSize++;
    //Creating an array of size sendSize
    sendBuff = (uint8 *)malloc(sendSize*sizeof(uint8));

    if( ((recSize = 9+3+numChannels*2) % 2) != 0 )
        recSize++;
    //Creating an array of size recSize
    recBuff = (uint8 *)malloc(recSize*sizeof(uint8));

    sendBuff[1] = (uint8)(0xF8);  //Command byte
    sendBuff[2] = (sendSize - 6)/2;  //Number of data words 
    sendBuff[3] = (uint8)(0x00);  //Extended command number

    sendBuff[6] = 0;  //Echo

    //Setting DAC0 with 2.5 volt output
    sendBuff[7] = 34;  //IOType is DAC0

    //Value is 2.5 volts (in binary)
    getDacBinVoltCalibrated(caliInfo, 0, 2.5, &sendBuff[8]);

    sendBuff[9] = 26;    //IOType is PortStateRead

 //Setting AIN read commands
    for( j = 0; j < numChannels; j++ )
    {
        sendBuff[10 + j*3] = 1;  //IOType is AIN

        //Positive Channel (bits 0 - 4), LongSettling (bit 6) and QuickSample (bit 7)
        sendBuff[11 + j*3] = j + (longSettling&(0x01))*64 + (quickSample&(0x01))*128;
        sendBuff[12 + j*3] = 31;  //Negative Channel is single-ended
    }

    if( (sendSize % 2) != 0 )
        sendBuff[sendSize - 1] = 0;

    extendedChecksum(sendBuff, sendSize);

    time = getTickCount();
    //	COUT("time "<<time);

    for( i = 0; i < numIterations; i++ )
    {
        //Sending command to U3
        if( (sendChars = LJUSB_Write(hDevice, sendBuff, sendSize)) < sendSize )
        {
            if( sendChars == 0 )
                printf("Feedback error (Iteration %d): write failed\n", i);
            else
                printf("Feedback error (Iteration %d): did not write all of the buffer\n", i);
            ret = -1;
            goto cleanmem;
        }
    //Reading response from U3
        if( (recChars = LJUSB_Read(hDevice, recBuff, recSize)) < recSize )
        {
            if( recChars == 0 )
            {
                printf("Feedback error (Iteration %d): read failed\n", i);
                ret = -1;
                goto cleanmem;
            }
            else
                printf("Feedback error (Iteration %d): did not read all of the expected buffer\n", i);
        }

        checksumTotal = extendedChecksum16(recBuff, recChars);
//	COUT("check "<<checksumTotal);
        if( (uint8)((checksumTotal / 256) & 0xFF) != recBuff[5] )
        {
            printf("Feedback error (Iteration %d): read buffer has bad checksum16(MSB)\n", i);
            ret = -1;
            goto cleanmem;
        }

        if( (uint8)(checksumTotal & 0xFF) != recBuff[4] )
        {
            printf("Feedback error (Iteration %d): read buffer has bad checksum16(LBS)\n", i);
            ret = -1;
            goto cleanmem;
}
    if( extendedChecksum8(recBuff) != recBuff[0] )
        {
            printf("Feedback error (Iteration %d): read buffer has bad checksum8\n", i);
            ret = -1;
            goto cleanmem;
        }

        if( recBuff[1] != (uint8)(0xF8) || recBuff[3] != (uint8)(0x00) )
        {
            printf("Feedback error (Iteration %d): read buffer has wrong command bytes \n", i);
            ret = -1;
            goto cleanmem;
        }

        if( recBuff[6] != 0 )
        {
            printf("Feedback error (Iteration %d): received errorcode %d for frame %d in Feedback response. \n", i, recBuff[6], recBuff[7]);
            ret = -1;
            goto cleanmem;
        }

        if( recChars != recSize )
        {
            ret = -1;
            goto cleanmem;
        }

        //Getting CIO digital states
        valueDIPort = recBuff[11];

//	    printf("here \n");
        //Getting AIN voltages
        for( j = 0; j < numChannels ; j++ )
        {
//	    COUT("here "<<j);
            ainBytes = recBuff[12+j*2] + recBuff[13+j*2]*256;
            if( hardwareVersion >= 1.30 )
                getAinVoltCalibrated_hw130(caliInfo, j, 31, ainBytes, &valueAIN[j]);
	 else
                getAinVoltCalibrated(caliInfo, &dacEnabled, 31, ainBytes, &valueAIN[j]);
        }
    }

    time = getTickCount() - time;
  //  printf("Milliseconds per iteration = %.3f\n", (double)time / (double)numIterations);
   // printf("\nDigital Input = %d\n", valueDIPort);
   // printf("\nAIN readings from last iteration:\n");

    for( j = 0; j < numChannels; j++ ){
       // printf("%.3f\n", valueAIN[j]);
	voltages[j]=valueAIN[j];
	}

cleanmem:
    free(sendBuff);
    free(recBuff);
    sendBuff = NULL;
    recBuff = NULL;

    return voltages;




}

int Labjack::AllIO(){
//function to read in the data from the labjack

    u3CalibrationInfo* caliInfo;
//declare a temporary handle to use in this
	HANDLE hDevice;
//assign the handle stored in the structure to this temporary handle
	hDevice = hDevice_;
   //set the pointer to the object in the current structure
    caliInfo = &caliInfo_;
    uint8 *sendBuff, *recBuff;
    uint16 checksumTotal, ainBytes;
    int sendChars, recChars, sendSize, recSize;
    int valueDIPort, ret, i, j;
    double valueAIN[16];
    long time, numIterations;
    double hardwareVersion;
    uint8 numChannels=numChannels_;
    uint8 quickSample=quickSample_;
    uint8 longSettling=longSettling_;
   int dacEnabled;
    hardwareVersion = caliInfo->hardwareVersion;

    for( i = 0; i < 16; i++ ){
        valueAIN[i] = 9999;
}
    valueDIPort = 0;
    numIterations = 100;  //Number of iterations (how many times Feedback will
                           //be called)

    //Setting up a Feedback command that will set CIO0-3 as input
    sendBuff = (uint8 *)malloc(14*sizeof(uint8));  //Creating an array of size 14
    recBuff = (uint8 *)malloc(10*sizeof(uint8));  //Creating an array of size 10

    sendBuff[1] = (uint8)(0xF8);  //Command byte
    sendBuff[2] = 4;  //Number of data words (.5 word for echo, 3.5 words for
                      //                      IOTypes and data)
    sendBuff[3] = (uint8)(0x00);  //Extended command number

    sendBuff[6] = 0;  //Echo
    sendBuff[7] = 29;  //IOType is PortDirWrite
    sendBuff[8] = 0;  //Writemask (for FIO)
    sendBuff[9] = 0;  //Writemask (for EIO)
 sendBuff[10] = 15;  //Writemask (for CIO)
    sendBuff[11] = 0;  //Direction (for FIO)
    sendBuff[12] = 0;  //Direction (for EIO)
    sendBuff[13] = 0;  //Direction (for CIO)
    extendedChecksum(sendBuff, 14);

    //Sending command to U3
    if( (sendChars = LJUSB_Write(hDevice, sendBuff, 14)) < 14 )
    {
        if( sendChars == 0 )
            printf("Feedback (CIO input) error : write failed\n");
        else
            printf("Feedback (CIO input) error : did not write all of the buffer\n");
        ret = -1;
	printf("here %d",i);
        goto cleanmem;
    }

    //Reading response from U3
    if( (recChars = LJUSB_Read(hDevice, recBuff, 10)) < 10 )
    {
        if( recChars == 0 )
        {
            printf("Feedback (CIO input) error : read failed\n");
            ret = -1;
            goto cleanmem;
        }
        else
            printf("Feedback (CIO input) error : did not read all of the buffer\n");
    }

    checksumTotal = extendedChecksum16(recBuff, 10);
    if( (uint8)((checksumTotal / 256) & 0xFF) != recBuff[5] )
    {
        printf("Feedback (CIO input) error : read buffer has bad checksum16(MSB)\n");
        ret = -1;
        goto cleanmem;
    }
  if( (uint8)(checksumTotal & 0xFF) != recBuff[4] )
    {
        printf("Feedback (CIO input) error : read buffer has bad checksum16(LBS)\n");
        ret = -1;
        goto cleanmem;
    }

    if( extendedChecksum8(recBuff) != recBuff[0] )
    {
        printf("Feedback (CIO input) error : read buffer has bad checksum8\n");
        ret = -1;
        goto cleanmem;
    }

    if( recBuff[1] != (uint8)(0xF8) || recBuff[3] != (uint8)(0x00) )
    {
        printf("Feedback (CIO input) error : read buffer has wrong command bytes \n");
        ret = -1;
        goto cleanmem;
    }

    if( recBuff[6] != 0 )
    {
        printf("Feedback (CIO input) error : received errorcode %d for frame %d in Feedback response. \n", recBuff[6], recBuff[7]);
        ret = -1;
        goto cleanmem;
    }

    free(sendBuff);
    free(recBuff);
 //Setting up Feedback command that will run 1000 times
    if( ((sendSize = 7+2+1+numChannels*3) % 2) != 0 )
        sendSize++;
    //Creating an array of size sendSize
    sendBuff = (uint8 *)malloc(sendSize*sizeof(uint8));

    if( ((recSize = 9+3+numChannels*2) % 2) != 0 )
        recSize++;
    //Creating an array of size recSize
    recBuff = (uint8 *)malloc(recSize*sizeof(uint8));

    sendBuff[1] = (uint8)(0xF8);  //Command byte
    sendBuff[2] = (sendSize - 6)/2;  //Number of data words 
    sendBuff[3] = (uint8)(0x00);  //Extended command number

    sendBuff[6] = 0;  //Echo

    //Setting DAC0 with 2.5 volt output
    sendBuff[7] = 34;  //IOType is DAC0

    //Value is 2.5 volts (in binary)
    getDacBinVoltCalibrated(caliInfo, 0, 2.5, &sendBuff[8]);

    sendBuff[9] = 26;    //IOType is PortStateRead

 //Setting AIN read commands
    for( j = 0; j < numChannels; j++ )
    {
        sendBuff[10 + j*3] = 1;  //IOType is AIN

        //Positive Channel (bits 0 - 4), LongSettling (bit 6) and QuickSample (bit 7)
        sendBuff[11 + j*3] = j + (longSettling&(0x01))*64 + (quickSample&(0x01))*128;
        sendBuff[12 + j*3] = 31;  //Negative Channel is single-ended
    }

    if( (sendSize % 2) != 0 )
        sendBuff[sendSize - 1] = 0;

    extendedChecksum(sendBuff, sendSize);

    time = getTickCount();
	COUT("time "<<time);

    for( i = 0; i < numIterations; i++ )
    {
        //Sending command to U3
        if( (sendChars = LJUSB_Write(hDevice, sendBuff, sendSize)) < sendSize )
        {
            if( sendChars == 0 )
                printf("Feedback error (Iteration %d): write failed\n", i);
            else
                printf("Feedback error (Iteration %d): did not write all of the buffer\n", i);
            ret = -1;
            goto cleanmem;
        }
    //Reading response from U3
        if( (recChars = LJUSB_Read(hDevice, recBuff, recSize)) < recSize )
        {
            if( recChars == 0 )
            {
                printf("Feedback error (Iteration %d): read failed\n", i);
                ret = -1;
                goto cleanmem;
            }
            else
                printf("Feedback error (Iteration %d): did not read all of the expected buffer\n", i);
        }

        checksumTotal = extendedChecksum16(recBuff, recChars);
//	COUT("check "<<checksumTotal);
        if( (uint8)((checksumTotal / 256) & 0xFF) != recBuff[5] )
        {
            printf("Feedback error (Iteration %d): read buffer has bad checksum16(MSB)\n", i);
            ret = -1;
            goto cleanmem;
        }

        if( (uint8)(checksumTotal & 0xFF) != recBuff[4] )
        {
            printf("Feedback error (Iteration %d): read buffer has bad checksum16(LBS)\n", i);
            ret = -1;
            goto cleanmem;
}
    if( extendedChecksum8(recBuff) != recBuff[0] )
        {
            printf("Feedback error (Iteration %d): read buffer has bad checksum8\n", i);
            ret = -1;
            goto cleanmem;
        }

        if( recBuff[1] != (uint8)(0xF8) || recBuff[3] != (uint8)(0x00) )
        {
            printf("Feedback error (Iteration %d): read buffer has wrong command bytes \n", i);
            ret = -1;
            goto cleanmem;
        }

        if( recBuff[6] != 0 )
        {
            printf("Feedback error (Iteration %d): received errorcode %d for frame %d in Feedback response. \n", i, recBuff[6], recBuff[7]);
            ret = -1;
            goto cleanmem;
        }

        if( recChars != recSize )
        {
            ret = -1;
            goto cleanmem;
        }

        //Getting CIO digital states
        valueDIPort = recBuff[11];

//	    printf("here \n");
        //Getting AIN voltages
        for( j = 0; j < numChannels ; j++ )
        {
//	    COUT("here "<<j);
            ainBytes = recBuff[12+j*2] + recBuff[13+j*2]*256;
            if( hardwareVersion >= 1.30 )
                getAinVoltCalibrated_hw130(caliInfo, j, 31, ainBytes, &valueAIN[j]);
	 else
                getAinVoltCalibrated(caliInfo, &dacEnabled, 31, ainBytes, &valueAIN[j]);
        }
    }

    time = getTickCount() - time;
    printf("Milliseconds per iteration = %.3f\n", (double)time / (double)numIterations);
    printf("\nDigital Input = %d\n", valueDIPort);
    printf("\nAIN readings from last iteration:\n");

    for( j = 0; j < numChannels; j++ ){
        printf("%.3f\n", valueAIN[j]);
	}

cleanmem:
    free(sendBuff);
    free(recBuff);
    sendBuff = NULL;
    recBuff = NULL;

    return ret;




}


long Labjack::getCalibrationInfo()
{	//pointer to the calibration information object
    u3CalibrationInfo* caliInfo;
   //set the pointer to the object in the current structure
    caliInfo = &caliInfo_;
    uint8 sendBuffer[8], recBuffer[40];
    uint8 cU3SendBuffer[26], cU3RecBuffer[38];
    int sentRec = 0, offset = 0, i = 0;

    //    COUT("Getting the Calibration information for the Labjack.\n");
    /* Sending ConfigU3 command to get hardware version and see if HV */
    cU3SendBuffer[1] = (uint8)(0xF8);  //Command byte
    cU3SendBuffer[2] = (uint8)(0x0A);  //Number of data words
    cU3SendBuffer[3] = (uint8)(0x08);  //Extended command number

    //Setting WriteMask0 and all other bytes to 0 since we only want to read the
    //response
    for( i = 6; i < 26; i++ )
        cU3SendBuffer[i] = 0;

    extendedChecksum(cU3SendBuffer, 26);

    sentRec = LJUSB_Write(hDevice_, cU3SendBuffer, 26);
    if( sentRec < 26 )
    {
        if( sentRec == 0 )
            goto writeError0;
        else
            goto writeError1;
    }

    sentRec = LJUSB_Read(hDevice_, cU3RecBuffer, 38);
    if( sentRec < 38 )
    {
        if( sentRec == 0 )
            goto readError0;
        else
            goto readError1;
    }

    if( cU3RecBuffer[1] != (uint8)(0xF8) || cU3RecBuffer[2] != (uint8)(0x10) ||
        cU3RecBuffer[3] != (uint8)(0x08))
        goto commandByteError;

    caliInfo->hardwareVersion = cU3RecBuffer[14] + cU3RecBuffer[13]/100.0;
    if( (cU3RecBuffer[37] & 18) == 18 )
        caliInfo->highVoltage = 1;
    else
        caliInfo->highVoltage = 0;

    for( i = 0; i < 5; i++ )
    {
        /* Reading block i from memory */
        sendBuffer[1] = (uint8)(0xF8);  //Command byte
        sendBuffer[2] = (uint8)(0x01);  //Cumber of data words
        sendBuffer[3] = (uint8)(0x2D);  //Extended command number
        sendBuffer[6] = 0;
        sendBuffer[7] = (uint8)i;  //Blocknum = i
        extendedChecksum(sendBuffer, 8);

        sentRec = LJUSB_Write(hDevice_, sendBuffer, 8);
        if( sentRec < 8 )
        {
            if( sentRec == 0 )
                goto writeError0;
            else
                goto writeError1;
        }

        sentRec = LJUSB_Read(hDevice_, recBuffer, 40);
        if( sentRec < 40 )
        {
            if( sentRec == 0 )
                goto readError0;
            else
                goto readError1;
        }

        if( recBuffer[1] != (uint8)(0xF8) || recBuffer[2] != (uint8)(0x11) ||
            recBuffer[3] != (uint8)(0x2D) )
            goto commandByteError;

        offset = i * 4;

        //Block data starts on byte 8 of the buffer
        caliInfo->ccConstants[offset] = FPuint8ArrayToFPDouble(recBuffer + 8, 0);
        caliInfo->ccConstants[offset + 1] = FPuint8ArrayToFPDouble(recBuffer + 8, 8);
        caliInfo->ccConstants[offset + 2] = FPuint8ArrayToFPDouble(recBuffer + 8, 16);
        caliInfo->ccConstants[offset + 3] = FPuint8ArrayToFPDouble(recBuffer + 8, 24);
    }

    caliInfo->prodID = 3;

    return 0;

writeError0:
    printf("Error : getCalibrationInfo write failed\n");
    return -1;
writeError1:
    printf("Error : getCalibrationInfo did not write all of the buffer\n");
    return -1;
readError0:
    printf("Error : getCalibrationInfo read failed\n");
    return -1;
readError1:
    printf("Error : getCalibrationInfo did not read all of the buffer\n");
    return -1;
commandByteError:
    printf("Error : getCalibrationInfo received wrong command bytes for ReadMem\n");
    return -1;

}



/**.......................................................................
 * Return true if we are connected to the usb device.
 */
bool Labjack::isConnected()
{
  return connected_;
}


/**.......................................................................
 * write a message to the port
 */
int Labjack::writeString(std::string message)
{
  int bytesWritten = 0;
  int len = message.size();

  //  CTOUT("message to send is: " << message << " length:" << len );


  if(fd_>0){
    bytesWritten = write(fd_, (const char*)message.c_str(), len);
  };

  //  CTOUT("bytes written: " << bytesWritten);

  return bytesWritten;
}


/**.......................................................................
 * Read bytes from the serial port and save the response
 */
int Labjack::readPort(LabjackMsg& msg)
{
  if(!isConnected() || fd_ < 0)
    return 0;

  int i,nbyte,waserr=0,nread=0;
  unsigned char line[DATA_MAX_LEN], *lptr=NULL;
  int ioctl_state=0;
  bool stopLoop = 0;
  TimeVal start, stop, diff;

  // Start checking how long this is taking.

  start.setToCurrentTime();

  // Set the line pointer pointing to the head of the line

  lptr = line;

  // See how many bytes are waiting to be read

  do {
    waitForResponse();
    ioctl_state=ioctl(fd_, FIONREAD, &nbyte);

#ifdef linux_i486_gcc
    waserr |= ioctl_state != 0;
#else
    waserr |= ioctl_state < 0;
#endif

    if(waserr) {
      ThrowError("IO control device Error");
    };

    /*
     * If a non-zero number of bytes was found, read them
     *
     if(nbyte > 0) {

     /*
     * Read the bytes one at a time
     */
    for(i=0;i < nbyte;i++) {
      if(read(fd_, lptr, 1) < 0) {
        COUT("readPort: Read error.\n");
        return 0;
      };

      if(*lptr != '\r' && nread < DATA_MAX_LEN) {
        lptr++;
        nread++;
      } else {
	// check if there are any more bytes on the port.  if there
	// are, discard the previous value and re-read
	ioctl_state=ioctl(fd_, FIONREAD, &nbyte);
	if (nbyte==0) {
	  stopLoop = 1;
	} else {
	  COUT("bytes still on buffer.  re-read");
	  stopLoop = 0;
	  lptr = line;
	  nread = 0;
	};
      };

    };

    /*
     * Check how long we've taken so far
     */
    stop.setToCurrentTime();
    diff = stop - start;
    
    //    COUT("time difference:  " << diff.getTimeInMicroSeconds());
    /*
     * If we're taking too long, exit the program
     */
    if(diff.getTimeInMicroSeconds() > USB_TIMEOUT_USEC){
      stopLoop = 1;
      return 2;
    };
  } while(stopLoop==0);

  /*
   * NULL terminate the line and print to stdout
   */
  *(lptr++) = '\0';

  /*
   * Forward the line to connected clients
   */
  strcpy(msg.responseReceived_, (char*) line);

  return 1;
}

/**.......................................................................
 * Read bytes from the serial port and save the response
 */
int Labjack::readPort(LabjackMsg& msg, bool withQ)
{
  if(!isConnected() || fd_ < 0)
    return 0;

  int i,nbyte,waserr=0,nread=0;
  unsigned char line[DATA_MAX_LEN], *lptr=NULL;
  int ioctl_state=0;
  bool stopLoop = 0;
  TimeVal start, stop, diff;

  // Start checking how long this is taking.

  start.setToCurrentTime();

  // Set the line pointer pointing to the head of the line

  lptr = line;

  // See how many bytes are waiting to be read

  do {
    waitForResponse();
    ioctl_state=ioctl(fd_, FIONREAD, &nbyte);

#ifdef linux_i486_gcc
    waserr |= ioctl_state != 0;
#else
    waserr |= ioctl_state < 0;
#endif

    if(waserr) {
      ThrowError("IO control device Error");
    };

    /*
     * If a non-zero number of bytes was found, read them
     *
     if(nbyte > 0) {

     /*
     * Read the bytes one at a time
     */
    for(i=0;i < nbyte;i++) {
      if(read(fd_, lptr, 1) < 0) {
        COUT("readPort: Read error.\n");
        return 0;
      };
      #if(0)
      // for debugging purposes, print out what we get
      print_bits(*lptr);

      if(isalnum(*lptr) || isprint(*lptr)) {
	COUT(*lptr);
      } else if(*lptr == '\r') {
	COUT("CR");
	//	lprintf(term, true ,stdout, " CR\n");
      } else if(*lptr == '\n') {
	COUT("\n");
	//	lprintf(term, true, stdout, " LF\n");
      } else if(*lptr == '\0') {
	COUT("NULL");
      } else {
	COUT(*lptr);
      }
      #endif

      if(*lptr != 'Q' && nread < DATA_MAX_LEN) {
        lptr++;
        nread++;
      } else {
	stopLoop = 1;
	};
    };

    /*
     * Check how long we've taken so far
     */
    stop.setToCurrentTime();
    diff = stop - start;
    
    //    COUT("time difference:  " << diff.getTimeInMicroSeconds());
    /*
     * If we're taking too long, exit the program
     */
    if(diff.getTimeInMicroSeconds() > USB_TIMEOUT_USEC && stopLoop==0){
      stopLoop = 1;
      return 2;
    };
  } while(stopLoop==0);

  /*
   * NULL terminate the line and print to stdout
   */
  *(lptr++) = '\0';

  /*
   * Forward the line to connected clients
   */
  strcpy(msg.responseReceived_, (char*) line);

  return 1;
}



/**.......................................................................
 * Wait for a response from the usb device
 */
void Labjack::waitForResponse()
{
  TimeVal timeout(0, SELECT_TIMEOUT_USEC, 0);

  // Do nothing if we are not connected to the servo.

  if(!isConnected())
    return;

  // Now wait in select until the fd becomes readable, or we time out

  int nready = select(fdSet_.size(), fdSet_.readFdSet(), NULL, fdSet_.exceptionFdSet(), timeout.timeVal());

  // If select generated an error, throw it
  if(nready < 0) {
    ThrowSysError("In select(): ");
  } else if(nready==0) {
    ThrowError("Timed out in select");
  } else if(fdSet_.isSetInException(fd_)) {
    ThrowError("Exception occurred on file descriptor");
  }
}

/**.......................................................................
 * Packs our request into an object of class LabjackMsg
 */
LabjackMsg Labjack::packCommand(LabjackMsg::Request req, int input)
{

  LabjackMsg msg;
  std::ostringstream os;

  msg.request_ = req;

  switch (req) {
  case LabjackMsg:: SET_DATA_TYPE:
    msg.expectsResponse_ = false;
    switch (input) {
    case 0:  // wants binary
      os << "\\r"; 
      break;
      
    default:  // wants ascii
      os << "`\r"; 
      break;
    };
    break;
    
  case LabjackMsg:: SET_UNITS:
    msg.expectsResponse_ = false;
    switch (input) {
    case 0:  // wants F
      os << "L\r"; 
      COUT("setting units: " << os.str());
      break;
      
    default:  // wants C
      os << ";\r"; 
      break;
    };
    break;

  case LabjackMsg:: QUERY_TEMP:
    msg.expectsResponse_ = true;
    switch (input) {
    case 1:
      os << "9\r"; 
      break;

    case 2:
      os << "0\r"; 
      break;

    case 3:
      os << "-\r"; 
      break;

    case 4:
      os << "=\r"; 
      break;

    case 5:
      os << "O\r"; 
      break;

    case 6:
      os << "P\r"; 
      break;

    case 7:
      os << "[\r"; 
      break;

    case 8:
      os << "]\r"; 
      break;

    default:
      os << "9\r"; 
      break;
    };
    break;

  case LabjackMsg:: QUERY_VOLTAGE:
    msg.expectsResponse_ = true;
    switch (input) {
    case 1:
      os << "Z\r"; 
      break;

    case 2:
      os << "X\r"; 
      break;

    case 3:
      os << "C\r"; 
      break;

    case 4:
      os << "V\r"; 
      break;

    case 5:
      os << "B\r"; 
      break;

    case 6:
      os << "N\r"; 
      break;

    case 7:
      os << "M\r"; 
      break;

    case 8:
      os << ",\r"; 
      break;

    default:
      os << "Z\r"; 
      break;
    };
    break;

  case LabjackMsg:: QUERY_ALL_TEMP:
    msg.expectsResponse_ = true;    
    os << "90-=OP[]'\n";
    break;

  case LabjackMsg:: QUERY_ALL_VOLT:
    msg.expectsResponse_ = true;    
    os << "ZXCVBNM,'";
    break;

  }
  msg.messageToSend_ = os.str();
  msg.cmdSize_ = msg.messageToSend_.size();
  
  return msg;
}

/**.......................................................................
 * Sends the command
 */
void Labjack::sendCommand(LabjackMsg& msg)
{
  // Don't try to send commands if we are not connected.

  if(!connected_)
    return;

  // Check that this is a valid command.

  if(msg.request_ == LabjackMsg::INVALID)
    ThrowError("Sending the Dlp Thermal Module an invalid command.");

  int status = writeString(msg.messageToSend_);
  
  // writeString should return the number of bytes requested to be sent.

  if(status != msg.cmdSize_) {
    ThrowSysError("In writeString()");
  }
}

/**.......................................................................
 * Sends the command, reads the response, and parses the response
 */
LabjackMsg Labjack::issueCommand(LabjackMsg::Request req, int input)
{
  // Set these up so that if we timeout, we try again.
  int numTimeOut = 0;
  int readStatus = 0;
  bool tryRead = 1;

  LabjackMsg msg;

  // First we pack the command
  msg = packCommand(req, input);


  // If the msg expects a response, we want to ensure we try a couple of times before timing out.
  if(!msg.expectsResponse_)
    return msg;

  while(tryRead) {
    try { 
      // Next we send the command
      sendCommand(msg);
      readStatus = readPort(msg);
    } catch(...) {
      // there was an error waiting for response
      readStatus = 2;
    };

    switch(readStatus) {
    case 1:
      // It read things fine.  Check that it's valid before exiting
      parseResponse(msg);
      tryRead = 0;
      break;
    case 2:
      // went through a time out.  re-issue commands
      numTimeOut++;
      break;

    case 0:
      // not communicating:
      numTimeOut++;
      break;
    };
    if(numTimeOut>5){
      // There's an error on the port.  throw the error
      ThrowError(" Dlp Usb Port timeout on response");
    };
  };

  // and return the msg with all our info in it
  return msg;
}

/**.......................................................................
 * Sends the command, reads the response, and parses the response
 */
LabjackMsg Labjack::issueCommand(LabjackMsg::Request req, int input, bool withQ)
{
  // Set these up so that if we timeout, we try again.
  int numTimeOut = 0;
  int readStatus = 0;
  bool tryRead = 1;

  LabjackMsg msg;

  // First we pack the command
  msg = packCommand(req, input);


  // If the msg expects a response, we want to ensure we try a couple of times before timing out.
  if(!msg.expectsResponse_)
    return msg;

  while(tryRead) {
    try { 
      // Next we send the command
      sendCommand(msg);
      readStatus = readPort(msg, withQ);
    } catch(...) {
      // there was an error waiting for response
      readStatus = 2;
    };

    switch(readStatus) {
    case 1:
      // It read things fine.  Check that it's valid before exiting
      parseResponse(msg, withQ);
      tryRead = 0;
      break;
    case 2:
      // went through a time out.  re-issue commands
      numTimeOut++;
      break;

    case 0:
      // not communicating:
      numTimeOut++;
      break;
    };
    if(numTimeOut>5){
      // There's an error on the port.  throw the error
      ThrowError(" Dlp Usb Port timeout on response");
    };
  };

  # if(0)
  // and return the msg with all our info in it
  COUT("msg: " << msg.responseReceived_);

  int i;
  for (i=0;i<NUM_DLP_TEMP_SENSORS; i++) {
    COUT("value at exit of parseResponse: " << msg.responseValueVec_[i]);
  };
  #endif
  return msg;
}

 
/**.......................................................................
 * Parse the response -- takes the string and turns it into a float.
 */
void Labjack::parseResponse(LabjackMsg& msg) 
{

  // The string response is responseReceived_;
  std::string respStr(msg.responseReceived_);
  String responseString(respStr);

  msg.responseValue_ = responseString.findNextStringSeparatedByChars("VCF").toFloat();

  return;
};


/**.......................................................................
 * Parse the response -- takes the string and turns it into a float.
 */
void Labjack::parseResponse(LabjackMsg& msg, bool withQ) 
{

  // The string response is responseReceived_;
  std::string respStr(msg.responseReceived_);
  String responseString(respStr);
  int i;
  
  for (i=0;i<NUM_DLP_TEMP_SENSORS; i++) {
    msg.responseValueVec_[i] = responseString.findNextStringSeparatedByChars("VCFQ ").toFloat();
  };
  return;
};



/**.......................................................................
 * Specific Command Sets
 */



