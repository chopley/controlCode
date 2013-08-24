//ROACH server code compile with cc tcpserver.c -o tcpserver -lpthread

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <byteswap.h>
#include <signal.h>
#include "structUDPCBASSpkt.h"
#include <setjmp.h>
#include <inttypes.h>
#define PORT 12345

#define TRY do{ jmp_buf ex_buf__; if( !setjmp(ex_buf__) ){
#define CATCH } else {
#define ETRY } }while(0)
#define THROW longjmp(ex_buf__, 1)


jmp_buf __exbuf;
int     __exvalue;

char *job[5];
struct UDPCBASSpkt *PKTptr;
struct UDPCBASSpkt cbassPKT;
struct circular_buffer buffer;
struct circular_buffer *bufferptr;
typedef struct circular_buffer
{
    void *buffer;     // data buffer
    void *buffer_end; // end of data buffer
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t sz;        // size of each item in the buffer
    void *head;       // pointer to head
    void *head10;	//pointer to 10 places before the head pointer
    void *tail;       // pointer to tail
}circular_buffer;

void cleanup();

void sigint_handler(int);

int gainCorrectionCalc(struct tempDataPacket *pack);


void sigint_handler(int dummy) {
	int i;
	for(i=0;i<20;i++){
    		printf("Received Ctrl-C\n");
		sleep(0.1);
	}
}

void cb_init(circular_buffer *cb, size_t capacity, size_t sz){
    //initiliase the circular buffer
    cb->buffer = malloc(capacity * sz);
    //printf("adding item %d \n",cb->buffer);
    //if(cb->buffer == NULL)
        // handle error
    cb->buffer_end = (char *)(cb->buffer + capacity * sz-1);
    cb->capacity = capacity;
    cb->count = 0;
    cb->sz = sz;
    cb->head = cb->buffer;
    //printf("head ptr init %d \n",cb->head);
    cb->head10=(char*)cb->buffer_end-10*cb->sz;//the pointer to the data stored from 10 packets ago
    cb->tail = cb->buffer;
    //printf("tail ptr init %d \n",cb->tail);
}

void cb_free(circular_buffer *cb)
{
    free(cb->buffer);
    // clear out other fields too, just to be safe
}

void cb_push_back(circular_buffer *cb, const void *item)
{//function to put data onto the ring buffer
    if(cb->head10 == cb->buffer_end){
        cb->head10 = cb->buffer;
    	//printf("resetting the head10pointer\n");
	}
    if(cb->head >= cb->buffer_end){
    	//printf("resetting the headpointer %d %d\n",cb->head,cb->buffer_end);
        cb->head = cb->buffer;
	}
    if(cb->count >= cb->capacity)
    {	printf("Ring buffer full without any reads\n");
    	cb->head=cb->buffer;
	cb->tail=cb->buffer;
    	cb->head10=(char*)cb->buffer_end-10*cb->sz;//the pointer to the data stored from 10 packets ago
    	cb->count=0; //
    	printf("Reset Ring buffer:Discarding data\n");
    }
        // handle error
    memcpy(cb->head, item, cb->sz);
    cb->head = (char*)cb->head + cb->sz;
    cb->head10=(char*)cb->head10+cb->sz;//increment the head pointer
    //printf("adding item %d %d\n",cb->sz,cb->head);
	
    cb->count++; //increment the count of how many objects have been stored
}

void cb_pop_front(circular_buffer *cb, void *item)
{//function to get data back off the ring buffer stack
    if(cb->count <= 0){
	printf("No data in ring buffer\n");
    }
        // handle error
   // memcpy(item, cb->tail, cb->sz); //memcpy from the pointer cb->tail to pointer item
    if((cb->count>0)){
    	memcpy(item, cb->tail, cb->sz); //memcpy from the pointer cb->head10 to pointer item-this gets the data from 10 packets ago to send to the control system
    	cb->tail = (char*)cb->tail + cb->sz;//try to increment the tail pointer
    //	printf("removing item %d %d\n",cb->sz,cb->tail);
    	if(cb->tail >= cb->buffer_end){ //if the tail pointer is to the end of the buffer thenwe need to set it to the beginning of the buffer
        	cb->tail = cb->buffer;
	//	printf("At cb->tail end%d",cb->sz);
		}
    cb->count--;//decrement the count of the number of objects in the ring buffer
	}
}

void cb_pop_front10(circular_buffer *cb, void *item)
{//function to get data back off the ring buffer stack
    if(cb->count == 0){
	printf("No data in ring buffer\n");
    }
        // handle error
   // memcpy(item, cb->tail, cb->sz); //memcpy from the pointer cb->tail to pointer item
   // printf("here in cb_pop front\n");
    if(cb->count!=0){
    	memcpy(item, cb->head10, cb->sz); //memcpy from the pointer cb->head10 to pointer item-this gets the data from 10 packets ago to send to the control system
    	cb->tail = (char*)cb->tail + cb->sz;//try to increment the tail pointer
    	if(cb->tail == cb->buffer_end){ //if the tail pointer is to the end of the buffer thenwe need to set it to the beginning of the buffer
        	cb->tail = cb->buffer;
	}
    	cb->count--;//decrement the count of the number of objects in the ring buffer
	}
    //printf("end in cb_pop front\n");
}



