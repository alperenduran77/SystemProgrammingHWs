# Makefile for 200104004024_main

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -pthread

# Target executable
TARGET = 200104004024_main

# Source files
SRCS = 200104004024_main.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(TARGET) $(OBJS)

# Phony targets
.PHONY: all clean
