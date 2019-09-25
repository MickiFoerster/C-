/*
 * create a context in the kernel using epoll_create
 * add and remove file descriptors to/from the context using epoll_ctl
 * wait for events in the context using epoll_wait
 */

struct epoll_event events[5];
int epfd = epoll_create(10);
for (i = 0; i < 5; i++) {
  static struct epoll_event ev;
  memset(&client, 0, sizeof(client));
  addrlen = sizeof(client);
  ev.data.fd = accept(sockfd, (struct sockaddr *)&client, &addrlen);
  ev.events = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev);
}

while (1) {
  int nfds;
  puts("round again");
  nfds = epoll_wait(epfd, events, 5, 10000);

  for (i = 0; i < nfds; i++) {
    memset(buffer, 0, MAXBUF);
    read(events[i].data.fd, buffer, MAXBUF);
    puts(buffer);
  }
}

/*
Epoll vs Select/Poll
We can add and remove file descriptor while waiting
epoll_wait returns only the objects with ready file descriptors
epoll has better performance – O(1) instead of O(n)
epoll can behave as level triggered or edge triggered (see man page)
epoll is Linux specific so non portable
*/