void *test_thread(void *arg){

	int sd,rc,n, cliLen,read_ret;
	int i,ij;
	int childfd;
	struct sockaddr_in cliAddr, servAddr;
        struct sockaddr_in server_addr,client_addr;    
	int optval;
	struct hostent *hostp; /* client host info */
	char *hostaddrp;
	int coordinate_type;
	char buf[1024],send_data[1024];
        int sock, connected, bytes_recieved , true = 1;  
        struct timeval t1,t2;
	double timediff,timediff_usec,tottimediff;
	char *returnDataPtr;
	unsigned int returnDataSize;
	extern struct circular_buffer buffer;//reference to the circular buffer struct set up in the other data readout thread
	extern struct circular_buffer* bufferptr;//pointer thats is set to the circular buffer
	struct UDPCBASSpkt controlSystemtcp; //local copy of data structure that will be sent to the control system
	char path_noise_diode[128];			// Path to acc_num counter data
	char path_gpiob0[128],path_gpiob1[128],path_gpiob2[128],path_gpiob3[128],path_gpiob4[128],path_gpiob5[128],path_gpiob6[128],path_gpiob7[128];			// Path to acc_num counter data
	FILE *fp;
	FILE *fp0,*fp1,*fp2,*fp3,*fp4,*fp5,*fp6;
	int writeVal,tempVal;
	int writeVal2;
	char commandValue,commandValue2;
	sprintf(path_noise_diode, "/proc/%s/hw/ioreg/gpio_set4", job[1]);
	sprintf(path_gpiob0, "/proc/%s/hw/ioreg/gpiob_set0", job[1]);
	sprintf(path_gpiob1, "/proc/%s/hw/ioreg/gpiob_set1", job[1]);
	sprintf(path_gpiob2, "/proc/%s/hw/ioreg/gpiob_set2", job[1]);
	sprintf(path_gpiob3, "/proc/%s/hw/ioreg/gpiob_set3", job[1]);
	sprintf(path_gpiob4, "/proc/%s/hw/ioreg/gpiob_set4", job[1]);
	sprintf(path_gpiob5, "/proc/%s/hw/ioreg/gpiob_set5", job[1]);
	sprintf(path_gpiob6, "/proc/%s/hw/ioreg/gpiob_set6", job[1]);
	sprintf(path_gpiob7, "/proc/%s/hw/ioreg/gpiob_set7", job[1]);

        printf("starting TCP server thread\n");


 
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
            perror("Setsockopt");
            exit(1);
        }
        
        server_addr.sin_family = AF_INET;         
        server_addr.sin_port = htons(PORT);     
        server_addr.sin_addr.s_addr = INADDR_ANY; 
        bzero(&server_addr.sin_zero,8); 

        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
            perror("Unable to bind");
            exit(1);
        }

	printf("Waiting for ROACH_BACKEND data commands on PORT TCP %u\n",PORT);
        fflush(stdout);
        strcpy(send_data,"brazil will never win the WC again!! \r");
	printf("size of cbass pkt %u\n",sizeof(cbassPKT));
	printf("%s\n",getDataCommand);		
	
	while(1)
	{	printf("Waiting for Connection\n");
		if(listen(sock,5)<0) //allows 5 request
			perror("Error on listening\n");
		cliLen=sizeof(cliAddr);
		childfd=accept(sock,(struct sockaddr *) &cliAddr,&cliLen);
		printf("Got connection\n");
		if(childfd <0)
			perror("Error on Accept\n");
		
	
	 	hostp = (struct hostent*)gethostbyaddr((const char *)&cliAddr.sin_addr.s_addr,sizeof(cliAddr.sin_addr.s_addr), AF_INET);

  	  	if (hostp == NULL)
     	 		perror("ERROR on gethostbyaddr");
   	 	hostaddrp = inet_ntoa(cliAddr.sin_addr);
   	 	if (hostaddrp == NULL){
	   		perror("ERROR on inet_ntoa\n");
			}
			
                n=1;
		TRY{	
			while(n>0){//while we're still able to send/receive data from the control system keep the tcp connection open
				gettimeofday(&t1,NULL);
				timediff=t1.tv_sec-t2.tv_sec;
				timediff_usec=t1.tv_usec-t2.tv_usec;
				tottimediff=timediff+timediff_usec/1000000;
			 //	printf("server established connection with %s (%s) %u\n",hostp->h_name, hostaddrp,PORT);

				  //first clear the incoming structure and then receive the binary data into it!
		//		printf("recv 1a blocking %d %d\n",n,childfd);
				
				memset(&buf[0], 0, sizeof(buf)); //zero the buffer
				n = recv(childfd, buf, 1024,0);
		//		printf("recv 1b blocking %d %d %s\n",n,childfd,buf);
				if (n <0){
					printf("ERROR reading from socket\n");
				//	close(childfd); 
					}
				if(n==0){
					printf("ERROR connection closed\n");
				}
		//		printf("server received %d %d %d bytes: %s\n", n, buf,childfd);
			//////////////////////definitions of the parsers///////////	
				if(!strncmp(buf,getDataCommand,10)){
			//		printf("getDataCommand %s %d\n",buf,n);
					cb_pop_front(bufferptr, &controlSystemtcp);//get the packet from 10 integrations ago to send to the control system
				//	printf("Sending the following in the struct :%d %d %d %d\n",controlSystemtcp.version,controlSystemtcp.data_ch0odd[2],controlSystemtcp.data_ch0odd[3]);
				//	printf("acc count 3  %lu  %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %d\n",controlSystemtcp.version,controlSystemtcp.tstart[0],controlSystemtcp.tstart[1],controlSystemtcp.tstart[2]);
					returnDataPtr=(char *)&controlSystemtcp;
					returnDataSize=sizeof(controlSystemtcp);
				}
				else if(!strncmp(buf,NoiseDiodeCommand,10)){
					printf("setNoiseDiode Command\n");
					commandValue=buf[11];	
					writeVal=atoi(&commandValue);
					printf("buf %s %d %d\n",buf,commandValue,writeVal);	
					fp = fopen(path_noise_diode, "w"); 
					fwrite(&writeVal, 4, 1, fp);
					fclose(fp);
					returnDataPtr=(char *)&NoiseDiodeCommand;
					returnDataSize=sizeof(NoiseDiodeCommand);
				}
				else if(!strncmp(buf,PhaseShifterCommand,10)){
					printf("PhaseShifter Command\n");
					commandValue=buf[11];	
					commandValue2=buf[12];	
					writeVal=atoi(&commandValue);
					writeVal2=atoi(&commandValue2);
					
					printf("buf %s %d %d %d\n",buf,commandValue,commandValue2,writeVal);	
					printf("%d %d\n",writeVal,writeVal2);	
					fp0 = fopen(path_gpiob0, "w"); 
					fp1 = fopen(path_gpiob1, "w"); 
					fp2 = fopen(path_gpiob2, "w"); 
					fp3 = fopen(path_gpiob3, "w"); 
					fp4 = fopen(path_gpiob6, "w"); 
					fp5 = fopen(path_gpiob7, "w");
					tempVal=(writeVal>>0&0x0001);
					fwrite(&tempVal, 4, 1, fp0);
					printf("Gpiob0 %d\n",tempVal);	
					tempVal=(writeVal>>1&0x0001);
					fwrite(&tempVal, 4, 1, fp1);
					printf("Gpiob1 %d\n",tempVal);	
					tempVal=(writeVal>>2&0x0001);
					fwrite(&tempVal, 4, 1, fp2);
					printf("Gpiob2 %d\n",tempVal);	
					tempVal=(writeVal>>3&0x0001);
					fwrite(&tempVal, 4, 1, fp3);
					printf("Gpiob3 %d\n",tempVal);	
					tempVal=(writeVal>>4&0x0001);
					fwrite(&tempVal, 4, 1, fp4);
					printf("Gpiob4 %d\n",tempVal);	
					tempVal=(writeVal>>5&0x0001);
					fwrite(&tempVal, 4, 1, fp5);
					printf("Gpiob5 %d\n",tempVal);	
					fclose(fp0);
					fclose(fp1);
					fclose(fp2);
					fclose(fp3);
					fclose(fp4);
					fclose(fp5);
					returnDataPtr=(char *)&PhaseShifterCommand;
					returnDataSize=sizeof(PhaseShifterCommand);
				}
				else {
					printf("Garbage sent\n");
					strncpy(Temp,buf,10);
				  	Temp[10] = '\0';
					strcat(Temp,",1");
					printf("Garbage sent %s \n",Temp);
					returnDataPtr=(char *)&Temp;
					returnDataSize=12;

				}
			////////////////////////end of Parser//////////////////////////////
				if(n>0){//i.e we have received data from the control system 
			//		printf("Attempting to send a reply %d\n",n);
					n=send(childfd, returnDataPtr,returnDataSize,0);
					if(n<0){
						printf("Error on sending reply\n");
						 }
					//printf("Sent %u bytes\n",n);       
					}
				
				
				//cb_pop_front10(bufferptr, &controlSystemtcp);//get the packet from 10 integrations ago to send to the control system
			//	printf("Sending Reply\n");

				}

			printf("server closing connection with control system\n");

	  		close(childfd);//close the connection if necessary
		}
		CATCH{
			printf("server closing connection with control system\n");
			close(childfd);
			printf("Exception in the TCP server catch\n");}
		ETRY;
	}


}





