# Makefile for MyShell - OS Assignment 03
CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS = -lreadline
SRC = src/main.c src/shell.c src/execute.c
OBJ = $(SRC:.c=.o)
BIN = bin/myshell

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(BIN)

