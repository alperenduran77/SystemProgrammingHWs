# Makefile to compile and run main.c with pthreads

CC = gcc
CFLAGS = -pthread
TARGET = main

all: $(TARGET) run

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