void *packROACHpacket_thread(void *arg){
        struct tempDataPacket tempPack;	
	int MSBshift;
	int test;
	int64_t val;
	unsigned int test2;	
        struct UDPCBASSpkt pkt;
        struct timeval t1,t2;
	double timediff,timediff_usec,tottimediff;
	char path_acc_cnt[128];			// Path to acc_num counter data
	char path_acc_len[128];			// Path to acc_num counter data
	char path_cnt_rst[128];			// Path to acc_num counter data
	char path_sync_en[128];			// Path to acc_num counter data
	char path_ctrl_sw[128];			// Path to acc_num counter data
	char path_timeStamp[128];			// Path to acc_num counter data
	char path_channel1evenMSB[128];			// Path to acc_num counter data
	char path_channel2evenMSB[128];			// Path to acc_num counter data
	char path_channel3evenMSB[128];			// Path to acc_num counter data
	char path_channel4evenMSB[128];			// Path to acc_num counter data
	char path_channel5evenMSB[128];			// Path to acc_num counter data
	char path_channel6evenMSB[128];			// Path to acc_num counter data
	char path_channel7evenMSB[128];			// Path to acc_num counter data
	char path_channel8evenMSB[128];			// Path to acc_num counter data
	char path_channel9evenMSB[128];			// Path to acc_num counter data
	char path_channel10evenMSB[128];			// Path to acc_num counter data
	char path_channel11evenMSB[128];			// Path to acc_num counter data
	char path_channel12evenMSB[128];			// Path to acc_num counter data
	char path_channel1evenLSB[128];			// Path to acc_num counter data
	char path_channel2evenLSB[128];			// Path to acc_num counter data
	char path_channel3evenLSB[128];			// Path to acc_num counter data
	char path_channel4evenLSB[128];			// Path to acc_num counter data
	char path_channel5evenLSB[128];			// Path to acc_num counter data
	char path_channel6evenLSB[128];			// Path to acc_num counter data
	char path_channel7evenLSB[128];			// Path to acc_num counter data
	char path_channel8evenLSB[128];			// Path to acc_num counter data
	char path_channel9evenLSB[128];			// Path to acc_num counter data
	char path_channel10evenLSB[128];			// Path to acc_num counter data
	char path_channel11evenLSB[128];			// Path to acc_num counter data
	char path_channel12evenLSB[128];			// Path to acc_num counter data
	char path_channel1oddMSB[128];			// Path to acc_num counter data
	char path_channel2oddMSB[128];			// Path to acc_num counter data
	char path_channel3oddMSB[128];			// Path to acc_num counter data
	char path_channel4oddMSB[128];			// Path to acc_num counter data
	char path_channel5oddMSB[128];			// Path to acc_num counter data
	char path_channel6oddMSB[128];			// Path to acc_num counter data
	char path_channel7oddMSB[128];			// Path to acc_num counter data
	char path_channel8oddMSB[128];			// Path to acc_num counter data
	char path_channel9oddMSB[128];			// Path to acc_num counter data
	char path_channel10oddMSB[128];			// Path to acc_num counter data
	char path_channel11oddMSB[128];			// Path to acc_num counter data
	char path_channel12oddMSB[128];			// Path to acc_num counter data
	char path_channel1oddLSB[128];			// Path to acc_num counter data
	char path_channel2oddLSB[128];			// Path to acc_num counter data
	char path_channel3oddLSB[128];			// Path to acc_num counter data
	char path_channel4oddLSB[128];			// Path to acc_num counter data
	char path_channel5oddLSB[128];			// Path to acc_num counter data
	char path_channel6oddLSB[128];			// Path to acc_num counter data
	char path_channel7oddLSB[128];			// Path to acc_num counter data
	char path_channel8oddLSB[128];			// Path to acc_num counter data
	char path_channel9oddLSB[128];			// Path to acc_num counter data
	char path_channel10oddLSB[128];			// Path to acc_num counter data
	char path_channel11oddLSB[128];			// Path to acc_num counter data
	char path_channel12oddLSB[128];			// Path to acc_num counter data
	char path_amp0realcoffs[128];			// Path to acc_num counter data
	char path_amp1realcoffs[128];			// Path to acc_num counter data
	char path_amp2realcoffs[128];			// Path to acc_num counter data
	char path_amp3realcoffs[128];			// Path to acc_num counter data
	char path_amp4realcoffs[128];			// Path to acc_num counter data
	char path_amp5realcoffs[128];			// Path to acc_num counter data
	char path_amp6realcoffs[128];			// Path to acc_num counter data
	char path_amp7realcoffs[128];			// Path to acc_num counter data
	char path_noiseDiode[128];			// Path to acc_num counter data
	char path_sync_cnt[128];			// Path to acc_num counter data
	char path_acc_sync_delay[128];			// Path to acc_num counter data
	char path_gpiob0[128],path_gpiob1[128],path_gpiob2[128],path_gpiob3[128],path_gpiob4[128],path_gpiob5[128],path_gpiob6[128],path_gpiob7[128];			// Path to acc_num counter data
	int noiseDiodeValue,statusBit;
	int gpiob0Val,gpiob1Val,gpiob2Val,gpiob3Val,gpiob4Val,gpiob5Val,gpiob6Val,gpiob7Val,gpiob8Val;
	unsigned int coffsinit[32];
	long long offsetQ[32],offsetU[32];
	int writeVal;
	unsigned int i;
	FILE *fp,*fp1;
	FILE *fpCh2MSBeven,*fpCh3MSBeven,*fpCh1MSBeven,*fpCh4MSBeven,*fpCh5MSBeven,*fpCh6MSBeven;				// File pointer
	FILE *fpCh2MSBodd,*fpCh3MSBodd,*fpCh1MSBodd,*fpCh4MSBodd,*fpCh5MSBodd,*fpCh6MSBodd;				// File pointer
	FILE *fpCh2LSBeven,*fpCh3LSBeven,*fpCh1LSBeven,*fpCh4LSBeven,*fpCh5LSBeven,*fpCh6LSBeven;				// File pointer
	FILE *fpCh2LSBodd,*fpCh3LSBodd,*fpCh1LSBodd,*fpCh4LSBodd,*fpCh5LSBodd,*fpCh6LSBodd;				// File pointer
	extern char *job[5];
        int acc_old;
        int acc_new;
        int acc_cntr;
	int tDiff[10];
        int timeStamp;
	extern	struct UDPCBASSpkt cbassPKT;
	signal(SIGINT, cleanup);
	sprintf(path_acc_cnt, "/proc/%s/hw/ioreg/Subsystem1_readAccumulation", job[1]);
	sprintf(path_acc_len, "/proc/%s/hw/ioreg/acc_len", job[1]);
	sprintf(path_cnt_rst, "/proc/%s/hw/ioreg/cnt_rst", job[1]);
	sprintf(path_sync_en, "/proc/%s/hw/ioreg/sync_en", job[1]);
	sprintf(path_sync_cnt, "/proc/%s/hw/ioreg/sync_en", job[1]);
	sprintf(path_timeStamp, "/proc/%s/hw/ioreg/Subsystem1_timeStamp", job[1]);
///////////////////////////////////////
	sprintf(path_channel1evenMSB, "/proc/%s/hw/ioreg/Subsystem1_ch1_msb", job[1]);
	sprintf(path_channel2evenMSB, "/proc/%s/hw/ioreg/Subsystem1_ch3_msb", job[1]);
	sprintf(path_channel3evenMSB, "/proc/%s/hw/ioreg/Subsystem1_ch5_msb", job[1]);
	sprintf(path_channel4evenMSB, "/proc/%s/hw/ioreg/Subsystem1_ch7_msb", job[1]);
	sprintf(path_channel5evenMSB, "/proc/%s/hw/ioreg/Subsystem1_ch9_msb", job[1]);
	sprintf(path_channel6evenMSB, "/proc/%s/hw/ioreg/Subsystem1_ch11_msb", job[1]);
	sprintf(path_channel1oddMSB, "/proc/%s/hw/ioreg/Subsystem1_ch2_msb", job[1]);
	sprintf(path_channel2oddMSB, "/proc/%s/hw/ioreg/Subsystem1_ch4_msb", job[1]);
	sprintf(path_channel3oddMSB, "/proc/%s/hw/ioreg/Subsystem1_ch6_msb", job[1]);
	sprintf(path_channel4oddMSB, "/proc/%s/hw/ioreg/Subsystem1_ch8_msb", job[1]);
	sprintf(path_channel5oddMSB, "/proc/%s/hw/ioreg/Subsystem1_ch10_msb", job[1]);
	sprintf(path_channel6oddMSB, "/proc/%s/hw/ioreg/Subsystem1_ch12_msb", job[1]);
	//////////////////////////////
	sprintf(path_channel1evenLSB, "/proc/%s/hw/ioreg/Subsystem1_ch1_lsb", job[1]);
	sprintf(path_channel2evenLSB, "/proc/%s/hw/ioreg/Subsystem1_ch3_lsb", job[1]);
	sprintf(path_channel3evenLSB, "/proc/%s/hw/ioreg/Subsystem1_ch5_lsb", job[1]);
	sprintf(path_channel4evenLSB, "/proc/%s/hw/ioreg/Subsystem1_ch7_lsb", job[1]);
	sprintf(path_channel5evenLSB, "/proc/%s/hw/ioreg/Subsystem1_ch9_lsb", job[1]);
	sprintf(path_channel6evenLSB, "/proc/%s/hw/ioreg/Subsystem1_ch11_lsb", job[1]);
	sprintf(path_channel1oddLSB, "/proc/%s/hw/ioreg/Subsystem1_ch2_lsb", job[1]);
	sprintf(path_channel2oddLSB, "/proc/%s/hw/ioreg/Subsystem1_ch4_lsb", job[1]);
	sprintf(path_channel3oddLSB, "/proc/%s/hw/ioreg/Subsystem1_ch6_lsb", job[1]);
	sprintf(path_channel4oddLSB, "/proc/%s/hw/ioreg/Subsystem1_ch8_lsb", job[1]);
	sprintf(path_channel5oddLSB, "/proc/%s/hw/ioreg/Subsystem1_ch10_lsb", job[1]);
	sprintf(path_channel6oddLSB, "/proc/%s/hw/ioreg/Subsystem1_ch12_lsb", job[1]);
/////////////////////////////////
	sprintf(path_amp0realcoffs, "/proc/%s/hw/ioreg/amp_EQ0_coeff_real", job[1]);
	sprintf(path_amp1realcoffs, "/proc/%s/hw/ioreg/amp_EQ1_coeff_real", job[1]);
	sprintf(path_amp2realcoffs, "/proc/%s/hw/ioreg/amp_EQ2_coeff_real", job[1]);
	sprintf(path_amp3realcoffs, "/proc/%s/hw/ioreg/amp_EQ3_coeff_real", job[1]);
	sprintf(path_amp4realcoffs, "/proc/%s/hw/ioreg/amp_EQ4_coeff_real", job[1]);
	sprintf(path_amp5realcoffs, "/proc/%s/hw/ioreg/amp_EQ5_coeff_real", job[1]);
	sprintf(path_amp6realcoffs, "/proc/%s/hw/ioreg/amp_EQ6_coeff_real", job[1]);
	sprintf(path_amp7realcoffs, "/proc/%s/hw/ioreg/amp_EQ7_coeff_real", job[1]);
	sprintf(path_acc_sync_delay, "/proc/%s/hw/ioreg/acc_sync_delay", job[1]);
	sprintf(path_ctrl_sw, "/proc/%s/hw/ioreg/ctrl_sw", job[1]);
	sprintf(path_noiseDiode, "/proc/%s/hw/ioreg/gpio_set4", job[1]);
	sprintf(path_gpiob0, "/proc/%s/hw/ioreg/gpiob_set0", job[1]);
	sprintf(path_gpiob1, "/proc/%s/hw/ioreg/gpiob_set1", job[1]);
	sprintf(path_gpiob2, "/proc/%s/hw/ioreg/gpiob_set2", job[1]);
	sprintf(path_gpiob3, "/proc/%s/hw/ioreg/gpiob_set3", job[1]);
	sprintf(path_gpiob4, "/proc/%s/hw/ioreg/gpiob_set4", job[1]);
	sprintf(path_gpiob5, "/proc/%s/hw/ioreg/gpiob_set5", job[1]);
	sprintf(path_gpiob6, "/proc/%s/hw/ioreg/gpiob_set6", job[1]);
	sprintf(path_gpiob7, "/proc/%s/hw/ioreg/gpiob_set7", job[1]);

	


	printf("Acc Counter: %s \n",path_acc_cnt);
	extern struct UDPCBASSpkt *PKTptr;
	extern struct circular_buffer buffer;
	extern struct circular_buffer* bufferptr;
	int packedDataCounter;//counter to keep track of the number of integrations
	int readStat[20];
	bufferptr=&buffer;
	cb_init(bufferptr,50,sizeof(struct UDPCBASSpkt)); //allocate space for ten data packets for the ring buffer


	fp = fopen(path_acc_sync_delay, "w"); 
	writeVal=4;
	fwrite(&writeVal, 4, 1, fp);
	fclose(fp);
	//initialise registers on the ROACH//////	
	fp = fopen(path_acc_len, "w"); 
	writeVal=78125-2;
	fwrite(&writeVal, 4, 1, fp);
	fclose(fp);
	
 
	fp = fopen(path_cnt_rst, "w"); 
	writeVal=1;
	fwrite(&writeVal, 4, 1, fp);
	writeVal=1;
	fwrite(&writeVal, 4, 1, fp);
	fclose(fp);
	fp = fopen(path_sync_en, "w"); 
	writeVal=1;
	fwrite(&writeVal, 4, 1, fp);
	fclose(fp);
	fp = fopen(path_ctrl_sw, "w"); 
	writeVal=43690;
	fwrite(&writeVal, 4, 1, fp);
	fclose(fp);
	printf("herexxxx %d",sizeof(cbassPKT));
	cbassPKT.version=1;
	cbassPKT.dataCount=10;
	cbassPKT.version=1; //version numbers of the Polarisation beging at 0
	
	encodeNetwork(&cbassPKT);
	printf("herexxx223x");	
	acc_old=0;
	acc_cntr=0;
	acc_new=0;
	packedDataCounter=0;
	timeStamp=0;
	cb_push_back(bufferptr,&cbassPKT); //add the last packet to the circular buffer	
	for(i=0;i<32;i++){
		offsetQ[i]=1;
		offsetU[i]=1;
	
	}
	while(1){
		packedDataCounter++;
		if(packedDataCounter==1){
		}	
		if(packedDataCounter==10){
			packedDataCounter=0;
			cbassPKT.tend=t1.tv_usec;
			cbassPKT.int_count=acc_new;
			encodeNetwork(&cbassPKT);
			cb_push_back(bufferptr,&cbassPKT); //add the last packet to the circular buffer
		}
	//FIRST WAIT FOR THE ROACH BEFORE SENDING ANY DATA
		gettimeofday(&t2,NULL);
		
		while(acc_new==acc_old){
			fp = fopen(path_acc_cnt, "r"); 
			fread(&acc_new, 4, 1, fp);
			fclose(fp);
			
			}
		acc_old = acc_new;

		//stephen ordered the readout differently to mine: his is LL Q U RR TL1 TL2
		//Mine is RR LL Q U Tl2 Tl2
		//So Channel 1->fpCh4 RR(1)->RR(4)
		//Channel2 ->fpCh1 LL(2)->LL(1)
		//Channel3 ->fpCh2 Q(3)->Q(2)
		//Channel4 ->fpCh3 U(4)->U(3)
		//Channel5 ->fpCh5 TL1(5) -> TL1(5)
		//Channel6 ->fpCh6 TL1(6) -> TL1(6)
		fpCh4MSBeven = fopen(path_channel1evenMSB, "r"); //RR 
		fpCh1MSBeven = fopen(path_channel2evenMSB, "r"); //LL
		fpCh2MSBeven = fopen(path_channel3evenMSB, "r"); //Q
		fpCh3MSBeven = fopen(path_channel4evenMSB, "r");  //U
		fpCh5MSBeven = fopen(path_channel5evenMSB, "r");  //TL1
		fpCh6MSBeven = fopen(path_channel6evenMSB, "r");  //Tl2
		fpCh4MSBodd = fopen(path_channel1oddMSB, "r"); 
		fpCh1MSBodd = fopen(path_channel2oddMSB, "r"); 
		fpCh2MSBodd = fopen(path_channel3oddMSB, "r"); 
		fpCh3MSBodd = fopen(path_channel4oddMSB, "r"); 
		fpCh5MSBodd = fopen(path_channel5oddMSB, "r"); 
		fpCh6MSBodd = fopen(path_channel6oddMSB, "r"); 
		//////////////////
		fpCh4LSBeven = fopen(path_channel1evenLSB, "r"); 
		fpCh1LSBeven = fopen(path_channel2evenLSB, "r"); 
		fpCh2LSBeven = fopen(path_channel3evenLSB, "r"); 
		fpCh3LSBeven = fopen(path_channel4evenLSB, "r"); 
		fpCh5LSBeven = fopen(path_channel5evenLSB, "r"); 
		fpCh6LSBeven = fopen(path_channel6evenLSB, "r"); 
		fpCh4LSBodd = fopen(path_channel1oddLSB, "r"); 
		fpCh1LSBodd = fopen(path_channel2oddLSB, "r"); 
		fpCh2LSBodd = fopen(path_channel3oddLSB, "r"); 
		fpCh3LSBodd = fopen(path_channel4oddLSB, "r"); 
		fpCh5LSBodd = fopen(path_channel5oddLSB, "r"); 
		fpCh6LSBodd = fopen(path_channel6oddLSB, "r"); 
	
		//printf("position %d \n",packedDataCounter*vectorLength);
		readStat[0]=fread(&tempPack.data_ch0evenMSB[0],sizeof(int),32,fpCh1MSBeven); //LL
		readStat[0]=fread(&tempPack.data_ch1evenMSB[0],sizeof(int),32,fpCh2MSBeven); //Q
		readStat[0]=fread(&tempPack.data_ch2evenMSB[0],sizeof(int),32,fpCh3MSBeven); //U
		readStat[0]=fread(&tempPack.data_ch3evenMSB[0],sizeof(int),32,fpCh4MSBeven); //RR
		readStat[0]=fread(&tempPack.data_ch4evenMSB[0],sizeof(int),32,fpCh5MSBeven); //Tl1
		readStat[0]=fread(&tempPack.data_ch5evenMSB[0],sizeof(int),32,fpCh6MSBeven); //Tl2
		readStat[0]=fread(&tempPack.data_ch0oddMSB[0],sizeof(int),32,fpCh1MSBodd);
		readStat[0]=fread(&tempPack.data_ch1oddMSB[0],sizeof(int),32,fpCh2MSBodd);
		readStat[0]=fread(&tempPack.data_ch2oddMSB[0],sizeof(int),32,fpCh3MSBodd);
		readStat[0]=fread(&tempPack.data_ch3oddMSB[0],sizeof(int),32,fpCh4MSBodd);
		readStat[0]=fread(&tempPack.data_ch4oddMSB[0],sizeof(int),32,fpCh5MSBodd);
		readStat[0]=fread(&tempPack.data_ch5oddMSB[0],sizeof(int),32,fpCh6MSBodd);

		///////////////////////////////////////////////
		
		readStat[0]=fread(&tempPack.data_ch0evenLSB[0],sizeof(int),32,fpCh1LSBeven);
		readStat[0]=fread(&tempPack.data_ch1evenLSB[0],sizeof(int),32,fpCh2LSBeven);
		readStat[0]=fread(&tempPack.data_ch2evenLSB[0],sizeof(int),32,fpCh3LSBeven);
		readStat[0]=fread(&tempPack.data_ch3evenLSB[0],sizeof(int),32,fpCh4LSBeven);
		readStat[0]=fread(&tempPack.data_ch4evenLSB[0],sizeof(int),32,fpCh5LSBeven);
		readStat[0]=fread(&tempPack.data_ch5evenLSB[0],sizeof(int),32,fpCh6LSBeven);
		readStat[0]=fread(&tempPack.data_ch0oddLSB[0],sizeof(int),32,fpCh1LSBodd);
		readStat[0]=fread(&tempPack.data_ch1oddLSB[0],sizeof(int),32,fpCh2LSBodd);
		readStat[0]=fread(&tempPack.data_ch2oddLSB[0],sizeof(int),32,fpCh3LSBodd);
		readStat[0]=fread(&tempPack.data_ch3oddLSB[0],sizeof(int),32,fpCh4LSBodd);
		readStat[0]=fread(&tempPack.data_ch4oddLSB[0],sizeof(int),32,fpCh5LSBodd);
		readStat[0]=fread(&tempPack.data_ch5oddLSB[0],sizeof(int),32,fpCh6LSBodd);
		
	//get the Q and U channels correct///////////////////////////////////////
	
	//	for(i=0;i<32;i++){
	//		val = (int64_t)tempPack.data_ch1oddMSB[i]<<32 | tempPack.data_ch1oddLSB[i];	
	//		while((val+offsetQ[i]<0)){
	//			offsetQ[i]+=10000;
	//		}
	//		val+=offsetQ[i];
	//		tempPack.data_ch1oddMSB[i]=(int)(val);
			//printf("%lld \n",val);
	//		val=val>>3;
			//tempPack.data_ch1oddMSB[i]=0;
	//		val = (int64_t)tempPack.data_ch1evenMSB[i]<<32 | tempPack.data_ch1evenLSB[i];	
	//		val=val>>3;
	//		val+=offsetQ[i];
	//		tempPack.data_ch1evenMSB[i]=(int)(val);
			//printf("%lld \n",val);
		//tempPack.data_ch1evenMSB[i]=0;
	//		val = (int64_t)tempPack.data_ch2oddMSB[i]<<32 | tempPack.data_ch2oddLSB[i];	
	//		val=val>>3;
	//		while((val+offsetU[i]<0)){
	//			offsetU[i]+=10000;
	//		}
	//		val+=offsetU[i];
	//		tempPack.data_ch2oddMSB[i]=(int)(val);
			//printf("%lld \n",val);
			//tempPack.data_ch2oddMSB[i]=0;
	//		val = (int64_t)tempPack.data_ch2evenMSB[i]<<32 | tempPack.data_ch2evenLSB[i];	
	//		val=val>>3;
	//		val+=offsetU[i];
	//		tempPack.data_ch2evenMSB[i]=(int)(val);
			//printf("%lld \n",val);
			//tempPack.data_ch2evenMSB[i]=0;
	//	}
//		for(i=0;i<32;i++){
		//	tempPack.data_ch0oddMSB[i]=5;
		//	tempPack.data_ch0evenMSB[i]=6;
	//		tempPack.data_ch1oddMSB[i]=-20;
	//		tempPack.data_ch1evenMSB[i]=-21;
		//	tempPack.data_ch2oddMSB[i]=30;
		//	tempPack.data_ch2evenMSB[i]=31;
		//	tempPack.data_ch3oddMSB[i]=40;
		//	tempPack.data_ch3evenMSB[i]=41;
		//	tempPack.data_ch4oddMSB[i]=50;
		//	tempPack.data_ch4evenMSB[i]=51;
		//	tempPack.data_ch5oddMSB[i]=60;
		//	tempPack.data_ch5evenMSB[i]=61;
//		}
		//for(i=0;i<10;i++){
	//	}	
//////////////////////////
	
//		tempPack.data_ch1oddMSB[0]=test||test2;

//		printf("%d\n", (int)val);
//		printf("data0_ch3MSB %08x %08x %08x %08x %d %lld\n",tempPack.data_ch1oddMSB[0],tempPack.data_ch1oddLSB[0],test,test2,test+test2,val);
//		printf("data0_ch3MSB %d %d %d %d %d %d \n",tempPack.data_ch0oddMSB[0],tempPack.data_ch1oddMSB[0],tempPack.data_ch2oddMSB[0],tempPack.data_ch3oddMSB[0],tempPack.data_ch4oddMSB[0],tempPack.data_ch5oddMSB[0]);
//		printf("data0_ch3MSB %d %d %d %d %d %d \n",tempPack.data_ch0evenMSB[0],tempPack.data_ch1evenMSB[0],tempPack.data_ch2evenMSB[0],tempPack.data_ch3evenMSB[0],tempPack.data_ch4evenMSB[0],tempPack.data_ch5evenMSB[0]);
		MSBshift=gainCorrectionCalc(&tempPack);
		gainCorrection(&tempPack,8);
		///////////////////////////
		memcpy(&cbassPKT.data_ch0even[packedDataCounter*vectorLength],&tempPack.data_ch0evenMSB[0],sizeof(int)*32); //LL
		memcpy(&cbassPKT.data_ch1even[packedDataCounter*vectorLength],&tempPack.data_ch1evenMSB[0],sizeof(int)*32); //Q
		memcpy(&cbassPKT.data_ch2even[packedDataCounter*vectorLength],&tempPack.data_ch2evenMSB[0],sizeof(int)*32); //U
		memcpy(&cbassPKT.data_ch3even[packedDataCounter*vectorLength],&tempPack.data_ch3evenMSB[0],sizeof(int)*32); //RR
		memcpy(&cbassPKT.data_ch4even[packedDataCounter*vectorLength],&tempPack.data_ch4evenMSB[0],sizeof(int)*32); //Tl1
		memcpy(&cbassPKT.data_ch5even[packedDataCounter*vectorLength],&tempPack.data_ch5evenMSB[0],sizeof(int)*32); //Tl2
		memcpy(&cbassPKT.data_ch0odd[packedDataCounter*vectorLength],&tempPack.data_ch0oddMSB[0],sizeof(int)*32);
		memcpy(&cbassPKT.data_ch1odd[packedDataCounter*vectorLength],&tempPack.data_ch1oddMSB[0],sizeof(int)*32);
		memcpy(&cbassPKT.data_ch2odd[packedDataCounter*vectorLength],&tempPack.data_ch2oddMSB[0],sizeof(int)*32);
		memcpy(&cbassPKT.data_ch3odd[packedDataCounter*vectorLength],&tempPack.data_ch3oddMSB[0],sizeof(int)*32);
		memcpy(&cbassPKT.data_ch4odd[packedDataCounter*vectorLength],&tempPack.data_ch4oddMSB[0],sizeof(int)*32);
		memcpy(&cbassPKT.data_ch5odd[packedDataCounter*vectorLength],&tempPack.data_ch5oddMSB[0],sizeof(int)*32);
		for(i=0;i<64;i++){
			tempPack.data_ch0oddMSB[i]=0;
			tempPack.data_ch0evenMSB[i]=0;
			tempPack.data_ch1oddMSB[i]=0;
			tempPack.data_ch1evenMSB[i]=0;
			tempPack.data_ch2oddMSB[i]=0;
			tempPack.data_ch2evenMSB[i]=0;
			tempPack.data_ch3oddMSB[i]=0;
			tempPack.data_ch3evenMSB[i]=0;
			tempPack.data_ch4oddMSB[i]=0;
			tempPack.data_ch4evenMSB[i]=0;
			tempPack.data_ch5oddMSB[i]=0;
			tempPack.data_ch5evenMSB[i]=0;
		}


		fclose(fpCh1MSBeven);
		fclose(fpCh2MSBeven);
		fclose(fpCh3MSBeven);
		fclose(fpCh4MSBeven);
		fclose(fpCh5MSBeven);
		fclose(fpCh6MSBeven);
		fclose(fpCh1MSBodd);
		fclose(fpCh2MSBodd);
		fclose(fpCh3MSBodd);
		fclose(fpCh4MSBodd);
		fclose(fpCh5MSBodd);
		fclose(fpCh6MSBodd);
		fclose(fpCh1LSBeven);
		fclose(fpCh2LSBeven);
		fclose(fpCh3LSBeven);
		fclose(fpCh4LSBeven);
		fclose(fpCh5LSBeven);
		fclose(fpCh6LSBeven);
		fclose(fpCh1LSBodd);
		fclose(fpCh2LSBodd);
		fclose(fpCh3LSBodd);
		fclose(fpCh4LSBodd);
		fclose(fpCh5LSBodd);
		fclose(fpCh6LSBodd);
	/////////////////////////////////////////////	
		tDiff[packedDataCounter]=timeStamp;
		fp1 = fopen(path_timeStamp, "r"); 
			fread(&timeStamp, 4, 1, fp1);
		fclose(fp1);
		fp1 = fopen(path_noiseDiode, "r"); 
			fread(&noiseDiodeValue, 4, 1, fp1);
		fclose(fp1);
	//	fp1 = fopen(path_gpiob0, "r"); 
	//		fread(&gpiob0Val, 4, 1, fp1);
	//	fclose(fp1);
	//	fp1 = fopen(path_gpiob1, "r"); 
	//		fread(&gpiob1Val, 4, 1, fp1);
	//	fclose(fp1);
	//	fp1 = fopen(path_gpiob2, "r"); 
	//		fread(&gpiob2Val, 4, 1, fp1);
	//	fclose(fp1);
	//	fp1 = fopen(path_gpiob3, "r"); 
	//		fread(&gpiob3Val, 4, 1, fp1);
	//	fclose(fp1);
	//	fp1 = fopen(path_gpiob4, "r"); 
	///		fread(&gpiob4Val, 4, 1, fp1);
	//	fclose(fp1);
	//	fp1 = fopen(path_gpiob5, "r"); 
	//		fread(&gpiob5Val, 4, 1, fp1);
	//	fclose(fp1);
	//	fp1 = fopen(path_gpiob6, "r"); 
	//		fread(&gpiob6Val, 4, 1, fp1);
	//	fclose(fp1);
	//	fp1 = fopen(path_gpiob7, "r"); 
	//		fread(&gpiob7Val, 4, 1, fp1);
	//	fclose(fp1);
	//	fp = fopen(path_acc_len, "w"); 
	//		fread(&cbassPKT.int_len,4,1,fp);
	//	fclose(fp);

		statusBit=0;
		statusBit=statusBit |= (noiseDiodeValue<<0);
		statusBit=statusBit |= (gpiob0Val<<1);
		statusBit=statusBit |= (gpiob1Val<<2);
		statusBit=statusBit |= (gpiob2Val<<3);
		statusBit=statusBit |= (gpiob3Val<<4);
		statusBit=statusBit |= (gpiob6Val<<5);
		statusBit=statusBit |= (gpiob7Val<<5);
		cbassPKT.tstart[packedDataCounter]=timeStamp;//packet timestamp
		cbassPKT.data_switchstatus[packedDataCounter]=statusBit; //status register
				
		tDiff[packedDataCounter]=(int)timeStamp-(int)tDiff[packedDataCounter];
	  	if(tDiff[packedDataCounter]<=0){
			printf("1PPS %d %d %d %d\n",timeStamp,acc_cntr,acc_new,tDiff[packedDataCounter]);
			acc_cntr=acc_new;
		}
		
		gettimeofday(&t1,NULL);
		timediff=t1.tv_sec-t2.tv_sec;
		timediff_usec=t1.tv_usec-t2.tv_usec;
		tottimediff=timediff+timediff_usec/1000000;


		}
	cb_free(bufferptr); //free the circular buffer



}

