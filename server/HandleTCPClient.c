#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h> /* for close() */
#include <stdlib.h>
#include <string.h>

#define RCVBUFSIZE 32 /* Size of receive buffer */
#define USERFILE_PATH "./data/users.dat"

void DieWithError(char *errorMessage); /* Error handling function */
void SendUserList(int clientSocket, char *filename);
  
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

      fprintf(stderr,"Request: %s\nlength: %d", requestBuffer, bytesToRead);
      
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
  FILE *file;
  char *line = NULL;
  //char buffer[64];
  size_t len = 0;
  ssize_t read;
  int prefix;
  
  file = fopen(filename, "r");
  if(file == NULL)
    DieWithError("fopen() failed");
  
  while((read = getline(&line, &len, file)) > 0)
  {
    line[read-1] = '\0'; //replace newline
    
    /* send length of message */
    prefix = (int)read;
    if(send(clientSocket, &prefix, sizeof(int), 0) != sizeof(int))
      DieWithError("send() failed");
    
    //send username
    if (send(clientSocket, line, read, 0) != read)
      DieWithError("send() failed");
  }
  
  /* signal end of messages*/
  prefix = 0;
  if(send(clientSocket, &prefix, sizeof(int), 0) != sizeof(int))
    DieWithError("send() failed");
    
  free(line);
  fclose(file);
}
