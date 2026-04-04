#include <openssl/evp.h>
#include <stdlib.h>

unsigned int size_sha512() {
	return EVP_MD_size(EVP_sha512());
}

int cmp(const void *a, const void *b) {
	unsigned char va = *(unsigned char *) a, vb = *(unsigned char *) b;

	if(va < vb) return -1;
	if(va == vb) return 0;
	return 1;
}

unsigned char *calculate_sha512(unsigned char *buf, unsigned int buf_size) {
	EVP_MD_CTX *mdctx;
	unsigned char *sha512_digest1, *sha512_digest2;
	unsigned int sha512_digest_len = size_sha512();
    
	mdctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex(mdctx, EVP_sha512(), NULL);
	EVP_DigestUpdate(mdctx, buf, buf_size);
	sha512_digest1 = (unsigned char *) malloc(sha512_digest_len);
	EVP_DigestFinal_ex(mdctx, sha512_digest1, &sha512_digest_len);
	EVP_MD_CTX_free(mdctx);

	qsort(buf, buf_size, sizeof(unsigned char), cmp);

	mdctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex(mdctx, EVP_sha512(), NULL);
	EVP_DigestUpdate(mdctx, buf, buf_size);
	sha512_digest2 = (unsigned char *) malloc(sha512_digest_len);
	EVP_DigestFinal_ex(mdctx, sha512_digest2, &sha512_digest_len);
	EVP_MD_CTX_free(mdctx);

	for(int i=0; i < sha512_digest_len; i++)
		sha512_digest1[i] ^= sha512_digest2[i];

	free(sha512_digest2);

	return sha512_digest1;
}

