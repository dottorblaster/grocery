SOURCE := ./src/*.c
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

build_docker:
	sudo docker build -t grocery -f docker/Dockerfile .

build_docker_test:
	sudo docker build -t grocery_test -f docker/Dockerfile.test .

docker_test:
	sudo docker run -d -P -p 8080:8080 --name grtest -v $(shell pwd):/grocery grocery_test \
		make debug run

all: debug

clean:
	rm -rf *.o grocery grocery.dSYM
