CC = gcc
CD = gdb

PROG_NAME = project2

SOURCES = src/main.c src/dedupe.c src/hash_functions.c

VER = c23

BUILD_DIR = ./build

CFLAGS = -std=$(VER) -O3 -g -Wall -Wconversion -fanalyzer -fsanitize=address,undefined,leak -fsanitize-trap=undefined

TEST_INPUT = input.txt

TEST_OUTPUT_NAME = out

LIBS = -lcrypto

run: build
	$(BUILD_DIR)/$(PROG_NAME)

build: $(SOURCES)
	mkdir -p $(BUILD_DIR)
	$(CC) -o $(PROG_NAME) $(CFLAGS) $(LIBS) $(SOURCES)
	mv $(PROG_NAME) $(BUILD_DIR) 

clean:
	rm -rf $(BUILD_DIR)
	rm out1.txt
	rm out2.txt

debug: build
	$(CD) $(BUILD_DIR)/$(PROG_NAME)

test: build $(TEST_INPUT)
	echo Running tests...
	echo 
	cat $(TEST_INPUT)
	$(BUILD_DIR)/$(PROG_NAME) $(TEST_INPUT) 4 $(TEST_OUTPUT_NAME)1.txt
	$(BUILD_DIR)/$(PROG_NAME) $(TEST_INPUT) 2 $(TEST_OUTPUT_NAME)2.txt
	echo
	echo ~~ Results ~~
	echo 
	echo out1.txt:
	echo ---
	head out1.txt
	echo ---
	echo 
	echo out2.txt:
	echo ---
	head out2.txt
	echo ---
