#include <stdio.h>      /* Contains common I/O functions */
#include <sys/types.h>  /* Contains definitions of data types used in system calls */
#include <sys/socket.h> /* Includes definitions of structures needed for sockets */
#include <netinet/in.h> /* Contains constants and structures needed for internet domain addresses. */
#include <unistd.h>     /* Contains standard unix functions */
#include <stdlib.h>     /* For atoi() and exit() */
#include <string.h> 	/* For memset() */
#include <time.h>       /* For retrieving the time */
#include <limits.h>	/* For limits */
#include "TCPLib.h"     /* For tcp_recv, tcp_recv_size, tcp_send_size, and tcp_send */


/* The maximum size of the file chunk */
#define MAX_FILE_CHUNK_SIZE 100

#define MAXLINE 4096

/**
 * Returns the smallest of the two integers
 * @param fInt - the first integer
 * @param sInt - the second integer
 * @return - the smallest of the two integers
 */
int min(const int& fInt, const int& sInt)
{
	/* Find the smallest integer */
	if(fInt < sInt) return fInt;
	
	return sInt;
}

int data_socket(int port)
{
	int listenfd = -1;
	sockaddr_in serverAddr;

	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		perror("Data socket failed. Server side.");
		exit(-1);
	}

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_port = htons(port);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(listenfd, (sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("bind for data socket fail. Server side.");
		exit(-1);
	}

	if(listen(listenfd, 1) < 0)
	{
		perror("listen failed for data socket. Server side.");
		exit(-1);
	}
	return(listenfd);
}

int setup_connection(int socket)
{
	int connfd = -1;
	sockaddr_in cliAddr;
	socklen_t cliLen = sizeof(cliAddr);

	if((connfd = accept(socket, (sockaddr *)&cliAddr, &cliLen)) < 0)
	{
		perror("accept failed for data_socket. Server side");
		exit(-1);
	}
	return(connfd);
}

/**
 * Receives the file name from the specified socket
 * @param socket - the socket
 * @param fileName - the file name
 */
void recvFileName(const int& socket, char* fileName)
{
	
	/* The file name size */
	int fileNameSize = -1;
	
	/* Get the size of the file name */
	if((fileNameSize = tcp_recv_size(socket)) < 0)
	{
		perror("tcp_recv_size");
		exit(-1);
	}
	
	/* Get the file name */	
	if(tcp_recv(socket, fileName, fileNameSize) < 0)
	{
		perror("tcp_recv");
		exit(-1);
	}
	
	/* NULL terminate the file name */
	fileName[fileNameSize] = '\0';	
}

/* The maximum file name size */
#define MAX_FILE_NAME_SIZE 100

