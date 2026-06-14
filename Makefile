CC = cc
CFLAGS = -Wall -Wextra

all: build/main

build:
	mkdir -p build

build/main: build/main.o build/lexer.o
	$(CC) $(CFLAGS) $^ -o build/main

build/main.o: src/main.c | build
	$(CC) $(CFLAGS) -c src/main.c -o $@

build/lexer.o: src/lexer.c src/lexer.h | build
	$(CC) $(CFLAGS) -c src/lexer.c -o $@

clean:
	rm -rf ./build

