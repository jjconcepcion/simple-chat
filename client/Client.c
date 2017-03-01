#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */
#include <stdlib.h> /* for atoi() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */

#define RCVBUFSIZE 32 /* Size of receive buffer */

void DieWithError(char *errorMessage);
void PrintMenuOptions();
void ConnectToServer(int sock, struct sockaddr_in serverAddr);

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
    
    int error;
    switch(option) {
      case '0': 
        ConnectToServer(sock, serverAddr);
        break;
      case '1': 
        printf("you entered: %c\n", option);
        break;
      case '2': 
        printf("you entered: %c\n", option);
        break;
      case '3': 
        printf("you entered: %c\n", option);
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

void PrintMenuOptions(void) {
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
