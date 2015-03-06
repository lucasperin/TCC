#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include <stdlib.h>
struct ByteArray
{
unsigned char* data;
unsigned int size;
};

struct ByteArray* XorB(struct ByteArray* hmac1, struct ByteArray* hmac2)
{
	struct ByteArray* ret = (struct ByteArray*) malloc(sizeof(struct ByteArray));
	ret->data = (unsigned char*) malloc(hmac1->size*sizeof(unsigned char));
	ret->size = hmac1->size;
	unsigned int i = 0;
	for(; i < hmac1->size ; i++)
	{
		ret->data[i] = hmac1->data[i] ^ hmac2->data[i];
	}

	return ret;
}
#endif
