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

#define MAXLINE 4096

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

int create_socket(int port,char *addr)
{
 int sockfd;
 struct sockaddr_in servaddr;


 //Create a socket for the client
 //If sockfd<0 there was an error in the creation of the socket
 if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
  printf("Problem in creating the socket\n");
  exit(2);
 }

 //Creation of the socket
 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr= inet_addr(addr);
 servaddr.sin_port =  htons(port); //convert to big-endian order

 //Connection of the client to the socket
 if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
  printf("Problem in creating data channel\n");
  exit(3);
 }

return(sockfd);
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
	char terminal_input[100];
		
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
	
	/* Get the file name */
	//const char* fileName = argv[3];
		
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

	while(fgets(terminal_input, 100, stdin) != NULL){
		tcp_send(connfd, terminal_input, 100); // Send command entered by user to server through control connection.
		char *token, *dummy;
		dummy = terminal_input; // Don't know what this is for yet
		token = strtok(dummy, " "); // Used for separating command from file

		/*if(strcmp("quit\n", terminal_input) == 0){ // If quit is entered close control connection from client side and close program
			printf("Quitting...\n");
			close(connfd);
			return 0;
		}*/
		if(strcmp("quit\n", terminal_input) == 0){
                printf("The client has quit\n");
                close(connfd);
                return 0;
            }
		else if(strcmp("put",token) == 0){
			int data_port,datasock,lSize,num_blks,num_last_blk,i;
			char port[MAXLINE], buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE];
			FILE *fp;
			tcp_recv(connfd, port, MAXLINE);
			data_port = atoi(port);
			datasock = create_socket(data_port, argv[1]);
			token = strtok(NULL," \n");
			if((fp = fopen(token,"r")) != NULL){
				tcp_send(connfd, "1",  MAXLINE);
				fseek(fp, 0, SEEK_END);
				lSize = ftell (fp);
				rewind (fp);
				num_blks = lSize/MAXLINE;
				num_last_blk = lSize%MAXLINE; 
				sprintf(char_num_blks,"%d",num_blks);
				tcp_send(connfd, char_num_blks,  MAXLINE);

				for(i= 0; i < num_blks; i++) { 
					fread (buffer,sizeof(char), MAXLINE,fp);
					tcp_send(datasock, buffer,  MAXLINE);
				}
				sprintf(char_num_last_blk,"%d",num_last_blk);
				tcp_send(connfd, char_num_last_blk,  MAXLINE);
				if (num_last_blk > 0) { 
					fread (buffer,sizeof(char),num_last_blk,fp);
					tcp_send(datasock, buffer,  MAXLINE);
				}
				fclose(fp);
				printf("File upload done.\n");
			}
		}
		else if (strcmp("ls\n", terminal_input) == 0){
			char buff[100], check[100] = "1", port[100]; // Buffers for checks and connection information
			int data_port, datasock; // Variables for socket and port 
			tcp_recv(connfd, port, 100); // Receiving the port from the server
			data_port = atoi(port); // storing port in variable
			datasock = create_socket(data_port, argv[1]); // setup new socket based on IP and port of server to set up data connection with server
			while(strcmp("1",check) == 0){ // if there is still data to recieve from server stay in loop
				tcp_recv(datasock,check,100);
				if(strcmp("0",check) == 0) // break when no data is left to recieve from server 
				break;
				tcp_recv(datasock, buff, 100); // store files and directories in this buffer
				printf("%s", buff); // Output files and directories here.
			}
		}
		else{
			printf("Error in command. Check Command\n");
		}
		printf("ftp>");
	}
			
	return 0;
}





