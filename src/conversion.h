#include <string.h>
#include <unistd.h>
#include "logger.h"
#include "headers.h"

int is_image(char *tok);
int has_quality(char *tok);
int conversion_quality(char *str);
char * cached_filename(char *buf, int quality);
int cachehit(char *buf, hcontainer *headers);
int convert_img(char *str, char *dest, int quality);