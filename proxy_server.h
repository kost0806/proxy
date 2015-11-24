#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include "proxy.h"

typedef enum {false = 0, true} boolean;

void proxy_service(proxy *p, proxy_server *p_server);
