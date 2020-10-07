#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <unistd.h>

typedef struct {
    int turnoff;
    int input;
} reader_args_t;

void* reader_task(void* argv) {
    reader_args_t *arg = (reader_args_t*) argv;
    int epfd = epoll_create(4);

    struct epoll_event event;
    event.data.fd = arg->turnoff;
    event.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, arg->turnoff, &event);
    event.data.fd = arg->input;
    event.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, arg->input, &event);

    for (;;) {
        struct epoll_event events[2];
        int nfds = epoll_wait(epfd, events, 2, 10000);
        for (int i=0; i<nfds; ++i) {
            char buf[4096];
            ssize_t n = read(events[i].data.fd, buf, sizeof(buf));
            if (n>0) {
                buf[n] = '\0';
                puts(buf);
            }
        }
    }

    return NULL;
}

int main() {
    pthread_t tid;
    int rc;

    reader_args_t args = {
        .turnoff = 1,
        .input = STDIN_FILENO
    };
    rc = pthread_create(&tid, NULL, reader_task, &args);
    if (rc!=0) {
        perror("pthread_create() failed");
        exit(1);
    }

    rc = pthread_join(tid, NULL);
    if (rc!=0) {
        perror("pthread_join() failed");
        exit(1);
    }
    return 0;
}
