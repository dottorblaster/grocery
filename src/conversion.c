#include "conversion.h"

int is_image(char *tok) {
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
    if (!strncmp(&tok[0], "image/png", strlen("image/png"))) {
        return 1;
    }
    if (!strncmp(&tok[1], "image/png", strlen("image/png"))) {
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
        if (is_image(tok) && has_quality(tok)) {
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
    strcpy(retval, str);
    strcat(retval, "-");
    strcat(retval, quality_s);

    return retval;
}

int cachehit(char *buf, hcontainer *headers) {
	char *cachedfn, *toaccess;
	int q;
	if (!strcmp(headers[0].val, "")) { return 0; }

    if ((q = conversion_quality(headers[0].val)) == 0) { return 0; }
    cachedfn = cached_filename(buf, q);

    toaccess = malloc(sizeof(cachedfn)+8);
    strcpy(toaccess, "./cache/");
	strcat(toaccess, cachedfn);

    if (access(toaccess, F_OK | R_OK) == 0) {
        logger(LOG, "Cache hit!", "serving the cached file");
	} else {
        logger(LOG, "Cache didn't hit", "still need to convert the file");
        convert_img(buf, cachedfn, q);
    }

    strcpy(buf, cachedfn);
	return 1;
}

char * build_convert_cmd(char *source, char *dest, int quality) {
    char *cmd = malloc(sizeof(source)+sizeof(dest)+21);

    sprintf(
        cmd,
        "%s %s %s %d %s",
        "convert",
        source,
        "-quality",
        quality,
        dest
    );

    return cmd;
}

int convert_img(char *source, char *dest, int quality) {
    char *www_src, *cache_dest, *cmd;

    www_src = malloc(sizeof(source)+6);
    strcpy(www_src, "./www/");
	strcat(www_src, source);

    cache_dest = malloc(sizeof(dest)+8);
    strcpy(cache_dest, "./cache/");
	strcat(cache_dest, dest);

    cmd = build_convert_cmd(www_src, cache_dest, quality);

    logger(LOG, "RUNNING", cmd);

    system(cmd);

    return 1;
}
