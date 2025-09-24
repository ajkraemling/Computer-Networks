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
#include <sys/time.h>

#define BUFFER_SIZE 10240

static void die(const char *s) { perror(s); exit(1);}

int main (int argc, char **argv) {
	char* url = argv[1];
	char *fileout = "webout";
	char option[64] = "null";
	int port = 80;
	char host[256];
	char path[1024] = "/";
	int pkt = 0;
	int nf = 0;
	int ping = 0;
	int info = 0;

	if (argc > 4) {
		die("usage: netclient [option(-f, -nf, -ping)] [filename]");
	}
	if (argc > 2) {
		strcpy(option, argv[2]);
		if (strcmp(option, "-f") == 0) fileout = argv[3]; 
		else if (strcmp(option, "-ping") == 0) ping = 1;
		else if (strcmp(option, "-pkt") == 0) {pkt = 1; nf = 1;}
		else if (strcmp(option, "-nf") == 0) nf = 1;
		else if (strcmp(option, "-info") == 0) info = 1;
	}

	int sock; // socket descriptor

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		die("socket failed");

	if (strncmp(url, "http://", 7) == 0) url += 7;

	char *slach = strchr(url, '/');
	if (slach) {
		strcpy(path, slach);
		*slach = '\0';
	}
	char *colon = strchr(url, ':');
	if (colon) {
		*colon = '\0';
		port = atoi(colon+1);
	}
	strcpy(host, url);
	
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	//servaddr.sin_addr.s_addr = inet_addr(iphost);
	servaddr.sin_port = htons(port); // must be in network byte order

	struct hostent *ptrh; /* pointer to a host table entry */
	ptrh = gethostbyname(host); /* iphost can either be hostname or dotted decimal */
	if (((char *)ptrh) == NULL)
		die("invalid host");
	memcpy(&servaddr.sin_addr, ptrh->h_addr, ptrh->h_length);

	char ipstr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &servaddr.sin_addr, ipstr, sizeof(ipstr));

	struct timeval start, end;
	gettimeofday(&start, NULL);
	
	// Establish a TCP connection to the server
	if (connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
		die("connect failed");
	
	gettimeofday(&end, NULL);
	
	if (ping) {
		close(sock);
		long rtt_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
		printf("%s RTT %ld ms\n", ipstr, rtt_ms);
		
		return 0;
	}

	char req[2048];
	snprintf(req, sizeof(req), 
			"GET %s HTTP/1.0\r\n"
			"HOST: %s\r\n"
			"\r\n", path, host);
	
	// Note: send(sock,buf,len,0) is equivalent to write(sock,buf,len)
	if (send(sock, req, strlen(req), 0) < 0)
		die("send failed");

	FILE *fp = NULL;
	if (!nf) {
		fp = fopen(fileout, "w");
		if (!fp) die("file open failed");
	}

	struct pkt_record {
		double time;
		int bytes;
	};

	struct pkt_record pkt_log[1024];
	int pkt_count = 0;

	// Recieve response from server
	int n;
	char buf[BUFFER_SIZE]; /* buffer for receiving */
	while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
		if (nf && !pkt) {
			fwrite(buf, 1, n, stdout);
		} else if (pkt) {
			gettimeofday(&end, NULL);
			double t_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;			
			pkt_log[pkt_count].time = t_ms;
			pkt_log[pkt_count].bytes = n;
			pkt_count++;
		} else if (fp) fwrite(buf, 1, n, fp);
	}

	if (fp) fclose(fp);

	if (info) {
		struct tcp_info information;
		socklen_t info_len = sizeof(information);
		if (getsockopt(sock, IPPROTO_TCP, TCP_INFO, &information, &info_len) == 0) {
			double rtt_ms = information.tcpi_rtt / 1000.0;
			double rtt_var = information.tcpi_rttvar / 1000.0;

			printf("TCP RTT: %.3f ms\n", rtt_ms);
			printf("TCP RTT var: %.3f ms\n", rtt_var);	
		} else {
			perror("getsockopt TCP_INFO failed");
		}
	
	}
	
	close(sock);

	if (pkt) {
		for (int i = 0; i < pkt_count; i++) {
			printf("%.6f %d\n", pkt_log[i].time, pkt_log[i].bytes);
		}
	}

	return 0;
}
