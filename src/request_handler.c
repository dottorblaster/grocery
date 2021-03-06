#include "request_handler.h"

// Request identifier.
char * whichreq(char *buf) {
	if (!strncmp(&buf[0],"GET", 3) || !strncmp(&buf[0],"get", 3)) { return "get"; }
	if (!strncmp(&buf[0],"HEAD", 4) || !strncmp(&buf[0],"head", 4)) { return "head"; }
	return "other";
}

// Unsupported method handler. Here we print out to the response buffer a 405 Unsupported Method.
void handle_unsupported_method(int sock_fd, char *buf) {
	sprintf(
		buf,
		"HTTP/1.1 405 Method Not Allowed\nServer: grocery/%d.0\n\nConnection: close\nAllow: GET, HEAD\n\n",
		VERSION
	);
	write(sock_fd, buf, strlen(buf));

	exit(0);
}

// Handler for GETs.
void handle_get(int sock_fd, char *buf, char *ext, hcontainer *headers) {
	int fle;
	long ln, rt;
	char fn[sizeof(buf) + 11];

	// Here we check for cache-hit and set an eventual cache directory prefix
	// otherwise we use `./www`
	if (is_image(ext) && cachehit(&buf[5], headers)) {
		strcpy(fn, "./cache/");
		strcat(fn, &buf[5]);
	} else {
		strcpy(fn, "./www/");
		strcat(fn, &buf[5]);
	}

	// ... then we try to open the file.
	// If we can't open it, we fire a 404 Not Found.
	if ((fle = open(fn, O_RDONLY)) == -1) {
		logger(NOTFOUND, "not found", &buf[5]);
		handle_error(NOTFOUND, sock_fd);
	}

	logger(LOG, "GET", &buf[5]);
	ln = (long)lseek(fle, (off_t)0, SEEK_END);
	lseek(fle, (off_t)0, SEEK_SET);

	// Print the response' header to the response buffer
	sprintf(
		buf,
		"HTTP/1.1 200 OK\nServer: grocery/%d.0\nContent-Length: %ld\nConnection: %s\nContent-Type: %s\n\n",
		VERSION,
		ln,
		(strlen(headers[1].val) != 0 && !strncmp(&headers[1].val[1], "close", 5)) ? "close" : "keep-alive",
		ext
	);
	write(sock_fd, buf, strlen(buf));

	// Then write the actual file to the buffer.
	while ((rt = read(fle, buf, BUFSIZE)) > 0) {
		write(sock_fd, buf, rt);
	}

	close(fle);
	sleep(1);

	// Check for a keepalive header, then dispatch another handler or close everything and die
	if (strlen(headers[1].val) != 0 && !strncmp(&headers[1].val[1], "close", 5)) {
		close(sock_fd);
		free(headers);
		free(ext);
		exit(1);
	} else {
		free(headers);
		free(ext);
		request_handler(sock_fd, 1);
	}
}

// HEAD handler.
// Very similar to the one above, but without the file body.
void handle_head(int sock_fd, char *buf, char *ext) {
	int fle;
	long ln;
	char fn[sizeof(buf) + 6];

	strcpy(fn, "./www/");
	strcat(fn, &buf[5]);
	if ((fle = open(fn, O_RDONLY)) == -1) {
		logger(NOTFOUND, "not found:", &buf[5]);
		handle_error(NOTFOUND, sock_fd);
	}
	logger(LOG, "HEAD", &buf[5]);
	ln = (long)lseek(fle, (off_t)0, SEEK_END);
	lseek(fle, (off_t)0, SEEK_SET);
	sprintf(
		buf,
		"HTTP/1.1 200 OK\nServer: grocery/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n",
		VERSION,
		ln,
		ext
	);
	write(sock_fd, buf, strlen(buf));

	sleep(1);
	exit(0);
}

// Generic request handler.
void request_handler(int sock_fd, int keepalive) {
	long rt, ln, i;
	static char buf[BUFSIZE+1];
	char *method, *ext;

	hcontainer headers[3];

	// We declare a struct of supported extensions/filetypes
	struct {
		char *ext;
		char *filetype;
	} extensions [] = {
		{"css", "text/css"	},
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

	// Read the request then check for errors
	rt = read(sock_fd, buf, BUFSIZE);
	if (rt == -1) {
		logger(ERROR, "Error reading request", "read()");
		exit(0);
	}
	if (rt == 0) {
		exit(0);
	}

	(rt > 0 && rt < BUFSIZE) ? (buf[rt] = 0) : (buf[0] = 0);

	// Check for headers through `hlook`, then
	// populate the `headers` struct
	if (keepalive == 0) {
		headers[0].key = "Accept";
		headers[0].val = hlook("Accept: ", buf);
		headers[1].key = "Connection";
		headers[1].val = hlook("Connection:", buf);
		headers[2].key = "User-Agent";
		headers[2].val = hlook("User-Agent:", buf);
	} else {
		headers[0].key = "Accept";
		headers[0].val = "";
		headers[1].key = "Connection";
		headers[1].val = "";
		headers[2].key = "User-Agent";
		headers[2].val = "";
	}

	for (i=5;i<BUFSIZE;i++) {
		if(buf[i] == ' ') {
			buf[i] = 0;
			break;
		}
	}
	// index.html standard support!
	if (!strncmp(&buf[0],"GET /\0", 6) || !strncmp(&buf[0],"get /\0", 6)) {
		strcpy(buf, "GET /index.html");
	}
	if (!strncmp(&buf[0],"HEAD /\0", 7) || !strncmp(&buf[0],"head /\0", 7)) {
		strcpy(buf, "HEAD /index.html");
	}

	// Check if the file extension is supported, then
	// fire a 403 Forbidden if this is not the case
	ext = "";
	for (i = 0; extensions[i].ext != 0; i++) {
		ln = strlen(extensions[i].ext);
		if (!strncmp(&buf[strlen(buf)-ln], extensions[i].ext, ln)) {
			ext = extensions[i].filetype;
		}
	}
	if (strlen(ext) == 0) {
		logger(FORBIDDEN, "File extension not supported", buf);
		handle_error(FORBIDDEN, sock_fd);
	}

	// Discriminate incoming requests.
	// Should I use GET' handler? HEAD's one?
	// Should I stay or should I go?
	method = whichreq(buf);
	if (!strncmp(&method[0], "get", 3)) {
		handle_get(sock_fd, buf, ext, headers);
	} else if (!strncmp(&method[0], "head", 4)) {
		handle_head(sock_fd, buf, ext);
	} else {
		handle_unsupported_method(sock_fd, buf);
	}
}
