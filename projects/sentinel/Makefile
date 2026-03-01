# Compiler to use
CC=gcc

# Compiler flags: enable warnings, optimize, include debug symbols, add pthread support
CFLAGS=-Wall -Wextra -O2 -g Iinclude -pthread

# Find all .c source files in src/ directory
SRC=$(wildcard src/*.c)

# Convert each .c file to corresponding .o obj file
OBJ=$(SRC:.c=.o)

# Name the final executable file
BIN=sentinel

# Default target: build the sentinel binary
all: $(BIN)

# Link all object files to create the sentinel executable
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Clean: remove the executable and all object files
clean:
	rm -f $(BIN) $(OBJ)