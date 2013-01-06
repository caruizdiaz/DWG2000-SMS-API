/*
 * global.h
 *
 *  Created on: Oct 20, 2011
 *      Author: caruizdiaz
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>

typedef unsigned int 	_uint;
typedef unsigned short 	_ushort;
typedef unsigned char 	_uchar;
typedef unsigned int 	_bool;

typedef struct str
{
	char *s;
	int len;
} str_t;

#define FALSE			0
#define TRUE			1

#define L_DEBUG			0
#define L_WARNING		1
#define L_ERROR			2

#ifdef WITH_DEBUG
#	define LOG(LEVEL, format, ...) 				\
			if (LEVEL == L_DEBUG) { 			\
				fprintf(stderr, "[DEBUG] ");	\
				fprintf(stderr, format, __VA_ARGS__); 	\
			} 									\
			else if (LEVEL == L_WARNING) { 		\
				fprintf(stderr, "[WARNING] ");  			\
				fprintf(stderr, format, __VA_ARGS__); 	\
			} 									\
			else if (LEVEL == L_ERROR) { 		\
				fprintf(stderr, "[ERROR] ");  			\
				fprintf(stderr, format, __VA_ARGS__); 	\
			}
#else
#	define LOG(LEVEL, format, ...) 				\
		if (LEVEL == L_ERROR) { 				\
			fprintf(stderr, "[DEBUG] ");  				\
			fprintf(stderr, format, __VA_ARGS__); 		\
		}
#endif

#define STR_ALLOC(_str_, _size_) _str_.s       = (char *) calloc(_size_, sizeof(char)); \
                                                                  _str_.len = _size_;

#define STR_FREE(_str_) free(_str_.s);

#define STR_FREE_NON_0(_str_) if (_str_.s != NULL) free(_str_.s);

#define str_copy(_str_to_, _str_from_) _str_to_.s = calloc(_str_from_.len + 1, sizeof(char)); \
									   _str_to_.len = _str_from_.len; \
									   strcpy(_str_to_.s, _str_from_.s); \

void hexdump(void *ptr, int buflen);
short swap_bytes_16(short input);
int swap_bytes_32(int input);

#endif /* UTIL_H_ */
