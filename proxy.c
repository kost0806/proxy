#include "proxy.h"

char *LOG_FILE_NAME = "proxy.log";

/*	프록시 서버를 열어준다.
 *	@param
 *	proxy_server *proxy_buf: 프록시 서버의 기본적인 정보가 담길 struct proxy_server변수의 주소
 *
 *	@return
 *	void
 */
void proxy_init(proxy_server *proxy_buf, int port) {
	struct sockaddr_in server;

	memset(proxy_buf, 0, sizeof(proxy_server));

	proxy_buf->proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
	proxy_buf->port = port;

	assert(proxy_buf->proxy_fd > 0);

	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	assert(bind(proxy_buf->proxy_fd, (struct sockaddr *)&server, sizeof(struct sockaddr)) != -1);
	assert(listen(proxy_buf->proxy_fd, 5) != -1);
}

/*	클라이언트를 accept 한다.
 *	@param
 *	proxy *p: 클라이언트와 타겟 서버의 정보가 담긴 proxy 구조체의 주소
 *
 *	@return
 *	FLAG
 *		PROXY_ACCEPT_FAILED : 클라이언트 accept 실패
 *		PROXY_ACCEPT_SUCCESS : 클라이언트 accept 성공
 */
int proxy_accept(proxy *p, proxy_server *p_server) {
	p->client_addr_size = sizeof(struct sockaddr);
	p->client_fd = accept(p_server->proxy_fd, (struct sockaddr*)&(p->client), &(p->client_addr_size));
	if (p->client_fd == -1)
		return PROXY_ACCEPT_FAILED;
	return PROXY_ACCEPT_SUCCESS;
}

/*	클라이언트로부터 요청을 받는다.
 *	@param
 *	proxy *p : 클라이언트와 타겟 서버의 정보가 담긴 proxy 구조체의 포인터
 *
 *	@return
 *	FLAG
 *		PROXY_GET_REQUEST_FAILED : 클라이언트 request 받기 실패
 *		PROXY_GET_REQUEST_SUCCESS : 클라이언트 request 받기 성공
 */
int proxy_get_request(proxy *p) {
	struct log log_buf;
	char buf[REQUEST_BUF_SIZE];
	char ip_buf[16];
	int n;

	memset(buf, 0, sizeof(buf));
	//memset(log_buf, 0, sizeof(log_buf));

	n = read(p->client_fd, buf, REQUEST_BUF_SIZE);
	if (n < 0)
		return PROXY_GET_REQUEST_FAILED;
	else if (n == 0)
		return PROXY_GET_REQUEST_EMPTY;

	inet_ntop(AF_INET, &(p->client.sin_addr.s_addr), ip_buf, sizeof(ip_buf));
	fprintf(stdout, "from %s read[%d]:\n%s\n", ip_buf, n, buf);
	strncpy(p->last_request, buf, PROXY_LAST_REQ_SIZE);

	//strcpy(log_buf.ip, ip_buf);
	//log_buf.log_time = time(NULL);
	//log_buf.size = (size_t)n;
	//sscanf(buf, "GET %s", log_buf.url);
	complete_proxy_struct(p);
	//proxy_log(log_buf);

	return PROXY_GET_REQUEST_SUCCESS;
}

/*	타겟 서버로 리퀘스트 전달
 *
 *	@param
 *	proxy *p : 클라이언트와 타겟 서버의 정보가 담긴 proxy 구조체의 포인터
 *
 *	@return
 *	FLAG
 *		PROXY_SEND_REQUEST_SUCCESS : 타겟서버로 request 전송 성공
 *		PROXY_SEND_REQUEST_FAILED_S : 타겟서버로 request 전송 실패(socket error)
 *		PROXY_SEND_REQUEST_FAILED_C : 타겟서버로 request 전송 실패(connect error)
 */
