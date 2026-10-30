CC      = gcc
CFLAGS  = -Wall -Wextra -std=c17 -D_POSIX_C_SOURCE=200809L -Iinclude -g -pthread
LDFLAGS = -pthread

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=build/%.o)

all: torneo

torneo: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c include/komikan.h | build
	$(CC) $(CFLAGS) -c $< -o $@

test_tablero: build/tablero.o tests/test_tablero.c include/komikan.h | build
	$(CC) $(CFLAGS) -o $@ tests/test_tablero.c build/tablero.o $(LDFLAGS)

test: test_tablero
	./test_tablero

build:
	mkdir -p build

clean:
	rm -rf build torneo test_tablero

.PHONY: all test clean
