/*
 * udpserver.c - A UDP echo server
 * usage: udpserver <port_for_server>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUFSIZE 1008


typedef struct packet{
  int seq;
  int size;
  char data[BUFSIZE];
}packet;

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket file descriptor - an ID to uniquely identify a socket by the application program */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n,m; /* message byte size */
  int drp_prob;
  /*
   * check command line arguments
   */
  if (argc != 3) {
    fprintf(stderr, "usage: %s <port_for_server> <drop probability>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);
  drp_prob = atoi(argv[2]);

  /*
   * socket: create the socket
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /*
   * bind: associate the parent socket with a port
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  /*
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  //while (1)
  //{

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);


    n = recvfrom(sockfd, buf, BUFSIZE, 0,(struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0) error("ERROR in recvfrom");
    printf("Reached here\n");
    printf("Contents of Buf : %s\n", buf);
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL) error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL) error("ERROR on inet_ntoa\n");
    //printf("hii i am here\n");
    printf("server received datagram from %s (%s)\n",hostp->h_name, hostaddrp);



    int i=0;
    int first =0, second, third;
    //char * strncpy ( char * destination, const char * source, size_t num );
    while(1)
    {
      if(buf[i] == '|' && first == 0) first = i;
      if(buf[i] == '|' && first !=0) second = i;
      if(buf[i] == '\0') {third = i;break;}
      i++;
    }
    //printf("%s\n",buf);
    printf("First : %d, Second : %d, third : %d\n",first,second,third);
    char filename[first+1];
    char size[second - first];
    char chunks[third - second];


    strncpy( filename, buf, first);
    filename[first] = '\0';
    printf("filename : %s\n",filename);

    strncpy ( size, buf+first+1, second-first-1);
    size[second-first] = '\0';
    printf("SIZE : %s\n",size);

    strncpy ( chunks, buf+second+1, third-second-1);
    chunks[third-second] = '\0';
    printf("CHUNKS : %s\n",chunks);

    int chunks_count = atoi(chunks);
    printf("chunks : %d\n",chunks_count);

    char fname[20]="new_";
    strcat(fname, filename);
    printf("Creating the copied output file : %s\n",fname);


    //printf("%s\n",fname);
    int fd,count =0;
    if((fd=open(fname,O_CREAT|O_WRONLY,0600))==-1)
    {
       perror("open fail");
       exit (3);
    }
    printf("File opened sucessfully\n");

  	int seq_received, seq_last_ack;
  	//int m;


   packet* buff = (packet*)malloc(1*sizeof(packet));
   int sz = sizeof(packet);
   bzero(buff, sz);
   int chunks_counter =0;
   printf("outside while \n");
   drp_prob=0;
   int drop_count = drp_prob*chunks_count, drop_counter =0;
   while(chunks_counter < chunks_count)
   {

      int k = rand()%2;
      if(k==0 && drop_counter < drop_count)
      {
        drop_counter++;
        continue;
      }
   	  bzero(buff,sz);
   	  n = recvfrom(sockfd, buff, sz, 0,(struct sockaddr *) &clientaddr, &clientlen);
      printf("Received %d\n",chunks_counter);
   	  seq_received = buff->seq;
      printf("sequence received is : %d\n",seq_received);
      if(chunks_counter > 0)
      {
        	if (seq_received == seq_last_ack+1)
        	{
            if((m=write(fd,buff->data,buff->size))==-1)
            {
               perror("write fail");
               exit (6);
            }
            count=count+m;
            bzero(buf,BUFSIZE);

            sprintf(buf,"%04d",seq_received);
          	m = sendto(sockfd, buf, strlen(buf), 0,(struct sockaddr *) &clientaddr, clientlen);
          	if(n < 0) perror("ERROR in sendto");
            else seq_last_ack = seq_received;
            chunks_counter++;
        	}

       		else
       		{

       			bzero(buf,BUFSIZE);
       			sprintf(buf,"%04d",seq_last_ack);
          	m = sendto(sockfd, buf, strlen(buf), 0,(struct sockaddr *) &clientaddr, clientlen);
          	if (n < 0) error("ERROR in sendto");
       		}
      }

      else
      {
         if((m=write(fd,buff->data,buff->size))==-1)
            {
               perror("write fail");
               exit (6);
            }

            printf("write once done\n");
            count=count+m;
            bzero(buf,BUFSIZE);

            sprintf(buf,"%04d",seq_received);
            m = sendto(sockfd, buf, strlen(buf), 0,(struct sockaddr *) &clientaddr, clientlen);
            if(n < 0) perror("ERROR in sendto");
            else seq_last_ack = seq_received;
            chunks_counter++;
      }

      printf("outside write once done\n");

   }

   close(fd);


   /*
   * gethostbyaddr: determine who sent the datagram
   */

   //printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

   /*
   * sendto: echo the input back to the client
   */
}
