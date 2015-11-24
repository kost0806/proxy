#include <stdio.h>
#include <stdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "proxy.h"

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

void proxy_get_request(proxy *p) {
	struct log log_buf;
	char buf[REQUEST_BUF_SIZE];
	char ip_buf[16];
	int n;

	memset(buf, 0, sizeof(buf));
	memset(log_buf, 0, sizeof(struct log));

	n = read(p->client_fd, buf, REQUEST_BUF_SIZE);

	inet_ntop(AF_INET, &(p->client.in_addr.addr.s_addr), ip_buf, sizeof(ip_buf));
	fprintf(stdout, "from %s read[%d]:\n%s\n", ip_buf, n, buf);

	strcpy(log_buf.ip, ip_buf, 15);
	log_buf.time = time(NULL);
	log_buf.size = (size_t)n;
	//log_buf.url;
}

void proxy_send_request(proxy *p) {

}

void proxy_get_response(proxy *p) {

}

void proxy_send_response(proxy *p) {

}

void proxy_log(char *file_name, struct log) {
	
}
