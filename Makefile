CC = gcc
CD = gdb

PROG_NAME = dedup

SOURCES = main.c dedupe.c hash_functions.c

VER = c23

BUILD_DIR = ./build

CFLAGS = -std=$(VER) -O3 -g -Wall -Wconversion -fanalyzer -fsanitize=address,undefined,leak -fsanitize-trap=undefined

TEST_INPUT = input.txt

TEST_OUTPUT_NAME = output

LIBS = 

run: build
	$(BUILD_DIR)/$(PROG_NAME)

build: $(SOURCES)
	mkdir -p $(BUILD_DIR)
	$(CC) -o $(PROG_NAME) $(CFLAGS) $(LIBS) $(SOURCES)
	mv $(PROG_NAME) $(BUILD_DIR) 

clean:
	rm -rf $(BUILD_DIR)

debug: build
	$(CD) $(BUILD_DIR)/$(PROG_NAME)

test: build $(TEST_INPUT)
	echo Running tests...
	cat $(TEST_INPUT)
	$(BUILD_DIR)/$(PROG_NAME) $(TEST_INPUT) 4 $(TEXT_OUTPUT_NAME)1.txt
	$(BUILD_DIR)/$(PROG_NAME) $(TEST_INPUT) 2 $(TEXT_OUTPUT_NAME)2.txt
	echo Results:
	head out*.txt
