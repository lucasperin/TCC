#ifndef CRYPTOFUNCTIONS_H
#define CRYPTOFUNCTIONS_H
#include "base64.h"

/* OpenSSL includes */
#include <openssl/dsa.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/hmac.h>
#include <openssl/des.h>

/* chave 128bits */
static const char key[32] = { 0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
							  0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};
static const char iv[32] = { 0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
		  0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};
EVP_CIPHER_CTX en, de;

/**
 * Create an 256 bit key and IV using the supplied key_data. salt can be added for taste.
 * Fills in the encryption and decryption ctx objects and returns 0 on success
 **/
int aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx,
             EVP_CIPHER_CTX *d_ctx)
{
  int i, nrounds = 5;
  unsigned char key[32], iv[32];

  /*
   * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
   * nrounds is the number of times the we hash the material. More rounds are more secure but
   * slower.
   */
  i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);
  if (i != 32) {
    printf("Key size is %d bits - should be 256 bits\n", i);
    return -1;
  }

  EVP_CIPHER_CTX_init(e_ctx);
  EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
  EVP_CIPHER_CTX_init(d_ctx);
  EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

  return 0;
}

/*
 * Encrypt *len bytes of data
 * All data going in & out is considered binary (unsigned char[])
 */
unsigned char *aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len)
{
  /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
  int c_len = *len + 256, f_len = 0;
  unsigned char *ciphertext = malloc(c_len);

  /* allows reusing of 'e' for multiple encryption cycles */
  EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL);

  /* update ciphertext, c_len is filled with the length of ciphertext generated,
    *len is the size of plaintext in bytes */
  EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len);

  /* update ciphertext with the final remaining bytes */
  EVP_EncryptFinal_ex(e, ciphertext+c_len, &f_len);

  *len = c_len + f_len;
  return ciphertext;
}

/*
 * Decrypt *len bytes of ciphertext
 */
unsigned char *aes_decrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext, int *len)
{
  /* plaintext will always be equal to or lesser than length of ciphertext*/
  int p_len = *len, f_len = 0;
  unsigned char *plaintext = malloc(p_len);

  EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL);
  EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len);
  EVP_DecryptFinal_ex(e, plaintext+p_len, &f_len);

  *len = p_len + f_len;
  return plaintext;
}

void print_hex(unsigned char* toPrint, unsigned int length){
	unsigned int i = 0;
	for(; i < length-1; i++)
	{
		printf("%X:", toPrint[i]);
	}
	printf("%X", toPrint[length-1]);
	printf("\n");
}

struct ByteArray* generateHMAC(unsigned char* message)
{
	struct ByteArray* ret = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	unsigned char result[EVP_MAX_MD_SIZE];

	HMAC(EVP_sha1(), key, sizeof(key), message, sizeof(message), result, &ret->size);

	ret->data = (unsigned char*)malloc(ret->size);
	memcpy(ret->data, result, ret->size);
//	print_hex(ret, *resultLength);
	return ret;
}

int validadeHMAC(char* message, unsigned char* hmacToCompare)
{
	struct ByteArray* hmac = generateHMAC((unsigned char*)message);
	int toReturn = memcmp(hmac->data, hmacToCompare, hmac->size);
	free(hmac);//TODO
	return toReturn;
}

unsigned char* Xor(unsigned char* hmac1, unsigned char* hmac2, unsigned int size)
{
	unsigned char* ret = (unsigned char*) malloc(size*sizeof(unsigned char));
	unsigned int i = 0;
	for(; i < size ; i++)
	{
		ret[i] = hmac1[i] ^ hmac2[i];
	}
	return ret;
}

struct ByteArray* cipher(struct ByteArray* toCipher)
{
	struct ByteArray* ret = (struct ByteArray*) malloc(sizeof(struct ByteArray));
    ret->data = aes_encrypt(&en, toCipher->data, &toCipher->size);
	ret->size = toCipher->size;
//	free(toCipher);//TODO esse ponteiro deveria ser liberado pelo 'generateHistory' DA SEG FAULT
	return ret;
}

struct ByteArray* generateHistory(struct ByteArray* hmac1, struct ByteArray* hmac2)
{
	if(hmac1 != hmac2) {
		struct ByteArray* toCipher = XorB(hmac1, hmac2);
		return generateHMAC(toCipher->data);
	} else {
		return generateHMAC(hmac1->data);
	}
}

struct ByteArray* decipher(struct ByteArray* toDecipher)
{
	struct ByteArray* ret = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	ret->data = aes_decrypt(&de, toDecipher->data, &toDecipher->size);
	ret->size = toDecipher->size;
	return ret;
}

int validateHistory(struct ByteArray* history, struct ByteArray* hmac1, struct ByteArray* hmac2)
{
	struct ByteArray* HistoryToCompare = generateHistory(hmac1, hmac2);

	int ret = memcmp(history->data, HistoryToCompare->data, 20);
	free(HistoryToCompare);
	return ret;
}

#endif
