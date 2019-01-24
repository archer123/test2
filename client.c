
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#define PACKETSIZE 4096

int create_socket(const char* ip, int port) {
  struct sockaddr_in client;
  memset(&client, 0, sizeof(client));
  client.sin_family      = AF_INET;
  client.sin_port        = port;
  client.sin_addr.s_addr = htons(INADDR_ANY);

  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in other;
  memset(&other, 0, sizeof(other));
  other.sin_family = AF_INET;
  other.sin_port   = htons(port);

  if (inet_aton(ip, &other.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }

  if (connect(fd, (struct sockaddr*)&other, sizeof(other)) != 0) {
    perror("connect");
    exit(1);
  }

  // get port for every socket
  socklen_t sother = sizeof(other);
  int s = getsockname(fd, (struct sockaddr *)&other, &sother);
  if (s<0){
    perror("getsockname");
  }
  else{
    printf("port number %d\n", ntohs(other.sin_port));
  }

  return fd;
}

int send_packet(int fd) {
  char buffer[PACKETSIZE];
  memset(buffer, ' ', (unsigned int)sizeof(buffer));
  int ret = write(fd, buffer, PACKETSIZE);
  if (ret != PACKETSIZE) {
    perror("send");
    exit(1);
  }
  return ret;
}



int main(int argc,const char* argv[]) {
  
  int nbsocket = 1;
  const char* ip = "132.227.122.38";
  bool updown = true;

  if (argc >2){
    ip = argv[1];
    nbsocket = atoi(argv[2]);
    updown = (strcmp(argv[3],"UP") == 0);
  }


  printf("transport to %s using %d sockets\n", ip, nbsocket);

  uint32_t sent = 0;
  uint32_t recved = 0;
  time_t next = time(NULL) + 1;
  int maxfd = 0;
  fd_set fds;
  FD_ZERO(&fds);
  int fd;

  for (int i = 0; i < nbsocket; ++i) {
    if(updown){
      fd = create_socket(ip, 5000);
    }else {
      printf("%d\n",5100);
      fd = create_socket(ip, 5100);
    }


    //send_packet(fd);
    FD_SET(fd, &fds);
    if (fd > maxfd) {
      maxfd = fd;
    }
  }

  while (1) {
    time_t now = time(NULL);
    if(updown){
      // Upload part
      char sbuffer[32];
      if (now > next) {
        snprintf(sbuffer, sizeof(sbuffer), "%u mbits/s",  sent * PACKETSIZE * 8 / 1000000 );
        printf("Sent %u packets per second, upload throughput is %s\n", sent, sbuffer);

        next++;

        sent = 0;
      }
      for (int i = 3; i <= maxfd; ++i) {
        if (FD_ISSET(i, &fds)) {
          send_packet(i);
          sent ++;
          // printf("send data %d\n", ret);
        }
      }
    }else{

      //Download part
      char rbuffer[32];
      if (now > next) {
        snprintf(rbuffer, sizeof(rbuffer), "%u mbits/s",  recved * PACKETSIZE * 8 / 1000000 );
        printf("recieve %u packets per second, download throughput is %s\n", recved, rbuffer);

        next++;

        recved = 0;
      }
      for (int i = 3; i <= maxfd; ++i) {
        if (FD_ISSET(i, &fds)) {

          char buffer[PACKETSIZE];
          int n = read(i, buffer, sizeof(buffer));
          recved += n;
          //printf("n: %d",n);

        }
      }
    }
  }
}
