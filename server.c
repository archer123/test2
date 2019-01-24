
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include <pthread.h>


#define PACKETSIZE 2000


int send_packet(int fd) {
  char buffer[PACKETSIZE];

  memset(buffer + sizeof(struct timeval), ' ', (unsigned int)sizeof(buffer) - (unsigned int)sizeof(struct timeval));
  int ret = send(fd, buffer, PACKETSIZE, 0);
  if (ret != PACKETSIZE) {
    perror("send");
    exit(1);
  }
  return ret;
}


int accept_any(int fds[], unsigned int count, struct sockaddr *addr, socklen_t *addrlen, int *whichfd)
{
    fd_set readfds;
    int maxfd, fd;
    unsigned int i;
    int status;

    FD_ZERO(&readfds);
    maxfd = -1;
    for (i = 0; i < count; i++) {
        FD_SET(fds[i], &readfds);
        if (fds[i] > maxfd)
            maxfd = fds[i];
    }
    status = select(maxfd + 1, &readfds, 0, 0, 0);
    if (status < 0)
        return -1;
    fd = -1;
    for (i = 0; i < count; i++)
        if (FD_ISSET(fds[i], &readfds)) {
            fd = fds[i];
            break;
        }
    if (fd == -1)
        return -1;
    else
        return accept(fd, addr, addrlen);
}




void *set_socket(void *port) {

  int thisport = *((int*)port);
  printf("this port : %d\n", thisport);

  struct sockaddr_in server;

  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(thisport);

  server.sin_addr.s_addr = htons(INADDR_ANY);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(1);
  }
  // Set SO_REUSEADDR on.
  int on = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
    perror("setsockopt");
    exit(1);
  }

  int flags = fcntl(fd, F_GETFL, 0);

  if (flags == -1) {
    perror("fcntl");
    exit(1);
  }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
    perror("fcntl");
    exit(1);
  }

  

  if (bind(fd, (struct sockaddr*)&server, sizeof(server)) != 0) {
    perror("bind");
    exit(1);
  }
  

  if (listen(fd, 127) != 0) {
    perror("listen");
    exit(1);
  }

  fd_set fds;
  int maxfd = fd;
  struct timeval tv;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  while (1) {
    fd_set rfds;
    memcpy(&rfds, &fds, sizeof(fds));
    tv.tv_sec  = 1;
    tv.tv_usec = 0;

    int ready = select(maxfd + 1, &rfds, 0, 0, &tv);

    if (ready == -1) {
      perror("select");
      exit(1);
    }

    if (ready == 0) {

      continue;
    }

    for (int i = 3; i <= maxfd; i++) {
      if (FD_ISSET(i, &rfds)) {
        //printf ("%d, %d \n", i, fd);
        if (i == fd) {
          while (1) {
            int nfd = accept(fd, 0, 0);

            if (nfd == -1) {
              if (errno != EWOULDBLOCK) {
                perror("accept");
                exit(1);
              }

              break;
            }

            FD_SET(nfd, &fds);

            if (nfd > maxfd) {
              maxfd = nfd;
            }
          }
        } else {

          char buffer[PACKETSIZE];

          while (1) {
            printf("thisport %d\n", thisport);
            if (thisport == 5000){
              int n = recv(i, buffer, PACKETSIZE, MSG_DONTWAIT);

              if (n != PACKETSIZE) {
                if (errno == EWOULDBLOCK) {
                  break;
                } else if (errno == ECONNRESET) {
                  // Close the socket.
                  n = 0;
                } else {
                  perror("recv");
                  exit(1);
                }
              }
              printf ("recv data %d\n", n);

              if (n == 0) {

                close(i);

                FD_CLR(i, &fds);

                if (i == maxfd) {
                  while (!FD_ISSET(maxfd, &fds)) {
                    maxfd--;
                  }
                }

                break;
              }
            }
            
            if (thisport == 5100){
              printf("sending \n" );          
              int n = send_packet(i);
              
            }
            

          }
        }
      }
    }
  }
}



int main() {

  // int fdup = create_socket(5000);
  // int fddown = create_socket(5100);

  // int fds[2] = {fdup, fddown};
  // unsigned int count = 2;
  // int* whichfd = NULL;
  // printf("%d\n", count);
  // if (accept_any(fds, count, 0, 0, whichfd)==-1){
  //     perror("accept");
  // }
  // bool updown = (whichfd == fdup);

   pthread_t threads[2];
   int index[2];
   index[0] = 5000;
   index[1] = 5100;

   for (int i = 0; i < 2; i++) {
        printf("main() : 创建线程, %d\n", i);

        int ret = pthread_create(&threads[i], NULL, set_socket, (void*)&(index[i]));
        if (ret != 0) {
            printf("pthread_create error: error_code = %d\n", ret);
            exit(-1);
        }
        
    }
    pthread_exit(NULL);
}