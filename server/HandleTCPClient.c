#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h> /* for close() */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define RCVBUFSIZE 128 /* Size of receive buffer */
#define USERFILE_PATH "./data/users.dat"
#define PASSWD_PATH "./data/passwd"
#define MESSAGE_DIR "./messages/"
#define MAX_USERS 100
#define MAX_NAME_LEN 20
#define MSG_BUF_SIZE 4096
#define MAX_PATH_LEN 1024
#define MAX_MSG 100
#define MAX_MSG_LEN 4096

enum code { LOGIN, LIST, SEND, GET, OK, FAIL};

typedef struct message {
  unsigned int length;
  enum code opCode;
  unsigned int bodyLength;
  char * body;
  
} Message;

extern char userList[MAX_USERS][MAX_NAME_LEN+1];
extern unsigned int userListCount;

void DieWithError(char *errorMessage); /* Error handling function */
void Authenticate(int clientSocket, char *path);
void itoa(int n, char s[]);

Message *readMessageFromSocket(int sock);
Message *createMessage(enum code opcode, char* body);
int sendMessage(int sock, Message *msg);
void freeMessage(Message *msg);
Message *UserList();
int StoreUserMessage(int clientSocket, Message *request, char *directory);
Message *UserMessages(Message *request, char *directory);
  
void HandleTCPClient(int clientSocket)
{
  Message *request, *response;
  int status;

  for(;;) {
    request = readMessageFromSocket(clientSocket);
    
    if(request == NULL)
      exit(1);
    
    switch(request->opCode) {
      case LIST:
        response = UserList();
        printf("Return user list!\n");
        break;
      case SEND:
        status = StoreUserMessage(clientSocket, request, MESSAGE_DIR);
        if(status == 0)
          response = createMessage(OK,"");
        else
          response = createMessage(FAIL, "server failed to store message\n");
        break;
      case GET:
        response = UserMessages(request, MESSAGE_DIR);
        break;
      default:
        printf("DEFAULT\n");
        break;
    }
    
    sendMessage(clientSocket, response);
    freeMessage(request);
    freeMessage(response);
  }
}

Message *UserList() {
  Message *response;
  char *name, *body, *startOfList;
  unsigned int bytesCopied, index, nameLength;
  
  /* allocate maximum space for newline delimited list of names */
  body = (char*) malloc(userListCount*(MAX_NAME_LEN+1)+1);
  bzero(body,userListCount*(MAX_NAME_LEN+1)+1);
  /* insert total user count into first 4 bytes of body */
  memcpy(body, &userListCount, sizeof(int));
  
  bytesCopied = sizeof(int);
  startOfList = body + sizeof(int);
  index = 0;
  /* add each username to message body */
  while(index < userListCount) { 
    name = userList[index];
    nameLength = strlen(name);
    
    strncat(startOfList, name, nameLength);
    strncat(startOfList, "\n", 1);
    
    bytesCopied += nameLength+1;
    index++;
  }
  
  response = (Message*) malloc(sizeof(Message));
  response->opCode = OK;
  response->bodyLength = bytesCopied;
  response->body = realloc(body, bytesCopied+1);
  response->length = sizeof(int)*2 + sizeof(enum code) + bytesCopied + 1;
  
  return response;
}

int StoreUserMessage(int clientSocket, Message *request, char *directory) {
  char path[MAX_PATH_LEN+1] = {0};
  char *name, *message;
  FILE *file;
  int status = 0;
  
  name = strtok(request->body, ":");
  message = strtok(NULL, "");
  /* generate appropriate message file path */
  strncat(path, directory, strlen(directory));
  strncat(path, name, strlen(name));
  
  file = fopen(path, "a");
  if(file == NULL)
    status = -1;
  fprintf(file, "%s\n", message);
  fclose(file);
  
  fprintf(stderr,"A message to: %s\n", name);
  fprintf(stderr,"Message: %s\n", message);
  
  return status;
}

Message *UserMessages(Message *request, char *directory) {
  Message *response = NULL;
  char path[MAX_PATH_LEN+1] = {0};
  char *body, *startOfMessages, *name;
  unsigned int bytesCopied, bytesAllocated, messageCount;
  FILE *file;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  
  /* generate path to user's named message file */
  strncat(path, directory, strlen(directory));
  strncat(path, request->body, request->bodyLength);
  
  /* allocate space for newline delimited list of messages */
  bytesAllocated = MAX_MSG_LEN+1;
  body = (char*) malloc(bytesAllocated);
  bzero(body,bytesAllocated);
  startOfMessages = body + sizeof(int);
  
  file = fopen(path, "r");
  if (file == NULL) {
    response = createMessage(FAIL, "Messsage(s) could not be retrieved\n");
  }
  
  bytesCopied = sizeof(int); // bytes reserved for final message count
  messageCount = 0;
  while ((read = getline(&line, &len, file)) != -1) {
    /* grow allocated space if necessary */
    if( bytesAllocated <= (bytesCopied + read)) {
      body = (char*) realloc(body, bytesAllocated+MAX_MSG_LEN+1);
      startOfMessages = body + sizeof(int);
    }
      
    strncat(startOfMessages, line, strlen(line));
    bytesCopied += strlen(line);
    messageCount++;
  }
  
  response = (Message*) malloc(sizeof(Message));
  memcpy(body, &messageCount, sizeof(int)); // insert total message count
  body[bytesCopied] = '\0';
  response->opCode = OK;
  response->bodyLength = bytesCopied;
  response->body = realloc(body, bytesCopied+1);
  response->length = sizeof(int)*2 + sizeof(enum code) + bytesCopied + 1;

  free(line);
  fclose(file);
  
  name = request->body;
  printf("Send back %s's messages!\n", name);
  
  return response;
}

void Authenticate(int clientSocket, char *path)
{
  int bytesToRead;
  char recvBuffer[RCVBUFSIZE] = {0};
  char recvUserName[MAX_NAME_LEN] = {0};
  char recvPassWord[64] = {0};
  char *token;
     
  /* get credentials */
  if(recv(clientSocket, &bytesToRead, sizeof(int), 0) <= 0)
    DieWithError("recv() failed or connection closed prematurely"); 
  if(recv(clientSocket, recvBuffer, bytesToRead, 0) <= 0)
    DieWithError("recv() failed or connection closed prematurely"); 
  
  token = strtok(recvBuffer, ":");
  strncpy(recvUserName, token, strlen(token));
  printf("Log in User name is %s\n", recvUserName);
  
  token = strtok(NULL, ":");
  strncpy(recvPassWord, token, strlen(token));
  printf("Log in Password is %s\n", recvPassWord);
  
}
