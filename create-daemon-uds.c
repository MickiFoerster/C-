#include <syslog.h>
#include <signal.h>
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
#include <sys/stat.h>
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
    syslog(LOG_DEBUG, "received: %s\n", buf);
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

  unlink(buf);
  sd = socket(PF_LOCAL, SOCK_STREAM, 0);
  if (sd < 0)
  {
    syslog(LOG_ERR, "socket() failed: %s", strerror(errno));
    exit(1);
  }

syslog(LOG_DEBUG, "hello");
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, buf, sizeof(addr.sun_path));

  rc = bind(sd, (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0)
  {
    syslog(LOG_ERR, "bind() failed: %s", strerror(errno));
    close(sd);
    exit(1);
  }

syslog(LOG_DEBUG, "hello");
  rc = listen(sd, 1);
  if (rc < 0)
  {
    syslog(LOG_ERR, "listen() failed: %s", strerror(errno));
    close(sd);
    exit(1);
  }

  addrlen = sizeof(struct sockaddr_in);
  channel = accept(sd, (struct sockaddr *)&addr, &addrlen);
  rc = fcntl(channel, F_SETFL, fcntl(channel, F_GETFL) | O_NONBLOCK);
  assert(rc == 0);

  syslog(LOG_INFO, "start task");
  while (shouldTaskContinue(channel))
  {
    int sleeptime = (rand() + 100000) % 1000000;
    syslog(LOG_DEBUG, "sleep for %dms\n", sleeptime / 1000);
    usleep(sleeptime);
  }

  syslog(LOG_DEBUG, "terminating\n");
  close(channel);

  exit(0);
}

void startDaemon() {
  int i;
  struct sockaddr_un addr;
  int rc;
  int client;
  char buf[256];
  pid_t pid;

  snprintf(buf, 256, "%sthread%d", CHANNEL_PATH, rand());
  fprintf(stderr, "UDS path is %s\n", buf);
   
  pid = fork();
  if (pid==-1) {
    perror("fork() failed");
    exit(EXIT_FAILURE);
  } else if (pid!=0) { // parent process
    exit(EXIT_SUCCESS);
  }
  // child process

  // get session leader
  rc = setsid();
  if (rc==-1) {
    perror("setsid() failed");
    exit(EXIT_FAILURE);
  }

  // ignore signals
  {
    int sigs[] = {SIGHUP, SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};
    struct sigaction action;
    int i;

    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    for(i=0; i<sizeof(sigs)/sizeof(sigs[0]); ++i) {
      if (sigaction(sigs[i], &action, NULL) < 0) {
        perror("sigaction() failed");
        exit(EXIT_FAILURE);
      }
    }
  }

  pid = fork();
  if (pid==-1) {
    perror("fork() failed");
    exit(EXIT_FAILURE);
  } else if (pid!=0) { // parent process
    exit(EXIT_SUCCESS);
  }
  // child process
  chdir("/");
  umask(0);

  for(i=sysconf(_SC_OPEN_MAX); i>=0; --i) {
    close(i);
  }

  openlog("daemon", LOG_PID | LOG_CONS | LOG_NDELAY, LOG_LOCAL0);
  syslog(LOG_DEBUG, "daemon ready");
  task(buf);
  syslog(LOG_DEBUG, "daemon terminates");
}

int main()
{
  srand(time(NULL));
  startDaemon();
  return 0;
}