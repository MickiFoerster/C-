#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

#define CHANNEL_PATH "/tmp/UNIX_DOMAIN_SOCKET_FILE"
static const char msg[] = "Please terminate!\n";
static bool prgContinue = true;

static void *reader(void *argv) {
  char buf[32];
  struct epoll_event events[5];
  int channel = *((int *)argv);
  int rc;
  int epoll_fd = 0;
  struct epoll_event event;
  struct epoll_event events[4];
  int event_count;

  free(argv);
  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    syslog(LOG_ERR, "could not create epoll instance\n");
    close(channel);
    exit(1);
  }

  event.events = EPOLLIN;
  event.data.fd = channel;
  rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, channel, &event);
  if (rc != 0) {
    perror("epoll_ctl() failed");
    close(channel);
    exit(1);
  }

  syslog(LOG_DEBUG, "reader started with connection %d", channel);
  while (prgContinue) {
    int i;

    event_count = epoll_wait(epoll_fd, events, sizeof(events), 1000);
    syslog(LOG_DEBUG, "%d file descriptors are ready to read", event_count);
    for (i = 0; i < event_count; i++) {
      ssize_t n = read(channel, buf, sizeof(buf));
      switch (n) {
      case -1:
        switch (errno) {
        case EWOULDBLOCK:
          continue;
          break;
        default:
          syslog(LOG_DEBUG, "read() failed: %s", strerror(errno));
          exit(1);
          break;
        }
        break;
      case 0:
        break;
      default:
        buf[n] = '\0';
        syslog(LOG_DEBUG, "received: %s %ld\n", buf, n);
        if (strcmp(buf, msg) == 0) {
          prgContinue = false;
        } else {
          syslog(LOG_DEBUG, "buf: %s %ld", buf, strlen(buf));
          syslog(LOG_DEBUG, "msg: %s %ld", msg, strlen(msg));
        }
        break;
      }
    }
  }
  close(channel);
  syslog(LOG_DEBUG, "reader stopped");
  return NULL;
}

void newReader(int channel) {
  int *client;
  int rc;
  pthread_t tid;

  syslog(LOG_DEBUG, "client accepted");
  rc = fcntl(channel, F_SETFL, fcntl(channel, F_GETFL) | O_NONBLOCK);
  assert(rc == 0);
  client = (int *)malloc(sizeof(int));
  assert(client);
  *client = channel;
  syslog(LOG_DEBUG, "client connection %d", *client);
  rc = pthread_create(&tid, NULL, reader, client);
  if (rc < 0) {
    syslog(LOG_DEBUG, "could not create thread for new connection: %s",
           strerror(errno));
    exit(1);
  }
  rc = pthread_detach(tid);
  if (rc < 0) {
    syslog(LOG_DEBUG, "could not detach thread for new connection: %s",
           strerror(errno));
    exit(1);
  }
}

void *listenerThread(void *argv) {
  struct sockaddr_un addr;
  int sd;
  int rc;
  int channel;
  socklen_t addrlen;
  char *buf = (char *)argv;

  syslog(LOG_DEBUG, "listener started");
  unlink(buf);
  sd = socket(PF_LOCAL, SOCK_STREAM, 0);
  if (sd < 0) {
    syslog(LOG_ERR, "socket() failed: %s", strerror(errno));
    exit(1);
  }

  rc = fcntl(sd, F_SETFL, fcntl(sd, F_GETFL) | O_NONBLOCK);
  assert(rc == 0);

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, buf, sizeof(addr.sun_path));

  rc = bind(sd, (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0) {
    syslog(LOG_ERR, "bind() failed: %s", strerror(errno));
    close(sd);
    exit(1);
  }

  rc = listen(sd, 1);
  if (rc < 0) {
    syslog(LOG_ERR, "listen() failed: %s", strerror(errno));
    close(sd);
    exit(1);
  }

  syslog(LOG_DEBUG, "start server loop");

  while (prgContinue) {
    addrlen = sizeof(struct sockaddr_in);
    syslog(LOG_DEBUG, "start accept loop");
    while (prgContinue) {
      channel = accept(sd, (struct sockaddr *)&addr, &addrlen);
      if (channel == -1) {
        if (errno == EWOULDBLOCK) {
          usleep(1000);
          continue;
        }
        syslog(LOG_DEBUG, "accept() failed: %s", strerror(errno));
        exit(1);
      }
      break;
    }

    if (prgContinue) {
      newReader(channel);
    }
  }
  syslog(LOG_DEBUG, "listener stopped");

  return NULL;
}

void task(char *buf) {
  int rc;
  pthread_t tid;

  syslog(LOG_INFO, "start task");
  rc = pthread_create(&tid, NULL, listenerThread, buf);
  if (rc < 0) {
    perror("could not create listener thread");
    exit(1);
  }
  rc = pthread_detach(tid);
  if (rc < 0) {
    perror("could not detach listener thread");
    exit(1);
  }

  while (prgContinue) {
    int sleeptime = (rand() % 10000000) + 1000000;
    syslog(LOG_DEBUG, "sleep for %dms\n", sleeptime / 1000);
    usleep(sleeptime);
  }

  syslog(LOG_DEBUG, "terminating\n");

  exit(0);
}

void startDaemon() {
  int i;
  int rc;
  // char buf[256];
  pid_t pid;

  // snprintf(buf, 256, "%sthread%d", CHANNEL_PATH, rand());
  // fprintf(stderr, "UDS path is %s\n", buf);

  pid = fork();
  if (pid == -1) {
    perror("fork() failed");
    exit(EXIT_FAILURE);
  } else if (pid != 0) { // parent process
    exit(EXIT_SUCCESS);
  }
  // child process

  // get session leader
  rc = setsid();
  if (rc == -1) {
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
    for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); ++i) {
      if (sigaction(sigs[i], &action, NULL) < 0) {
        perror("sigaction() failed");
        exit(EXIT_FAILURE);
      }
    }
  }

  pid = fork();
  if (pid == -1) {
    perror("fork() failed");
    exit(EXIT_FAILURE);
  } else if (pid != 0) { // parent process
    exit(EXIT_SUCCESS);
  }
  // child process
  chdir("/");
  umask(0);

  for (i = sysconf(_SC_OPEN_MAX); i >= 0; --i) {
    close(i);
  }

  openlog("daemon", LOG_PID | LOG_CONS | LOG_NDELAY, LOG_LOCAL0);
  syslog(LOG_DEBUG, "daemon ready");
  task(CHANNEL_PATH);
  syslog(LOG_DEBUG, "daemon terminates");
}

int main() {
  srand(time(NULL));
  startDaemon();
  return 0;
}
