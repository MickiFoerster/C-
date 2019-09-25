all: select poll epoll

select : main.c select.c
	$(CC) -o $@ -DIOMULTIPLEXING=SELECT $<
poll: main.c poll.c
	$(CC) -o $@ -DIOMULTIPLEXING=POLL $<
epoll: main.c epoll.c
	$(CC) -o $@ -DIOMULTIPLEXING=EPOLL $<
