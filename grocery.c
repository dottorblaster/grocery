#include "grocery.h"

void print_help() {
	printf("This is a simple help string");
}

void preliminary_checks(int argc, char **argv) {
	if (argc != 2 || !strcmp(argv[1], "-h")) {
		print_help();
		exit(0);
	}
	if (atoi(argv[1]) < 0 || atoi(argv[1]) > 60000) {
		logger(ERROR, "Invalid port number", argv[1]);
		exit(0);
	}
	if (access("./www", F_OK | R_OK) != 0) {
		logger(ERROR, "Subdirectory 'www' does not exist", "check that");
		exit(0);
	}
	if(access('./cache', F_OK | R_OK) != 0) {
		logger(LOG, "Subdirectory 'cache' does not exist", "creating");
		mkdir("./cache", 0700);
	}
}

void spawn_server(char **argv) {
	int pid, lfd, sock_fd, optv;
	static struct sockaddr_in cli;
	static struct sockaddr_in server;

	socklen_t l;

	/* Just ignore SIGCHLD and SIGHUP, we don't care. */
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(atoi(argv[1]));

	optv = 1;
	l = sizeof(optv);
	if((lfd = socket(AF_INET, SOCK_STREAM, 0)) <0) { logger(ERROR, "Error during a syscall", "socket()"); }
	if (setsockopt(lfd, SOL_SOCKET, SO_KEEPALIVE, &optv, l) < 0) { logger(ERROR, "Error during a syscall", "setsockopt()"); }
	if (bind(lfd, (struct sockaddr *)&server, sizeof(server)) < 0) { logger(ERROR, "Error during a syscall", "bind()"); }
	if (listen(lfd, 64) < 0) { logger(ERROR, "Error during a syscall", "listen()"); }

	printf("grocery listening on port %s\n", argv[1]);

	for (;;) {
		l = sizeof(cli);

		if ((sock_fd = accept(lfd, (struct sockaddr *)&cli, &l)) < 0) { logger(ERROR, "Error during a syscall", "accept"); }

		if ((pid = fork()) < 0) {
			logger(ERROR, "Error during a syscall", "fork()");
		}
		else {
			if (pid == 0) {
				request_handler(sock_fd, 0);
			}
		}
	}
}

int main(int argc, char **argv) {
	preliminary_checks(argc, argv);
	spawn_server(argv);
}
