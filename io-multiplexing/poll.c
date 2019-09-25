/*
Poll vs Select
* poll( ) does not require that the user calculate the value of the highest-numbered 
  file descriptor +1. poll( ) is more efficient for large-valued file descriptors. 
  Imagine watching a single file descriptor with the value 900 via select() - the 
  kernel would have to check each bit of each passed-in set, up to the 900th bit.
* select( )’s file descriptor sets are statically sized.
* With select( ), the file descriptor sets are reconstructed on return, so each 
  subsequent call must reinitialize them. The poll() system call separates the input
  (events field) from the output (revents field), allowing the array to be reused 
  without change.
* The timeout parameter to select() is undefined on return. Portable code needs to 
  reinitialize it. This is not an issue with pselect()
* select() is more portable, as some Unix systems do not support poll()
*/

for (i = 0; i < 5; i++)
{
	memset(&client, 0, sizeof(client));
	addrlen = sizeof(client);
	pollfds[i].fd = accept(sockfd, (struct sockaddr *)&client, &addrlen);
	pollfds[i].events = POLLIN;
}

sleep(1);

while (1)
{
	puts("round again");
	poll(pollfds, 5, 50000);

	for (i = 0; i < 5; i++)
	{
		if (pollfds[i].revents & POLLIN)
		{
			pollfds[i].revents = 0;
			memset(buffer, 0, MAXBUF);
			read(pollfds[i].fd, buffer, MAXBUF);
			puts(buffer);
		}
	}
}