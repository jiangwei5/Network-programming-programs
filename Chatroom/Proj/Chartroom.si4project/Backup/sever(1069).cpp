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

#define USER_LIMIT 5			// 最大用户数数量
#define BUFFER_SIZE 64			// 读缓冲区的大小
#define FD_LIMIT 65535			// 文件描述符数量限制

/* 客户数据： 客户端socket地址、 待写到客户端的数据的位置、从客户端读入的数据 */
struct client_data
{
	struct sockaddr_in address;
	char* write_buf;
	char buf[BUFFER_SIZE];
};

int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);			// 获取文件描述符旧的状态标志
	int new_option = old_option | O_NONBLOCK;		// 设置非阻塞标志
	fcntl(fd, F_SETFL, new_option);					
	return old_option;								// 返回文件描述符旧的状态标志，以便日后恢复该状态标志
}


int main(int argc, char * argv [])
{
	if(argc <= 2)
	{
		printf("usuage: %s ip_address port_number\n", basename(argv[0]));
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);

	int ret = 0;
	struct sockaddr_in address; 			// 用于IPV4
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;		// 地址族和协议族对应
	inet_pton(AF_INET, ip, &address.sin_addr); // 把点分十进制ip地址转化成网络字节序的ip地址
	address.sin_port = htons(port);		// 主机字节序->网络字节序（小端->大端）

	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);

	ret = bind(listenfd, (struct sockaddr*) &address, sizeof(address));		// 命名
	assert(ret != -1);

	ret = listen(listenfd, 5);
	assert(ret != -1);

	client_data* users = new client_data[FD_LIMIT];
	pollfd fds[USER_LIMIT + 1];
	int user_counter = 0;
	for(int i = 1; i <= USER_LIMIT; ++i)
	{
		fds[i].fd = -1;
		fds[i].events = 0;
	}
	fds[0].fd = listenfd;
	fds[0].events = POLLIN | POLLERR;
	fds[0].revents = 0;

	while(1)
	{
		ret = poll(fds, user_counter + 1, -1);
		if(ret < 0)
		{
			printf("poll failure \n");
			break;
		}


		for(int i = 0; i < user_counter + 1; ++i)
		{
			if((fds[i].fd == listenfd) && (fds[i].revents & POLLIN))
			{
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof(client_address);
				// 貌似是连接来了建立新的connfd加入到监听队列当中
				int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
				if(connfd < 0)
				{
					printf("errno is: %d\n", errno);
					continue;
				}
				if(user_counter >= USER_LIMIT)
				{
					const char* info = "too mani users\n";
					printf("%s", info);
					send(connfd, info, strlen(info), 0);
					close(connfd);
					continue;
				}
				/* 对于新的连接， 同时修改fds和users数组。 */
				user_counter++;
				user[connfd].address = client_address;
				setnonblocking(connfd);
				fds[user_counter].fd = connfd;
				fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
				fds[user_counter].revents = 0;
				printf("comes a new user, now have %d users\n", user_counter);
			}
			else if(fds[i].revents & POLLERR)
			{
				printf(" get an error form %d\n", fds[i].id);
				char errors[100];
				memset(errors, '\0', 100);
				socklen_t length = sizeof(errors);
				if(getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0) // 专门用来读取文件描述符属性的方法 SO_ERROR 获取并清除socket错误状态
				{
					printf("get socket option failed\n");
				}
				continue;
			}
			else if(fds[i].revents & POLLIN)
			{
				/* 如果客户端关闭连接，则服务器也关闭对应的连接，并将用户总数减 1*/
				users[fds[i].fd] = users[fds[user_counter].fd];						// 第fd个user的数据变成最后一个user的数据，并关闭
				close(fds[i].fd);
				fds[i] = fds[user_counter];
				i--;
				user_counter--;
				printf("a client left\n");
			}
			else if(fds[i].revent & POLLIN)
			{
				int connfd = fds[i].fd;
				memset(users[connfd].buf, '\0', BUFFER_SIZE);
				ret = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);
				printf("get %d bytes of client data %s from %d\n", ret, user[connfd].buf, connfd);
				if(ret < 0)
				{
					if(errno != EAGAIN)				// errno 哪来的？？？？？？？？？
					{
						close(connfd);
						users[fds[i].fd] = users[fds[user_counter].fd];					
						close(fds[i].fd);
						fds[i] = fds[user_counter];
						i--;
						user_counter--;
					}
				}
				else if(ret == 0)
				{

				}
				else
				/* 接收到客户端数据，则通知其他socket连接准备写数据 */
				{
					for(int j = 1; j <= user_counter; ++j)
					{
						if(fds[j].fd == connfd)				
						{
							continue;
						}
						fds[j].events |= ~POLLIN;						// 告诉内核不监听可读事件
						fds[j].events |= POLLOUT;						// 监听可写事件
						users[fds[j].fd].write_buf = users[connfd].buf;
					}
				}
			}
			else if(fds[i].revents & POLLOUT)
			{
				int connfd = fds[i].fd;
				if(! users[connfd].writr_buf)
				{
					continue;
				}
				ret = send(connfd, users[connfd].write_buf, stelen(users[connfd].write_buf), 0);
				user[connfd].write_buf = NULL;
				fds[i].events |= ~POLLOUT;			// 不用监听可写事件
				fds[i].events |= POLLIN;			// 去监听可读事件
			}
		}
	}
	
	delete [] users;
	close(listenfd);
	return 0;
}



































