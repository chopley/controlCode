/*
 *  structUDPCBASSpkt.h
 *  CPacketize
 *  CJC 8/8/2011
 *  Based on header by Danny Price on 18/01/2011.
 *
 */
 
#define kDataperPacket 10
#define vectorLength 32 //size of FFT (32)

//data structure definition
struct UDPCBASSpkt {
  int version; // 4 byte
  int data_size; // 4  //Size of the structure in bytes
  int dataCount; //4 byte the number of frames in a packet i.e 10
  int int_count; // 4 byte //the integration counts
  int tstart[10]; // 40 byte
  int tend; // 4 byte
  int int_len; // 4 byte
  int reserved1; // 4 byte
  int reserved2; // 4 byte
  int data_ch0odd[kDataperPacket*vectorLength];//LL
  int data_ch0even[kDataperPacket*vectorLength];//LL
  int data_ch1odd[kDataperPacket*vectorLength]; //Q
  int data_ch1even[kDataperPacket*vectorLength];//Q
  int data_ch2odd[kDataperPacket*vectorLength]; //U
  int data_ch2even[kDataperPacket*vectorLength]; //U
  int data_ch3odd[kDataperPacket*vectorLength]; //RR
  int data_ch3even[kDataperPacket*vectorLength]; //RR
  int data_ch4odd[kDataperPacket*vectorLength];//Tl1
  int data_ch4even[kDataperPacket*vectorLength]; //Tl1
  int data_ch5odd[kDataperPacket*vectorLength]; //Tl2
  int data_ch5even[kDataperPacket*vectorLength]; //Tl2
  int data_switchstatus[kDataperPacket]; //Switch Status Noise diode etc
<<<<<<< HEAD
  int tsecond[10]; // 40 byte
=======
  int tsecond[10];
>>>>>>> a18a587ea24a82712a2d7c9c3e4290830b3ef55b
};

struct tempDataPacket{
	int data_ch0oddMSB[32];
	int data_ch1oddMSB[32];
	int data_ch2oddMSB[32];
	int data_ch3oddMSB[32];
	int data_ch4oddMSB[32];
	int data_ch5oddMSB[32];
	int data_ch0oddLSB[32];
	int data_ch1oddLSB[32];
	int data_ch2oddLSB[32];
	int data_ch3oddLSB[32];
	int data_ch4oddLSB[32];
	int data_ch5oddLSB[32];
	int data_ch0evenMSB[32];
	int data_ch1evenMSB[32];
	int data_ch2evenMSB[32];
	int data_ch3evenMSB[32];
	int data_ch4evenMSB[32];
	int data_ch5evenMSB[32];
	int data_ch0evenLSB[32];
	int data_ch1evenLSB[32];
	int data_ch2evenLSB[32];
	int data_ch3evenLSB[32];
	int data_ch4evenLSB[32];
	int data_ch5evenLSB[32];

	int shift;

};

//definitions of strings used by the parser
char getDataCommand[]= "GETDATAXXX";
char NoiseDiodeCommand[]= "NDIODEXXXX";
char PhaseShifterCommand[]= "PHASESHIFT";
char intensityShiftCommand[]= "INTSHIFTXX";
char polarisationShiftCommand[]= "POLSHIFTXX";
char GarbageReturn[]="GARBAGEXXX";
char changeModeCommand[]="CHANGEMODE";
char resetBufferCommand[]="RESETBUFFX";
char Temp[15];
///////////////////////////
