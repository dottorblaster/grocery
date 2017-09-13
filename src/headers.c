#include "headers.h"

// Determine an header's length
// iterating on the buffer.
int headers_l(char *buf) {
	int i, res = 0;
	for (i = 0; i < BUFSIZE; i++) {
		if (buf[i] == '\n') {
			res++;
		}
	}
	res > 2 ? (res = res - 2) : (res = 0);
	return res;
}

// Look for a particular header in the buffer.
// Takin' a label, the buffer, and iterate on the
// buffer until we find the Ring.
char * hlook(char *label, char *buf) {
	char *tk, *str, *dbuf;
	dbuf = str = strdup(buf);
	while ((tk = strsep(&str, "\n")) != NULL) {
		if (!strncmp(&tk[0], label, strlen(label))) {
			strsep(&tk, ": ");
			free(dbuf);
			return strdup(tk);
		}
	}
	free(dbuf);
	return "";
}
