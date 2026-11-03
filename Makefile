CC      = gcc
CFLAGS  = -Wall -Wextra -std=c17 -D_POSIX_C_SOURCE=200809L -Iinclude -g -pthread
LDFLAGS = -pthread

SRC   = $(wildcard src/*.c)
OBJ   = $(SRC:src/%.c=build/%.o)
TESTS = test_tablero test_log test_aleatorio

all: torneo

torneo: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c include/komikan.h | build
	$(CC) $(CFLAGS) -c $< -o $@

# Cada test se enlaza solo con el módulo que prueba
test_tablero: tests/test_tablero.c build/tablero.o include/komikan.h | build
	$(CC) $(CFLAGS) -o $@ tests/test_tablero.c build/tablero.o $(LDFLAGS)

test_log: tests/test_log.c build/log.o include/komikan.h | build
	$(CC) $(CFLAGS) -o $@ tests/test_log.c build/log.o $(LDFLAGS)

test_aleatorio: tests/test_aleatorio.c build/aleatorio.o include/komikan.h | build
	$(CC) $(CFLAGS) -o $@ tests/test_aleatorio.c build/aleatorio.o $(LDFLAGS)

test: $(TESTS)
	./test_tablero
	./test_log
	./test_aleatorio

build:
	mkdir -p build

clean:
	rm -rf build torneo $(TESTS) *.dSYM *.log

.PHONY: all test clean