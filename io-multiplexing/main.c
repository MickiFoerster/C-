/* Example basing on https://devarea.com/linux-io-multiplexing-select-vs-poll-vs-epoll/#.XYpu3OYzYcn" */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#define MAXBUF 256
#define SELECT 1
#define POLL 2

#if IOMULTIPLEXING == POLL
#include <poll.h>
#elif IOMULTIPLEXING == EPOLL
#include <sys/epoll.h>
#endif

void child_process(void) {
  sleep(2);
  char msg[MAXBUF];
  struct sockaddr_in addr = {0};
  int n, sockfd, num = 1;
  srandom(getpid());
  /* Create socket and connect to server */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(2000);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));

  printf("child {%d} connected \n", getpid());
  while (1) {
    int sl = (random() % 10) + 1;
    num++;
    sleep(sl);
    sprintf(msg, "Test message %d from client %d", num, getpid());
    n = write(sockfd, msg, strlen(msg)); /* Send message */
  }
}

int main() {
  char buffer[MAXBUF];
  int fds[5];
#if IOMULTIPLEXING == POLL
  struct pollfd pollfds[5];
#endif
  struct sockaddr_in addr;
  struct sockaddr_in client;
  int i, max = 0;
  socklen_t addrlen;
  int sockfd;
  fd_set rset;
  for (i = 0; i < 5; i++) {
    if (fork() == 0) {
      child_process();
      exit(0);
    }
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(2000);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
  listen(sockfd, 5);

#if IOMULTIPLEXING == SELECT
  fprintf(stderr, "select\n");
#include "select.c"
#elif IOMULTIPLEXING == POLL
  fprintf(stderr, "poll\n");
#include "poll.c"
#elif IOMULTIPLEXING == EPOLL
  fprintf(stderr, "epoll\n");
#include "epoll.c"
#endif

  return 0;
}
