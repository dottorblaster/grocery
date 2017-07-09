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
#include "headers.h"
#include "conversion.h"

char * whichreq(char *buf);
void handle_get(int sock_fd, char *buf, char *ext, hcontainer *headers);
void request_handler(int fd, int keepalive);
