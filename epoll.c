#include <pthread.h>
#include <stdio.h>     // for fprintf()
#include <string.h>    // for strncmp
#include <sys/epoll.h> // for epoll_create1(), epoll_ctl(), struct epoll_event
#include <unistd.h>    // for close(), read()

#define MAX_EVENTS 5
#define READ_SIZE 10

void *epoll(void *argv) {
  int running = 1;
  int event_count;
  int i;
  size_t bytes_read;
  char read_buffer[READ_SIZE + 1];
  struct epoll_event event;
  struct epoll_event events[MAX_EVENTS];
  int epoll_fd = epoll_create1(0);

  if (epoll_fd == -1) {
    fprintf(stderr, "Failed to create epoll file descriptor\n");
    return NULL;
  }

  event.events = EPOLLIN;
  event.data.fd = 0;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event)) {
    fprintf(stderr, "Failed to add file descriptor to epoll\n");
    close(epoll_fd);
    return NULL;
  }

  while (running) {
    printf("\nPolling for input...\n");
    event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 30000);
    printf("%d ready events\n", event_count);
    for (i = 0; i < event_count; i++) {
      printf("Reading file descriptor '%d' -- ", events[i].data.fd);
      bytes_read = read(events[i].data.fd, read_buffer, READ_SIZE);
      printf("%zd bytes read.\n", bytes_read);
      read_buffer[bytes_read] = '\0';
      printf("Read '%s'\n", read_buffer);

      if (!strncmp(read_buffer, "stop\n", 5))
        running = 0;
    }
  }

  if (close(epoll_fd)) {
    fprintf(stderr, "Failed to close epoll file descriptor\n");
    return NULL;
  }

  return NULL;
}

int main() {
  pthread_t tid;
  int rc = pthread_create(&tid, NULL, epoll, NULL);
  if (rc != 0) {
    fprintf(stderr, "could not create thread\n");
    return 1;
  }
  rc = pthread_detach(tid);
  if (rc != 0) {
    fprintf(stderr, "could not detach thread\n");
    return 1;
  }

  pthread_exit(NULL);

  return 0;
}