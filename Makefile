CC = cc
CFLAGS = -Wall -Wextra

all: build/main

build:
	mkdir -p build

build/main: build/main.o build/base.o build/lexer.o build/parser.o
	$(CC) $(CFLAGS) $^ -o build/main

build/main.o: src/main.c | build
	$(CC) $(CFLAGS) -c src/main.c -o $@

build/parser.o: src/parser.c src/parser.h | build
	$(CC) $(CFLAGS) -c src/parser.c -o $@

build/lexer.o: src/lexer.c src/lexer.h | build
	$(CC) $(CFLAGS) -c src/lexer.c -o $@

build/base.o: src/base.c src/base.h | build
	$(CC) $(CFLAGS) -c src/base.c -o $@

clean:
	rm -rf ./build

