// Opens child process that is steered with help of pseudo terminal device
// This code bases on example code found here:
// https://man7.org/tlpi/code/online/dist/pty/pty_fork.c.html

#if !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE < 600
#define _XOPEN_SOURCE 600
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *reader(void *argv) {
  int masterFd = *(int *)argv;
  for (;;) {
    unsigned char buf[4096];
    ssize_t n = read(masterFd, buf, sizeof(buf));
    if (n > 0) {
      for (int i = 0; i < n; ++i) {
        printf("%c", buf[i]);
      }
      fflush(stdout);
    }
  }
}

int main(void) {
  int masterFd = open("/dev/ptmx", O_RDWR | O_NOCTTY); // Open pty master
  if (masterFd == -1) {
    perror("open(/dev/ptmx) failed");
    exit(1);
    }

    if (grantpt(masterFd) == -1) { // Grant access to slave pty
      perror("grantpt() failed");
      close(masterFd);
      exit(1);
    }

    if (unlockpt(masterFd) == -1) { // Unlock slave pty
      perror("unlockpt() failed");
      close(masterFd);
      exit(1);
    }

    char *slaveName = ptsname(masterFd); // Get slave pty name
    if (slaveName == NULL) {
      perror("ptsname() failed");
      close(masterFd);
      exit(1);
    }

    pid_t childPid = fork(); // fork child process
    if (childPid == -1) {    // fork failed
      perror("fork() failed");
      close(masterFd);
      exit(1);
    } else if (childPid != 0) { // Parent process
      pthread_t tid;
      if (pthread_create(&tid, NULL, reader, &masterFd)) {
        perror("pthread_create failed");
        close(masterFd);
        exit(1);
      }
      if (pthread_detach(tid)) {
        perror("pthread_detach failed");
        close(masterFd);
        exit(1);
      }

      sleep(3);
      const char cmds[2][32] = {"ls -l\n", "hostname\n"};
      printf("%s", cmds[0]);
      write(masterFd, cmds[0], strlen(cmds[0]));
      sleep(3);
      printf("%s", cmds[1]);
      write(masterFd, cmds[1], strlen(cmds[1]));
      sleep(3);
    } else { // Child process

      if (setsid() == -1) { // start new session
        perror("setsid() failed");
        close(masterFd);
        exit(1);
      }

      close(masterFd); // not needed in child

      int slaveFd = open(slaveName, O_RDWR); // Becomes controlling tty
      if (slaveFd == -1) {
        perror("open(slavePty) failed");
        close(masterFd);
        exit(1);
      }

      if (dup2(slaveFd, STDIN_FILENO) != STDIN_FILENO ||
          dup2(slaveFd, STDOUT_FILENO) != STDOUT_FILENO ||
          dup2(slaveFd, STDERR_FILENO) != STDERR_FILENO) {
        perror("Redirecting standard I/O devices failed");
        close(masterFd);
        exit(1);
      }

      if (slaveFd > STDERR_FILENO) {
        close(slaveFd); // No longer needed file descriptor
      }

      char *const args[] = {NULL};
      if (execv("/bin/bash", args) < 0) {
        perror("execv() failed");
        close(masterFd);
        exit(1);
      }
      assert(0 && "This point won't be reached");
    }

    return 0;
}
