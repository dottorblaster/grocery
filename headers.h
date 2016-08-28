#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"

typedef struct {
	char *key;
	char *val;
} hcontainer;

int headers_l(char *buf);
