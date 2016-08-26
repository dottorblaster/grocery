#include "request_handler.h"

void request_handler(int sock_fd) {
	int buflen, sup;
	long rt, ln, i;
	static char buf[BUFSIZE+1];

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
	(rt > 0 && rt < BUFSIZE) ? (buf[rt] = 0) : (buf[0] = 0);
	for (i=4;i<BUFSIZE;i++) {
		if(buf[i] == ' ') {
			buf[i] = 0;
			break;
		}
	}
	(!strncmp(&buf[0],"GET /\0",6) || !strncmp(&buf[0],"get /\0",6)) && (strcpy(buf,"GET /index.html"));

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

	logger(LOG, "Request", buf);
	handle_error(NOTFOUND, sock_fd);
}