int proxy_send_request(proxy *p) {
	char request_buf[REQUEST_BUF_SIZE];
	char last_req_tmp[REQUEST_BUF_SIZE];
	char *last_req_ptr;
	char *req_buf_ptr = request_buf;
	char *t, *bp;

	char first_line[3][100];
	char file[100];

	char ip_buf[16];
	int n, cnt = 0, i, k = 0;

	memset(request_buf, 0, REQUEST_BUF_SIZE);
	memset(file, 0, 100);

	if ((p->server_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		return PROXY_SEND_REQUEST_FAILED_S;
	}

	if (connect(p->server_fd, (struct sockaddr *)&(p->server), sizeof(p->server)) == -1) {
		return PROXY_SEND_REQUEST_FAILED_C;
	}

	fcntl(p->server_fd, F_SETFD, O_NONBLOCK);
	fcntl(p->client_fd, F_SETFD, O_NONBLOCK);

	strcpy(last_req_tmp, p->last_request);
	t = strtok_r(last_req_tmp, "\n", &bp);

	sscanf(t, "%s %s %s", first_line[0], first_line[1], first_line[2]);
	for (i = 0; i < strlen(first_line[1]); ++i) {
		if (first_line[1][i] == '/' && cnt < 3) {
			cnt++;
		}
		if (cnt == 3) {
			file[k++] = first_line[1][i];
		}
	}
	sprintf(request_buf, "%s %s %s", first_line[0], file, "HTTP/1.0\r\n");

	while (t = strtok_r(NULL, "\n", &bp)) {
		if (!strncmp(t, "Proxy-Connection", 16) || !strncmp(t, "Connection", 10)) {
			strcat(request_buf, "Connection: close\r\n");
		}
		else if (!strncmp(t, "Keep-Alive", 10) || !strncmp(t, "If", 2)) {
			continue;
		}
		else {
			strcat(request_buf, t);
			strcat(request_buf, "\n");
		}
	}
	strcat(request_buf, "\r\n");

	n = write(p->server_fd, request_buf, REQUEST_BUF_SIZE);
	inet_ntop(AF_INET, &(p->server.sin_addr.s_addr), ip_buf, sizeof(ip_buf));
	fprintf(stdout, "\n\nto %s write[%d]:\n%s\n", ip_buf, n, request_buf);

	return PROXY_SEND_REQUEST_SUCCESS;
}

/*	서버로부터 response를 받고 클라이언트에게 전달한다.
 *
 *	@param
 *	proxy *p : 프록시 구조체의 주소
 *
 *	@return
 *	void
 */
void proxy_get_response(proxy *p) {
	char ip_buf_s[16];
	char ip_buf_c[16];
	int r, s;
	char buf[RESPONSE_BUF_SIZE];
	char *ptr;
	int hflag = 0;

	struct log log_buf;

	inet_ntop(AF_INET, &(p->server.sin_addr.s_addr), ip_buf_s, sizeof(ip_buf_s));
	inet_ntop(AF_INET, &(p->client.sin_addr.s_addr), ip_buf_c, sizeof(ip_buf_c));
	while((r = read(p->server_fd, buf, RESPONSE_BUF_SIZE)) > 0) {
		fprintf(stdout, "from %s read[%d]:\n\n", ip_buf_s, r);
		s = write(p->client_fd, buf, r);
		fprintf(stdout, "to %s write[%d]:\n\n", ip_buf_c, s);
		if (!hflag) {
			ptr = buf;
			while (ptr - (char *)buf < RESPONSE_BUF_SIZE) {
				if (!strncmp(ptr, "\r\n\r\n", 4)) {
					hflag = 1;
					p->obj_size += s - (ptr - (char *)buf) - 4;
					break;
				}
				ptr++;
			}
		}
		else {
			p->obj_size += s;
		}
	}

	close(p->server_fd);
	close(p->client_fd);
	strcpy(log_buf.ip, ip_buf_c);
	log_buf.log_time = time(NULL);
	log_buf.size = (size_t)p->obj_size;
	sscanf(p->last_request, "GET %s", log_buf.url);
	proxy_log(log_buf);
	fprintf(stdout, "done===================================\n");
}

/*	클라이언트로부터 온 request를 logging한다.
 *
 *	@param
 *	struct log log_buf : log의 정보가 담긴 구조체
 *
 *	@return
 *		LOG_SUCCESS : log 쓰기 성공
 *		LOG_FAILED : log 쓰기 실패
 */
void proxy_log(struct log log_buf) {
	struct tm *tm;
	char *time;
	char buf[1024];
	char t_buf[100];

	memset(buf, 0, 1024);

	log_fd = open(LOG_FILE_NAME, O_WRONLY|O_CREAT|O_APPEND, 0644);
	if (log_fd == -1) {
		fprintf(stderr, "log file open failed");
		return;
	}

	tm = (struct tm *)localtime(&(log_buf.log_time));
	time = (char *)asctime(tm);

	strncpy(t_buf, time, strlen(time) - 1);

	sprintf(buf, "%s: %s %s %d\n", t_buf, log_buf.ip, log_buf.url, log_buf.size);
	write(log_fd, buf, 1024);
	close(log_fd);
}

/*	proxy 구조체를 완성한다.
 *
 *	@param
 *	proxy *p : 클라이언트와 타겟 서버의 정보를 담고있는 proxy 구조체의 포인터
 *
 *	@return
 *	FLAG
 */
int complete_proxy_struct(proxy *p) {
	char *req_ptr = p->last_request;
	char server_host[128];
	char tmp[10];
	int i = 0;
	int cnt = 0;

	char ip_buf[16];
	struct hostent *host_entry;

	while (*req_ptr != '\0') {
		if (!strncmp(req_ptr, "GET", 3)) {
			sscanf(req_ptr, "%s %s", tmp, p->target_file_name);
		}
		else if (!strncmp(req_ptr, "Host", 4)) {
			sscanf(req_ptr, "%s %s", tmp, server_host);
		}
		req_ptr++;
	}


	// inet_ntop(AF_INET, &(p->client.sin_addr.s_addr), ip_buf, sizeof(ip_buf));
	printf("server_host: %s\n", server_host); 
	if ((host_entry = gethostbyname(server_host)) == NULL)
		return FAIL_TO_GET_HOST;

	p->server.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr *)host_entry->h_addr_list[0]));
	inet_ntop(AF_INET, (struct in_addr *)host_entry->h_addr_list[0], ip_buf, sizeof(ip_buf));
	printf("target: %s\n", ip_buf);

	p->server.sin_family = AF_INET;
	p->server.sin_port = htons(80);
	p->obj_size = 0;
}
