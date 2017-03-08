#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */
#include <stdlib.h> /* for atoi() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */

#define RCVBUFSIZE 128  /* Size of receive buffer */
#define MSG_BUF_SIZE 4096
#define MAX_NAME_LEN 20

enum code { LOGIN, LIST, SEND, GET, OK, FAIL};

typedef struct message {
  unsigned int length;
  enum code opCode;
  unsigned int bodyLength;
  char * body;
  
} Message;

void DieWithError(char *errorMessage);
void PrintMenuOptions();
void ConnectToServer(int sock, struct sockaddr_in serverAddr);
void Login(int sock);
void GetUserList(int sock);
void SendTextMessage(int sock);
void GetTextMessages(int sock);
Message *createMessage(enum code opcode, char* body);
int sendMessage(int sock, Message *msg);
Message *readMessageFromSocket(int sock);

char userName[MAX_NAME_LEN] = "Bob";

int main(int argc, char *argv[])
{
  int sock; /* Socket descriptor */
  struct sockaddr_in serverAddr; /* Echo server address */
  char option;

  /* Create a reliable, stream socket using TCP */
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithError(" socket () failed") ;
  
  for(;;)
  {
    PrintMenuOptions();
    scanf("%c", &option);
    while(getchar() != '\n');
    
    switch(option) {
      case '0': 
        ConnectToServer(sock, serverAddr);
        break;
      case '1': 
        GetUserList(sock);
        break;
      case '2': 
        SendTextMessage(sock);
        break;
      case '3': 
        GetTextMessages(sock);
        break;
      case '4': 
        printf("you entered: %c\n", option);
        break;
      case '5': 
        printf("you entered: %c\n", option);
        break;
      default:
        printf("Invalid option\n");
    }
  }
  
  close(sock);
  exit(0);
}

void PrintMenuOptions(void)
{
  printf("----------------------------------------\n");
  printf("Command:\n");
  printf("0. Connect to the server\n");
  printf("1. Get the user list\n");
  printf("2. Send a message\n");
  printf("3. Get my messages\n");
  printf("4. Initiate a chat with my friend\n");
  printf("5. Chat with my friend\n");
  printf("Your option<enter a number>: ");
}

