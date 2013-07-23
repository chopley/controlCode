/*****************************************************************************/
/* This is a simple terminal program for controlling the PMAC motion
   controller from Sun/UNIX. It reads and send keyboard commands,
   allows to download a file into the PMAC, and sends control codes. 
   Linux Serial Programming HOWTO used for hints. 
   Maybe there should be checksuming - see <CONTROL-N> entry in manual. */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>   /* Provides open */
#include <termios.h> /* Provides tcgetattr, tcsetattr etc */
#include <unistd.h>  /* Provides read and write */
#include <errno.h>   /* Provides error decoding */
#include <string.h>

#define READLINE_LIBRARY
#include <readline.h>
#include <history.h>

#define DEV       "/dev/ttyS0"
#define BAUDRATE  B38400
#include <iostream>

int fd;
struct termios oldtio,newtio,checktio;
char tx[256], rx[256];

/*****************************************************************************/
void reset_port (void)
{
  tcsetattr (fd, TCSANOW, &oldtio);
  printf("Serial port is restored to initial state\n");
}

/*****************************************************************************/
int downloadprog(char* name)
{
  FILE *prog;
  char *sza_dir,filepath[256];
  
  sprintf(filepath, "%s", name);
  
  prog=fopen(filepath,"r");
  if(prog==0)
  {
    perror(filepath);
    return(-1);
  }
  
  while(1)
  {
    int  ntx,i,linestart;

    /* Get a line of prog */
    fgets(tx,255,prog);
    if(feof(prog))
      break;

    ntx=strlen(tx);

    /* Truncate comments to speed download */
    for(i=0;i<ntx;i++)
      if(tx[i]==';')
	ntx=i+1;
    tx[ntx-1]= '\r';  /* Terminate with \r (not \n) */

    /* Strip off leading spaces */
    for(linestart=0;linestart<ntx-1;linestart++)
      if(tx[linestart]!=32)
	break;
    ntx-=linestart;

    /* Don't send lines with just a \r */
    if(ntx<2)
      continue;

    /* Send to PMAC */
    ntx=write(fd,&tx[linestart],ntx);

    /* Print to screen */
    tx[ntx-1+linestart]= '\0';
    printf("ntx=%02d %s\n",ntx,&tx[linestart]);

    /* If no response command good - timeout after 0.1 sec */
    read(fd,rx,1);
    if(rx[0]=='\a')
    {
      printf("Error in program!\n");
      rx[read(fd,rx,255)]=0;   /* Read and terminate string */
      printf("%s",rx);   /* Print it */     
      return(-1);
    }
  }
  return(0);
}

/*****************************************************************************/
int main (void)
{
  int ntx,nbyte;

  printf("\nPMAC TERMINAL PROGRAM\n\n");
  printf("To get out type exit<enter>\n");
  printf("To download PMAC code type dl<enter>\n");
  printf("To send a control code type ^X<enter>\n\n");

  /* Get file descriptor for serial port */
  fd=open(DEV,O_RDWR);
  if (fd<0) {
    perror(DEV);
    return 1;
  }

  /* Get the existing port state and setup auto restore on exit */
  if(tcgetattr(fd,&oldtio)<0)
    printf("tcgetattr error: %s\n",strerror(errno));
  atexit (reset_port); 

  bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */

  /* Input flags -
     ICRNL   : map CR to NL */
  newtio.c_iflag = ICRNL;

  /* Output flags */
  newtio.c_oflag = 0;

  /* Control flags -
     BAUDRATE: Set bps rate.
     CS8     : 8n1 (8bit,no parity,1 stopbit)
     CREAD   : enable receiving character
     For some reason if use CRTSCTS to request output hw flow control
     read after write hangs system!!! */
  newtio.c_cflag = BAUDRATE | CS8 | CREAD;

  /* Local flags. Use non-canonical input mode - see man termio */
  newtio.c_lflag = 0;

  /* Set timeout for read */
  newtio.c_cc[VMIN]=0;   /* No minimum number of char */
  newtio.c_cc[VTIME]=1;  /* Timeout - unit 0.1 sec */

  /* now clean the modem line and activate the settings for the port */
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);
  printf("Serial port is configured\n");

  /* Setup PMAC comms */
  printf("Setup PMAC response i var\n");
  sprintf(tx,"i3=1 i6=1\r");
  nbyte=write(fd,tx,strlen(tx));
  
  /* Try and read response */
  nbyte=read(fd,rx,1);
  if(nbyte==0)
  {
    printf("No response from PMAC!\n");
    printf("Check the serial comm and try again\n");
    exit(1);
  }
  else
    printf("PMAC responds\n\n");

  /* Main loop */
  while(1)
  {
    int com_code;
    char *line_read = (char *) NULL;
    char* name=NULL, *tok=NULL;
    char cpy[256];

    /* Get a commmand using readline */
    line_read = readline("PMAC:> ");
    if (*line_read)
      add_history(line_read);
    strncpy(tx, line_read, 255);
    strncpy(cpy, line_read, 255);
    free(line_read);

    /* Catch special commands */
    com_code=0;
    tok = strtok(cpy," ");

    if(strstr(tok,"exit")!=0) {
      com_code=1;
    }

    if(strstr(tok,"dl")!=0) {
      com_code=2;
      name = strtok(NULL,"");
    }

    switch(com_code)
    {
    case 1:
      exit(0);
      break;
    case 2:
      downloadprog(name);
      break;
    default:
      /* Terminate with \r */
      ntx=strlen(tx);
      tx[ntx]= '\r';
      ntx++;
      
      /* Catch ^X\r control sequence and turn into a <CNTRL-X> char */
      if(tx[0]=='^')
      {
	if(tx[1]>=97)  /* Lower case */
	  tx[0]=tx[1]-96;
	else          /* Upper case */
	  tx[0]=tx[1]-64;
	tx[1]='\r';   /* According to <CNTRL-M> desciption in manual
			 <CR> is not needed for <CNTRL-X> commands */
	ntx=2;
      }

      /* Send to PMAC */
      write(fd,tx,ntx);
      
      while(1)  /* keep reading till read times out */
      {
	nbyte=read(fd,rx,255);
	if(nbyte==0) break;
	rx[nbyte]=0;
	printf("%s",rx);
      }
    }
  }
}

/*****************************************************************************/
