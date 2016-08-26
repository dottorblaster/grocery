#include "request_handler.h"

char * whichreq(char *buf) {
	if (!strncmp(&buf[0],"GET", 3) || !strncmp(&buf[0],"get", 3)) { return "get"; }
	if (!strncmp(&buf[0],"HEAD", 4) || !strncmp(&buf[0],"head", 4)) { return "head"; }
	return "other";
}

void request_handler(int sock_fd) {
	int buflen, sup;
	long rt, ln, i;
	static char buf[BUFSIZE+1];
	char *method;

	int fle;

	struct {
		char *ext;
		char *filetype;
	} extensions [] = {
		{"gif", "image/gif" },
		{"jpg", "image/jpg" },
		{"jpeg","image/jpeg"},
		{"png", "image/png" },
		{"ico", "image/ico" },
		{"zip", "image/zip" },
		{"gz",	"image/gz"	},
		{"tar", "image/tar" },
		{"htm", "text/html" },
		{"html","text/html" },
		{0, 0}
	};

	rt = read(sock_fd, buf, BUFSIZE);
	if (rt == -1 || rt == 0) {
		logger(FORBIDDEN, "Error reading request", "read()");
		handle_error(FORBIDDEN, sock_fd);
	}

	// TODO
	// Parse headers here

	(rt > 0 && rt < BUFSIZE) ? (buf[rt] = 0) : (buf[0] = 0);
	for (i=5;i<BUFSIZE;i++) {
		if(buf[i] == ' ') {
			buf[i] = 0;
			break;
		}
	}
	(!strncmp(&buf[0],"GET /\0", 6) || !strncmp(&buf[0],"get /\0", 6)) && (strcpy(buf, "GET /index.html"));
	(!strncmp(&buf[0],"HEAD /\0", 7) || !strncmp(&buf[0],"head /\0", 7)) && (strcpy(buf, "HEAD /index.html"));

	sup = 0;
	for (i = 0; extensions[i].ext != 0; i++) {
    ln = strlen(extensions[i].ext);
		if (!strncmp(&buf[strlen(buf)-ln], extensions[i].ext, ln)) {
			sup = 1;
		}
	}
	if (sup == 0) {
		logger(FORBIDDEN, "File extension not supported", buf);
		handle_error(FORBIDDEN, sock_fd);
	}

	method = whichreq(buf);
	if (!strncmp(&method[0], "get", 3)) {
		// TODO
		// Handle GET reqs
	} else if (!strncmp(&method[0], "head", 4)) {
		// TODO
		// Handle HEAD reqs
	} else {
		// TODO
		// Handle unsupported methods
	}
}