void ConnectToServer(int sock, struct sockaddr_in serverAddr) 
{
  const int LEN = 20;
  char serverIP[LEN];
  char port[LEN];
  int serverPort;
  
  printf("Enter the IP address: ");
  fgets(serverIP, sizeof(serverIP), stdin);
  
  printf("Enter the port number: ");
  fgets(port, sizeof(port), stdin);
  serverPort = atoi(port);
  
  /* Construct the server address structure */
  memset(&serverAddr, 0, sizeof(serverAddr)); /* Zero out structure */
  serverAddr.sin_family = AF_INET; /* Internet address family */
  serverAddr.sin_addr.s_addr = inet_addr(serverIP); /* Server IP address */
  serverAddr.sin_port = htons(serverPort); /* Server port */

  printf("Connecting...\n");
  /* Establish the connection to the echo server */
  if (connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    DieWithError(" connect () failed");
  
  printf("Connected!\n");
  
  //Login(sock);
}

void Login(int sock) {
  char request[] = "LOGIN";
  char user[MAX_NAME_LEN] = {0};
  char password[32] = {0};
  char credentials[128] = {0};
  int length;
  
  printf("Username: ");
  fgets(user, sizeof(user), stdin);
  printf("Password: ");
  fgets(password, sizeof(password), stdin);
  
  strncat(credentials, user, strlen(user)-1);
  strncat(credentials, ":", 1);
  strncat(credentials, password, strlen(password)-1);
  
  
  /* send request segment */
  length = strlen(request);
  if(send(sock, &length, sizeof(int), 0) != sizeof(int))
    DieWithError("send() sent a different number of bytes than expected");
  if(send(sock, request, length, 0) != length)
    DieWithError("send() sent a different number of bytes than expected");
  
  /* send credentials */
  length = strlen(credentials);
    if(send(sock, &length, sizeof(int), 0) != sizeof(int))
    DieWithError("send() sent a different number of bytes than expected");
  if(send(sock, credentials, length, 0) != length)
    DieWithError("send() sent a different number of bytes than expected");
  
  /* receive ok response */
  printf("\n");
}

void GetUserList(int sock)
{
  int status, userCount;
  char *userList;
  Message *request, *response;
  
  request = createMessage(LIST, "");
  status = sendMessage(sock, request);
  freeMessage(request);
  if(status < 0)
    fprintf(stderr, "Error when sending request for user list\n");
  
  response = readMessageFromSocket(sock);
  
  if(response->opCode == OK) {
    memcpy(&userCount, response->body, sizeof(int));
    printf("There are %d user(s)\n", userCount);
    userList = response->body + sizeof(int);
    printf("%s\n", userList);  
  }
  else {
    fprintf(stderr, "Error: %s\n", response->body);
  }
  freeMessage(response);
  
}

void SendTextMessage(int sock)
{
  char request[] = "SEND_MSG";
  char user[MAX_NAME_LEN] = {0};
  char message[MSG_BUF_SIZE] = {0};
  char buffer[MAX_NAME_LEN+MSG_BUF_SIZE+1] = {0};
  char sendBuffer[RCVBUFSIZE] = {0};
  int sendLength;
  char *remaining;
  
  printf("Please enter the user name: ");
  fgets(user, sizeof(user), stdin);
  
  printf("Please enter the message: ");
  fgets(message, sizeof(message), stdin);
 
  /* format for transmission (user:message) */
  strncat(buffer,user, strlen(user)-1); // exclude newline
  strncat(buffer,":", 1); //delimiter
  strncat(buffer, message, strlen(message));
  
  
  /* send request message */
  sendLength = strlen(request);
  if(send(sock, &sendLength, sizeof(int), 0) != sizeof(int))
    DieWithError("send() sent a different number of bytes than expected");
  if(send(sock, request, sendLength, 0) != sendLength)
    DieWithError("send() sent a different number of bytes than expected");
  
  remaining = buffer; // start of next message segment
  
  /* send text message in multiple transmissions */
  while(strlen(remaining) > RCVBUFSIZE)
  {
    strncpy(sendBuffer, remaining, RCVBUFSIZE);
    
    sendLength = RCVBUFSIZE;
    
    if(send(sock, &sendLength, sizeof(int), 0) != sizeof(int))
      DieWithError("send() failed");
    if (send(sock, sendBuffer, sendLength, 0) != sendLength)
      DieWithError("send() failed");
  
    remaining += RCVBUFSIZE; //advance RCVBUFSIZE character positions
  }
  
  /* send final message segment */
  strncpy(sendBuffer, remaining, RCVBUFSIZE);
  sendLength = strlen(sendBuffer);
  
  if(send(sock, &sendLength, sizeof(int), 0) != sizeof(int))
    DieWithError("send() failed");
  if (send(sock, sendBuffer, sendLength, 0) != sendLength)
    DieWithError("send() failed");

  /* signal end of transmission */
  sendLength = 0;
  if(send(sock, &sendLength, sizeof(int), 0) != sizeof(int))
    DieWithError("send() failed");
     
  printf("\n");
}

void GetTextMessages(int sock) {
  const char request[] = "GET_MSG\0";
  char recvBuffer[RCVBUFSIZE] = {0};
  int bytesToRead = 0;
  int bytesToSend;
  int prefix;
  
  bytesToSend = strlen(request);
  
  /* send request */
  prefix = bytesToSend;
  if(send(sock, &prefix, sizeof(int), 0) != sizeof(int))
    DieWithError("send() sent a different number of bytes than expected");
  if(send(sock, request, bytesToSend, 0) != bytesToSend)
    DieWithError("send() sent a different number of bytes than expected");
  
  /* send username */ 
  bytesToSend = strlen(userName);
  if(send(sock, &bytesToSend, sizeof(int), 0) != sizeof(int))
    DieWithError("send() sent a different number of bytes than expected");
  if(send(sock, userName, bytesToSend, 0) != bytesToSend)
    DieWithError("send() sent a different number of bytes than expected");

  /* bytesToRead gets length of next message */
  if(recv(sock, &bytesToRead, sizeof(int), 0) <= 0)
    DieWithError("recv() failed or connection closed prematurely");
  
  while(bytesToRead > 0)
  {
    /* recieve message segments */
    if(recv(sock, recvBuffer, bytesToRead, 0) <= 0)
      DieWithError("recv() failed or connection closed prematurely");
    
    printf("%s\n",recvBuffer);
    
    if (recv(sock, &bytesToRead, sizeof(int), 0) <= 0)
      DieWithError("recv() failed or connection closed prematurely");
  }
  
  printf("\n");
}
