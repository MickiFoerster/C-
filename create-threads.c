#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

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

void *task(void *argv)
{
    const char answer[] = "Terminating";
    int *channel = (int *)argv;

    assert(argv);

    while (shouldTaskContinue(*channel))
    {
        int sleeptime = (rand() + 100000) % 1000000;
        fprintf(stderr, "sleep for %dms\n", sleeptime / 1000);
        usleep(sleeptime);
    }
    close(*channel);
    fprintf(stderr, "terminating\n");
    return NULL;
}

int startThread(void *(*task)(void *))
{
    pthread_t tid;
    int rc;
    int *ch = (int *)malloc(sizeof(int) * 2);

    rc = pipe(ch);
    assert(rc == 0);

    rc = fcntl(ch[0], F_SETFL, fcntl(ch[0], F_GETFL) | O_NONBLOCK);
    assert(rc == 0);

    rc = pthread_create(&tid, NULL, task, &ch[0]);
    assert(rc == 0);

    rc = pthread_detach(tid);
    assert(rc == 0);

    return ch[1];
}

int main()
{
    int i;
    const int numThreads = 32;
    int channels[numThreads];

    srand(time(NULL));

    for (i = 0; i < numThreads; ++i)
        channels[i] = startThread(task);

    sleep(2);
    for (i = 0; i < numThreads; ++i)
    {
        write(channels[i], msg, sizeof(msg));
        close(channels[i]);
    }

    pthread_exit(NULL);
    return 0;
}