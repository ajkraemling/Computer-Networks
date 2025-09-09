#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#define UBUF_SIZE 128
#define BUFFER_SIZE 1024

int port = 9999;
int visits = 1;

static void die(const char *s) { perror(s); exit(1);}

int main (int argc, char *argv[]) {

	if (argc > 2) {
		die("usage: netserver <server-port>");
	}
	else if (argc > 1) {
		port = atoi(argv[1]);
		if (port < 1024) die("Port not specified correctly as argument.");
	}

	int server_fd, client_fd, err;
	struct sockaddr_in server, client;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) die("Could not create socket");

	// Create a socket for TCP connection
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	// Allow port number to be reused
	int opt_val = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

	err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
	if (err < 0) die("Could not bind socket");

	err = listen(server_fd, 128);
	if (err < 0) die("Could not listen on socket");

	printf("Server is listening on %d\n", port);

	while (1) {
		socklen_t client_len = sizeof(client);
		client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);

		if (client_fd < 0) die("Could not establish new connection");

		// Read identity of client user
		char ubuf[UBUF_SIZE];
		int n = recv(client_fd, ubuf, UBUF_SIZE, 0);
		if (n < 0) die("Client read failed");
		ubuf[n] = '\0';

		// Create output buffer with user and visit count
		char outbuf[BUFFER_SIZE];
		visits++;
		sprintf(outbuf, "Hello %s, you are visitor number %d to this server\n", ubuf, visits);
		// Send back to client
		err = send(client_fd, outbuf, strlen(outbuf), 0);
		if (err < 0) die("Client write failed");
		close(client_fd); /* Completed this client request */
	}

	return 0;
}
