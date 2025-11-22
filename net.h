#ifndef NET_H
#define NET_H

int net_open_server(int port);
int net_open_client(const char *ip, int port);
int net_send_byte(int sockfd, unsigned char b);
int net_recv_byte(int sockfd, unsigned char *out);

#endif
