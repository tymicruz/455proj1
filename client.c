//**************************** ECHO CLIENT CODE **************************
// The echo client client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>//errno strerror(errno)
#include <sys/socket.h>

#include <sys/stat.h>//idk stat?
#include <sys/types.h>//idk stat?
#include <unistd.h>//idk stat:w

#include <time.h>
#include <dirent.h>

#include <netdb.h>
#include <fcntl.h>//used for 0_RDONLY

#define MAX 256

enum state_name{
	CONNECT,
	LOGIN,
	PASSWORD,
	DEAD
} client_state;

// Define variables
struct hostent *hp;              
struct sockaddr_in  server_addr;

int sock, r;

int SERVER_IP, SERVER_PORT; 
int welcome(char *buf);
int success(char *buf);
int failure(char *buf);

int fd;
// clinet initialization code

int client_init(char *argv[])
{
	printf("======= client init ==========\n");

	printf("1 : get server info\n");
	hp = gethostbyname(argv[1]);
	if (hp==0){
		printf("unknown host %s\n", argv[1]);
		exit(1);
	}

	SERVER_IP   = *(long *)hp->h_addr;
	SERVER_PORT = atoi(argv[2]);

	printf("2 : create a TCP socket\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock<0){
		printf("socket call failed\n");
		exit(2);
	}

	printf("3 : fill server_addr with server's IP and PORT#\n");
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = SERVER_IP;
	server_addr.sin_port = htons(SERVER_PORT);

  // Connect to server
	printf("4 : connecting to server ....\n");
	r = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (r < 0){
		printf("connect failed\n");
		exit(1);
	}

	printf("5 : connected OK to \007\n"); 
	printf("---------------------------------------------------------\n");
	printf("hostname=%s  IP=%s  PORT=%d\n", 
		(char*) hp->h_name, (char*)inet_ntoa(SERVER_IP), SERVER_PORT);
	printf("---------------------------------------------------------\n");

	printf("========= init done ==========\n");
}

