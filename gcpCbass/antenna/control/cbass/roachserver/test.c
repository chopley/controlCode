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

#define PORT 12345

#define TRY do{ jmp_buf ex_buf__; if( !setjmp(ex_buf__) ){
#define CATCH } else {
#define ETRY } }while(0)
#define THROW longjmp(ex_buf__, 1)


int main(int argc, char *argv[])
{
        int sock, connected, bytes_recieved , true = 1,r; 
        printf("Here started thread\n");
	int lsb,msb;
	int combined;
	int shift=4;
	unsigned int test;
	msb=4;
	lsb=0xffffffff;
	test=(unsigned int)lsb;
	combined=(msb<<shift)|(test>>(32-shift));
	printf("%09x,%09x,%09x\n",msb,test>>2,combined);



} 