int decodeNetwork(struct UDPCBASSpkt *pkt){
	//to encode the data into network byte order use this function
	
	int i;
 	printf("%f %d\n",(float)pkt->data_ch0odd[300],pkt->data_ch0odd[300]);       
	printf("decode %04x\n",pkt->data_ch0odd[300]);
	for(i=0;i<=kDataperPacket*vectorLength;i++){
		pkt->data_ch0odd[i] = bswap_32(pkt->data_ch0odd[i]);
		pkt->data_ch0even[i] = bswap_32(pkt->data_ch0even[i]);
		pkt->data_ch1odd[i] = bswap_32(pkt->data_ch1odd[i]);
		pkt->data_ch1even[i] = bswap_32(pkt->data_ch1even[i]);
		pkt->data_ch2odd[i] = bswap_32(pkt->data_ch2odd[i]);
		pkt->data_ch2even[i] = bswap_32(pkt->data_ch2even[i]);
		pkt->data_ch3odd[i] = bswap_32(pkt->data_ch3odd[i]);
		pkt->data_ch3even[i] = bswap_32(pkt->data_ch3even[i]);
		pkt->data_ch4odd[i] = bswap_32(pkt->data_ch4odd[i]);
		pkt->data_ch4even[i] = bswap_32(pkt->data_ch4even[i]);
		pkt->data_ch5odd[i] = bswap_32(pkt->data_ch5odd[i]);
		pkt->data_ch5even[i] = bswap_32(pkt->data_ch5even[i]);


	}

	printf("decode %04x\n",pkt->data_ch0odd[300]);
	
};


