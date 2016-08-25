#include "grocery.h"

void print_help() {
	printf("This is a simple help string");
	exit(0);
}

void preliminary_checks(int argc, char **argv) {
	if (argc != 2 || !strcmp(argv[1], "-h")) {
		print_help();
	}
	if (atoi(argv[1]) < 0 || atoi(argv[1]) > 60000) {
		logger(ERROR, "Invalid port number", argv[1]);
	}
}

void spawn_server(char **argv) {
	int i, pid, port, lfd, sock_fd;
	static struct sockaddr_in cli;
	static struct sockaddr_in server;

	socklen_t l;

	/* Just ignore SIGCHLD and SIGHUP, we don't care. */
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(atoi(argv[1]));

	if((lfd = socket(AF_INET, SOCK_STREAM, 0)) <0) { logger(ERROR, "Error during a syscall", "socket()"); }
	if (bind(lfd, (struct sockaddr *)&server, sizeof(server)) < 0) { logger(ERROR, "Error during a syscall", "bind()"); }
	if (listen(lfd, 64) < 0) { logger(ERROR, "Error during a syscall", "listen()"); }

	for (;;) {
		l = sizeof(cli);

		if ((sock_fd = accept(lfd, (struct sockaddr *)&cli, &l)) < 0) { logger(ERROR, "Error during a syscall", "accept"); }
		if ((pid = fork()) < 0) { logger(ERROR, "Error during a syscall", "fork()"); }

		close(lfd);
		if (pid == 0) {
			request_handler(sock_fd);
		}
	}
}

int main(int argc, char **argv) {
	preliminary_checks(argc, argv);
	spawn_server(argv);
}
