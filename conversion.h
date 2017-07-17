#include <string.h>
#include <unistd.h>
#include "logger.h"
#include "headers.h"

int conversion_quality(char *str);
int cachehit(char *buf, hcontainer *headers);
int convert_img(char *str, int quality);