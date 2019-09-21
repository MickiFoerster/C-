#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

typedef struct {
    int fds[2];
} channel;

static const char msg[] = "Please terminate!";

void* task(void* argv) {
    channel* ch = (channel*) argv;
    uint8_t buf[32];

    assert(argv);

    for(;;) {
        fprintf(stderr, "call read()\n");
        size_t n = read(ch->fds[0], buf, 32);
        fprintf(stderr, "read return code: %ld\n", n);
        switch(n) {
        case -1:
            switch(errno) {
            case EWOULDBLOCK:
                fprintf(stderr, "EWOULDBLOCK\n");
                break;
            default:
                perror("read() failed");
                break;
            }
            break;
        case 0:
            break;
        default:
            buf[n] = '\0';
            fprintf(stderr, "received: %s\n", buf);
            if (strcmp(buf, msg)==0) {
                goto exitloop;
            }
            break;
        }
        usleep(rand()+1000);
    }
exitloop:
    fprintf(stderr, "Task finished\n");
}

channel* startThread( void* (*task)(void*) ) {
    pthread_t tid;
    int rc;
    channel* ch = (channel*) malloc(sizeof(channel));

    rc = pipe(ch->fds);
    assert(rc==0);

    rc = fcntl(ch->fds[0], F_SETFL, fcntl(ch->fds[0], F_GETFL) | O_NONBLOCK);
    assert(rc==0);

    rc = pthread_create(&tid, NULL, task, ch);
    assert(rc==0);

    rc = pthread_detach(tid);
    assert(rc==0);

    return ch;
}

int main() {
    int i;
    const int numThreads = 1 ;
    channel* channels[numThreads];

    srand(time(NULL));
    channels[0] = startThread(task);

    sleep(2);
    write(channels[0]->fds[1], msg, sizeof(msg));

    /*
    for(i=0;i<numThreads;++i) {
        channels[i] = startThread
    }
    */
    pthread_exit(NULL);
    return 0;
}