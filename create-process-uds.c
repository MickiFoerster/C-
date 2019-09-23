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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#define CHANNEL_PATH "/tmp/UDSCHANNEL"
static const char msg[] = "Please terminate!";

bool shouldTaskContinue(int channel)
{
  char buf[32];
  size_t n = read(channel, buf, sizeof(buf));
  switch (n)
  {
  case -1:
    switch (errno)
    {
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
    if (strcmp(buf, msg) == 0)
    {
      return false;
    }
    break;
  }
  return true;
}

void task(const char *buf)
{
  const char answer[] = "Terminating";
  struct sockaddr_un addr;
  int sd;
  int rc;
  int channel;
  socklen_t addrlen;

  fprintf(stderr, "path is %s\n", buf);
  unlink(buf);
  sd = socket(PF_LOCAL, SOCK_STREAM, 0);
  if (sd < 0)
  {
    perror("socket() failed");
    exit(1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, buf, sizeof(addr.sun_path));

  rc = bind(sd, (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0)
  {
    perror("bind() failed");
    close(sd);
    exit(1);
  }

  rc = listen(sd, 1);
  if (rc < 0)
  {
    perror("listen() failed");
    close(sd);
    exit(1);
  }

  addrlen = sizeof(struct sockaddr_in);
  channel = accept(sd, (struct sockaddr *)&addr, &addrlen);
  rc = fcntl(channel, F_SETFL, fcntl(channel, F_GETFL) | O_NONBLOCK);
  assert(rc == 0);

  while (shouldTaskContinue(channel))
  {
    int sleeptime = (rand() + 100000) % 1000000;
    fprintf(stderr, "sleep for %dms\n", sleeptime / 1000);
    usleep(sleeptime);
  }

  fprintf(stderr, "terminating\n");
  close(channel);

  exit(0);
}

int startProcess(void (*task)(const char *))
{
  struct sockaddr_un addr;
  int rc;
  int client;
  char buf[256];
  pid_t pid;

  snprintf(buf, 256, "%sthread%d", CHANNEL_PATH, rand());
 
  pid = fork();
  if (pid==-1) {
    perror("fork() failed");
    exit(1);
  } else if (pid==0) { // child process
    task(buf);
    assert(0 && "Process MUST end inside of task()");
  }

  // parent process
  fprintf(stderr, "child process has been created with PID %d\n", pid);

  client = socket(PF_LOCAL, SOCK_STREAM, 0);
  if (client < 0)
  {
    perror("socket() failed");
    exit(1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, buf, sizeof(addr.sun_path));

  // Try to connect in a loop since connection may be not available at this point.
  // This loop will finally succeed since thread was successfully created above.
  while ((rc = connect(client, (struct sockaddr *)&addr, sizeof(addr)) != 0))
    ;

  return client;
}

int main()
{
  int i;
  const int numThreads = 32;
  int channel[numThreads];

  srand(time(NULL));

  for (i = 0; i < numThreads; ++i)
  {
    channel[i] = startProcess(task);
  }

  sleep(2);
  for (i = 0; i < numThreads; ++i)
  {
    fprintf(stderr, "Signal thread %d to terminate\n", i);
    write(channel[i], msg, sizeof(msg));
  }

  for (i = 0; i < numThreads; ++i)
  {
    char buf[1];
    fprintf(stderr, "Wait for thread %d to finish its termination\n", i);
    // Block until channel is closed by thread
    read(channel[i], buf, sizeof(buf));
    close(channel[i]);
    fprintf(stderr, "Thread %d has finished\n", i);
  }

  pthread_exit(NULL);
  return 0;
}