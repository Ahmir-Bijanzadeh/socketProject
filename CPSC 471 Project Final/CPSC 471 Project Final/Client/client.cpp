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

#define MAX_BUF_SIZE 4096

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

	/* The file size */
	int fileSize = -1;

	/* Stores commands from terminal*/
	char terminal_input[MAX_BUF_SIZE];
		
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

	while(fgets(terminal_input, MAX_BUF_SIZE, stdin) != NULL){
		tcp_send(connfd, terminal_input, MAX_BUF_SIZE); // Send command entered by user to server through control connection.
		char *token, *dummy;
		dummy = terminal_input;
		token = strtok(dummy, " "); // Used for separating command from file

		if(strcmp("quit\n", terminal_input) == 0){ // If quit is entered close control connection from client side and close program
                printf("The client has quit\n");
                close(connfd);
                return 0;
            }
		else if(strcmp("get",token) == 0){
			int data_port,datasock,lSize,num_blks,num_last_blk,i;
			char port[MAX_BUF_SIZE], buffer[MAX_BUF_SIZE],char_num_blks[MAX_BUF_SIZE],char_num_last_blk[MAX_BUF_SIZE],message[MAX_BUF_SIZE];
			FILE *fp;
			tcp_recv(connfd, port, MAX_BUF_SIZE);
			data_port=atoi(port);
			datasock=data_socket(data_port,argv[1]);
			token=strtok(NULL," \n");

			/* Get the file size */
			if((fileSize = tcp_recv_size(datasock)) < 0)
			{
				perror("Error in receiving file size. Server side");
				exit(-1);
			}

			printf("Downloading: %s (%d bytes)...\n", token, fileSize);

			tcp_recv(connfd,message,MAX_BUF_SIZE);
			if(strcmp("1",message)==0){
				if((fp=fopen(token,"w"))==NULL)
					printf("Error in creating file\n");
				else
				{
					tcp_recv(connfd, char_num_blks, MAX_BUF_SIZE);
					num_blks=atoi(char_num_blks);
					for(i= 0; i < num_blks; i++) { 
						tcp_recv(datasock, buffer, MAX_BUF_SIZE);
						fwrite(buffer,sizeof(char),MAX_BUF_SIZE,fp);
						//cout<<buffer<<endl;
					}
					tcp_recv(connfd, char_num_last_blk, MAX_BUF_SIZE);
					num_last_blk=atoi(char_num_last_blk);
					if (num_last_blk > 0) { 
						tcp_recv(datasock, buffer, MAX_BUF_SIZE);
						fwrite(buffer,sizeof(char),num_last_blk,fp);
						//cout<<buffer<<endl;
					}
					fclose(fp);
					printf("File download done.\n");
				}
			}
		}	
		else if(strcmp("put",token) == 0){
			int data_port,datasock,lSize,num_blks,num_last_blk,i;
			char port[MAX_BUF_SIZE], buffer[MAX_BUF_SIZE],char_num_blks[MAX_BUF_SIZE],char_num_last_blk[MAX_BUF_SIZE];
			FILE *fp;
			tcp_recv(connfd, port, MAX_BUF_SIZE);
			data_port = atoi(port);
			datasock = data_socket(data_port, argv[1]);
			token = strtok(NULL," \n");

			int fileSize = -1;
			fileSize = getFileSize(token);
			
			/* Did we successfully obtain the file size ? */
			if(fileSize < 0)
			{
				perror("Error in obtaining the file size. Client side");
				exit(-1);
			}
			/* Send the file size */
			if(tcp_send_size(datasock, fileSize) < 0)
			{
				perror("Error in sending file size. Client side");
				exit(-1);
			}
	
			if((fp = fopen(token,"r")) != NULL){
				tcp_send(connfd, "1",  MAX_BUF_SIZE);
				fseek(fp, 0, SEEK_END);
				lSize = ftell (fp);
				rewind (fp);
				num_blks = lSize/MAX_BUF_SIZE;
				num_last_blk = lSize%MAX_BUF_SIZE; 
				sprintf(char_num_blks,"%d",num_blks);
				tcp_send(connfd, char_num_blks,  MAX_BUF_SIZE);

				for(i= 0; i < num_blks; i++) { 
					fread (buffer,sizeof(char), MAX_BUF_SIZE,fp);
					tcp_send(datasock, buffer,  MAX_BUF_SIZE);
				}
				sprintf(char_num_last_blk,"%d",num_last_blk);
				tcp_send(connfd, char_num_last_blk,  MAX_BUF_SIZE);
				if (num_last_blk > 0) { 
					fread (buffer,sizeof(char),num_last_blk,fp);
					tcp_send(datasock, buffer,  MAX_BUF_SIZE);
				}
				fclose(fp);
				printf("File upload done.\n");
			}
		}
		else if (strcmp("ls\n", terminal_input) == 0){
			char buff[MAX_BUF_SIZE], port[MAX_BUF_SIZE]; // Buffers for checks and connection information
			int data_port, datasock; // Variables for socket and port 
			unsigned int temp_size = 1;
			tcp_recv(connfd, port, MAX_BUF_SIZE); // Receiving the port from the server
			data_port = atoi(port); // storing port in variable
			datasock = data_socket(data_port, argv[1]); // setup new socket based on IP and port of server to set up data connection with server
			while(temp_size != 0){ // Continue to output buff until 
				temp_size = tcp_recv_size(datasock);
				if( temp_size == 0)
					break;
				tcp_recv(datasock, buff, MAX_BUF_SIZE);
				printf("%s", buff);
			}
			printf("Client got out of the loop\n");
		}
		else{
			printf("Error in command. Check Command\n");
		}
		printf("ftp>");
	}
			
	return 0;
}





