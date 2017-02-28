#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h> /* for atoi() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */

#define MAXPENDING 5 /* Maximum outstanding connection requests */
#define LOCALHOST "127.0.0.1"
#define DEFAULT_PORT 8000

void DieWithError(char *errorMessage); /* Error handling function */
void HandleTCPClient(int clientSocketet); /* TCP client handling function */

int main(int argc, char *argv[])
{
  int serverSocket; /* Socket descriptor for server */
  int clientSocket; /* Socket descriptor for client */
  struct sockaddr_in serverAddr; /* Local address */
  struct sockaddr_in clientAddr; /* Client address */
  unsigned short serverPort; /* Server port */
  unsigned int clntLen; /* Length of client address data structure */

  if (argc > 2) /* Test for correct number of arguments */
  {
    fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]) ;
    exit(1);
  }

  if(argc == 2)
    serverPort = atoi(argv[1]); /* First arg: local port */
  else
    serverPort = DEFAULT_PORT;

  /* Create socket for incoming connections */
  if ((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithError( "socket () failed") ;

  /* Construct local address structure */
  memset(&serverAddr, 0, sizeof(serverAddr)); /* Zero out structure */
  serverAddr.sin_family = AF_INET; /* Internet address family */
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
  serverAddr.sin_port = htons(serverPort); /* Local port */

  /* Bind to the local address */
  if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    DieWithError ( "bind () failed");
  
  /* Mark the socket so it will listen for incoming connections */
  if (listen(serverSocket, MAXPENDING) < 0)
    DieWithError("listen() failed") ;
  
  printf("Server Started!\n");
  printf("Listen on %s:%d\n" , LOCALHOST, serverPort);
  
  for (;;) /* Run forever */
  {
    /* Set the size of the in-out parameter */
    clntLen = sizeof(clientAddr);
    /* Wait for a client to connect */
    if ((clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &clntLen)) < 0)
      DieWithError("accept() failed");
    /* clientSocket is connected to a client! */
    printf("Handling client %s\n", inet_ntoa(clientAddr.sin_addr));
    HandleTCPClient (clientSocket);
  }
/* NOT REACHED */
}
