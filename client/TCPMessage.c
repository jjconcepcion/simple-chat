#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdbool.h>

enum code { LOGIN, LIST, SEND, GET, OK, FAIL};

typedef struct message {
  unsigned int length;
  enum code opCode;
  unsigned int bodyLength;
  char * body;
  
} Message;

Message *createMessage(enum code opcode, char* body) {
  Message *msg;
  
  msg = (Message*) malloc(sizeof(Message));
  msg->opCode = opcode;
  msg->bodyLength = strlen(body);
  msg->body = (char*) malloc(msg->bodyLength + 1);
  if(msg->bodyLength > 0)
    strncpy(msg->body, body, msg->bodyLength);
  msg->body[msg->bodyLength] = '\0';
  msg->length = sizeof(int)*2 + sizeof(enum code) + msg->bodyLength + 1;

  return msg;
}

void freeMessage(Message *msg) {
  if(msg->body != NULL) {
    free(msg->body);
  }
  free(msg);
}

Message *readMessageFromSocket(int sock) {
  Message *msg = (Message*) malloc(sizeof(Message));
  bool error = false;
  
  /* get message metadata: length, opcode, bodylength */
  if( recv(sock, &(msg->length), sizeof(int), 0) <=0 )
    error = true;
  if( recv(sock, &(msg->opCode), sizeof(enum code), 0) <=0 )
    error = true;
  if( recv(sock, &(msg->bodyLength), sizeof(int), 0) <=0 )
    error = true;
  
  msg->body = (char*) malloc(msg->bodyLength+1); 
  /* get body */
  if(msg->bodyLength > 0) {
    if( recv(sock, msg->body, msg->bodyLength, 0) <=0 )
      error = true;  
  }
  msg->body[msg->bodyLength] = '\0';
  
  if(error) {
    free(msg);
    msg = NULL;
  }
  
  return msg;
}

int sendMessage(int sock, Message *msg) {
  int error = -1;
  int retval = 0;
  
  /* send individual message sections: length, opcode, bodyLength, body */
  if(send(sock, &(msg->length), sizeof(int), 0) != sizeof(int))
    retval = error; 
  if(send(sock, &(msg->opCode), sizeof(enum code), 0) != sizeof(enum code))
    retval = error;
  if(send(sock, &(msg->bodyLength), sizeof(int), 0) != sizeof(int))
    retval = error;
  if(msg->bodyLength > 0) {
    if(send(sock, msg->body, msg->bodyLength, 0) != msg->bodyLength)
      retval = error;
  }
 
  return retval;
}
