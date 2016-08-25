#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <netinet/in.h>
#include "globals.h"
#include "logger.h"
#include "request_handler.h"

void print_help();
void preliminary_checks(int argc, char **argv);
void spawn_server(char **argv);
