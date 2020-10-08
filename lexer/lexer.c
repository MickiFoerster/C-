#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct {
    char pattern[32];
    int match_idx;
    void* (*handler)(void*);
} pattern_t;

const pattern_t patterns[2] = {
    {.pattern = " login: ", .handler = NULL, .match_idx = 0},
    {.pattern = "AAAA", .handler = NULL, .match_idx = 0}};
const unsigned num_patterns = sizeof(patterns)/sizeof(patterns[0]);

int lexer(unsigned char buf, ssize_t n) {
  /*
      * at least one match (first match counts)
      * no pattern matches
          * is enough input available or could a pattern match as soon as more
     bytes are available
             -> read more until it is clear that no pattern matches
          * enough input available -> go to next position



 What if all patterns do not match? Then search starts from next position
in buffer. But this is potentially from last call to lexer()
  */
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = 0; j < num_patterns; ++j) {
      if (patterns[j].match_idx < 0) {
        continue;
      }
      pattern_t *p = &patterns[j];
      if (p->pattern[p->match_idx] == buf[i]) {
        p->match_idx++;
      } else {
        p->match_idx = -1;
      }

      // End of pattern? => match found
      if (p->pattern[p->match_idx] == '\0') {
        for (size_t k = 0; k < num_patterns; ++k) {
          patterns[k].match_idx = 0;
        }
        return j;
      }
    }
  }

  return -1;
}

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
      unsigned char buf[4096];
      ssize_t n = read(events[i].data.fd, buf, sizeof(buf));
      fprintf(stderr, "read() returned %ld bytes\n", n);
      if (n > 0) {
        int match = lexer(buf, n);
        if (match >= 0) {
          fprintf(stderr, "Found match %s\n", patterns[match].pattern);
        }
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
