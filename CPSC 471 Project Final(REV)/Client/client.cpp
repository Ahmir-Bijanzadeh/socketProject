#include <stdio.h>      /* Contains common I/O functions */
#include <sys/types.h>  /* Contains definitions of data types used in system calls */
#include <sys/socket.h> /* Includes definitions of structures needed for sockets */
#include <netinet/in.h> /* Contains constants and structures needed for internet domain addresses. */
#include <unistd.h>     /* Contains standard unix functions */
#include <stdlib.h>     /* For atoi() and exit() */
#include <string.h> 	/* For memset() */
#include <arpa/inet.h>  /* For inet_pton() */
#include <time.h>       /* For retrieving the time */
#include <limits.h>	/* For the maximum integer size */
#include <sys/stat.h>   /* For stat() */
#include "TCPLib.h"     /* Defines tcp_recv, tcp_send, tcp_recv_size, tcp_recv_size */
#include <sys/sendfile.h> /* For sendfile() */
#include <fcntl.h>        /* For O_RDONLY */

/*Max Buffer size, used to not have a number that just reapears in our code*/
#define MAXBUFFER 4096

/**
 * Returns the file size 
 * @param fileName - the file name
 * @return - the corresponding file size
 */

int getFileSize(const char* fileName)
{
	/* The structure containing the file information */
	struct stat fileInfo;
	
	/* Get the file size */
	if(stat(fileName, &fileInfo) < 0)
		return -1;
	
	/* Return the file size */
	return fileInfo.st_size;
}

