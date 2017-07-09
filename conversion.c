#include "conversion.h"

int is_image_jpeg(char *tok) {
    if (!strncmp(&tok[0], "image/jpeg", strlen("image/jpeg"))) {
        return 1;
    }
    if (!strncmp(&tok[0], "image/jpg", strlen("image/jpg"))) {
        return 1;
    }
    if (!strncmp(&tok[1], "image/jpeg", strlen("image/jpeg"))) {
        return 1;
    }
    if (!strncmp(&tok[1], "image/jpg", strlen("image/jpg"))) {
        return 1;
    }
    return 0;
}

int has_quality(char *tok) {
    if (strstr(tok, ";q=") != NULL) {
        return 1;
    }
    return 0;
}

int conversion_quality(char *header) {
    char *tok, *str, *dbuf;
    double quality;
    int retval;

    dbuf = str = strdup(header);
    while ((tok = strsep(&str, ",")) != NULL) {
        if (is_image_jpeg(tok) && has_quality(tok)) {
            strsep(&tok, ";q=");
            tok += 2;
            quality = atof(tok)*100;
            retval = (int) quality;
            free(dbuf);
            return retval;
        }
    }
    free(dbuf);
	return 0;
}

int cachehit(char *buf, hcontainer *headers) {
	char *header;
	int q;
	if (!strcmp(headers[0].val, "")) { return 0; }

	q = conversion_quality(headers[0].val);
	
	return 0;
}

int convert_img(char *str, int quality) {
    // TODO
}