#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "hash_functions.h"

int compare_hashes(unsigned char *a, unsigned char *b, int n) {
	for(int i=0; i < n; i++)
		if(a[i] != b[i])
			return 0;
	return 1;
}

// Function name: dedupe
// Description:   Computes a hash for each chunk of the input file, and the obtained hashes
//                to each other to determine the number of unique chunks in the file
void dedupe(char *filename, int chunk_size, char *output) {
	FILE *fp;
	char *buffer = (char *) malloc(chunk_size*sizeof(char));
	unsigned char **hashes = NULL;
	int hash_size = size_sha512(), n_hashes = 0;

	// load chunks of the input file and hash them
	fp = fopen(filename, "r");
	assert(fp != NULL);
	while(fread(buffer, sizeof(char), chunk_size, fp) == chunk_size) {
		hashes = (unsigned char **) realloc(hashes, (n_hashes+1)*sizeof(unsigned char *));
		hashes[n_hashes] = calculate_sha512((unsigned char *)buffer, chunk_size);
		n_hashes++;
	}
	fclose(fp);

	int mask[n_hashes];
	for(int i=0; i < n_hashes; i++)
		mask[i] = 0;
	for(int i=0; i < n_hashes; i++)
		for(int j=i+1; j < n_hashes; j++)
			if(compare_hashes(hashes[i], hashes[j], hash_size)) {	
				mask[j] = 1;
				break;
			}

	// print results
	fp = fopen(output, "w");
	assert(fp != NULL);
	for(int i=0; i < n_hashes; i++)
		fprintf(fp, "%d", mask[i]);
	fprintf(fp, "\n");
	fclose(fp);

	// release stuff
	free(buffer);
	for(int i=0; i < n_hashes; i++)
		free(hashes[i]);
	free(hashes);
}

