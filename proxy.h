#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#define MAX_CACHE_SIZE 5 * 1024
#define MAX_OBJECT_SIZE 512
#define REQUEST_BUF_SIZE 1024

#define PROXY_ACCEPT_SUCCESS 0x1000
#define PROXY_ACCEPT_FAILED 0x1001

int log_fd;

typedef struct {
	int client_fd;  // proxy client socket fd
	int server_fd;  // proxy server socket fd
	struct sockaddr_in client;
	struct sockaddr_in server;
	size_t client_addr_size;
	char target_file_name[100]; //target file's name
} proxy;

typedef struct {
	int proxy_fd; // proxy's fd
	int port; // proxy's port
} proxy_server;

struct log {
	char ip[16];
	time_t log_time;
	size_t size;
	char url[256];
};

void proxy_init(proxy_server *proxy_buf, int port);
int proxy_accept(proxy *p, proxy_server *p_server);
void proxy_get_request(proxy *p);
void proxy_send_request(proxy *p);
void proxy_get_response(proxy *p);
void proxy_send_response(proxy *p);
void proxy_log(struct log log_buf);

