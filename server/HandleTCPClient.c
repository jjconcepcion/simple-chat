#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h> /* for close() */
#include <stdlib.h>
#include <string.h>

#define RCVBUFSIZE 32 /* Size of receive buffer */
#define USERFILE_PATH "./data/users.dat"
#define MAX_USERS 100
#define MAX_NAME_LEN 20

void DieWithError(char *errorMessage); /* Error handling function */
void SendUserList(int clientSocket, char *filename);
void itoa(int n, char s[]);
  
void HandleTCPClient(int clientSocket)
{
  char requestBuffer[RCVBUFSIZE] = {0}; /* Buffer for echo string */
  int bytesToRead;

  for(;;) {
    bytesToRead = 0;
    
    if (recv(clientSocket, &bytesToRead, sizeof(int), 0) < 0)
      DieWithError("recv() failed");

    while (bytesToRead > 0)
    {
      /* Receive request from client */
      if (recv(clientSocket, requestBuffer, bytesToRead, 0) < 0)
        DieWithError("recv() failed");

      if (strncmp(requestBuffer, "USERLIST", bytesToRead) == 0)
      {
        SendUserList(clientSocket, USERFILE_PATH);
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

void SendUserList(int clientSocket, char *filename) {
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
