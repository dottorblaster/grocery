#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "globals.h"
#include "error_handler.h"
#include "headers.h"

char * whichreq(char *buf);
void request_handler(int fd);
