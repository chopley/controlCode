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
#include "structUDPCBASSpkt.h"

#define PORT 12345
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

void cb_init(circular_buffer *cb, size_t capacity, size_t sz){
    //initiliase the circular buffer
    cb->buffer = malloc(capacity * sz);
    if(cb->buffer == NULL)
        // handle error
    cb->buffer_end = (char *)cb->buffer + capacity * sz;
    cb->capacity = capacity;
    cb->count = 0;
    cb->sz = sz;
    cb->head = cb->buffer;
    cb->head10=(char*)cb->buffer_end-10*cb->sz;//the pointer to the data stored from 10 packets ago
    cb->tail = cb->buffer;
}

void cb_free(circular_buffer *cb)
{
    free(cb->buffer);
    // clear out other fields too, just to be safe
}

void cb_push_back(circular_buffer *cb, const void *item)
{//function to put data onto the ring buffer
    if(cb->count == cb->capacity)
    {	printf("Ring buffer full without any reads\n");
    	cb->head=cb->buffer;
	cb->tail=cb->buffer;
    	cb->head10=(char*)cb->buffer_end-10*cb->sz;//the pointer to the data stored from 10 packets ago
    	cb->count=0; //increment the count of how many objects have been stored
    	printf("Reset Ring buffer:Discarding data\n");
    }
        // handle error
    memcpy(cb->head, item, cb->sz);
    cb->head = (char*)cb->head + cb->sz;
    cb->head10=(char*)cb->head10+cb->sz;//increment the head pointer
	
    if(cb->head10 == cb->buffer_end)
        cb->head10 = cb->buffer;
    if(cb->head == cb->buffer_end)
        cb->head = cb->buffer;
    cb->count++; //increment the count of how many objects have been stored
}

void cb_pop_front(circular_buffer *cb, void *item)
{//function to get data back off the ring buffer stack
    if(cb->count == 0){
	printf("No data in ring buffer\n");
    }
        // handle error
   // memcpy(item, cb->tail, cb->sz); //memcpy from the pointer cb->tail to pointer item
    memcpy(item, cb->tail, cb->sz); //memcpy from the pointer cb->head10 to pointer item-this gets the data from 10 packets ago to send to the control system
    cb->tail = (char*)cb->tail + cb->sz;//try to increment the tail pointer
    if(cb->tail == cb->buffer_end) //if the tail pointer is to the end of the buffer thenwe need to set it to the beginning of the buffer
        cb->tail = cb->buffer;
    cb->count--;//decrement the count of the number of objects in the ring buffer
}

void cb_pop_front10(circular_buffer *cb, void *item)
{//function to get data back off the ring buffer stack
    if(cb->count == 0){
	printf("No data in ring buffer\n");
    }
        // handle error
   // memcpy(item, cb->tail, cb->sz); //memcpy from the pointer cb->tail to pointer item
    memcpy(item, cb->head10, cb->sz); //memcpy from the pointer cb->head10 to pointer item-this gets the data from 10 packets ago to send to the control system
    cb->tail = (char*)cb->tail + cb->sz;//try to increment the tail pointer
    if(cb->tail == cb->buffer_end) //if the tail pointer is to the end of the buffer thenwe need to set it to the beginning of the buffer
        cb->tail = cb->buffer;
    cb->count--;//decrement the count of the number of objects in the ring buffer
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

	extern struct circular_buffer buffer;//reference to the circular buffer struct set up in the other data readout thread
	extern struct circular_buffer* bufferptr;//pointer thats is set to the circular buffer
	struct UDPCBASSpkt controlSystemtcp; //local copy of data structure that will be sent to the control system

        printf("starting thread\n");
 
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


	while(1)
	{
		if(listen(sock,5)<0) //allows 5 request
			error("Error on listening\n");
		cliLen=sizeof(cliAddr);
		childfd=accept(sock,(struct sockaddr *) &cliAddr,&cliLen);
		if(childfd <0)
			error("Error on Accept\n");

	
	 	hostp = (struct hostent*)gethostbyaddr((const char *)&cliAddr.sin_addr.s_addr,sizeof(cliAddr.sin_addr.s_addr), AF_INET);

  	  	if (hostp == NULL)
     	 		error("ERROR on gethostbyaddr");
   	 	hostaddrp = inet_ntoa(cliAddr.sin_addr);
   	 	if (hostaddrp == NULL){
   		error("ERROR on inet_ntoa\n");
			}
			
                n=1;
		while(n>0){//while we're still able to send/receive data from the control system keep the tcp connection open
			gettimeofday(&t1,NULL);
			timediff=t1.tv_sec-t2.tv_sec;
			timediff_usec=t1.tv_usec-t2.tv_usec;
			tottimediff=timediff+timediff_usec/1000000;
		 //	printf("server established connection with %s (%s) %u\n",hostp->h_name, hostaddrp,PORT);

			  //first clear the incoming structure and then receive the binary data into it!
			n=1;
			n = recv(childfd, buf, 1024,0);
			if (n < 0) 
				error("ERROR reading from socket");
			printf("server received %d bytes: %s", n, buf);
			cb_pop_front10(bufferptr, &controlSystemtcp);//get the packet from 10 integrations ago to send to the control system
			n=send(childfd, &controlSystemtcp,sizeof(controlSystemtcp),0); 
			printf("Sent %u bytes\n",n);       
		}

		printf("server closing connection with control system\n");

	  	close(childfd);//close the connection if necessary
	}


}





