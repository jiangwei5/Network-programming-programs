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

	struct sockaddr_in server_address; 			// ����IPV4
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;		// ��ַ���Э�����Ӧ
	inet_pton(AF_INET, ip, &server_address.sin_addr); // �ѵ��ʮ����ip��ַת���������ֽ����ip��ַ
	server_address.sin_port = htons(port);		// �����ֽ���->�����ֽ���С��->��ˣ�

	int sockfd = socket(PF_INET, SOCK_STREAM, 0); 	//SOCK_STREAM ��������֮��Ӧ�������ݱ�����
	assert(sockfd >= 0);

	if(connect(sockfd, (struct sockaddr*)& server_address, sizeof(server_address)) < 0) // ǿ��ת����ͨ��socket��ַ����
	{
		printf("connection failed\n");
		close(sockfd);
		return 1; 
	}

	pollfd fds[2];
	/* ע���ļ�������0(��׼����)���ļ�������sockfd�ϵĿɶ��¼� */
	fds[0].fd = 0;										// fd ָ���ļ�������
	fds[0].events = POLLIN;								// events ����poll����fd�ϵ���Щ�¼�������һϵ���¼��İ�λ��
	fds[0].revents = 0;									// ���ں��޸ģ�֪ͨӦ�ó���fd��ʵ�ʷ�������Щ�¼�
	fds[1].fd = sockfd;
	fds[1].events = POLLIN | POLLRDHUP;					// POLLIN ��ʾ���ݿɶ���POLLRDHUP TCP���ӱ��Է��رգ����߶Է��ر���д����
	fds[1].revents = 0;

	char read_buf[BUFFER_SIZE];
	int pipefd[2];
	int ret = pipe(pipefd);
	assert(ret != -1);

	while(1)
	{
		ret = poll(fds, 2, -1);						// nfds ��ʾpoll�������ϵĴ�С�� timeout����ָ��poll�ĳ�ʱֵ����λ�Ǻ��롣��timeoutΪ-1ʱ��poll������Զ������ֱ��ĳ��ʱ�䷢������timeoutΪ0ʱ��poll�����������ء�
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
			recv(fds[1].fd, read_buf, BU FFER_SIZE - 1, 0);		// ���ﲻ�ó���4��
			printf("%s\n", read_buf);
		}

		if(fd[0].revents & POLLIN)
		{
			/* ʹ��splice���û����������ֱ��д��sockfd��(�㿽��)*/
			ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);		//NULL ��ʾ������/����������ĵ�ǰƫ��λ�ö���
			ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
		}
		
	}
	close(sockfd);
	return 0;
}












































