#ifndef RANDOMSTRING_H
#define RANDOMSTRING_H

#include <string.h>
#include <time.h>
#include <stdlib.h>

static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";


char* generateString(int size)
{
	char* ret = calloc(size + 1, sizeof(char));
	int i = 0;

	struct timeval time;
	gettimeofday(&time, 0);
	srand ( time.tv_usec );

	for (; i < size; i ++)
	{
		int index = random() % 41;
		char cat = (char)alphanum[index];
		ret[i] = cat;
	}
	ret[i] = '\0';
	return ret;
}

char* generateRandomSizeString(int maxSize)
{
	int size = ((int)random()) % maxSize;;
	char* ret = calloc(size + 1, sizeof(char));
	int i = 0;

	struct timeval time;
	gettimeofday(&time, 0);
	srand ( time.tv_usec );

	for (; i < size; i ++)
	{
		int index = random() % 41;
		char cat = (char)alphanum[index];
		ret[i] = cat;
	}
	ret[i] = '\0';
	return ret;
}

#endif
