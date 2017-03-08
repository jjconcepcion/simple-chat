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
#define MAX_PATH_LEN 64
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
void StoreUserMessage(int clientSocket, char *directory);
void SendUserMessages(int clientSocket, char *directory);
void Authenticate(int clientSocket, char *path);
void itoa(int n, char s[]);

Message *readMessageFromSocket(int sock);
Message *createMessage(enum code opcode, char* body);
int sendMessage(int sock, Message *msg);
void freeMessage(Message *msg);
Message *UserList();
  
void HandleTCPClient(int clientSocket)
{
  Message *request, *response;

  for(;;) {
    request = readMessageFromSocket(clientSocket);
    
    if(request == NULL)
      exit(1);
    
    switch(request->opCode) {
      case LIST:
        response = UserList();
        printf("Return user list!\n");
        break;
      default:
        printf("DEFAULT\n");
        break;
    }
    
    sendMessage(clientSocket, response);
    freeMessage(response);
    
    /*
    bytesToRead = 0;
    
    if (recv(clientSocket, &bytesToRead, sizeof(int), 0) < 0)
      DieWithError("recv() failed");

    while(bytesToRead > 0)
    {
     
      if (recv(clientSocket, requestBuffer, bytesToRead, 0) < 0)
        DieWithError("recv() failed");

      if(strncmp(requestBuffer, "USERLIST", bytesToRead) == 0)
      {
        SendUserList(clientSocket, USERFILE_PATH);
      }
      else if(strncmp(requestBuffer, "SEND_MSG", bytesToRead) == 0) 
      {
        StoreUserMessage(clientSocket, MESSAGE_DIR);
      }
      else if(strncmp(requestBuffer, "GET_MSG", bytesToRead) == 0) 
      {
        SendUserMessages(clientSocket, MESSAGE_DIR);
      }
      else if(strncmp(requestBuffer, "LOGIN", bytesToRead) == 0)
      {
        Authenticate(clientSocket, PASSWD_PATH);
      }
      
      if (recv(clientSocket, &bytesToRead, sizeof(int), 0) < 0)
        DieWithError("recv() failed");
    } 
    */
    
  }
}

Message *UserList() {
  Message *response;
  char *name, *body, *startOfList;
  unsigned int bytesCopied, index, nameLength, count;
  
  /* allocate maximum space for newline delimited list of names */
  body = (char*) malloc(userListCount*(MAX_NAME_LEN+1)+1);
  /* insert total user count into first 4 bytes of body */
  count = userListCount;
  memcpy(body, &count, sizeof(int));
  
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

void StoreUserMessage(int clientSocket, char *directory)
{
  FILE *file;
  char path[MAX_PATH_LEN] = {0};
  char data[MSG_BUF_SIZE+MAX_NAME_LEN+1];
  char recvBuffer[RCVBUFSIZE]= {0};
  char *user, *msg;
  int bytesToRead;
  
  /* bytesToRead gets length of next message */
  if(recv(clientSocket, &bytesToRead, sizeof(int), 0) <= 0)
    DieWithError("recv() failed or connection closed prematurely");
  
  while(bytesToRead > 0)
  {
    /* recieve message segments */
    if(recv(clientSocket, recvBuffer, bytesToRead, 0) <= 0)
      DieWithError("recv() failed or connection closed prematurely");
    
    strncat(data, recvBuffer, bytesToRead);
    
    if (recv(clientSocket, &bytesToRead, sizeof(int), 0) <= 0)
      DieWithError("recv() failed or connection closed prematurely");
  }
  
  /* parse relevant tokens */
  user = strtok(data, ":");
  msg = strtok(NULL, "\n");
  
  /* generate path for message file */
  strncat(path, directory, strlen(directory));
  strncat(path, user, strlen(user));
  
  /* append to file */
  file = fopen(path, "a");
  fprintf(file, "%s\n", msg);
  fclose(file);
}

void SendUserMessages(int clientSocket, char *directory)
{
  char recvBuffer[RCVBUFSIZE] = {0};
  char sendBuffer[RCVBUFSIZE] = {0};
  char user[MAX_NAME_LEN] = {0};
  char path[MAX_PATH_LEN] = {0};
  int bytesToRead;
  
  /* get username */
  if(recv(clientSocket, &bytesToRead, sizeof(int), 0) <= 0)
    DieWithError("recv() failed or connection closed prematurely"); 
  if(recv(clientSocket, recvBuffer, bytesToRead, 0) <= 0)
    DieWithError("recv() failed or connection closed prematurely"); 
  
  strncpy(user, recvBuffer, bytesToRead);
  
  /* generate path to message file */
  strncat(path, directory, strlen(directory));
  strncat(path, user, strlen(user));
  
  /* open file for reading */
  char messages[MAX_MSG][MAX_MSG_LEN] = {0};
  FILE *file;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int count;
  int bytesToSend;
  
  file = fopen(path, "r");
  if(file == NULL)
    DieWithError("fopen() failed");
  
  /* read message file into messages */
  count = 0;
  while((read = getline(&line, &len, file)) > 0)
  {
    line[read-1] = '\0'; //replace newline
    strncpy(messages[count], line, read);
    count++;
  }
  
  char openingMsg[32] = {0};
  char tmpStr1[] = "You have \0";
  char tmpStr2[] = " message(s)\0";
  char number[10] = {0};
  itoa(count, number);
  
  strncat(openingMsg, tmpStr1, strlen(tmpStr1));
  strncat(openingMsg, number, strlen(number));
  strncat(openingMsg, tmpStr2, strlen(tmpStr2));
  
  bytesToSend = strlen(openingMsg);
  if(send(clientSocket, &bytesToSend, sizeof(int), 0) != sizeof(int))
    DieWithError("send() failed");
  if (send(clientSocket, openingMsg, bytesToSend, 0) != bytesToSend)
    DieWithError("send() failed");
  
  /* send messages */
  int index = 0;
  char *remaining;
  while(index < count)
  {
    remaining = messages[index];
    fprintf(stderr, "%lu:%s", strlen(messages[index]), messages[index]);
    /* message in segments */
    while(strlen(remaining) > RCVBUFSIZE) 
    {
      strncpy(sendBuffer, remaining, RCVBUFSIZE);
      
      bytesToSend = RCVBUFSIZE;
      if(send(clientSocket, &bytesToSend, sizeof(int), 0) != sizeof(int))
        DieWithError("send() failed");
      if (send(clientSocket, sendBuffer, bytesToSend, 0) != bytesToSend)
        DieWithError("send() failed");
      
      remaining += RCVBUFSIZE;
    }
    
    /* send final segment */
    bytesToSend = strlen(remaining);
    if(send(clientSocket, &bytesToSend, sizeof(int), 0) != sizeof(int))
      DieWithError("send() failed");
    if (send(clientSocket, remaining, bytesToSend, 0) != bytesToSend)
      DieWithError("send() failed");

    index++; //next message
  }
  
  /* signal end of messages*/
  bytesToSend = 0;
  if(send(clientSocket, &bytesToSend, sizeof(int), 0) != sizeof(int))
    DieWithError("send() failed");
    
  free(line);
  fclose(file);
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