main(int argc, char *argv[ ], char *env[])
{
	int i, j;
	char buf[MAX], buf2[MAX];
	char mini_buf[1];
	client_state = CONNECT;
	uint16_t network_byte_order = 0;

	if (argc < 3){
		printf("Usage : myclient <ServerName> <SeverPort>\n");
		exit(1);
	}

	client_init(argv);

	printf("********  processing loop  *********\n");
	while (1){

		//printf("\ninput a line : ");
		bzero(buf, MAX);                // zero out line[ ]
		bzero(mini_buf, 1);
		__fpurge(stdin);
		i = 0; j = 0;

		switch(client_state)
		{
			case CONNECT:
				printf("\nCONNECT STATE:\n");
				//wait for message
				//read until new-line char
				do{
					//printf("client waiting p...\n");
					j = recv(sock, mini_buf, 1, 0);
	    			buf[i++] = mini_buf[0];//get one char at a time

	    		}while(i < MAX && j != 0 && mini_buf[0] != '\n'); 

	    		//terminate last char;
	    		buf[i] = 0;
	    		//printf("server sent: -->%s<--\n", buf);
	    		//printf("done max%d, i%d, j%d\n", MAX, i, j);
	    		//check is server sent "Welcome\n"
	    		if(welcome(buf))
	    		{
	    			printf("server welcomes us with open arms!\n");
	    			client_state = LOGIN;
	    		}
	    		else
	    		{
	    			printf("server did not welcome us. client_state = DEAD.\n");
	    			client_state = DEAD;
	    		}

    			break;

    		case LOGIN:	
	    		printf("\nLOGIN STATE:\n");

	    		//prompt num id and send to server
	    		i = 0;
	    		bzero(buf, MAX);
	    		printf("enter num id (%d digits max): ", MAX - 1); // -1 to take new-line into account
	    		fgets(buf, MAX, stdin);
	    		 //if the type more than 256 just clear
	    		__fpurge(stdin);
	    		//see how many bytes you are sending
	    		while(*(buf + i) && i < MAX)
	    		{
	    			i++;
	    		}
	    		printf("sending %d bytes\n", i);
	    		send(sock, buf, i, 0);


	    		bzero(buf, MAX);
	    		printf("enter name (%d chars max): ", MAX - i - 1); // -1 to take new-line into account
	    		i = 0;
	    		fgets(buf, MAX, stdin);
	    		 //if the type more than 256 just clear
	    		__fpurge(stdin);
	    		//see how many bytes you are sending
	    		while(*(buf + i) && i < MAX)
	    		{
	    			i++;
	    		}
	    		printf("sending %d bytes\n", i);
	    		send(sock, buf, i, 0);

	    		bzero(buf, MAX);

	    		i = 0;
	    		//recv server response
				do{
					//printf("client waiting p...\n");
					j = recv(sock, mini_buf, 1, 0);
	    			buf[i++] = mini_buf[0];//get one char at a time

	    		}while(i < MAX && j != 0 && mini_buf[0] != '\n'); 

	    		buf[i] = 0;

	    		if(success(buf))
	    		{
	    			printf("Success, valid ID and Name submitted.\n");
	    			client_state = PASSWORD;
	    		}
	    		else if(failure(buf))
	    		{
	    			printf("Failure, invalid ID and Name submitted\n");
	    			client_state = DEAD;
	    		}    		
	    		else
	    		{
	    			printf("something went wrong\n");
	    			client_state = DEAD;
	    		}

    			break;

    		case PASSWORD:
    			printf("\nPASSWORD STATE:\n");
    			printf("enter password (255 chars max): ");
	    		fgets(buf, MAX, stdin);
	    		 //if the type more than 256 just clear
	    		__fpurge(stdin);

	    		i = 0;
	    		//get password len
	    		while((*(buf + i) && *(buf + i) != '\n') && i < MAX)
	    		{
	    			i++;
	    		}

	    		if(i == 0)
	    		{
	    			printf("invalid password\n");
	    			client_state = DEAD;
	    			break;
	    		}

	    		network_byte_order = htons(((uint16_t)i));

	    		printf("sending password length: htons(%d)\n", i);
	    		//stop at new-line or null char or i max which ever 1st
	    		send(sock, &network_byte_order, sizeof(uint16_t), 0);
	    		printf("sending password\n");

	    		//sent password one byte at a time
	    		j = 0;
				do
				{
					mini_buf[0] = buf[j++];
					printf("send pw[%d]: %c\n", j - 1, mini_buf[0]);
					send(sock, mini_buf, 1, 0);

	    		}while(j < i); 	


	    		bzero(buf, MAX);

	    		//get final one byte at a time
	    		if(j = recv(sock, &network_byte_order, sizeof(uint16_t), 0))
	    		{
	    			network_byte_order = ntohs(network_byte_order);
	    			printf("message len: %d\n", network_byte_order);

	    			i = 0;

	    			printf("message:\n");
	    			do{

	    				j = recv(sock, mini_buf, 1, 0);

	    				printf("%c", mini_buf[0]);              
              			buf[i++] = mini_buf[0];//get one char at a time

          			}while(i < network_byte_order && j);

           	 		printf("\n");
           	 		buf[i] = 0; //null terminate the string 
        		}
        		else
        		{
        			client_state = DEAD;
        			break;
        		}

	    		printf("\nagain:\n%s\n", buf);

	    		client_state = DEAD;    		

	    		break;


    		case DEAD:
	    		printf("DEAD => exiting..\n");
	    		exit(2);
	    		break;
    		//fgets(buf, MAX, stdin);         // get a line (end with \n) from stdin


    	}
    }
}

int welcome(char *buf)
{
	int i = 0;
	char tmp;

	while(*(buf + i) && i < MAX)
	{
		//printf("%c\n", *(buf + i));
		i++;
	}

	if (i <= 0) return 0;

	if(buf[i-1] != '\n') return 0; //last char must be \n terminated

	tmp = buf[i-1];
	buf[i-1] = 0;//null terminate string at the new-line char

	if(strcmp(buf, "Welcome") == 0)
	{
		buf[i - 1] = tmp;
	 return 1;
	}

	buf[i-1] = tmp; //restore new-line char

	return 0;
}

int success(char *buf)
{
	int i = 0;
	char tmp;

	while(*(buf + i) && i < MAX)
	{
		//printf("%c\n", *(buf + i));
		i++;
	}

	if (i <= 0) return 0;

	if(buf[i-1] != '\n') return 0; //last char must be \n terminated

	tmp = buf[i-1];
	buf[i-1] = 0;//null terminate string at the new-line char

	if(strcmp(buf, "Success") == 0) 
	{
		buf[i-1] = tmp; //restore new-line char
		return 1;
	}

	buf[i-1] = tmp; //restore new-line char

	return 0;
}

int failure(char *buf)
{
	int i = 0;
	char tmp;

	while(*(buf + i) && i < MAX)
	{
		//printf("%c\n", *(buf + i));
		i++;
	}

	if (i <= 0) return 0;

	if(buf[i-1] != '\n') return 0; //last char must be \n terminated

	tmp = buf[i-1];
	buf[i-1] = 0;//null terminate string at the new-line char

	if(strcmp(buf, "Failure") == 0) 
	{
		buf[i-1] = tmp; //restore new-line char
		return 1;
	}

	buf[i-1] = tmp; //restore new-line char

	return 0;
}

