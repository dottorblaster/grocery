#include "conversion.h"

int conversion_quality(char *str) {
	printf(str);
	printf("\n");
	return 0;
}

int cachehit(char *buf, hcontainer *headers) {
	char *header, *pbuf;
	int q;
	if (!strcmp(headers[0].val, "")) { return 0; }
	q = conversion_quality(strdup(headers[0].val));
	
	return 0;
}

int convert_img(char *str, int quality) {
    // TODO
}