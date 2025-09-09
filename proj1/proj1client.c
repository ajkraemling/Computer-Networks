#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pwd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1024

char *iphost = "localhost"; /* default host name */

int port = 9999; /* default port number */

static void die(const char *s) { perror(s); exit(1);}

int main (int argc, char **argv) {

	if (argc > 3) {
		die("usage: netclient [server-ip] [server-port]");
	}
	if (argc > 2) {
		port = atoi(argv[2]);
		if (port <= 0) die("bad port number.");
	}
	if (argc > 1)
		iphost = argv[1];

	int sock; // socket descriptor

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die ("socket failed");

	// Construct a server address structure
	
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	//servaddr.sin_addr.s_addr = inet_addr(iphost);
	servaddr.sin_port = htons(port); // must be in network byte order

	struct hostent *ptrh; /* pointer to a host table entry */
	ptrh = gethostbyname(iphost); /* iphost can either be hostname or dotted decimal */
	if (((char *)ptrh) == NULL)
		die("invalid host");
	memcpy(&servaddr.sin_addr, ptrh->h_addr, ptrh->h_length);

	// Establish a TCP connection to the server
	if (connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
		die("connect failed");

	/* send our user name */
	struct passwd *ppwd;
	if ((ppwd = getpwuid(getuid())) == NULL)
		die("could not get user name");
	size_t len = strlen(ppwd->pw_name);
	// Note: send(sock,buf,len,0) is equivalent to write(sock,buf,len)
	if (send(sock, ppwd->pw_name, len, 0) != len)
		die("send failed");

	// Recieve first response from server
	int n;
	char buf[BUFFER_SIZE]; /* buffer for receiving */
	n = recv(sock, buf, sizeof(buf), 0);
	write(1, buf, n); /* write bytes to stdout */

	// Prompt user for name
	char name[100];
	printf("Client: Please enter your name: ");
	scanf("%s", &name);
	
	// Send name to server
	if (send(sock, name, strlen(name), 0) != strlen(name))
		die("Sending name failed");

	// Loop through, getting server responses and prompting for number
	while (1) {
		printf("Client: What is your guess for an integer between 1 and 100? ");
		int num;
		scanf("%d", &num);
		uint32_t guess = htonl(num);
		if (send(sock, &guess, sizeof(guess), 0) != sizeof(guess))
			die("Sending guess failed");
		
		int server_resp = recv(sock, buf, sizeof(buf), 0);
		buf[server_resp] = '\0';
		printf("%s", buf);

		if (strstr(buf, "Correct") != NULL) {
			printf("Client: end connection\n");
			close(sock);
			return 0;
		}
	}	
	close(sock);
	return 0;
}
