#include <stdio.h>
#include <stdlib.h>
#include "proxy_server.h"

extern **environ;

int main(int argc, char **argv) {
	proxy_server server;
	proxy proxy_svc;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s port_number\n", argv[0]);
		return 1;
	}

	proxy_init(&server, (int)atol(argv[1]));

	while (true) {
		proxy_service(&proxy_svc, &server);
	}
}

void proxy_service(proxy *p, proxy_server *p_server) {
	if (proxy_accept(p, p_server) == PROXY_ACCEPT_FAILED) {
		fprintf(stderr, "accept failed\n");
		return;
	}
	switch(proxy_get_request(p)) {
		case PROXY_GET_REQUEST_FAILED :
			fprintf(stderr, "proxy_get_request: READ ERROR(%x)\n", PROXY_GET_REQUEST_FAILED);
			return;
		case PROXY_GET_REQUEST_EMPTY :
			fprintf(stderr, "proxy_get_request: EMPTY STRING(%x)\n", PROXY_GET_REQUEST_EMPTY);
			return;
	}
	switch(proxy_send_request(p)) {
		case PROXY_SEND_REQUEST_FAILED_S :
			fprintf(stderr, "proxy_send_request: SOCKET ERROR(%x)\n", PROXY_SEND_REQUEST_FAILED_S);
			return;
		case PROXY_SEND_REQUEST_FAILED_C :
			fprintf(stderr, "proxy_send_request: CONNECT ERROR(%x)\n", PROXY_SEND_REQUEST_FAILED_C);
			return;
	}
	proxy_get_response(p);
}
