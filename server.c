#include<stdlib.h>
#include<pthread.h>
#include<sys/socket.h>
#include<sys/types.h>       //pthread_t , pthread_attr_t and so on.
#include<stdio.h>
#include<netinet/in.h>      //structure sockaddr_in
#include<arpa/inet.h>       //Func : htonl; htons; ntohl; ntohs
#include<assert.h>          //Func :assert
#include<string.h>          //Func :memset
#include<unistd.h>          //Func :close,write,read
#include<errno.h>
#include<signal.h>

#define SOCK_PORT 5000
#define SOCK_PORT2 5100
#define PAKCETSIZE 4096


static int whichfd;
static void Data_handle_up(void * sock_fd);
static void Data_handle_down(void * sock_fd);

int accept_any(int fds[], unsigned int count, struct sockaddr *addr, socklen_t *addrlen)
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
    whichfd = fd;
    if (fd == -1)
        return -1;
    else
        return accept(fd, addr, addrlen);
}

int main()
{

    int sockfd_server;
    int sockfd;
    int fd_temp;
    struct sockaddr_in s_addr_in;
    struct sockaddr_in s_addr_client;
    int client_length;

    sockfd_server = socket(AF_INET,SOCK_STREAM,0);  //ipv4,TCP
    assert(sockfd_server != -1);

    //before bind(), set the attr of structure sockaddr.
    memset(&s_addr_in,0,sizeof(s_addr_in));
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);  //trans addr from uint32_t host byte order to network byte order.
    s_addr_in.sin_port = htons(SOCK_PORT);          //trans port from uint16_t host byte order to network byte order.
    fd_temp = bind(sockfd_server,(&s_addr_in),sizeof(s_addr_in));

    if(fd_temp == -1)
    {
        fprintf(stderr,"bind error!\n");
        exit(1);
    }

    fd_temp = listen(sockfd_server,127);
    if(fd_temp == -1)
    {
        fprintf(stderr,"listen error!\n");
        exit(1);
    }

    int sockfd_server2;
    int fd_temp2;
    struct sockaddr_in s_addr_in2;

    sockfd_server2 = socket(AF_INET,SOCK_STREAM,0);  //ipv4,TCP
    assert(sockfd_server2 != -1);

    //before bind(), set the attr of structure sockaddr.
    memset(&s_addr_in2,0,sizeof(s_addr_in2));
    s_addr_in2.sin_family = AF_INET;
    s_addr_in2.sin_addr.s_addr = htonl(INADDR_ANY);  //trans addr from uint32_t host byte order to network byte order.
    s_addr_in2.sin_port = htons(SOCK_PORT2);          //trans port from uint16_t host byte order to network byte order.
    fd_temp2 = bind(sockfd_server2,(&s_addr_in2),sizeof(s_addr_in2));

    if(fd_temp2 == -1)
    {
        fprintf(stderr,"bind error!\n");
        exit(1);
    }

    fd_temp2 = listen(sockfd_server2,127);
    if(fd_temp2 == -1)
    {
        fprintf(stderr,"listen error!\n");
        exit(1);
    }

    int sockfds[2] = {sockfd_server, sockfd_server2};

    while(1)
    {
        printf("waiting for new connection...\n");
        pthread_t thread_id;
        client_length = sizeof(s_addr_client);
        printf("what new connection? \n");
        //Block here. Until server accpets a new connection.
        sockfd = accept_any(sockfds, 2,(&s_addr_client),(socklen_t *)(&client_length));
        if(sockfd == -1)
        {
            fprintf(stderr,"Accept error!\n");
            continue;                               //ignore current socket ,continue while loop.
        }
        printf("whichfd %d, sockfd_server %d, sockfd_server2 %d\n", whichfd, sockfd_server, sockfd_server2 );
        printf("A new connection occurs!\n");
        if(whichfd == sockfd_server){
          int ret = pthread_create(&thread_id,NULL,(void *)(&Data_handle_up),(void *)(&sockfd));
          if(ret == -1)
          {
              fprintf(stderr,"pthread_create error!\n");
              break;                                  //break while loop
          }
        }else if(whichfd == sockfd_server2){
          int ret = pthread_create(&thread_id,NULL,(void *)(&Data_handle_down),(void *)(&sockfd));
          if(ret == -1)
          {
              fprintf(stderr,"pthread_create error!\n");
              break;                                  //break while loop
          }
        }
    }

    //Clear
    int ret = close(sockfd_server); //shut down the all or part of a full-duplex connection.
    assert(ret != -1);

    printf("Server shuts down\n");
    return 0;
}

static void Data_handle_up(void * sock_fd)
{
    int fd = *((int *)sock_fd);
    int i_recvBytes;
    char data_recv[PAKCETSIZE];

    while(1)
    {
        //printf("waiting for request...\n");
        //Reset data.
        memset(data_recv,0,PAKCETSIZE);

        i_recvBytes = read(fd,data_recv,PAKCETSIZE);
        if(i_recvBytes == 0)
        {
            printf("Maybe the client has closed\n");
            break;
        }
        if(i_recvBytes == -1)
        {
            perror("read");
        }
        //printf("read from client : %s\n",data_recv);
    }

    //Clear
    printf("terminating current client_connection...\n");
    close(fd);            //close a file descriptor.
    pthread_exit(NULL);   //terminate calling thread!
}



static void Data_handle_down(void * sock_fd)
{
    signal(SIGPIPE, SIG_IGN);
    int fd = *((int *)sock_fd);
    int i_sendBytes;
    char data_to_send[PAKCETSIZE];

    while(1)
    {
        //Reset data.
        memset(data_to_send,' ',PAKCETSIZE);

        i_sendBytes = write(fd,data_to_send,PAKCETSIZE);


        if(i_sendBytes == 0)
        {
          printf("Maybe the client has closed\n");
          break;
        }
        if(i_sendBytes == -1)
        {
          break;
        }
    }

    //Clear
    printf("terminating current client_connection...\n");
    close(fd);            //close a file descriptor.
    pthread_exit(NULL);   //terminate calling thread!
}