int encodeNetwork(struct UDPCBASSpkt *pkt){
	//to encode the data into network byte order use this function
	
	int i;
	pkt->version = htonl(pkt->version);
	pkt->data_size = htonl(pkt->data_size);
	pkt->dataCount = htonl(pkt->dataCount);
	pkt->int_count = htonl(pkt->int_count);
	for(i=0;i<10;i++){
		pkt->tstart[i]=htonl(pkt->tstart[i]);
	}
 	//printf("%f %d\n",(float)pkt->data_ch0odd[300],pkt->data_ch0odd[300]);       
	//printf("encode %04x\n",pkt->data_ch0odd[300]);
	for(i=0;i<=kDataperPacket*vectorLength;i++){
		pkt->data_ch0odd[i] = htonl(pkt->data_ch0odd[i]);
		pkt->data_ch0even[i] = htonl(pkt->data_ch0even[i]);
		pkt->data_ch1odd[i] = htonl(pkt->data_ch1odd[i]);
		pkt->data_ch1even[i] = htonl(pkt->data_ch1even[i]);
		pkt->data_ch2odd[i] = htonl(pkt->data_ch2odd[i]);
		pkt->data_ch2even[i] = htonl(pkt->data_ch2even[i]);
		pkt->data_ch3odd[i] = htonl(pkt->data_ch3odd[i]);
		pkt->data_ch3even[i] = htonl(pkt->data_ch3even[i]);
		pkt->data_ch4odd[i] = htonl(pkt->data_ch4odd[i]);
		pkt->data_ch4even[i] = htonl(pkt->data_ch4even[i]);
		pkt->data_ch5odd[i] = htonl(pkt->data_ch5odd[i]);
		pkt->data_ch5even[i] = htonl(pkt->data_ch5even[i]);
		pkt->data_switchstatus[i] = htonl(pkt->data_switchstatus[i]);


	}

	//printf("encode %04x\n",pkt->data_ch0odd[300]);
	
};

