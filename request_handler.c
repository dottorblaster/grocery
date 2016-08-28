#include "request_handler.h"

char * whichreq(char *buf) {
	if (!strncmp(&buf[0],"GET", 3) || !strncmp(&buf[0],"get", 3)) { return "get"; }
	if (!strncmp(&buf[0],"HEAD", 4) || !strncmp(&buf[0],"head", 4)) { return "head"; }
	return "other";
}

void handle_get(int sock_fd, char *buf, hcontainer *headers) {
	/* code */
}

void handle_head(int sock_fd, char *buf) {
	/* code */
}

void request_handler(int sock_fd) {
	int sup, hlen;
	long rt, ln, i;
	static char buf[BUFSIZE+1];
	char *method;

	hcontainer headers[3];

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
		logger(ERROR, "Error reading request", "read()");
		handle_error(ERROR, sock_fd);
	}

	(rt > 0 && rt < BUFSIZE) ? (buf[rt] = 0) : (buf[0] = 0);

	hlen = headers_l(buf);

	headers[0].key = "Accept";
	headers[0].val = hlook("Accept:", buf);
	headers[1].key = "Connection";
	headers[1].val = hlook("Connection:", buf);
	headers[2].key = "User-Agent";
	headers[2].val = hlook("User-Agent:", buf);

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
		//handle_get(/* params here */);
	} else if (!strncmp(&method[0], "head", 4)) {
		//handle_head(/* params here */);
	} else {
	//	handle_unsupported_method(/* params here */);
	}
}
