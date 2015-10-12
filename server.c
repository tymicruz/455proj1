#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>//errno strerror(errno)
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>

enum state_name{
	CONNECT,
	LOGIN,
	PASSWORD,
  FINALE,
  KILL
} server_state;

#define  MAX 256
#define NUM_IDS 5
// Define variables:
struct sockaddr_in  server_addr, client_addr, name_addr;
struct hostent *hp;

int  sock, newsock;                  // socket descriptors
int  serverPort;                     // server port number
int  r, length, n;                   // help variablesi

int ids[NUM_IDS] = {1, 22, 333, 4444, 1738};
char* names[NUM_IDS] = {"carl", "mike", "su", "tyler", "monty"};
char* passwords[NUM_IDS] = {"one1", "two2", "three3", "four4", "remyboyz"};

char* congrats[3] = { "Congratulations ", "; you've just revealed the password for ", " to the world!"};
char* password_fail = "Password incorrect.";
char user_id[10];

int validate(char *buf);
int password(char *buf, int user);

// Server initialization code:

int server_init(char *name, int port)
{
	printf("==================== server init ======================\n");   
   // get DOT name and IP address of this host

	printf("1 : get and show server host info\n");
	hp = gethostbyname(name);
	if (hp == 0){
		printf("unknown host\n");
		exit(1);
	}
	printf("    hostname=%s  IP=%s\n",
		(char*)hp->h_name,  (char*)inet_ntoa(*(long *)hp->h_addr));

   //  create a TCP socket by socket() syscall
	printf("2 : create a socket\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0){
		printf("socket call failed\n");
		exit(2);
	}

	printf("3 : fill server_addr with host IP and PORT# info\n");
   // initialize the server_addr structure
   server_addr.sin_family = AF_INET;                  // for TCP/IP
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // THIS HOST IP address  
   server_addr.sin_port = htons(3000);   // let kernel assign port

   printf("4 : bind socket to host info\n");
   // bind syscall: bind the socket to server_addr info
   r = bind(sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
   if (r < 0){
   	printf("bind failed\n");
   	exit(3);
   }

   printf("5 : find out Kernel assigned PORT# and show it\n");
   // find out socket port number (assigned by kernel)
   length = sizeof(name_addr);
   r = getsockname(sock, (struct sockaddr *)&name_addr, &length);
   if (r < 0){
   	printf("get socketname error\n");
   	exit(4);
   }

   // show port number
   serverPort = ntohs(name_addr.sin_port);   // convert to host ushort
   printf("    Port=%d\n", serverPort);

   // listen at port with a max. queue of 5 (waiting clients) 
   printf("5 : server is listening ....\n");
   listen(sock, 5);
   printf("===================== init done =======================\n");
}


main(int argc, char *argv[], char* env[])
{
	char *hostname;
	char buf[MAX], mini_buf[1];

	int i = 0, user = 0;
	int port = 0;
  bzero(buf, MAX);

  uint16_t network_byte_order = 0;
    server_state = CONNECT;

	//strcpy(hd,getHomeDir(env));

   if (argc < 2) //default to localhost 127.0.0.1:3000 if no args given
   {
   	hostname = "localhost";
   	port = 3000;
   }
  	else if (argc < 3) //if only one arg, assume it is port #
  	{	
  		hostname = "localhost";
  		port = atoi(argv[1]);
  	}
  	else
  	{
  		hostname = argv[1];
  		port = atoi(argv[2]);
  	}

  	server_init(hostname, port); 
 
   // Try to accept a client request
  	while(1){

      bzero(buf, MAX);
      bzero(mini_buf, 1);

  		switch(server_state)
  		{

  			case CONNECT:
          printf("\nCONNECT STATE:\n");
          printf("server: accepting new connection ....\n");
       // Try to accept a client connection as descriptor newsock
    			length = sizeof(client_addr);
    			newsock = accept(sock, (struct sockaddr *)&client_addr, &length);
    			printf("client socket: %d\n", newsock);
    			if (newsock < 0){
    				printf("server: accept error\n");
    				exit(1);
    			}
    			printf("server: accepted a client connection from\n");
    			printf("-----------------------------------------------\n");
    			printf("        IP=%s  port=%d\n", (char*)inet_ntoa(client_addr.sin_addr.s_addr),
    			ntohs(client_addr.sin_port));
    			printf("-----------------------------------------------\n");
    			printf("sending \"Welcome\\n\" message\n");

    			i = send(newsock, "Welcome\n", 8, 0);

    			server_state = LOGIN;
    			break;


  			case LOGIN:
          printf("\nLOGIN STATE:\n");

          //recv num id from client
    			if(n = recv(newsock, buf, MAX, 0))
          {
            printf("(id)received %d bytes\n", n);
            //printf("client num id: %s", buf);
          }
          else
          {
            //client died probably
            server_state = KILL; 
          }

          //recv name from client (s(ave in same buffer start at offset, n)
          if(server_state != KILL && (n = recv(newsock, (buf + n), MAX - n, 0)))
          {
            printf("(name)received %d bytes\n", n);
            printf("client info:\n%s", buf);
          }
          else//no room left in buffer mayber or client dies
          {
            server_state = KILL; 
          }

          //validate the user's info
          if(server_state != KILL )
          {
            if(user = validate(buf))
            {
              user--;
              printf("sending \"Success\\n\"\n");
              i = send(newsock, "Success\n", 8, 0);
              server_state = PASSWORD;
            }
            else
            {
              printf("sending \"Failure\\n\"\n");
              i = send(newsock, "Failure\n", 8, 0);
              server_state = KILL;
            }
          }

          break;

        case PASSWORD:
          printf("\nPASSWORD STATE:\n");
          if(n = recv(newsock, &network_byte_order, sizeof(uint16_t), 0))
          {
            network_byte_order = ntohs(network_byte_order);
            printf("password len: %d\n", network_byte_order);

            i = 0;
            //__fpurge()
              //get password (one byte at a time)
            do{

              n = recv(newsock, mini_buf, 1, 0);
              printf("recv pw[%d]: %c\n", i, mini_buf[0]);              
              buf[i++] = mini_buf[0];//get one char at a time

              //if n == 0 kill connection

            }while(i < network_byte_order && n);

            buf[i] = 0; //null terminate the string 

            if(password(buf, user))
            {

              printf("password matched\n");

              //create message
              n = 0;
              bzero(buf, MAX);

              //add congrats part 1
              i = strlen(congrats[0]);
              memcpy(buf, congrats[0], i);
              n += i;

              //add name 
              i = strlen(names[user]);
              memcpy(buf + n, names[user], i);
              n += i;

              //add congrats part 2
              i = strlen(congrats[1]);
              memcpy(buf + n, congrats[1], i);
              n += i;

              //add id
              sprintf(user_id, "%d", ids[user]);
              i = strlen(user_id);
              memcpy(buf + n, user_id, i);
              n += i;

              //add congrats part 3
              i = strlen(congrats[2]);
              memcpy(buf + n, congrats[2], i);
              n += i;

              network_byte_order = htons(((uint16_t)n));
              printf("sending message length: htons(%d)\n", n);
              //stop at new-line or null char or i max which ever 1st
              send(newsock, &network_byte_order, sizeof(uint16_t), 0);
              printf("sending message:\n");

              //send message one byte at a time
              i = 0;
              do
              {
                mini_buf[0] = buf[i++];
                printf("%c", mini_buf[0]);
                send(newsock, mini_buf, 1, 0);

              }while(i < n);  

               //printf("final message %d:\n%s\n", n, buf);
              //send(newsock, buf, n, 0);
              buf[i] = 0;
              printf("\n");
              

              //send(newsock, &network_byte_order, size(uint16_t), 0);
            }
            else
            {
              bzero(buf, MAX);
              printf("password doesn't match. kill client.\n");
              
              i = 0;
              n = strlen(password_fail);

              network_byte_order = htons(((uint16_t)n));
              printf("sending message length: htons(%d)\n", n);
              //stop at new-line or null char or i max which ever 1st
              send(newsock, &network_byte_order, sizeof(uint16_t), 0);
              printf("sending message:\n");

              //send message one byte at a time
              i = 0;
              do
              {
                mini_buf[0] = password_fail[i++];
                printf("%c", mini_buf[0]);
                send(newsock, mini_buf, 1, 0);

              }while(i < n);  

               //printf("final message %d:\n%s\n", n, buf);
              //send(newsock, buf, n, 0);
              printf("\n");
              //send(newsock, "Password incorrect.", MAX,0);
              //server_state = KILL;
            }
          }
          else
          {
            printf("no password or client died\n");
            server_state = KILL;
          }
          
          server_state = KILL;
          break;

        case KILL:
        printf("killing socket %d\n", newsock);
          close(newsock);
          server_state = CONNECT;
          //exit(newsock);

  			}
  		}
  	}

int validate(char *buf)
{
  int i = 0, newlines = 0, n1 = 0, n2 = 0;
  char *id = 0, *name = 0;
  int num_id = 0;

  while(*(buf + i) && i < MAX)
  {
    if(*(buf + i) == '\n')
    {
      newlines++;

      //get position of first new-line
      if(newlines == 1)
      {
        n1 = i;
      }
      //get position of second new-line
      else if (newlines == 2)
      {
        n2 = i;
      }
    }

    i++;
  }


  if(newlines != 2) return 0;

  //split id from buf
  id = (char *)malloc(sizeof(char) * (n1 + 1));
  memcpy(id, buf, n1);
  id[n1] = 0;

  //split name from buf
  name = (char *)malloc(sizeof(char) * (n2 - n1));
  memcpy(name, buf + n1 + 1, n2 - n1 - 1);
  name[n2 - n1 - 1] = 0;

  num_id = atoi(id);

  if(num_id == 0) return 0;

  for(i = 0; i < NUM_IDS; i++)
  {
    if(num_id == ids[i]) break; //id exists in the ids array
  }

  if(i >= NUM_IDS) return 0; // not found

  //at this point i is the index
  if(strcmp(names[i], name) == 0)
  {
    return i + 1; //otherwise user 1 would look like false
  }

  return 0;
}

int password(char *buf, int user)
{

  if((user < 0) || user >= NUM_IDS) return 0;

  if(strcmp(buf, passwords[user]) == 0) return 1;

  return 0;
}

