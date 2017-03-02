#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h> /* for close() */
#include <stdlib.h>
#include <string.h>

#define RCVBUFSIZE 32 /* Size of receive buffer */
#define USERFILE_PATH "./data/users.dat"
#define MESSAGE_DIR "./messages/"
#define MAX_USERS 100
#define MAX_NAME_LEN 20
#define MSG_BUF_SIZE 4096
#define MAX_PATH_LEN 64
#define MAX_MSG 100
#define MAX_MSG_LEN 4096

void DieWithError(char *errorMessage); /* Error handling function */
void SendUserList(int clientSocket, char *filename);
void StoreUserMessage(int clientSocket, char *directory);
void SendUserMessages(int clientSocket, char *directory);
void itoa(int n, char s[]);
  
void HandleTCPClient(int clientSocket)
{
  char requestBuffer[RCVBUFSIZE] = {0}; /* Buffer for echo string */
  int bytesToRead;

  for(;;) {
    bytesToRead = 0;
    
    if (recv(clientSocket, &bytesToRead, sizeof(int), 0) < 0)
      DieWithError("recv() failed");

    while(bytesToRead > 0)
    {
      /* Receive request from client */
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
      else 
      {
        fprintf(stderr, "SOMETHING ELSE\n"); 
      }
      
      if (recv(clientSocket, &bytesToRead, sizeof(int), 0) < 0)
        DieWithError("recv() failed");
    } 
    
  }
}

void SendUserList(int clientSocket, char *filename)
{
  char users[MAX_USERS][MAX_NAME_LEN] = {0};
  int count, index, prefix;
  FILE *file;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  
  file = fopen(filename, "r");
  if(file == NULL)
    DieWithError("fopen() failed");
  
  /* read users file into array */
  count = 0;
  while((read = getline(&line, &len, file)) > 0)
  {
    line[read-1] = '\0'; //replace newline
    
    strncpy(users[count], line, read);
    count++;
  }
  
  char openingMsg[32] = {0};
  char tmpStr1[] = "There are \0";
  char tmpStr2[] = " user(s)\0";
  char number[10] = {0};
  itoa(count, number);
  
  strncat(openingMsg, tmpStr1, strlen(tmpStr1));
  strncat(openingMsg, number, strlen(number));
  strncat(openingMsg, tmpStr2, strlen(tmpStr2));
  
  len = strlen(openingMsg);
  
  prefix = len;
  if(send(clientSocket, &prefix, sizeof(int), 0) != sizeof(int))
    DieWithError("send() failed");
  
  if (send(clientSocket, openingMsg, len, 0) != len)
    DieWithError("send() failed");
  
  /* send user names */
  index = 0;
  while(index < count)
  {
    len = strlen(users[index]);
    
    fprintf(stderr, "%s\n", users[index]);
        
    /* send length of message */
    prefix = (int)len;
    if(send(clientSocket, &prefix, sizeof(int), 0) != sizeof(int))
      DieWithError("send() failed");
    
    //send username
    if (send(clientSocket, users[index], len, 0) != len)
      DieWithError("send() failed");
    
    index++;
  }
  
  /* signal end of messages*/
  prefix = 0;
  if(send(clientSocket, &prefix, sizeof(int), 0) != sizeof(int))
    DieWithError("send() failed");
    
  free(line);
  fclose(file);
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
  
  fprintf(stderr, "SENDING...\n");
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
      
      fprintf(stderr, ">%s\n", sendBuffer);
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
