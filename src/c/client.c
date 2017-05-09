
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 1024
#define SEND_FILE -2
#define CLOSE_SOCKET -3

typedef struct sockaddr_in my_socket;
typedef struct hostent my_server;
char *fileName = NULL;

void removeSubstring(char *s,const char *toremove)
{
  while( s=strstr(s,toremove) )
    memmove(s,s+strlen(toremove),1+strlen(s+strlen(toremove)));
}

int sendMessage(char *buf, int *sockfd){
  printf("Please enter msg: ");
  fscanf(stdin, "%s", buf);
  int result;

  /* compare if you read a tag to send the file*/
  result = send(*sockfd, buf, strlen(buf), 0);
  if(strcmp(buf, ":file") == 0) result = SEND_FILE;
  else if(strcmp(buf, "$die") == 0) result = CLOSE_SOCKET;

  return result;
}

int receiveMessage(char *buf, int *sockfd){
  int bytes = recv(*sockfd, buf, BUFSIZE, 0);
  fprintf(stdout, "server> %s \n", buf);

  return bytes;
}

int sendFile(char *buf, int *sockfd){
  FILE *inputFile;
  int fileSize, len;

  fprintf(stdout, "Indicate file's path: ");
  len = fscanf(stdin, "%s", buf);
  buf[strlen(buf)] = '\0';

  /* Keep the fileName so we can overwrite it once the server
   * replies to the request.
   */
  if(fileName != NULL) free(fileName);
  fileName = (char *) malloc(strlen(buf) + 1);
  strcpy(fileName, buf);
  // fileName[strlen(buf)] = '\0';
  // fprintf(stdout, "File to copy %s\n", fileName);


  inputFile = fopen(buf, "r");
  if(inputFile == NULL){
    fprintf(stderr, "ERROR while opening the file: %s\n", buf);
    exit(1);
  }

  /*
  * First send a message to the server, indicating that we will send a file
  * Then, lets read and send the file
  * Finally, tell the server your reached the end of the file with :eof
  */
  len = sprintf(buf, ":file");
  buf[len] = '\0';
  send(*sockfd, buf, strlen(buf), 0);

  while(fgets(buf, BUFSIZE, inputFile) != NULL){
    send(*sockfd, buf, strlen(buf), 0);
  }

  len = sprintf(buf, ":eof");
  buf[len] = '\0';
  send(*sockfd, buf, strlen(buf), 0);

  fclose(inputFile);
  return 0;
}

int receiveFile(char *buf, int *sockfd){
  FILE *out;
  short finished = 0;

  out = fopen(fileName, "w");
  if(out == NULL){
    fprintf(stdout, "Error while opening the file %s\n", fileName);
    exit(1);
  }

  while(!finished){
    bzero(buf, BUFSIZE);

    recv(*sockfd, buf, BUFSIZE, 0);
    if(strstr(buf, ":eof") != NULL){
      finished = 1;
      removeSubstring(buf, ":eof");
    }

    fprintf(out, "%s\n", buf);
  }

  fprintf(stdout, "INFO> File stored as: %s\n", fileName);
  fclose(out);
  return 10;
}


int main(int argc, char **argv) {
  int sockfd, portno, msgBytes;
  my_socket serveraddr;
  my_server *server;

  char *hostname;
  // char buf[BUFSIZE];
  char *buf = (char *) malloc(BUFSIZE + 1);
  short writeFlag = 0;
  short readFileFlag = 0;
  short terminate = 0;

  if (argc != 3) {
   fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
   exit(1);
  }
  hostname = argv[1];
  portno = atoi(argv[2]);


  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    fprintf(stderr, "ERROR opening socket");
    exit(1);
  }

  /* gethostbyname: get the server's DNS entry */
  server = gethostbyname(hostname);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host as %s\n", hostname);
    exit(1);
  }

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy(
    (char *)server->h_addr,
    (char *)&serveraddr.sin_addr.s_addr,
    server->h_length
  );
  serveraddr.sin_port = htons(portno);


  /* connect: create a connection with the server */
  if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0){
    fprintf(stderr, "ERROR connecting\n");
    exit(1);
  }

  writeFlag = 1;
  do{
    /* Clean the buffer before sending or recieving data */
    bzero(buf, BUFSIZE);

    if(writeFlag){
      msgBytes = sendMessage(buf, &sockfd);
      if(msgBytes == SEND_FILE){
        sendFile(buf, &sockfd);
        readFileFlag = 1;
      }
      else if(msgBytes == CLOSE_SOCKET){
        terminate = 1;
        fprintf(stdout, "Error closing the socket\n");
        continue;
      }

      writeFlag = 0;
    }
    else{
      if(readFileFlag == 1){
        msgBytes = receiveFile(buf, &sockfd);
        terminate = 1;
      }
      else{
        msgBytes = receiveMessage(buf, &sockfd);

      }
      writeFlag = 1;

      if(msgBytes < 0){
        perror("Something went wrong with the server");
        terminate = 1;
      }
    }

  }while(!terminate);

  close(sockfd);
  return 0;
}
