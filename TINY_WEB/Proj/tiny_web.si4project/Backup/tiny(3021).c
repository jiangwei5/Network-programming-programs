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
	
	/* 打开和返回一个监听描述符，这个描述符准备好在端口port上接收连接请求*/
	listenfd = Open_listenfd(argv[1]);
	while(1){
		clientlen = sizeof(clientaddr);
		/* 在clientaddr中填写客户端套接字地址 */
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

		/* 将套接字地址结构转换成相应的主机和服务名字符串 */
		Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		doit(connfd);
		Close(connfd);
	}
}




/*
	doit函数值支持“GET”方法，其他方法请求的话则会发送一条错误信息，主程序返回，并等待下一个请求。
	否则，我们读并忽略请求报头。然后，我们将uri解析为一个文件名和一个可能的CGI参数，并且设置一
	个标志位，表明请求的是静态内容还是动态内容。通过stat函数判断文件是否存在。
	最后，如果请求的是静态内容，我们需要检验它是否是一个普通文件，并且可读。条件通过，则我们服务器
	向客户端发送静态内容；相似的，如果请求的是动态内容，就核实该文件是否为可执行文件，如果是则执行该
	文件，并提供动态功能。
*/
void doit(int fd)
{
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio, fd);			// 将描述符fd和地址rp处的一个类型为rio_t的读缓冲区联系起来
	Rio_readlineb(&rio, buf, MAXLINE);
	printf("Request headers:\n");
	printf("%s", buf);
	sscanf(buf, "%s %s %s", method, uri, version);

	if(strcasecmp(method, "GET")){				// 字符相同返回0
		clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");	// 通过已连接描述符来写错误信息
		return;
	}
	read_requesthdrs(&rio); 

	is_static = parse_uri(uri, filename, cgiargs);		// 是静态内容还是动态内容，并设置参数和文件名
	if(stat(filename, &sbuf) < 0){						// 检索关于文件的信息
		clienterror(fd, filename, "404", "Not found", "Tiny couldn't read the file");
		return;
	}

	if(is_static){
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){			// 是否是常规文件、是否可读
			clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
			return;
		}
		serve_static(fd, filename, sbuf.st_size);
	}
	else {
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){			// 是否为常规文件，是否可执行
			clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
			return;
		}
		serve_dynamic(fd, filename, cgiargs);
	}
}




/* 
	void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
   	Tiny 缺乏一个实际服务器的许多错误处理特性。然而，它会检查一些明显的错误，并把它们报告给
   	客户端，下面的clienterror函数发送一个HTTP响应到客户端，在响应中包含相应的状态码和状态信息，
   	响应主体中包含一个HTML文件，向浏览器的用户解释这个错误。
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
	Rio_writen(fd, buf, strlen(buf));						// 将buf中的字符串写到文件描述符fd中
	Rio_writen(fd, body, strlen(body));
}


/*
	void read_requesthdrs(rio_t *rp);
	Tiny不需要请求报头中的任何信息，这个函数就是来跳过这些请求报头的,读这些请求报头，
	直到空行，然后返回。
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
	根据uri中是否含有cgi-bin来判断请求的是静态内容还是动态内容。如果没有cgi-bin，则说明请求的是静态
	内容。那么，我们需要把cgiargs置NULL，然后获得文件名，如果我们请求的uri最后为“/”，则自动添加上
	home.html。比如说，我请求的是“/”，则返回的文件名为./home.html，而我们请求/test.jpg，则返回的文
	件名为./test.jpg.如果uri中含有cgi-bin，则说明请求的是动态内容。那么，我们需要把参数拷贝到cgiargs
	中，把腰执行的文件路径写入filename。距离来说，uri为/cgi-bin/addr?12&45,则cigargs中存放的是12&45，
	filename中存放的是./cgi-bin/addr

	index(uri,'?') : 找出uri字符串中第一个出现参数‘？’的地址，并将此地址返回。
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
	打开文件名为filename的文件，把它映射到一个虚拟存储器空间，将文件的filesize字节映射到从地址srcp
	开始的虚拟存储区域。关闭文件描述符srcfd，把虚拟存储去的数据写入fd描述符，最后释放虚拟存储器区域。
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
	通过派生一个子进程并在子进程的上下文中运行一个cgi程序（可执行文件），来提供各种类型的
	动态内容。
	
	setenv("QUERY_STRING",cgiargs,1) :设置QUERY_STRING环境变量。
*/
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
	char buf[MAXLINE], *emptylist[] = { NULL };  // 为什么可以不指定大小

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