int data_socket(int port, char* addr)
{
	int connfd = -1;
	sockaddr_in serverAddr;

	if((connfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		perror("Data socket failed. Client side.");
		exit(-1);
	}

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(addr);

	if(connect(connfd, (sockaddr*)&serverAddr, sizeof(sockaddr)) < 0)
	{
		perror("connect failed for data socket. Client side.");
		exit(-1);
	}

	return(connfd);
}

/**
 * Transmits the file name
 * @param socket - the socket to send the file name over
 * @param fileName - the file name to send
 */
void sendFileInfo(const int& socket, const char* fileName)
{
	/* Get the size of the file name */
	int fileNameSize = strlen(fileName);
	
	/* Send the size of the file name */
	if(tcp_send_size(socket, fileNameSize) < 0)
	{
		perror("tcp_send_size");
		exit(-1);
	}
	
	/* Send the actual file name */	
	if(tcp_send(socket, fileName, fileNameSize) < 0)
	{
		perror("tcp_send");
		exit(-1);
	}
}

int main(int argc, char** argv)
{
	/* The port number */	
	int port = -1;
	
	/* The file descriptor representing the connection to the client */
	int connfd = -1;
	
	/* The number of bytes sent in one shot */
	int numSent = 0;
	
	/* The total number of bytes sent */
	off_t totalNumSent = 0;
	
	/* The size of the file name */
	int fileNameSize = -1;

	/* Stores commands from terminal*/
	char terminal_input[MAXBUFFER];
		
	/* The structures representing the server address */
	sockaddr_in serverAddr;
	
	/* Stores the size of the client's address */
	socklen_t servLen = sizeof(serverAddr);	
	
	
	/* Make sure the user has provided the port number to
	 * listen on
	 */
	if(argc != 3)
	{
		/* Report an error */
		fprintf(stderr, "USAGE: %s <SERVER IP> <SERVER PORT #>", argv[0]);
		exit(-1);	
	}
	
	/* Get the port number */
	port = atoi(argv[2]);
		
	/* Make sure that the port is within a valid range */
	if(port < 0 || port > 65535)	
	{
		fprintf(stderr, "Invalid port number\n");
		exit(-1);
	} 
	
	/* Connect to the server
	*  AF_ INET: IPv4 communication betwen processes on different hosts
	*  SOCK_STREAM: TCP 
	*  0 stands for using IP protocol
	*/
	if((connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		exit(-1);
	}
	
		
	/* Set the structure to all zeros */
	memset(&serverAddr, 0, sizeof(serverAddr));
		
	/* Set the server family */
	serverAddr.sin_family = AF_INET;
	
	/* Convert the port number to network representation */	
	serverAddr.sin_port = htons(port);
	
	
	/**
	 * Convert the IP from the presentation format (i.e. string)
	 * to the format in the serverAddr structure.
	 */
	if(!inet_pton(AF_INET, argv[1], &serverAddr.sin_addr))
	{
		perror("inet_pton");
		exit(-1);
	}
	
	
	/* Lets connect to the client. This call will return a socket used 
	 * used for communications between the server and the client.
	 */
	if(connect(connfd, (sockaddr*)&serverAddr, sizeof(sockaddr))<0)
	{
		perror("connect");
		exit(-1);
	}	
	
	printf("ftp>");

	while(fgets(terminal_input, MAXBUFFER, stdin) != NULL){
		tcp_send(connfd, terminal_input, MAXBUFFER); // Send command entered by user to server through control connection.
		char *token, *dummy;
		dummy = terminal_input;
		token = strtok(dummy, " "); // Used for separating command from file

		if(strcmp("quit\n", terminal_input) == 0){ // If quit is entered close control connection from client side and close program
                printf("The client has quit the session.\n");
                close(connfd);
                return 0;
            }
		else if(strcmp("get",token) == 0){
			int dataPort,datasock,streamSize,numBlocks,numLastblock,i;
			char port[MAXBUFFER], buffer[MAXBUFFER],char_numBlocks[MAXBUFFER],char_numLastblock[MAXBUFFER],message[MAXBUFFER];
			FILE *fp;
			tcp_recv(connfd, port, MAXBUFFER);
			dataPort=atoi(port);
			datasock=data_socket(dataPort,argv[1]);
			token=strtok(NULL," \n");
			tcp_recv(connfd,message,MAXBUFFER);
			if(strcmp("1",message)==0){
				if((fp=fopen(token,"w"))==NULL)
					printf("Error in creating file\n");
				else
				{
					tcp_recv(connfd, char_numBlocks, MAXBUFFER);
					numBlocks=atoi(char_numBlocks);
					for(i= 0; i < numBlocks; i++) { 
						tcp_recv(datasock, buffer, MAXBUFFER);
						fwrite(buffer,sizeof(char),MAXBUFFER,fp);
						//cout<<buffer<<endl;
					}
					tcp_recv(connfd, char_numLastblock, MAXBUFFER);
					numLastblock=atoi(char_numLastblock);
					if (numLastblock > 0) { 
						tcp_recv(datasock, buffer, MAXBUFFER);
						fwrite(buffer,sizeof(char),numLastblock,fp);
						//cout<<buffer<<endl;
					}
					fclose(fp);
					printf("File download complete.\n");
				}
			}
		}	
		else if(strcmp("put",token) == 0){
			int dataPort,datasock,streamSize,numBlocks,numLastblock,i;
			char port[MAXBUFFER], buffer[MAXBUFFER],char_numBlocks[MAXBUFFER],char_numLastblock[MAXBUFFER];
			FILE *fp;
			tcp_recv(connfd, port, MAXBUFFER);
			dataPort = atoi(port);
			datasock = data_socket(dataPort, argv[1]);
			token = strtok(NULL," \n");
			if((fp = fopen(token,"r")) != NULL){
				tcp_send(connfd, "1",  MAXBUFFER);
				fseek(fp, 0, SEEK_END);
				streamSize = ftell (fp);
				rewind (fp);
				numBlocks = streamSize/MAXBUFFER;
				numLastblock = streamSize%MAXBUFFER; 
				sprintf(char_numBlocks,"%d",numBlocks);
				tcp_send(connfd, char_numBlocks,  MAXBUFFER);

				for(i= 0; i < numBlocks; i++) { 
					fread (buffer,sizeof(char), MAXBUFFER,fp);
					tcp_send(datasock, buffer,  MAXBUFFER);
				}
				sprintf(char_numLastblock,"%d",numLastblock);
				tcp_send(connfd, char_numLastblock,  MAXBUFFER);
				if (numLastblock > 0) { 
					fread (buffer,sizeof(char),numLastblock,fp);
					tcp_send(datasock, buffer,  MAXBUFFER);
				}
				fclose(fp);
				printf("File upload complete.\n");
			}
		}
		else if (strcmp("ls\n", terminal_input) == 0){
			char buff[MAXBUFFER], port[MAXBUFFER]; // Buffers for checks and connection information
			int dataPort, datasock; // Variables for socket and port 
			unsigned int tempSize = 1;
			tcp_recv(connfd, port, MAXBUFFER); // Receiving the port from the server
			dataPort = atoi(port); // storing port in variable
			datasock = data_socket(dataPort, argv[1]); // setup new socket based on IP and port of server to set up data connection with server
			while(tempSize != 0){ // Continue to output buff until 
				tempSize = tcp_recv_size(datasock);
				if( tempSize == 0)
					break;
				tcp_recv(datasock, buff, MAXBUFFER);
				printf("%s", buff);
			}
			//printf("Client exited the loop\n");
		}
		else{
			printf("Invalid command! Check Command and try again.\n");
		}
		printf("ftp>");
	}
			
	return 0;
}