void cleanup()
{
    //close(server);
    printf("Received CTRL-C");
    exit(0);
    return;
} /* cleanup() */

int gainCorrectionCalc(struct tempDataPacket *pack){
	int xorig,x,a;
	x=*pack->data_ch0oddMSB;
	xorig=x;
	a=0;
	while(x>0){
		x=x>>1;
		a++;
	}
	//printf("testing %d %d %d\n",xorig,x,a);
	return a;

}
int gainCorrection(struct tempDataPacket *pack,int MSBshift){
	int i;
	int oddMSBch0[32],oddLSBch0[32],oddMSBch1[32],oddLSBch1[32],oddMSBch2[32],oddLSBch2[32],oddMSBch3[32],oddLSBch3[32],oddMSBch4[32],oddLSBch4[32],oddMSBch5[32],oddLSBch5[32];
	int evenMSBch0[32],evenLSBch0[32],evenMSBch1[32],evenLSBch1[32],evenMSBch2[32],evenLSBch2[32],evenMSBch3[32],evenLSBch3[32],evenMSBch4[32],evenLSBch4[32],evenMSBch5[32],evenLSBch5[32];
	unsigned int temp0,temp1,temp2,temp3,temp4,temp5;
 	int MSBshiftPol;



	MSBshift=MSBshift;
	MSBshiftPol=7;
	memcpy(oddMSBch0,pack->data_ch0oddMSB,sizeof(int)*32);
	memcpy(oddLSBch0,pack->data_ch0oddLSB,sizeof(int)*32);
	memcpy(oddMSBch1,pack->data_ch1oddMSB,sizeof(int)*32);
	memcpy(oddLSBch1,pack->data_ch1oddLSB,sizeof(int)*32);
	memcpy(oddMSBch2,pack->data_ch2oddMSB,sizeof(int)*32);
	memcpy(oddLSBch2,pack->data_ch2oddLSB,sizeof(int)*32);
	memcpy(oddMSBch3,pack->data_ch3oddMSB,sizeof(int)*32);
	memcpy(oddLSBch3,pack->data_ch3oddLSB,sizeof(int)*32);
	memcpy(oddMSBch4,pack->data_ch4oddMSB,sizeof(int)*32);
	memcpy(oddLSBch4,pack->data_ch4oddLSB,sizeof(int)*32);
	memcpy(oddMSBch5,pack->data_ch5oddMSB,sizeof(int)*32);
	memcpy(oddLSBch5,pack->data_ch5oddLSB,sizeof(int)*32);
	memcpy(evenMSBch0,pack->data_ch0evenMSB,sizeof(int)*32);
	memcpy(evenLSBch0,pack->data_ch0evenLSB,sizeof(int)*32);
	memcpy(evenMSBch1,pack->data_ch1evenMSB,sizeof(int)*32);
	memcpy(evenLSBch1,pack->data_ch1evenLSB,sizeof(int)*32);
	memcpy(evenMSBch2,pack->data_ch2evenMSB,sizeof(int)*32);
	memcpy(evenLSBch2,pack->data_ch2evenLSB,sizeof(int)*32);
	memcpy(evenMSBch3,pack->data_ch3evenMSB,sizeof(int)*32);
	memcpy(evenLSBch3,pack->data_ch3evenLSB,sizeof(int)*32);
	memcpy(evenMSBch4,pack->data_ch4evenMSB,sizeof(int)*32);
	memcpy(evenLSBch4,pack->data_ch4evenLSB,sizeof(int)*32);
	memcpy(evenMSBch5,pack->data_ch5evenMSB,sizeof(int)*32);
	memcpy(evenLSBch5,pack->data_ch5evenLSB,sizeof(int)*32);

	//printf("LL = %04x %04x \n",oddMSBch0[5],oddLSBch0[5]);
	//printf("Q = %04x %04x \n",oddMSBch1[5],oddLSBch1[5]);
	//printf("U = %04x %04x \n",oddMSBch2[5],oddLSBch2[5]);
	
	for(i=0;i<32;i++){
		temp0=(unsigned int)oddLSBch0[i];
		temp1=(unsigned int)oddLSBch1[i];
		temp2=(unsigned int)oddLSBch2[i];
		temp3=(unsigned int)oddLSBch3[i];
		temp4=(unsigned int)oddLSBch4[i];
		temp5=(unsigned int)oddLSBch5[i];
		oddMSBch0[i]=(oddMSBch0[i]<<(MSBshift))|temp0>>(32-MSBshift);
		oddMSBch1[i]=(oddMSBch1[i]<<(MSBshiftPol))|temp1>>(32-MSBshiftPol);
		oddMSBch2[i]=(oddMSBch2[i]<<(MSBshiftPol))|temp2>>(32-MSBshiftPol);
		oddMSBch3[i]=(oddMSBch3[i]<<(MSBshift))|temp3>>(32-MSBshift);
		oddMSBch4[i]=(oddMSBch4[i]<<(MSBshift))|temp4>>(32-MSBshift);
		oddMSBch5[i]=(oddMSBch5[i]<<(MSBshift))|temp5>>(32-MSBshift);
		oddMSBch1[i]+=65535;
		oddMSBch2[i]+=65535;
		temp0=(unsigned int)evenLSBch0[i];
		temp1=(unsigned int)evenLSBch1[i];
		temp2=(unsigned int)evenLSBch2[i];
		temp3=(unsigned int)evenLSBch3[i];
		temp4=(unsigned int)evenLSBch4[i];
		temp5=(unsigned int)evenLSBch5[i];
		evenMSBch0[i]=(evenMSBch0[i]<<(MSBshift))|temp0>>(32-MSBshift);
		evenMSBch1[i]=(evenMSBch1[i]<<(MSBshiftPol))|temp1>>(32-MSBshiftPol);
		evenMSBch2[i]=(evenMSBch2[i]<<(MSBshiftPol))|temp2>>(32-MSBshiftPol);
		evenMSBch3[i]=(evenMSBch3[i]<<(MSBshift))|temp3>>(32-MSBshift);
		evenMSBch4[i]=(evenMSBch4[i]<<(MSBshift))|temp4>>(32-MSBshift);
		evenMSBch5[i]=(evenMSBch5[i]<<(MSBshift))|temp5>>(32-MSBshift);
		evenMSBch1[i]+=65535;
		evenMSBch2[i]+=65535;
	}
//	printf("LL = %d Q %d U %d \n",oddMSBch0[5],oddMSBch1[5],oddMSBch2[5]);
	evenMSBch0[0]=0; //zero the DC Bin of Ch0
	evenMSBch1[0]=0; //zero the DC Bin of Ch0
	evenMSBch2[0]=0; //zero the DC Bin of Ch0
	evenMSBch3[0]=0; //zero the DC Bin of Ch0
	evenMSBch4[0]=0; //zero the DC Bin of Ch0
	evenMSBch5[0]=0; //zero the DC Bin of Ch0
	
	oddMSBch0[0]=0; //zero the DC Bin of Ch0
	oddMSBch1[0]=0; //zero the DC Bin of Ch0
	oddMSBch2[0]=0; //zero the DC Bin of Ch0
	oddMSBch3[0]=0; //zero the DC Bin of Ch0
	oddMSBch4[0]=0; //zero the DC Bin of Ch0
	oddMSBch5[0]=0; //zero the DC Bin of Ch0
	//printf("Q = %04x %04x \n",oddMSBch1[5],oddLSBch1[5]);
	//printf("U = %04x %04x \n",oddMSBch2[5],oddLSBch2[5]);

	memcpy(pack->data_ch0oddMSB,oddMSBch0,sizeof(int)*32);
	memcpy(pack->data_ch1oddMSB,oddMSBch1,sizeof(int)*32);
	memcpy(pack->data_ch2oddMSB,oddMSBch2,sizeof(int)*32);
	memcpy(pack->data_ch3oddMSB,oddMSBch3,sizeof(int)*32);
	memcpy(pack->data_ch4oddMSB,oddMSBch4,sizeof(int)*32);
	memcpy(pack->data_ch5oddMSB,oddMSBch5,sizeof(int)*32);
	memcpy(pack->data_ch0evenMSB,evenMSBch0,sizeof(int)*32);
	memcpy(pack->data_ch1evenMSB,evenMSBch1,sizeof(int)*32);
	memcpy(pack->data_ch2evenMSB,evenMSBch2,sizeof(int)*32);
	memcpy(pack->data_ch3evenMSB,evenMSBch3,sizeof(int)*32);
	memcpy(pack->data_ch4evenMSB,evenMSBch4,sizeof(int)*32);
	memcpy(pack->data_ch5evenMSB,evenMSBch5,sizeof(int)*32);
		
}

