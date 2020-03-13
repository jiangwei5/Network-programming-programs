#define _GUN_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>

#define BUFFER_SIZE 64

int main(int argc, char* argv[])
{
	if(argc <= 2)
	{
		printf("usuage: %s ip_address port_number\n", basename(argv[0]));
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);

	struct sockaddr_in server_address; 			// 用于IPV4
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;		// 地址族和协议族对应
	inet_pton(AF_INET, ip, &server_address.sin_addr); // 把点分十进制ip地址转化成网络字节序的ip地址
	server_address.sin_port = htons(port);		// 主机字节序->网络字节序（小端->大端）

	int sockfd = socket(PF_INET, SOCK_STREAM, 0); 	//SOCK_STREAM 流服务，与之对应的是数据报服务
	assert(sockfd >= 0);

	if(connect(sockfd, (struct sockaddr*)& server_address, sizeof(server_address)) < 0) // 强制转化成通用socket地址类型
	{
		printf("connection failed\n");
		close(sockfd);
		return 1; 
	}

	pollfd fds[2];
	/* 注册文件描述符0(标准输入)和文件描述符sockfd上的可读事件 */
	fds[0].fd = 0;										// fd 指定文件描述符
	fds[0].events = POLLIN;								// events 告诉poll监听fd上的哪些事件，它是一系列事件的按位或
	fds[0].revents = 0;									// 由内核修改，通知应用程序fd上实际发生了哪些事件
	fds[1].fd = sockfd;
	fds[1].events = POLLIN | POLLRDHUP;					// POLLIN 表示数据可读，POLLRDHUP TCP连接被对方关闭，或者对方关闭了写操作
	fds[1].revents = 0;

	char read_buf[BUFFER_SIZE];
	int pipefd[2];
	int ret = pipe(pipefd);
	assert(ret != -1);

	while(1)
	{
		ret = poll(fds, 2, -1);						// nfds 表示poll监听集合的大小。 timeout参数指定poll的超时值，单位是毫秒。当timeout为-1时，poll调用永远阻塞，直到某个时间发生；当timeout为0时，poll调用立即返回。
		if(ret < 0)
		{
			printf("poll failure\n");
			break;
		}

		if(fds[1].revent & POLLRDHUP)
		{
			printf("server close the connection\n");
			break;
		}
		else if(fds[1].revent & POLLIN)
		{
			memset(read_buf, '\0', BUFFER_SIZE);
			recv(fds[1].fd, read_buf, BU FFER_SIZE - 1, 0);		// 这里不用除以4吗？
			printf("%s\n", read_buf);
		}

		if(fd[0].revents & POLLIN)
		{
			/* 使用splice将用户输入的数据直接写到sockfd上(零拷贝)*/
			ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);		//NULL 表示从输入/输出数据流的当前偏移位置读入
			ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
		}
		
	}
	close(sockfd);
	return 0;
}












































