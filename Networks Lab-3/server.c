#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include "encoding.c"

#define LEN 1024

void handler(int sockfd, struct sockaddr_in *c_address);   			//function to handle each client connection independently
//some predefined messages
char * ack = "ACK";
char * serv = "Connection Closed !!";

//for taking command line arguments
int main(int argc, char *argv[])
{
	if (argc < 2)  				//no. of arguments should be 2 i.e. executable + port no.
	{
       fprintf(stderr,"Error, no port no. provided\n");
       exit(1);
    }
    //declaring
	int sockfd, newsockfd, portn, n;
	socklen_t clientlen;
	char buffer[LEN+1];
	struct sockaddr_in s_address, c_address;
	/*
		socket() creates an endpoint for communication and returns a file descriptor (sockfd) referring to that endpoint
	*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("Error in creating socket");
		exit(1);
	}
	bzero((char *) &s_address, sizeof(s_address));
    portn = atoi(argv[1]);
    s_address.sin_family = AF_INET;			//for IPv4 connection
    //address of server using INADDR_ANY the socket accepts connections to all the IPs of the machine
    s_address.sin_addr.s_addr = INADDR_ANY;		
    s_address.sin_port = htons(portn);
    bzero(&s_address.sin_zero,8);
    /*
		The bind() system call binds a socket to an address and 
		port number on which the server will run passed as argument.
    */
    if (bind(sockfd, (struct sockaddr *) &s_address, sizeof(s_address)) < 0)
    {
        perror("Error in binding");			//if socket is already in use
        exit(1);
    }
    /*  The listen() system call allows the process to listen on the socket passed as argument for connections.
		The second argument is the size of the backlog queue and for most systems it is 5.
	*/
    if (listen(sockfd, 5) < 0)
    {
    	perror("Error in listening");
    	exit(1);
    }
    clientlen = sizeof(c_address);
    
    //waiting for connections from different clients
    while(1)
    {
    	/*
			The accept() system call causes the process to block until a client connects to the server and
			returns a new file descriptor (newsockfd), and all communication on this connection should be done using
		    this new fd. The second argument is a reference pointer to the address of the client.
    	*/
    	newsockfd = accept(sockfd, (struct sockaddr *) &c_address, &clientlen);
    	if (newsockfd < 0)
    	{
			perror("Error in accepting");
			exit(1);
		}
		int flag = fork(); 		//for the clients can run independently from each other
        bzero(buffer, LEN+1);	//initializing buffer

   		if(flag == 0)
   		{
   			// the child process used to communicate with the current client with whom the connection was accepted
			close(sockfd);
			handler(newsockfd, &c_address);  //calling function to handle each client connection independently
			break;
		}
		else if(flag < 0)
		{
			printf("Error in forking\n");
			break;
		}
		else close(newsockfd);
    }
	return 0;
}

void handler(int sockfd, struct sockaddr_in *c_address)
{
	char *client_ip = inet_ntoa(c_address->sin_addr);     //ip address of client
	int portn = c_address->sin_port;                     //port of client
	char buffer[LEN+1];			//for writing or sending messages (ACK) to sockfd
	char m[LEN];   				//for storing messages from client
	int n;

	while(1)
	{
		//clearing old values in buffer and 'm' array
		bzero(buffer, LEN+1);
		bzero(m, LEN);
		n = read(sockfd, m, LEN);			//reading from client
		write(1, "\n", 1);
		printf("%s::%d :- Received Message: ",client_ip,portn);             //printing client ip and port
		fflush(stdout);						//clear the output buffer
		write(1, m+1, n-1);					//writing received message excluding m[0] i.e. message type
		write(1, "\n", 1);
		char *decoded = decode(m+1);		//decoding the message excluding m[0] i.e. message type
		printf("%s::%d :- Original Message: ",client_ip,portn);
		fflush(stdout);
		write(1, decoded, strlen(decoded));		//writing the original(decoded) message
		write(1, "\n", 1);
		//for closing the connection
		if(m[0]=='3')
		{
			printf("%s::%d :- ", client_ip, portn);
			fflush(stdout);
			write(1, serv, strlen(serv));
			write(1, "\n", 1);
			break;
		}
		char * ack1 = encode(ack);           //encoding
		snprintf(buffer, sizeof(buffer), "%c%s", '2', ack1);      //attaching type to ACK 
		write(sockfd, buffer,  strlen(buffer));                   //writing to client
	}
	close(sockfd);       //freeing resources
	exit(0);
}