void *packROACHpacket_thread(void *arg){
		
        struct UDPCBASSpkt pkt;
        struct timeval t1,t2;
	double timediff,timediff_usec,tottimediff;
	char path_acc_cnt[128];			// Path to acc_num counter data
<<<<<<< tcpserverThreadRoach.c
	char path_acc_len[128];			// Path to acc_num counter data
	char path_cnt_rst[128];			// Path to acc_num counter data
	char path_sync_en[128];			// Path to acc_num counter data
	char path_ctrl_sw[128];			// Path to acc_num counter data
	char path_timeStamp[128];			// Path to acc_num counter data
	char path_channel0even[128];			// Path to acc_num counter data
	int writeVal;
=======
>>>>>>> 1.1
	unsigned int i;
	FILE *fp;						// File pointer
	extern char *job[5];
        int acc_old;
        int acc_new;
        long timeStamp;
	extern	struct UDPCBASSpkt cbassPKT;
	sprintf(path_acc_cnt, "/proc/%s/hw/ioreg/acc_cnt", job[1]);
<<<<<<< tcpserverThreadRoach.c
	sprintf(path_acc_len, "/proc/%s/hw/ioreg/acc_len", job[1]);
	sprintf(path_channel0even, "/proc/%s/hw/ioreg/Subsystem_ch1_lsb", job[1]);
	sprintf(path_cnt_rst, "/proc/%s/hw/ioreg/cnt_rst", job[1]);
	sprintf(path_sync_en, "/proc/%s/hw/ioreg/sync_en", job[1]);
	sprintf(path_timeStamp, "/proc/%s/hw/ioreg/timeStamp", job[1]);
	sprintf(path_ctrl_sw, "/proc/%s/hw/ioreg/ctrl_sw", job[1]);
=======
>>>>>>> 1.1
	printf("Acc Counter: %s \n",path_acc_cnt);
	extern struct UDPCBASSpkt *PKTptr;
	extern struct circular_buffer buffer;
	extern struct circular_buffer* bufferptr;
	int packedDataCounter;//counter to keep track of the number of integrations

	bufferptr=&buffer;
	cb_init(bufferptr,50,sizeof(struct UDPCBASSpkt)); //allocate space for ten data packets for the ring buffer
<<<<<<< tcpserverThreadRoach.c


	//initialise registers on the ROACH//////	
	fp = fopen(path_acc_len, "w"); 
	writeVal=78125;
	fwrite(&writeVal, 4, 1, fp);
	fclose(fp);
	
	fp = fopen(path_cnt_rst, "w"); 
	writeVal=0;
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
	printf("herexxxx");	
	/////////////////////////////
=======
	
		
	
>>>>>>> 1.1
	
	for(i=0;i<320;i++){
		cbassPKT.data_ch0odd[i]=i*10;
		cbassPKT.data_ch0even[i]=i*10;
		cbassPKT.data_ch1odd[i]=i;
		cbassPKT.data_ch1even[i]=i;
		cbassPKT.data_ch2odd[i]=i;
		cbassPKT.data_ch2even[i]=i;
		cbassPKT.data_ch3odd[i]=i;
		cbassPKT.data_ch3even[i]=i;
		cbassPKT.data_ch4odd[i]=i;
		cbassPKT.data_ch4even[i]=i;
		cbassPKT.data_ch5odd[i]=i;
		cbassPKT.data_ch5even[i]=i;

	}
	encodeNetwork(&cbassPKT);
	acc_old=0;
	acc_new=0;
	packedDataCounter=0;
	timeStamp=0;
	while(1){
		packedDataCounter++;
		if(packedDataCounter==1){
	//		cbassPKT.tstart[packedDataCounter]=timeStamp;//packet timestamp
		}	
		if(packedDataCounter==10){
			packedDataCounter=0;
			cbassPKT.tend=t1.tv_usec;
			cbassPKT.dataCount=(int)acc_new;
			cb_push_back(bufferptr,&cbassPKT); //add the last packet to the circular buffer	
<<<<<<< tcpserverThreadRoach.c
			printf("acc count %d %d \n",timeStamp,acc_new);
			//printf("Time Diff %d %d %d %d %f\n",t1.tv_sec,t1.tv_usec,t2.tv_sec,t2.tv_usec,tottimediff);
=======
			printf("acc count %d %d \n",acc_new,sizeof(unsigned int));
			printf("Time Diff %d %d %d %d %f\n",t1.tv_sec,t1.tv_usec,t2.tv_sec,t2.tv_usec,tottimediff);
>>>>>>> 1.1
		}
	//FIRST WAIT FOR THE ROACH BEFORE SENDING ANY DATA
		gettimeofday(&t2,NULL);
		while(acc_new==acc_old){
			fp = fopen(path_acc_cnt, "r"); 
			fread(&acc_new, 4, 1, fp);
		//	printf("%ld\n",acc_new);
			fclose(fp);
<<<<<<< tcpserverThreadRoach.c
		//	fp = fopen(path_timeStamp, "r"); 
		//	fread(&timeStamp, 4, 1, fp);
			//printf("%ld\n",timeStamp);
			
		//	fclose(fp);
			//printf("%d %d\n",acc_new,acc_old);
=======
//			printf("%d %d\n",acc_new,acc_old);
>>>>>>> 1.1
			}
		
		
		gettimeofday(&t1,NULL);
		timediff=t1.tv_sec-t2.tv_sec;
		timediff_usec=t1.tv_usec-t2.tv_usec;
		tottimediff=timediff+timediff_usec/1000000;
		acc_old = acc_new;


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


	}

	//printf("encode %04x\n",pkt->data_ch0odd[300]);
	
};
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

	job[1]=argv[1];//global variable for access in the loop
	printf("Here starting thread\n");
 	r = pthread_attr_init(&tattr);
	pthread_create(&testThread_id,NULL,test_thread,NULL);
	pthread_create(&packROACHpacket_thread_id,NULL,packROACHpacket_thread,NULL);

        printf("Here started thread\n");




        while(1){
	sleep(1);
	
	}
} 
