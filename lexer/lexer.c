#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct {
  int turnoff;
  int input;
} reader_args_t;

void *reader_task(void *argv) {
  reader_args_t *arg = (reader_args_t *)argv;
  int epfd = epoll_create(4);

  struct epoll_event event;
  event.data.fd = arg->input;
  event.events = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, arg->input, &event);
  event.data.fd = arg->turnoff;
  event.events = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, arg->turnoff, &event);

  for (;;) {
    struct epoll_event events[2];
    int nfds = epoll_wait(epfd, events, 2, 10000);
    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == arg->turnoff) {
        close(arg->input);
        return NULL;
      }
      fprintf(stderr, "fd %d can be read\n", events[i].data.fd);
      char buf[4096];
      ssize_t n = read(events[i].data.fd, buf, sizeof(buf));
      fprintf(stderr, "read() returned %ld bytes\n", n);
      if (n > 0) {
        buf[n] = '\0';
        puts(buf);
      }
    }
  }

  return NULL;
}

int main() {
  int pipe_to_thread[2];
  pthread_t tid;
  int rc;

  rc = pipe(pipe_to_thread);
  if (rc != 0) {
    perror("pipe() failed");
    exit(EXIT_FAILURE);
  }

  reader_args_t args = {.turnoff = pipe_to_thread[PIPE_READ],
                        .input = STDIN_FILENO};
  rc = pthread_create(&tid, NULL, reader_task, &args);
  if (rc != 0) {
    perror("pthread_create() failed");
    exit(EXIT_FAILURE);
  }

  // Signal TERM to thread
  sleep(5);
  fprintf(stderr, "Signal thread to terminate\n");
  ssize_t n = write(pipe_to_thread[PIPE_WRITE], "", 1);
  if (n < 0) {
    perror("write() failed");
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "write() wrote %ld bytes\n", n);

  rc = pthread_join(tid, NULL);
  if (rc != 0) {
    perror("pthread_join() failed");
    exit(EXIT_FAILURE);
  }
  return 0;
}
