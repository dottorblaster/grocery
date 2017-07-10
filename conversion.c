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

char * cached_filename(char *buf, int quality) {
    char *str;
    char *retval = malloc(sizeof(buf)+4);
    char quality_s[3];

    str = strdup(buf);

    sprintf(quality_s, "%d", quality);
    strcat(retval, str);
    strcat(retval, "-");
    strcat(retval, quality_s);

    return retval;
}

int cachehit(char *buf, hcontainer *headers) {
	char *header, *cachedfn;
	int q;
	if (!strcmp(headers[0].val, "")) { return 0; }

	q = conversion_quality(headers[0].val);
    cachedfn = cached_filename(buf, q);
	
	return 0;
}

int convert_img(char *str, int quality) {
    // TODO
}