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

docker_run:
	sudo docker run -d -P -p 8080:8080 -v $(shell pwd)/www:/grocery/www grocery

build_docker_test:
	sudo docker build -t grocery_test -f docker/Dockerfile.test .

docker_test:
	sudo docker run -d -P -p 8080:8080 --name grtest -v $(shell pwd):/grocery grocery_test \
		make debug run

markdown_to_pdf:
	pandoc -o docs/relazione.pdf docs/report_it.md \
	-V colorlinks -V mainfont:arial -V monofont:menlo -V fontsize="12pt" \
	--latex-engine=lualatex

all: debug

clean:
	rm -rf *.o grocery grocery.dSYM
