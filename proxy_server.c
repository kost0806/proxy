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
	proxy_get_request(p);
	proxy_send_reequest(p);
	proxy_get_response(p);
	proxy_send_response(p);
}
