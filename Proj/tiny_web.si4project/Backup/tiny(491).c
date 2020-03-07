#include "csapp.h"

void doit(int fd);

int main(int argc, char **argv)
{
	int listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	
	if(argc != 2)
	{
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	listenfd = Open_listenfd(argv[1]);
	while(1){
		clientlen = sizeof(clientaddr);
		confd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		Getnameinfo((SA *) &clientaddr, clientlen, hostname, port);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		doit(connfd);
		Close(connfd);
	}
}



























