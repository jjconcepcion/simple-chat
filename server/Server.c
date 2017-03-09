#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h> /* for atoi() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */
#include <stdbool.h>

#define MAXPENDING 5 /* Maximum outstanding connection requests */
#define LOCALHOST "127.0.0.1"
#define DEFAULT_PORT 8000
#define USERFILE_PATH "./data/users.dat"
#define PASSWD_PATH "./data/passwd"
#define MAX_NAME_LEN 20
#define MAX_PASS_LEN 64
#define MAX_USERS 100

typedef struct credential {
  char username[MAX_NAME_LEN+1];
  char password[MAX_PASS_LEN+1];
} Credential;

Credential credentials[MAX_USERS] = {0};
int totalUsers = 0;

char userList[MAX_USERS][MAX_NAME_LEN+1] = {0};
unsigned int userListCount = 0;

void DieWithError(char *errorMessage); /* Error handling function */
void HandleTCPClient(int clientSocketet); /* TCP client handling function */
void InitializeUserList(char* path);
void InitializeCredentialsArray(char *path);
Credential *ParseCredentials(char *authInfo);
bool IsAuthorized(Credential *user);

int main(int argc, char *argv[])
{
  int serverSocket; /* Socket descriptor for server */
  int clientSocket; /* Socket descriptor for client */
  struct sockaddr_in serverAddr; /* Local address */
  struct sockaddr_in clientAddr; /* Client address */
  unsigned short serverPort; /* Server port */
  unsigned int clntLen; /* Length of client address data structure */
  unsigned int connectCount = 0;

  if (argc > 2) /* Test for correct number of arguments */
  {
    fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]) ;
    exit(1);
  }

  if(argc == 2)
    serverPort = atoi(argv[1]); /* First arg: local port */
  else
    serverPort = DEFAULT_PORT;

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
  
  InitializeUserList(USERFILE_PATH);
  InitializeCredentialsArray(PASSWD_PATH);
  
  printf("Server Started!\n");
  printf("Listen on %s:%d\n" , LOCALHOST, serverPort);
  
  for (;;) /* Run forever */
  {
    /* Set the size of the in-out parameter */
    clntLen = sizeof(clientAddr);
    /* Wait for a client to connect */
    if ((clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &clntLen)) < 0)
      DieWithError("accept() failed");
    /* clientSocket is connected to a client! */
    connectCount++;
    printf("Client %d connected\n", connectCount);
    HandleTCPClient (clientSocket);
  }
/* NOT REACHED */
}

void InitializeUserList(char* path) {
  FILE *file;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
 
  file = fopen(path, "r");
  if (file == NULL) {
    DieWithError("Error opening Userlist file\n");
  }
  
  userListCount = 0;
  while ((read = getline(&line, &len, file)) != -1) {
    line[read-1] = '\0'; //replace newline
    strncpy(userList[userListCount], line, strlen(line));
    userListCount++;
  }

  free(line);
  fclose(file);
}


void InitializeCredentialsArray(char *path) {
  FILE *file;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char *username, *password;
 
  file = fopen(path, "r");
  if (file == NULL) {
    fprintf(stderr,"Error opening passwd file\n");
    exit(1);
  }
  
  totalUsers = 0;
  while ((read = getline(&line, &len, file)) != -1) {
    username = strtok(line, ":");
    password = strtok(NULL, "\n");
    
    strncpy(credentials[totalUsers].username, username,strlen(username));
    strncpy(credentials[totalUsers].password, password,strlen(password));
    totalUsers++;
  }

  free(line);
  fclose(file); 
}

Credential *ParseCredentials(char *authInfo) {
  char tokens[MAX_NAME_LEN+MAX_PASS_LEN+2] = {0};
  char *username, *password;
  Credential *cred;
  
  strncpy(tokens, authInfo, strlen(authInfo));
  username = strtok(tokens, ":");
  password = strtok(NULL, "\0");

  cred = (Credential*) malloc(sizeof(Credential));  
  bzero(cred, sizeof(Credential));
  strncpy(cred->username, username, strlen(username));
  strncpy(cred->password, password, strlen(password));
  
  return cred;
}

bool IsAuthorized(Credential *user) {
  bool authorized = false;
  Credential *next;
  unsigned int index, nameLength, passLength; 
  
  nameLength = strlen(user->username);
  passLength = strlen(user->password);
  
  index = 0;
  while(index < totalUsers) {
    next = &credentials[index];
    index++;
    
    if(nameLength != strlen(next->username) ||
       passLength != strlen(next->password)) {
      continue;
    }
    
    if((strncmp(user->username, next->username, nameLength) == 0) &&
       (strncmp(user->password, next->password, passLength)== 0)) {
      authorized = true;
      break;
    }
  }
  
  return authorized;
}
