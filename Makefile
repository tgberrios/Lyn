# Makefile completo para compilar Lyn
SRCS = src/main.c src/lexer.c src/parser.c src/ast.c src/compiler.c src/error.c src/optimizer.c src/memory.c src/module.c src/logger.c src/types.c
OBJS = $(SRCS:.c=.o)
TARGET = lyn

CC = gcc
CFLAGS = -Wall -Wextra -g

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) test.out output.c

test: $(TARGET)
	./$(TARGET) test.lyn

.PHONY: all clean test