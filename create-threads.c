#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

typedef struct {
    int fds[2];
} channel;

void* task(void* argv) {
    channel* ch = (channel*) argv;
    uint8_t buf[32];

    for(;;) {
        size_t n = read(ch->fds[0], buf, 32);
        fprintf(stderr, "read return code: %d\n", n);
        if (n>0) {
            buf[n] = '\0';
            fprintf(stderr, "received: %s\n", buf);
        }
        sleep(1);
    }
}

int main() {
    int i;
    const int numThreads = 1 ;
    channel channels[numThreads];

/*
    for(i=0;i<numThreads;++i) {
        channels[i] = startThread
    }
    */
    return 0;
}