void initCoeffs(){
	FILE *fp,*fpreg;
	extern char *job[5];
	char line[80];
	char filenames[16][32]={"amp0real.txt","amp1real.txt","amp2real.txt","amp3real.txt","amp4real.txt","amp5real.txt","amp6real.txt","amp7real.txt","amp0imag.txt","amp1imag.txt","amp2imag.txt","amp3imag.txt","amp4imag.txt","amp5imag.txt","amp6imag.txt","amp7imag.txt"};
	char registerNames[16][50]={"amp_EQ0_coeff_real","amp_EQ1_coeff_real","amp_EQ2_coeff_real","amp_EQ3_coeff_real","amp_EQ4_coeff_real","amp_EQ5_coeff_real","amp_EQ6_coeff_real","amp_EQ7_coeff_real","amp_EQ0_coeff_imag","amp_EQ1_coeff_imag","amp_EQ2_coeff_imag","amp_EQ3_coeff_imag","amp_EQ4_coeff_imag","amp_EQ5_coeff_imag","amp_EQ6_coeff_imag","amp_EQ7_coeff_imag"};
	char path_to_registerNames[16][50];
	//sprintf(path_amp0realcoffs, "/proc/%s/hw/ioreg/amp_EQ0_coeff_real", job[1]);
	unsigned int ampcoffs[35];
	printf("%s\n",&filenames[0]);	

	unsigned int i=0;
	unsigned int j=0;
	i=0;
	while(i<16){
		printf("i=%d\n",i);
		sprintf(path_to_registerNames[i],"/proc/%s/hw/ioreg/%s",job[1],registerNames[i]);
		printf("%s\n",path_to_registerNames[i]);
		fp=fopen(filenames[i], "r");
		fpreg=fopen(path_to_registerNames[i],"w");
		j=0;
		while(fgets(line, 80, fp) != NULL)
			{ /* get a line, up to 80 chars from fr.  done if NULL */
			 sscanf(line, "%ld", &ampcoffs[j]);
			 printf("%d %d\n",j,ampcoffs[j]);
			 j++;
			}
		printf("j=%d\n",j);
		fclose(fp);
		printf("j=%d\n",j);
		fwrite(&ampcoffs[0], 32*4, 1, fpreg);
		fclose(fpreg);
		printf("j=%d\n",j);
		i++;
	}
	
	
}


int main(int argc, char *argv[])
{
        int sock, connected, bytes_recieved , true = 1,r; 
	int t=1;
        char send_data [1024] , recv_data[1024];       
	pthread_t testThread_id;
	pthread_t packROACHpacket_thread_id;
        struct sockaddr_in server_addr,client_addr;    
        int sin_size;
	extern char *job[5];
	pthread_attr_t tattr;
	signal(SIGINT, cleanup);
	job[1]=argv[1];//global variable for access in the loop
	printf("Here starting thread\n");
 	r = pthread_attr_init(&tattr);
	//set up the Ctrl-C handlers
	signal(SIGINT, sigint_handler);
	signal(SIGKILL, sigint_handler);

	pthread_create(&testThread_id,NULL,test_thread,NULL);
	pthread_create(&packROACHpacket_thread_id,NULL,packROACHpacket_thread,NULL);
	initCoeffs();
        printf("Here started thread\n");




        while(1){
	sleep(2);
//	printf("here\n");	
	}
} 