SOURCE := grocery.c logger.c

CC := gcc
CFLAGS := -Wextra -Wall -Werror
DEBUG := -g

debug:
	$(CC) $(SOURCE) $(CFLAGS) $(DEBUG) -o grocery

release:
	$(CC) $(SOURCE) $(CFLAGS) -O2 -o grocery

all: debug

clean:
	rm -rf *.o grocery grocery.dSYM
