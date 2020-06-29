#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include "encoding.c"
#include<netdb.h>

#define LEN 1024

//some predefined messages
char * bye= "\nConnection Closed !!\nBye, Have a nice day !!\n";
char * close_con= "Close the Connection";
char * disp = "\nType the Message To Send : ";
char * serv = "Message from the server : ";

//for taking command line arguments
int main(int argc, char *argv[])
{
    if (argc < 3) 				//no. of arguments should be 3 i.e. executable + server ip + port no.
    {
       fprintf(stderr,"Provide all the required fields in the command line\n");
       exit(0);
    }
    int sockfd, portn, n;
    struct sockaddr_in s_address;
    struct hostent *serveraddress;

    char buffer[LEN+1];			//for writing or sending messages to sockfd
    char m[LEN];				//for storing messages that user will send
    portn = atoi(argv[2]);
    /*
		socket() creates an endpoint for communication and returns a file descriptor (sockfd) referring to that endpoint
	*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    { 
        perror("Error in creating socket");
        exit(1);
    }
    serveraddress = gethostbyname(argv[1]);  // gets the ip address of the server passed as the command line argument
    if (serveraddress == NULL) {
        fprintf(stderr,"Error, host does not exist\n");
        exit(1);
    }
    bzero((char *) &s_address, sizeof(s_address));
    s_address.sin_family = AF_INET;			//for IPv4 connection
    bcopy((char *)serveraddress->h_addr, (char *)&s_address.sin_addr.s_addr, serveraddress->h_length);
    s_address.sin_port = htons(portn);
    /*
		connect() used to establish a connection to the server returns 0 on success and -1 on failure.
    */
    if (connect(sockfd, (struct sockaddr *) &s_address, sizeof(s_address)) < 0) 
    { 
        perror("Error in connecting");
        exit(1);
    }
    printf("Client: \n");
    char inp[2];			//for storing decision value taken by user
    char *st = "Do you want to close the connection? - Enter 'Y' or 'y' for 'yes' otherwise enter any other key: " ;
    
    //loop to communicate any number of messages between client and server
    while(1)
    {
        write(1, st, strlen(st));
        read(0, inp, 2);				//getting response from user whether to close the connection
        if(inp[0]=='Y' || inp[0]=='y')           //if closing connection
        {
            char * encoded_message = encode(close_con);          //encoding
            snprintf(buffer, sizeof(buffer), "%c%s", '3', encoded_message);		//attaching type 3 to closing connection message
            write(sockfd, buffer, strlen(buffer));				//writing to socket
            write(1, bye, strlen(bye));							//closing message
            break;
        }
        write(1, disp, strlen(disp));
        //clearing old values in buffer and 'm' array
        bzero(buffer, LEN+1);
        bzero(m, LEN);
        fgets(m,1024,stdin);		// reading user message
        m[strlen(m)-1]='\0'; 
        if(strlen(m)==0)			//message empty
        {
        	printf("\nWarning: Last message sent was empty!!\n");
        }
        char *encoded_message = encode(m);		//encoding the message

        int x = strlen(encoded_message);
        encoded_message[x] = '\0';

        snprintf(buffer, sizeof(buffer), "%c%s", '1', encoded_message);			//attaching type to message
        write(sockfd, buffer, strlen(buffer));									//writing to socket
        //clearing old values in buffer and 'm' array
        bzero(buffer, LEN+1);
        bzero(m, LEN);
        int received = read(sockfd, m, 30);		//reading message (encoded ACK) from sockfd
        strcpy(m+1, decode(m+1));			//decoding the message excluding m[0] i.e. message type
        if(m[0] != '2')			//If ACK not received
        {
            write(1,"ACK not received !!\nResend the message !!\n",
            strlen("ACK not received !!\nResend the message !!\n")); 
            continue; 
        }       
        write(1, serv, strlen(serv));
        printf("%s:%d - ", argv[1], portn);
        fflush(stdout);								//clear the output buffer
        write(1, m+1, strlen(m)-1);					//writing ACK received
        write(1, "\n\n", 2);			
        bzero(encoded_message, strlen(encoded_message));
    }
    close(sockfd);      //closing socket freeing resources
    return 0;
}
