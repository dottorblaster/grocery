#include "conversion.h"

// Shall I need to convert that image? This function gives us a clue.
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

// Checker for headers: does this header have quality inside?
int has_quality(char *tok) {
    if (strstr(tok, ";q=") != NULL) {
        return 1;
    }
    return 0;
}

// Here we take an header and give back the conversion quality as an int.
int conversion_quality(char *header) {
    char *tok, *str, *dbuf;
    double quality;
    int retval;

    dbuf = str = strdup(header);
    // Iteration upon tokens returned by strsep
    while ((tok = strsep(&str, ",")) != NULL) {
        if (is_image(tok) && has_quality(tok)) {
            // strsep again upon a different token
            // to get our quality
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

// Compute a cached filename from a basic filename buffer
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

// Cache hit helper function.
int cachehit(char *buf, hcontainer *headers) {
	char *cachedfn, *toaccess;
	int q;
	if (!strcmp(headers[0].val, "")) { return 0; }

    // Checking if we have a conversion quality > 0
    // Then computing the correct filename
    if ((q = conversion_quality(headers[0].val)) == 0) { return 0; }
    cachedfn = cached_filename(buf, q);

    toaccess = malloc(sizeof(cachedfn)+8);
    strcpy(toaccess, "./cache/");
	strcat(toaccess, cachedfn);

    printf("%s\n", toaccess);

    // If the cached copy doesn't exist, we convert
    // the file on-the-fly with an helper
    if (access(toaccess, F_OK | R_OK) == 0) {
        logger(LOG, "Cache hit!", "serving the cached file");
	} else {
        logger(LOG, "Cache didn't hit", "still need to convert the file");
        convert_img(buf, cachedfn, q);
    }

    // Here we violate the single responsibility principle
    // editing the buffer and pointing it to another file.
    // The cached one.
    strcpy(buf, cachedfn);
	return 1;
}

// We use an sprintf-powered builder to build up a string
// then we issue a system command. We use this function as
// an abstraction layer upon `convert`.
int convert_img(char *source, char *dest, int quality) {
    char cmd[sizeof(source)+sizeof(dest)+211];

    sprintf(
        cmd,
        "convert ./www/%s -quality %d ./cache/%s",
        source,
        quality,
        dest
    );

    logger(LOG, "RUNNING", cmd);

    system(cmd);

    return 1;
}
