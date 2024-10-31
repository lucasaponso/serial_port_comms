# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -pthread

# Source files
SRC = src/main.c

# Output executable name
OUT = bin/main

# Default target
all: $(OUT)

# Rule to create the executable
$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

# Clean up the directory
clean:
	rm -f $(OUT)
