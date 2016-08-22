#include "logger.h"

void logger(int type, char *s1, char *s2, int socket_fd) {
	int fd;
	char logbuffer[BUFSIZE*2];

	switch(type) {
		case LOG:
			sprintf(logbuffer, "LOG: %s : %s", s1, s2);
			break;
		case ERROR:
			// TODO
			break;
		case NOTFOUND:
			// TODO
			break;
		case FORBIDDEN:
			// TODO
			break;
	}

	if ((fd = open("nweb.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0) {
		write(fd, logbuffer, strlen(logbuffer));
		write(fd, "\n", 1);
		close(fd);
	}
}
