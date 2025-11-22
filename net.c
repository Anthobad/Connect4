#include "net.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

int net_open_server(int port) {
	int server_fd = -1;
	int client_fd = -1;
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);
	int opt = 1;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("socket");
		return -1;
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		close(server_fd);
		return -1;
	}

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons((unsigned short)port);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind");
		close(server_fd);
		return -1;
	}

	if (listen(server_fd, 1) < 0) {
		perror("listen");
		close(server_fd);
		return -1;
	}

	client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
	if (client_fd < 0) {
		perror("accept");
		close(server_fd);
		return -1;
	}

	close(server_fd);
	return client_fd;
}

int net_open_client(const char *ip, int port) {
	int sockfd;
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons((unsigned short)port);

	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
		perror("inet_pton");
		close(sockfd);
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect");
		close(sockfd);
		return -1;
	}

	return sockfd;
}

int net_send_byte(int sockfd, unsigned char b) {
	ssize_t n;
	unsigned char buf[1];
	buf[0] = b;

	for (;;) {
		n = write(sockfd, buf, 1);
		if (n == 1)
			return 0;
		if (n < 0) {
			perror("write");
			return -1;
		}
	}
}

int net_recv_byte(int sockfd, unsigned char *out) {
	ssize_t n;
	unsigned char buf[1];

	for (;;) {
		n = read(sockfd, buf, 1);
		if (n == 1) {
			*out = buf[0];
			return 0;
		}
		if (n == 0)
			return -1;
		if (n < 0) {
			perror("read");
			return -1;
		}
	}
}
