#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */
#include <stdlib.h> /* for atoi() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */

#define RCVBUFSIZE 128  /* Size of receive buffer */
#define MAX_MSG_LEN 4096
#define MAX_NAME_LEN 20
#define MAX_PASS_LEN 64 

enum code { LOGIN, LIST, SEND, GET, OK, FAIL};

typedef struct message {
  unsigned int length;
  enum code opCode;
  unsigned int bodyLength;
  char * body;
  
} Message;

void PrintMenuOptions();
void ConnectToServer(int sock, struct sockaddr_in serverAddr);
void Login(int sock);
void GetTextMessages(int sock);

void DieWithError(char *errorMessage);
Message *createMessage(enum code opcode, char* body);
Message *readMessageFromSocket(int sock);
int sendMessage(int sock, Message *msg);
void freeMessage(Message *msg);
void GetUserList(int sock);
void SendTextMessage(int sock);

static char userName[MAX_NAME_LEN] = "Guest\0";

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
        Login(sock);
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
}

void Login(int sock) {
  char username[MAX_NAME_LEN+1] = {0};
  char password[MAX_PASS_LEN+1] = {0};
  char body[MAX_NAME_LEN+MAX_PASS_LEN+2] = {0};
  Message *request, *response;
  int status;
  
  printf("Welcome, please log in!\n");
  printf("Username: ");
  fgets(username, sizeof(username), stdin);
  printf("Password: ");
  fgets(password, sizeof(password), stdin);
  username[strlen(username)-1] = '\0';
  password[strlen(password)-1] = '\0';
  
  /* generate authorization string; omit newline */
  strncat(body, username, strlen(username));
  strncat(body, ":", 1);
  strncat(body, password, strlen(password));
  
  /* send request */
  request = createMessage(LOGIN, body);
  status = sendMessage(sock, request);
  freeMessage(request);
  if(status < 0)
    fprintf(stderr, "Error when sending login request\n");
  
  /* get login confirmation */
  response = readMessageFromSocket(sock);
  
  if(response->opCode == OK) {
    /* set authorized userName */
    strncpy(userName, username, strlen(username));
    userName[strlen(username)] = '\0';
    printf("%s\n", response->body);
  }
  else {
    printf("%s\n", response->body);
    close(sock);
  }
  freeMessage(response);
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
  Message *request, *response;
  char name[MAX_NAME_LEN] = {0};
  char message[MAX_MSG_LEN] = {0};
  char body[MAX_NAME_LEN + MAX_MSG_LEN + 2] = {0};
  unsigned int nameLength, messageLength;
  int status;
  
  printf("Please enter the user name: ");
  fgets(name, sizeof(name), stdin);
  nameLength = strlen(name) - 1; //exclude newline
  
  printf("Please enter the message: ");
  fgets(message, sizeof(message), stdin);
  messageLength = strlen(message) - 1; //exclude newline
 
  /* format body (recipient:message) */
  strncat(body, name, nameLength); 
  strncat(body, ":", 1);
  strncat(body, message, messageLength);
  
  /* send message */
  request = createMessage(SEND, body);
  status = sendMessage(sock, request);
  freeMessage(request);
  if(status < 0)
    fprintf(stderr, "Error when sending message\n");  
     
  /* wait for response */ 
  response = readMessageFromSocket(sock);
  if(response->opCode == OK) {
    printf("\nmessage sent successfully!\n");
  }
  else {
    fprintf(stderr, "Error: %s\n", response->body);
  } 
  freeMessage(response);
}

void GetTextMessages(int sock) {
  Message *request, *response;
  int status, messageCount;
  char *messages;
  
  request = createMessage(GET, userName);
  /* send request */
  status = sendMessage(sock, request);
  freeMessage(request);
  if(status < 0)
    fprintf(stderr, "Error when sending request for messages\n");
  
  /* wait for response */
  response = readMessageFromSocket(sock);
  if(response->opCode == OK) {
    memcpy(&messageCount, response->body, sizeof(int));
    printf("You have %d message(s)\n", messageCount);
    messages = response->body + sizeof(int);
    printf("%s\n", messages);  
  }
  else {
    fprintf(stderr, "Error: %s\n", response->body);
  }
  freeMessage(response);
}
