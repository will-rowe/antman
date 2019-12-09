# Compiler options
CC=gcc
CFLAGS=-c -ansi -g -Wall -O3 -std=c99 -lm
LDFLAGS=-I /usr/local/include
EXECUTABLE_NAME=antman

# Defines
SRC=./src
BIN=./bin
TEST=./tests
OBJ=$(BIN)
LIBFSWATCH=/usr/local/lib/libfswatch.dylib

# Files
INCL=$(shell find $(SRC) -name *.h)
SOURCE_FILES=$(shell find $(SRC) -name *.c)
EXECUTABLE_FILES=$(EXECUTABLE_NAME:%=$(BIN)/%)
OBJECT_FILES=$(SOURCE_FILES:%.c=$(OBJ)/%.o)

# Build
build: msg1 $(EXECUTABLE_FILES)

test:
	@echo "Running unit tests..."
	@echo " - compiling tests"
	@$(CC) $(TEST)/config_test.c -o $(TEST)/_test_config
	@$(CC) $(TEST)/heap_test.c -o $(TEST)/_test_heap
	@$(CC) $(TEST)/sketch_test.c -o $(TEST)/_test_sketch
	@echo " - launching test runner"
	@cd $(TEST) && ./run_unit_tests.sh
	@echo " - cleaning up"
	@rm ${TEST}/_test*

clean:
	@echo "Removing ./bin..."
	@rm -rf $(BIN)

.PHONY: build clean

# Build binary
$(EXECUTABLE_FILES): $(OBJECT_FILES)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBFSWATCH) -lpthread -lz
	@echo "Build successful!"

# Objects depend on these Libraries
$(OBJECT_FILES): $(INCL)

# Create objects
$(OBJECT_FILES): $(OBJ)/%.o: %.c
	@echo " - compiling" $<
	@mkdir -p $(@D)
	@$(CC) -o $@ $< $(CFLAGS)



# Messages
msg1:
	@echo "Running build..."
