#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */
#include <stdlib.h> /* for atoi() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */

#define RCVBUFSIZE 32 /* Size of receive buffer */
#define MSG_BUF_SIZE 4096
#define MAX_NAME_LEN 20

void DieWithError(char *errorMessage);
void PrintMenuOptions();
void ConnectToServer(int sock, struct sockaddr_in serverAddr);
void GetUserList(int sock);
void SendTextMessage(int sock);
void GetTextMessages(int sock);
  
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


  /* Establish the connection to the echo server */
  if (connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    DieWithError(" connect () failed");

}

void GetUserList(int sock)
{
  const char request[] = "USERLIST\0";
  char recvBuffer[RCVBUFSIZE] = {0};
  int bytesToRead = 0;
  int bytesToSend;
  int prefix;
  
  bytesToSend = strlen(request);
  
  /* send message length */
  prefix = bytesToSend;
  if(send(sock, &prefix, sizeof(int), 0) != sizeof(int))
    DieWithError("send() sent a different number of bytes than expected");

  /* send request */
  if(send(sock, request, bytesToSend, 0) != bytesToSend)
    DieWithError("send() sent a different number of bytes than expected");

  /* bytesToRead gets length of next message */
  if(recv(sock, &bytesToRead, sizeof(int), 0) <= 0)
    DieWithError("recv() failed or connection closed prematurely");
  
  while(bytesToRead > 0)
  {
    /* recieve response */
    if(recv(sock, recvBuffer, bytesToRead, 0) <= 0)
      DieWithError("recv() failed or connection closed prematurely");
    
    recvBuffer[bytesToRead] = '\0';
    
    printf("%s\n", recvBuffer);
    
    if (recv(sock, &bytesToRead, sizeof(int), 0) <= 0)
      DieWithError("recv() failed or connection closed prematurely");
  }
}

void SendTextMessage(int sock)
{
  char request[] = "SEND_MSG";
  char user[MAX_NAME_LEN];
  char message[MSG_BUF_SIZE];
  char buffer[MAX_NAME_LEN+MSG_BUF_SIZE+1];
  char sendBuffer[RCVBUFSIZE];
  int sendLength;
  char *remaining;
  
  printf("Please Enter the user name: ");
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
