#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h> 
#include "Data.h"

#define PORT "9999"   // port we're listening on
#define MAX_MESSAGE_SIZE 50 // Maximum message size

pthread_mutex_t mutexSync = PTHREAD_MUTEX_INITIALIZER;

int signal =0;
char Buffer[1024];

int Initialize(void);
void *Communicate(void *);
void *get_in_addr(struct sockaddr *sa);
void AcceptNewClients(fd_set &master , int &fdmax ,int listener);
void AcceptDataFromClients(int iClient , fd_set & master);
void SendHeartBeats(int iListener , int iClient);
void SendWorkForClients( int fdmax , int iListener , int iClient , int &signal);



//****************************************************************************************************
int main(void)
{
	pthread_t t;
	// char Buffer[256];
	int iListenSockId = Initialize();
	//Communicate(&iListenSockId);
	pthread_create(&t , NULL, Communicate , (void *)&iListenSockId); 
	while(1)
	{
		printf("Enter a message to Send :: ");
		fgets(Buffer , 255 , stdin);
		pthread_mutex_lock(&mutexSync);
		signal++;
		//printf("Message is %s %d \n", Buffer , signal);
		pthread_mutex_unlock(&mutexSync);
		sleep(1);
		//memset(&Buffer[0], 0, sizeof(Buffer));
			
	}
	
    return 0;
}

//****************************************************************************************************
int Initialize()
{
	int listener ;     // listening socket descriptor
	int rv ;
	struct addrinfo hints, *ai, *p;
	int yes=1;        // for setsockopt() 
	 // get us a socket and bind it
    
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) 
	{
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) 
	{
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) 
		{ 
            continue;
        }       
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) 
		{
            close(listener);
            continue;
        }
        break;
    }
    // if we got here, it means we didn't get bound
    if (p == NULL) 
	{
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai); // all done with this
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
	
	return listener;

}

//****************************************************************************************************
void * Communicate(void * id)
{
	int *iSockID = (int *) id;
	int listener =  *iSockID;

    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select() read
    int fdmax;        // maximum file descriptor number

    int i, j, rv;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
    // add the listener to the master set
    FD_SET(listener, &master);
	printf("Listener is %d \n" , listener);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
	//accept 3 clients

	
    // main loop
    for(;;) {
		memcpy(&read_fds , &master , sizeof(master));
		struct timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		int iResult = select(fdmax+1, &read_fds, NULL, NULL, &tv) ;
        if (iResult == -1) 
		{
            perror("select");
            exit(4);
        }
	
		// ADD NEW CONNECTIONS READ FROM CONNECTIONS	
		for(i = 0; i <= fdmax; i++)
		{			   
            if (FD_ISSET(i, &read_fds)) 
			{			
				if (i == listener) 
				{                                    
					AcceptNewClients(master , fdmax , listener );	
                } else 
				{
					AcceptDataFromClients(i , master);
                } 
            } 

			
        } 
        for(i = 0; i <= fdmax; i++) 
		{			
			//send work for clients
			SendWorkForClients(fdmax , listener , i , signal);
			//sending heart beats
			SendHeartBeats(listener , i );
		}

    } 
	return 0;
}

//****************************************************************************************************
void SendWorkForClients(int fdmax , int iListener , int iClient , int &signal)
{

		if(signal == 1 && iClient != iListener && iClient > iListener)
		{
			//send data to some client;
			//printf("data send to CLIENT DUMMY \n");
			//signal--;
			ServerData * data = new ServerData();
			data->iType = 2;
			strcpy(data->cMessage , Buffer );
			int numbytes  = write( iClient , data ,sizeof(*data));
			printf("Number of bytes written :: %d ID :: %d  \n" , numbytes , iClient);
		
			delete data;
		}
		if(signal == 1 && iClient != iListener && iClient > iListener && iClient == fdmax)
		{
			pthread_mutex_lock(&mutexSync);
			signal--;
			pthread_mutex_unlock(&mutexSync);
		}
	
}

//****************************************************************************************************
void SendHeartBeats(int iListener , int iClient)
{
	ServerData * data = new ServerData();
	time_t rawtime;
	time ( &rawtime );

	data->iType = 1;
	strcpy(data->cMessage , ctime (&rawtime)  );
	//char cHeartBeat []  = "HEARTBEAT";


		if(iClient!= iListener && iClient > iListener){
			int numbytes  = write( iClient , data , sizeof(*data)); 
			//printf("Number of bytes written :: %d ID :: %d  \n" , numbytes , i);
		}					
	
	delete data;
}
//****************************************************************************************************
void AcceptNewClients(fd_set&  master , int& fdmax ,int listener)
{
		socklen_t addrlen;
		struct sockaddr_storage remoteaddr; // client address
		char remoteIP[INET6_ADDRSTRLEN];		
		int newfd;        // newly accept()ed socket descriptor
		addrlen = sizeof(remoteaddr);
		ServerData * data = new ServerData();
		data->iType = 2;
		strcpy(data->cMessage , "SERVER MESSAGE :: ACCEPTED" );
        newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
		write(newfd , data , sizeof(*data));
		if (newfd == -1) 
		{
            perror("accept");
        }else 
		{
            FD_SET(newfd, &master); // add to master set
            if (newfd > fdmax) 
			{    // keep track of the max
				fdmax = newfd;
            }
            printf("selectserver: new connection from %s on socket %d\n", inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN), newfd);
        } 
		
		delete data;
}

//****************************************************************************************************
void AcceptDataFromClients(int iClient , fd_set& master)
{
	char buf[1024];    // buffer for client data
	int nbytes;
	
	nbytes = recv(iClient, buf, sizeof buf, 0);
	//printf("DISCONNECTING  %d \n", nbytes);
    if (nbytes <= 0) 
	{
    // got error or connection closed by client
		if (nbytes == 0) 
		{
			// connection closed
			printf("selectserver: socket %d hung up\n", iClient);
		} else 
		{
			perror("recv");
		}
        close(iClient); // bye!
        FD_CLR(iClient, &master); // remove from master set
    } else 
	{
    // we got some data from a client
		printf("CLIENT DATA :: %s",buf);
                       
    }

}

//****************************************************************************************************
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
	{
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
