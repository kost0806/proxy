#include <error.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {false = 0, true} boolean;

void fatal(char *msg);

void fatal(char *msg) {
	perror(msg);
	exit(1);
}