int main(int argc, char** argv)
{
	/* The port number */	
	int port = -1;
	
	/* The integer to store the file descriptor number
	 * which will represent a socket on which the server
	 * will be listening
	 */
	int listenfd = -1;
	
	/* The file descriptor representing the connection to the client */
	int connfd = -1;

	/* Varible for checking validity of command*/
	int checker = -1;

	/* The size of the time buffer */
	#define TIME_BUFF_SIZE 100
	
	/* The buffer used for receiving data from the server */
	char recvBuff[MAX_FILE_CHUNK_SIZE];
	
	/* The  */
	char fileName[MAX_FILE_NAME_SIZE];

	/* A buffer for storing command coming from client*/
	char tcp_recvbuff[MAXLINE];

	/* The file size */
	int fileSize = 0;
	
	/* The number of bytes received */
	int numRead = 0;	
		
	/* The pointer to the output file */
	FILE* fp;
			
	/* The structures representing the server and client
	 * addresses respectively
	 */
	sockaddr_in serverAddr, cliAddr;
	
	/* Stores the size of the client's address */
	socklen_t cliLen = sizeof(cliAddr);	
	
	/* Make sure the user has provided the port number to
	 * listen on
	 */
	if(argc != 2)
	{
		/* Report an error */
		fprintf(stderr, "USAGE: %s <PORT #>", argv[0]);
		exit(-1);	
	}
	
	/* Get the port number */
	port = atoi(argv[1]);
	
	/* Make sure that the port is within a valid range */
	if(port < 0 || port > 65535)	
	{
		fprintf(stderr, "Invalid port number\n");
		exit(-1);
	} 
	
	/* Print the port number */	
	fprintf(stderr, "Port  = %d\n", port); 	
	
	/* Create a socket that uses
	 * IPV4 addressing scheme (AF_INET),
	 * Supports reliable data transfer (SOCK_STREAM),
	 * and choose the default protocol that provides
	 * reliable service (i.e. 0); usually TCP
	 */
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		exit(-1);
	}
	
	
	/* Set the structure to all zeros */
	memset(&serverAddr, 0, sizeof(serverAddr));
		
	/* Convert the port number to network representation */	
	serverAddr.sin_port = htons(port);
	
	/* Set the server family */
	serverAddr.sin_family = AF_INET;
	
	/* Retrieve packets without having to know your IP address,
	 * and retrieve packets from all network interfaces if the
	 * machine has multiple ones
	 */
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	/* Associate the address with the socket */
	if(bind(listenfd, (sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("bind");
		exit(-1);
	}
	
	/* Listen for connections on socket listenfd.
	 * allow no more than 100 pending clients.
	 */
	if(listen(listenfd, 100) < 0)
	{
		perror("listen");
		exit(-1);
	}
	
	/* Wait forever for connections to come */
	while(true)
	{
		
		fprintf(stderr, "Waiting for somebody to connect on port %d\n", port);
				
		/* A structure to store the client address */
		if((connfd = accept(listenfd, (sockaddr *)&cliAddr, &cliLen)) < 0)
		{
			perror("accept");
			exit(-1);
		}
		
			
		fprintf(stderr, "Connected!\n");

		int data_port = 1024;
		while((checker = tcp_recv(connfd, tcp_recvbuff, MAXLINE)) > 0){
			printf("Command recived from client: %s", tcp_recvbuff);
			char *token, *dummy;
			dummy = tcp_recvbuff;
			token = strtok(dummy, " "); // token is the file given after the command

			if(strcmp("quit\n", tcp_recvbuff) == 0){
                printf("The client has quit\n");
                close(connfd);
                return 0;
            }
			else if (strcmp("get",token)==0)  {
				char port[MAXLINE],buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE];
				int datasock,lSize,num_blks,num_last_blk,i;
				FILE *fp;
				token=strtok(NULL," \n");
				printf("Downloading: %s\n ", token);
				data_port=data_port+1;//int dataport
				if(data_port==atoi(argv[1])){
					data_port=data_port+1;
				}
				sprintf(port,"%d",data_port);
				datasock=data_socket(data_port);				//creating socket for data connection
				tcp_send(connfd, port,MAXLINE);					//sending port no. to client
				datasock=setup_connection(datasock);					//client acepts connection
				if ((fp=fopen(token,"r"))!=NULL)
				{
					//size of file
					tcp_send(connfd,"1",MAXLINE);
					fseek (fp , 0 , SEEK_END);
					lSize = ftell (fp);
					rewind (fp);
					num_blks = lSize/MAXLINE;
					num_last_blk = lSize%MAXLINE; 
					sprintf(char_num_blks,"%d",num_blks);
					tcp_send(connfd, char_num_blks, MAXLINE);
					//cout<<num_blks<<"	"<<num_last_blk<<endl;

					for(i= 0; i < num_blks; i++) { 
						fread (buffer,sizeof(char),MAXLINE,fp);
						tcp_send(datasock, buffer, MAXLINE);
						//cout<<buffer<<"	"<<i<<endl;
					}
					sprintf(char_num_last_blk,"%d",num_last_blk);
					tcp_send(connfd, char_num_last_blk, MAXLINE);
					if (num_last_blk > 0) { 
						fread (buffer,sizeof(char),num_last_blk,fp);
						tcp_send(datasock, buffer, MAXLINE);
						//cout<<buffer<<endl;
					}
					fclose(fp);
					printf("Donwload complete!.\n");
		
				}
				else{
					tcp_send(connfd,"0",MAXLINE);
				}
   			}
			else if(strcmp("put", token) == 0){
				char port[MAXLINE],buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE],check[MAXLINE];
				int datasock,num_blks,num_last_blk,i;
				FILE *fp;
				token=strtok(NULL," \n");
				printf("Uploading: %s\n", token);
				data_port=data_port+1;
				if(data_port==atoi(argv[1])){
					data_port=data_port+1;
				}
				sprintf(port,"%d",data_port);
				datasock=data_socket(data_port);				//creating socket for data connection
				tcp_send(connfd, port,MAXLINE);					//sending port to client
				datasock=setup_connection(datasock);					//accept the connection
				tcp_recv(connfd,check,MAXLINE);
				printf("%s",check); 
				if(strcmp("1",check)==0){
					if((fp=fopen(token,"w"))==NULL)
						printf("Error creating file\n");
					else
					{
						tcp_recv(connfd, char_num_blks,MAXLINE);
						num_blks=atoi(char_num_blks);
						for(i= 0; i < num_blks; i++) { 
							tcp_recv(datasock, buffer, MAXLINE);
							fwrite(buffer,sizeof(char),MAXLINE,fp);
						}
						tcp_recv(connfd, char_num_last_blk, MAXLINE);
						num_last_blk=atoi(char_num_last_blk);
						if (num_last_blk > 0) { 
							tcp_recv(datasock, buffer, MAXLINE);
							fwrite(buffer,sizeof(char),num_last_blk,fp);
						}
						fclose(fp);
						printf("File upload complete!.\n");
					}
				}
			}
			else if (strcmp("ls\n",tcp_recvbuff) == 0){
				FILE *in;
				char temp[MAXLINE],port[MAXLINE];
				int datasock;
				unsigned Data_Size = 1;
				data_port=data_port+1;
				if(data_port==atoi(argv[1])){
					data_port=data_port+1;
				}
				sprintf(port,"%d",data_port);
				datasock=data_socket(data_port);			//creating socket for data connection
				tcp_send(connfd, port,MAXLINE);				//sending data connection port no. to client
				datasock=setup_connection(datasock);	 			//accepting connection from client
				if(!(in = popen("ls", "r"))){
					printf("error\n");
				}
				while(fgets(temp, sizeof(temp), in)!=NULL){
					tcp_send_size(datasock, Data_Size);
					tcp_send(datasock, temp,MAXLINE);
				}
				Data_Size = 0;
				tcp_send_size(datasock, Data_Size);
				printf("Server Got out of the loop\n");
				pclose(in);
			}
			else{
				printf("You printed a fake command!\n");
			}
		}
	}	
		
		
	return 0;
}





