CC = cc
CFLAGS = -Wall -Wextra

all: build/main

build:
	mkdir -p build

build/main: build/main.o
	$(CC) $(CFLAGS) $^ -o build/main

build/main.o: src/main.c | build
	$(CC) $(CFLAGS) -c src/main.c -o $@

clean:
	rm -rf ./build

