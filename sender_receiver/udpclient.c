/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define SLEEP_VAL 5
#define BUFSIZE 1000
#define WIN_SZ 3

typedef struct packet{
	int seq;
	int size;
	char data[BUFSIZE];
}packet;


static int alarm_fired = 0;
void mysig(int sig)
{
	pid_t pid;
	printf("PARENT : Received signal %d \n", sig);
	if (sig == SIGALRM)
	{
		alarm_fired = 1;
	}
}



/*
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv)
{
    int sockfd, portno, n;
    int serverlen;
	int bytes_recv;
	int seq_to_be_matched[3];
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf1[BUFSIZE], buf_ack[BUFSIZE];

    int sz = sizeof(packet);
    if (argc != 4)
    {
       fprintf(stderr,"usage: %s <hostname> <port> <filename>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */


    printf("Done till here\n");
    bzero(buf1, BUFSIZE);
    int fd;
    if ((fd = open(argv[3],O_RDONLY))==-1)
    {
		perror("open fail");
		return EXIT_FAILURE;
	}

    int lastpack_sz = 0,bytes_sent;
    struct stat st;
    fstat(fd, &st);
    int size = st.st_size;
    printf("Size = %d\n",size);
    int chunks = size/1000;
    if(size > chunks*1000)
    {
        lastpack_sz = size-(chunks*1000);
        chunks += 1;
    }
    printf("Chunks = %d\n",chunks);

    char chunks_arr[10], size_arr[10];
    sprintf(size_arr,"%d",size);

    printf("%s\n",size_arr);

    sprintf(chunks_arr,"%d",chunks);
    printf("%s\n",chunks_arr);

    strcpy(buf1, argv[3]);
    buf1[strlen(argv[3])] = '|';
    strcat(buf1, size_arr);
    printf("buff after size_concat : %s\n",buf1);

    buf1[strlen(argv[3])+strlen(size_arr)+1] ='|';
    strcat(buf1,chunks_arr);
    printf("buff after chunks_concat : %s\n",buf1);

    buf1[strlen(argv[3])+strlen(size_arr)+strlen(chunks_arr)+2] = '\0';
    serverlen = sizeof(serveraddr);
    printf("Sending %s to server\n", buf1);


    bytes_sent = sendto(sockfd, buf1, strlen(buf1), 0, (struct sockaddr *)&serveraddr, serverlen);
    bzero(buf1,BUFSIZE);
    int sequence_number =0,chunks_transferred = 0;
	int temp_sz = 1000;
	int bs_ptr=-1;
	int ack_ptr = -1;
	int seq_received ;
	int f;
	int new_winsz, win_size=3;
    packet* buff[chunks];
    int z=0;
    for(z=0;z<chunks;z++)
    {
    	buff[z]=(packet*)malloc(1*sizeof(packet));
    	bzero(buff[z], sz);
	    buff[z]->seq = z;
		if(z != chunks-1)
        {
            buff[z]->size = 1000;
            n=read(fd,buff[z]->data,1000);
        }
        else
        {
            buff[z]->size = lastpack_sz;
            n= read(fd,buff[z]->data,lastpack_sz);
        }
    }

    for(z=0;z<chunks;z++)
    {
    	printf("seq[%d] = %d\n",z,buff[z]->seq);
    }
    while(ack_ptr != chunks-1)
    {
				
		
		int i=0;
		while(i<win_size)
		{
			bytes_sent = sendto(sockfd, buff[ack_ptr+1+i], sz, 0, (struct sockaddr *)&serveraddr, serverlen);
			printf("sending chunk %d\n",ack_ptr+1+i);
			printf("bytes_sent = %d\n",bytes_sent);
			if (bytes_sent < 0) error("ERROR in sendto");
			else bs_ptr++;
			i++;
		}


		while(bs_ptr-ack_ptr == win_size)
		{
			f=0;
			alarm(SLEEP_VAL);
			(void) signal(SIGALRM, mysig);
			do
			{	bzero(buf_ack,BUFSIZE);
				bytes_recv = recvfrom(sockfd, buf_ack, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
				printf("Received  ack\n");
				int initial_ack = ack_ptr;
				if(bytes_recv > 0 )
				{
					seq_received = atoi(buf_ack);
					int i=0;
					while(i<win_size)
					{
						if(seq_received == initial_ack+1+i);
						{
							printf("Ack matched with %d\n",initial_ack+1+i);
							ack_ptr = initial_ack + i+1;
							printf("Ack_ptr = %d\n",ack_ptr);
							f = 1;
							alarm_fired = 1;
							new_winsz = win_size + i+1;
						}
						i++;
					}	
				}

			}while(!alarm_fired);

			if(f == 0)
			{
				int i=0;
				win_size = win_size/2;
				bs_ptr = ack_ptr;
				while(i<win_size)
				{
					bytes_sent = sendto(sockfd, buff[i], sz, 0, (struct sockaddr *)&serveraddr, serverlen);
					bs_ptr++;
					i++;
				}
			}
		}	
    }
    return 0;
}
