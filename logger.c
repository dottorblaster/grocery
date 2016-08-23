#include "logger.h"

void logger(int type, char *s1, char *s2) {
	int fd;
	char logbuffer[BUFSIZE*2];

	switch(type) {
		case LOG:
			sprintf(logbuffer, "LOG: %s : %s", s1, s2);
			break;
		case ERROR:
			sprintf(logbuffer, "500 INTERNAL SERVER ERROR: %s : %s", s1, s2);
			break;
		case NOTFOUND:
			sprintf(logbuffer, "404 NOTFOUND: %s : %s", s1, s2);
			break;
		case FORBIDDEN:
			sprintf(logbuffer, "403 FORBIDDEN: %s : %s", s1, s2);
			break;
	}

	if ((fd = open("grocery.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0) {
		write(fd, logbuffer, strlen(logbuffer));
		write(fd, "\n", 1);
		close(fd);
	}
}
