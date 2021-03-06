#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "globals.h"
#include "error_handler.h"
#include "logger.h"
#include "conversion.h"

char * whichreq(char *buf);
void handle_unsupported_method(int sock_fd, char *buf);
void handle_get(int sock_fd, char *buf, char *ext, hcontainer *headers);
void handle_head(int sock_fd, char *buf, char *ext);
void request_handler(int fd, int keepalive);
