all: project2

project2: main.c dedupe.c hash_functions.c dedupe.h hash_functions.h
	gcc -O3 main.c dedupe.c hash_functions.c -lcrypto -o project2

test:
	cat input.txt
	./project2 input.txt 4 out1.txt
	./project2 input.txt 2 out2.txt
	head out*.txt
