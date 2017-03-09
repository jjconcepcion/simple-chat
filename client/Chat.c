#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h> /* for atoi() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */ 
#include <stdio.h>

#define MAXPENDING 5 /* Maximum outstanding connection requests */
#define LOCALHOST "127.0.0.1"
#define MAX_NAME_LEN 20
#define MAX_MSG_LEN 4096

enum code { LOGIN, LIST, SEND, GET, OK, FAIL, CHAT};

typedef struct message {
  unsigned int length;
  enum code opCode;
  unsigned int bodyLength;
  char * body;
  
} Message;

extern char userName[MAX_NAME_LEN];

void DieWithError(char *errorMessage);
Message *createMessage(enum code opcode, char* body);
Message *readMessageFromSocket(int sock);
int sendMessage(int sock, Message *msg);
void freeMessage(Message *msg);
void HandleChatClient(int clientSocket);
void printQuitDialog();
void ConnectToServer(int sock); 
  
void HostChat() {
  int serverSocket; /* Socket descriptor for server */
  int clientSocket; /* Socket descriptor for client */
  struct sockaddr_in serverAddr; /* Local address */
  struct sockaddr_in clientAddr; /* Client address */
  unsigned short serverPort; /* Server port */
  unsigned int clntLen; /* Length of client address data structure */
  char userInput[6] = {0};

  printf("Please enter the port number you want to listen on: ");
  fgets(userInput, sizeof(userInput), stdin);
  serverPort = atoi(userInput);
  

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
  
  printf("I am Listening on %s:%d\n" , LOCALHOST, serverPort);
  
  for (;;) /* Run forever */
  {
    /* Set the size of the in-out parameter */
    clntLen = sizeof(clientAddr);
    /* Wait for a client to connect */
    if ((clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &clntLen)) < 0)
      DieWithError("accept() failed");
    /* clientSocket is connected to a client! */
    HandleChatClient(clientSocket);
  }
/* NOT REACHED */
}

void HandleChatClient(int clientSocket) {
  Message *incoming, *outgoing;
  int status;
  char friendName[MAX_NAME_LEN+1] = {0};
  char text[MAX_MSG_LEN+1] = {0};
  
  /* Exchange greetings */
  incoming = readMessageFromSocket(clientSocket);
  if(incoming == NULL || incoming->opCode != CHAT)
    exit(1);
  outgoing = createMessage(CHAT, userName);
  status = sendMessage(clientSocket, outgoing);
  if(status < 0)
    exit(1);
  
  strncpy(friendName, incoming->body, strlen(incoming->body));
  printf("%s is connected\n", friendName);
  
  /* carry on conversation */
  while(1) {
    freeMessage(incoming);
    freeMessage(outgoing);
    
    printQuitDialog();
    
    incoming = readMessageFromSocket(clientSocket);
    if(incoming == NULL || incoming->opCode != CHAT)
      break;
    fprintf(stderr,"%s: %s\n", friendName, incoming->body);
    
    printf("%s: ", userName);
    fgets(text, sizeof(text), stdin); 
    text[strlen(text)-1] = '\0';
    
    outgoing = createMessage(CHAT, text);
    status = sendMessage(clientSocket, outgoing);
    if(status < 0)
      break;
  }
  freeMessage(incoming);
  freeMessage(outgoing);
  
  close(clientSocket);
}

void JoinChat() {
  Message *incoming, *outgoing;  
  char friendName[MAX_NAME_LEN+1] = {0};
  char text[MAX_MSG_LEN+1] = {0};
  int status;
  int sock; /* Socket descriptor */
  
  /* Create a reliable, stream socket using TCP */
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithError(" socket () failed") ;
  
  ConnectToServer(sock);
  
  /* Intiate chat greetings */
  outgoing = createMessage(CHAT, userName);
  status = sendMessage(sock, outgoing);
  if(status < 0)
    exit(1);
  incoming = readMessageFromSocket(sock);
  if(incoming == NULL || incoming->opCode != CHAT)
    exit(1); 
  
  strncpy(friendName, incoming->body, strlen(incoming->body));
  
   
  /* carry on conversation */
  while(1) {
    freeMessage(incoming);
    freeMessage(outgoing);
    
    printQuitDialog();

    printf("%s: ", userName);
    fgets(text, sizeof(text), stdin); 
    text[strlen(text)-1] = '\0';
    
    outgoing = createMessage(CHAT, text);
    status = sendMessage(sock, outgoing);
    if(status < 0)
      break;
    
    incoming = readMessageFromSocket(sock);
    if(incoming == NULL || incoming->opCode != CHAT)
      break;
    
    printf("%s: %s\n", friendName, incoming->body);

  }
  freeMessage(incoming);
  freeMessage(outgoing);
  
  close(sock);
  exit(0);
}
void printQuitDialog() {
  printf("<Type \"Bye\" to stop the conversation>\n");
}
