#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>

int main(void) {
  int i;
  int n;
  const char *user = getenv("USER");
  char homefolder[strlen("/home/") + strlen(user) + 1];
  int watch_fd;
  int inotify_instance_fd = inotify_init();
  char *buffer;
  const size_t buffer_len = 1024 * sizeof(struct inotify_event);

  if (inotify_instance_fd < 0) {
    perror("inotify_init() failed");
  }

  snprintf(homefolder, sizeof(homefolder), "/home/%s", user);
  watch_fd =
      inotify_add_watch(inotify_instance_fd, homefolder, IN_CREATE | IN_DELETE);

  buffer = (char *)malloc(buffer_len);

  for (;;) {
    n = read(inotify_instance_fd, buffer, buffer_len);
    if (n < 0) {
      perror("read() failed");
    }

    for (i = 0; i < n;) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (event->len) {
        if (event->mask & IN_CREATE) {
          if (event->mask & IN_ISDIR) {
            printf("New directory %s created.\n", event->name);
          } else {
            printf("New file %s created.\n", event->name);
          }
        } else if (event->mask & IN_DELETE) {
          if (event->mask & IN_ISDIR) {
            printf("Directory %s deleted.\n", event->name);
          } else {
            printf("File %s deleted.\n", event->name);
          }
        }
      }
      i += sizeof(struct inotify_event) + event->len;
    }
  }

  free(buffer);
  inotify_rm_watch(inotify_instance_fd, watch_fd);
  close(inotify_instance_fd);

  return 0;
}
