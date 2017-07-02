SOURCE := grocery.c logger.c error_handler.c request_handler.c headers.c
BINARY := grocery
DEBUG_PORT := 8080

CC := gcc
CFLAGS := -Wextra -Wall
DEBUG := -g

debug:
	$(CC) $(SOURCE) $(CFLAGS) $(DEBUG) -o grocery

release:
	$(CC) $(SOURCE) $(CFLAGS) -O2 -o grocery

run:
	./$(BINARY) $(DEBUG_PORT)

all: debug

clean:
	rm -rf *.o grocery grocery.dSYM
