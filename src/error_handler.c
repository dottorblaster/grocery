#include "error_handler.h"

// Error handler shorthand function.
// We have a switch case and various writes to the buffer,
// each one with a static response.
void handle_error(int type, int socket_fd) {
	switch(type) {
		case ERROR:
			write(socket_fd, "HTTP/1.1 500 Internal Server Error\nContent-Length: 129\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Internal Server Error</h1>\nOperation gone bad. Sorry.\n</body></html>\n", 227);
			break;
		case NOTFOUND:
			write(socket_fd, "HTTP/1.1 404 Not Found\nContent-Length: 137\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n", 224);
			break;
		case FORBIDDEN:
			write(socket_fd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n", 271);
			break;
	}

	exit(3);
}
