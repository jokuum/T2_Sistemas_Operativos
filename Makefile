CC      = gcc
CFLAGS  = -Wall -Wextra -std=c17 -D_POSIX_C_SOURCE=200809L -Iinclude -g -pthread
LDFLAGS = -pthread

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=build/%.o)

torneo: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c include/komikan.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build torneo

.PHONY: clean