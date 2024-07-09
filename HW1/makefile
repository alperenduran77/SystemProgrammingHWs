# Define the compiler
CC=gcc

# Define any compile-time flags
CFLAGS=-Wall -Wextra -g

# Define the target executable name
TARGET=main

# Define the source file(s)
SRC=main.c

# Define the object file(s)
OBJ=$(SRC:.c=.o)

# Default target: build the application
all: $(TARGET)

# Rule to link the program
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Rule to compile the source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove compiled objects, executables, and log files
clean:
	rm -f $(TARGET) $(OBJ) operations.log *.txt

.PHONY: all clean make
