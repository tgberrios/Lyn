CC = gcc
CFLAGS = -I./src
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
TARGET = lyn

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) *.out *.c

run: all
	./$(TARGET) test.lyn
