#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "Data.h"


int main(int argc , char * argv [])
{
	int SockFD , PortNo , n , fdmax ,nbytes;
	struct sockaddr_in Server_Address;
	struct hostent *Server;
	fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
	FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	char Buffer[1024];
	memset(Buffer, 0, sizeof(Buffer));

	if(argc < 3)
	{
		printf("Error hostname port required \n");
		exit(0);
	}

	PortNo = atoi(argv[2]);

	// create a socket	
	SockFD =  socket(AF_INET , SOCK_STREAM , 0);

	if(SockFD < 0 )
	{
		perror("Error Creating Socket");
		exit(1);
	}
	
	Server = (struct hostent *) gethostbyname(argv[1]);
	if(Server == NULL)
	{
		printf("Error :: No Such Host \n");
		exit(0);
	}

	bzero((char *) &Server_Address , sizeof(Server_Address));
	Server_Address.sin_family =  AF_INET;
	bcopy((char *) Server->h_addr , (char *) &Server_Address.sin_addr.s_addr , Server->h_length);
	
	Server_Address.sin_port = htons(PortNo);
	
	//connect to the server
	int listener = connect(SockFD , (struct sockaddr *) &Server_Address , sizeof(Server_Address));
	if( listener < 0)
	{
		perror("Error Connecting \n");
		exit(1);
	}
	

	while(1)
	{	
		n = read(SockFD , Buffer , sizeof(Buffer));
		ServerData * data = new ServerData();
		data = (ServerData *) Buffer;
		
		if(n <= 0)
		{
			perror("Error receiving data \n");
			exit(1);
		}
		//int result = strcmp( Buffer, "HEARTBEAT" );
		if(data->iType == 2)
		{
			//sleep(1);	
			printf("Received :: %s", data->cMessage);
			write(SockFD , "CLIENT MESSAGE-->ACCEPTED\n" ,26);
		}	
		else if(data->iType == 1 )
		{
			//sleep(2);	
			printf("Heart Beating Server::  %s", data->cMessage);
		}
		memset(Buffer, 0, sizeof(Buffer));
		//
		
	}

	return 0;

}
