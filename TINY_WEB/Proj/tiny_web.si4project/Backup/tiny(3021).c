#include "csapp.h"

int main(int argc, char **argv)
{
	int listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	
	if(argc != 2){
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	
	/* �򿪺ͷ���һ�����������������������׼�����ڶ˿�port�Ͻ�����������*/
	listenfd = Open_listenfd(argv[1]);
	while(1){
		clientlen = sizeof(clientaddr);
		/* ��clientaddr����д�ͻ����׽��ֵ�ַ */
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

		/* ���׽��ֵ�ַ�ṹת������Ӧ�������ͷ������ַ��� */
		Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		doit(connfd);
		Close(connfd);
	}
}




/*
	doit����ֵ֧�֡�GET��������������������Ļ���ᷢ��һ��������Ϣ�������򷵻أ����ȴ���һ������
	�������Ƕ�����������ͷ��Ȼ�����ǽ�uri����Ϊһ���ļ�����һ�����ܵ�CGI��������������һ
	����־λ������������Ǿ�̬���ݻ��Ƕ�̬���ݡ�ͨ��stat�����ж��ļ��Ƿ���ڡ�
	������������Ǿ�̬���ݣ�������Ҫ�������Ƿ���һ����ͨ�ļ������ҿɶ�������ͨ���������Ƿ�����
	��ͻ��˷��;�̬���ݣ����Ƶģ����������Ƕ�̬���ݣ��ͺ�ʵ���ļ��Ƿ�Ϊ��ִ���ļ����������ִ�и�
	�ļ������ṩ��̬���ܡ�
*/
void doit(int fd)
{
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio, fd);			// ��������fd�͵�ַrp����һ������Ϊrio_t�Ķ���������ϵ����
	Rio_readlineb(&rio, buf, MAXLINE);
	printf("Request headers:\n");
	printf("%s", buf);
	sscanf(buf, "%s %s %s", method, uri, version);

	if(strcasecmp(method, "GET")){				// �ַ���ͬ����0
		clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");	// ͨ����������������д������Ϣ
		return;
	}
	read_requesthdrs(&rio); 

	is_static = parse_uri(uri, filename, cgiargs);		// �Ǿ�̬���ݻ��Ƕ�̬���ݣ������ò������ļ���
	if(stat(filename, &sbuf) < 0){						// ���������ļ�����Ϣ
		clienterror(fd, filename, "404", "Not found", "Tiny couldn't read the file");
		return;
	}

	if(is_static){
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){			// �Ƿ��ǳ����ļ����Ƿ�ɶ�
			clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
			return;
		}
		serve_static(fd, filename, sbuf.st_size);
	}
	else {
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){			// �Ƿ�Ϊ�����ļ����Ƿ��ִ��
			clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
			return;
		}
		serve_dynamic(fd, filename, cgiargs);
	}
}




/* 
	void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
   	Tiny ȱ��һ��ʵ�ʷ������������������ԡ�Ȼ����������һЩ���ԵĴ��󣬲������Ǳ����
   	�ͻ��ˣ������clienterror��������һ��HTTP��Ӧ���ͻ��ˣ�����Ӧ�а�����Ӧ��״̬���״̬��Ϣ��
   	��Ӧ�����а���һ��HTML�ļ�������������û������������
 */

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
	char buf[MAXLINE],body[MAXBUF];
 
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Web server</em>\r\n", body);
 
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, longmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));						// ��buf�е��ַ���д���ļ�������fd��
	Rio_writen(fd, body, strlen(body));
}


/*
	void read_requesthdrs(rio_t *rp);
	Tiny����Ҫ����ͷ�е��κ���Ϣ���������������������Щ����ͷ��,����Щ����ͷ��
	ֱ�����У�Ȼ�󷵻ء�
*/
void read_requesthdrs(rio_t *rp)
{
	char buf[MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);
	while(strcmp(buf, "\r\n")){
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return;
}


/*
	����uri���Ƿ���cgi-bin���ж�������Ǿ�̬���ݻ��Ƕ�̬���ݡ����û��cgi-bin����˵��������Ǿ�̬
	���ݡ���ô��������Ҫ��cgiargs��NULL��Ȼ�����ļ�����������������uri���Ϊ��/�������Զ������
	home.html������˵����������ǡ�/�����򷵻ص��ļ���Ϊ./home.html������������/test.jpg���򷵻ص���
	����Ϊ./test.jpg.���uri�к���cgi-bin����˵��������Ƕ�̬���ݡ���ô��������Ҫ�Ѳ���������cgiargs
	�У�����ִ�е��ļ�·��д��filename��������˵��uriΪ/cgi-bin/addr?12&45,��cigargs�д�ŵ���12&45��
	filename�д�ŵ���./cgi-bin/addr

	index(uri,'?') : �ҳ�uri�ַ����е�һ�����ֲ����������ĵ�ַ�������˵�ַ���ء�
*/
int parse_uri(char *uri, char *filename, char *cgiargs)
{
	char *ptr;

	if(!strstr(uri, "cgi-bin")){
		strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		if(uri[strlen(uri) - 1] == '/') 
			strcat(filename, "home.html");
		return 1;
	}
	else{
		ptr = index(uri, '?');
		if(ptr){
			strcpy(cgiargs, ptr + 1);
			*ptr = '\0';
		}
		else strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;
	}
}



/*
	���ļ���Ϊfilename���ļ�������ӳ�䵽һ������洢���ռ䣬���ļ���filesize�ֽ�ӳ�䵽�ӵ�ַsrcp
	��ʼ������洢���򡣹ر��ļ�������srcfd��������洢ȥ������д��fd������������ͷ�����洢������
*/
void serve_static(int fd, char *filename, int filesize)
{
	int srcfd;
	char *srcp, filetype[MAXLINE], buf[MAXBUF];

	/* Send response headers to client*/
	get_filetype(filename, filetype);
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: Tiny Web Servcer\r\n", buf);
	sprintf(buf, "%sConnection: close\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	Rio_writen(fd, buf, strlen(buf));
	printf("Response headers:\n");
	printf("%s", buf);

	/* Send response body to client*/
	srcfd = Open(filename, O_RDONLY, 0);
	srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
	Close(srcfd);
	Rio_writen(fd, srcp, filesize);
	Munmap(srcp, filesize);
}


void get_filetype(char * filename, char * filetype)
{
	if(strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if(strstr(filename, ".gif"))
		strcpy(filetype, "image.gif");
	else if(strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if(strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else
		strcpy(filetype, "text/plain");
	
}



/*
	ͨ������һ���ӽ��̲����ӽ��̵�������������һ��cgi���򣨿�ִ���ļ��������ṩ�������͵�
	��̬���ݡ�
	
	setenv("QUERY_STRING",cgiargs,1) :����QUERY_STRING����������
*/
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
	char buf[MAXLINE], *emptylist[] = { NULL };  // Ϊʲô���Բ�ָ����С

	/* Return first part of HTTP response */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Tiny Web Server\r\n");
	Rio_writen(fd, buf, strlen(buf));

	if(Fork() == 0){ /* Child */
		/* Real server would set all CGI vars here*/
		setenv("QUERY_STRING", cgiargs, 1);
		Dup2(fd, STDOUT_FILENO);
		Execve(filename, emptylist, environ);
	}
	wait(NULL); /* Parent waits for and reaps child*/
}















