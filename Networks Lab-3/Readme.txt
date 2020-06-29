This application is made by Aryan Raj, Sayak Dutta and Sachin Giri.

How to:
1) Compile the code:
gcc server.c -o server.out
gcc client.c -o client.out

no need to compile encoding.c  (used for importing functions)

2) Run the code:
Server:- <executableCode> <ServerPort>
Client:- <executableCode> <ServerIP> <ServerPort>
example:- ./server.out 7894
	  ./client.out 127.0.0.1 7894
	Note: 127.0.0.1 is localhost. We can use any other ip used by our computer.

*Some Details of the code:
(1) Maximum message length is 1024 (1024 including '\0') .                //we can change it
(2) Message types:-
    Type-1 Message: Normal client messages (completed/sent at the first occurence of '\n' or newline).
    Type-2 Message: server acknowledgement for message (type-1) received.
    Type-3 Message: client close connection request/message.
(3) Client asks the user after every message whether the user wants to close the connection by pressing 'y' or 'Y'.