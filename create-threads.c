#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char msg[] = "Please terminate!";

typedef struct {
  int cancel;
  int done;
} channel;

bool shouldTaskContinue(int channel) {
  char buf[32];
  size_t n = read(channel, buf, sizeof(buf));
  switch (n) {
  case -1:
    switch (errno) {
    case EWOULDBLOCK:
      break;
    default:
      perror("read() failed");
      return false;
      break;
    }
    break;
  case 0:
    break;
  default:
    buf[n] = '\0';
    fprintf(stderr, "received: %s\n", buf);
    if (strcmp(buf, msg) == 0) {
      return false;
    }
    break;
  }
  return true;
}

void *task(void *argv) {
  const char answer[] = "Terminating";
  channel *ch = (channel *)argv;
  int cancel = ch->cancel;
  int done = ch->done;

  assert(argv);

  while (shouldTaskContinue(cancel)) {
    int sleeptime = (rand() + 100000) % 1000000;
    fprintf(stderr, "sleep for %dms\n", sleeptime / 1000);
    usleep(sleeptime);
  }
  close(cancel);
  free(ch);
  fprintf(stderr, "terminating\n");
  write(done, "", 1);
  return NULL;
}

bool startThread(void *(*task)(void *), int *cancel, int *done) {
  pthread_t tid;
  int rc;
  int *chCancel = (int *)malloc(sizeof(int) * 2);
  int *chDone = (int *)malloc(sizeof(int) * 2);
  channel *ch = (channel *)malloc(sizeof(channel));

  rc = pipe(chCancel);
  assert(rc == 0);
  rc = pipe(chDone);
  assert(rc == 0);

  // end of pipe for reading should not be non-blocking
  rc = fcntl(chCancel[0], F_SETFL, fcntl(chCancel[0], F_GETFL) | O_NONBLOCK);
  assert(rc == 0);

  // set pipe ends for thread that is to be created
  ch->cancel = chCancel[0];
  ch->done = chDone[1];
  // set pipe ends for caller of startThread()
  *cancel = chCancel[1];
  *done = chDone[0];

  rc = pthread_create(&tid, NULL, task, ch);
  assert(rc == 0);

  rc = pthread_detach(tid);
  assert(rc == 0);

  return true;
}

int main() {
  int i;
  const int numThreads = 2;
  int cancel[numThreads];
  int done[numThreads];

  srand(time(NULL));

  for (i = 0; i < numThreads; ++i) {
    startThread(task, &cancel[i], &done[i]);
  }

  sleep(2);
  for (i = 0; i < numThreads; ++i) {
    fprintf(stderr, "Signal thread %d to terminate\n", i);
    write(cancel[i], msg, sizeof(msg));
    close(cancel[i]);
  }
  for (i = 0; i < numThreads; ++i) {
    char buf[1];
    size_t n;
    fprintf(stderr, "Wait for thread %d to finish its termination\n", i);
    n = read(done[i], buf, sizeof(buf));
    assert(n == 1);
    close(done[i]);
    fprintf(stderr, "Thread %d has finished\n", i);
  }

  pthread_exit(NULL);
  return 0;
}