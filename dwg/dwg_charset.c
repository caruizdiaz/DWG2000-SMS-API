/*
 * dwg_charset.c
 *
 *  Created on: Jan 5, 2013
 *      Author: carlos
 */
#include "dwg_charset.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int gsm7bit2ascii[] = { 64, 163, 36, 165, 232, 223, 249, 236, 242, 199, 10, 216, 248, 13, 197, 229, 0, 95, 0, 0, 0, 0,
		 	 	 	 	 	 	 0, 0, 0, 0, 0, 0, 198, 230, 223, 201, 32, 33, 34, 35, 164, 37, 38, 39, 40, 41, 42, 43, 44, 45,
		 	 	 	 	 	 	 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 161, 65, 66, 67, 68, 69, 70,
		 	 	 	 	 	 	 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 196, 204, 209, 220, 167,
		 	 	 	 	 	 	 191, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
		 	 	 	 	 	 	 117, 118, 119, 120, 121, 122, 228, 246, 241, 252, 224 };

static char ascii2gsm7bit[255];

void dwg_initialize_translation_table()
{
	int i;

	memset(ascii2gsm7bit, 0, sizeof(ascii2gsm7bit));

//	printf("wop\n");
	for (i = 0; i < 127; i++)
	{
		//printf("-> %i: %c %d\n", i, gsm7bit2ascii[i], (int) gsm7bit2ascii[i]);

		ascii2gsm7bit[gsm7bit2ascii[i]]	= i;
	}

//	printf("done\n");
}

void dwg_ascii2gsm7bit(str_t *ascii, str_t* gsm7bit)
{
	int i, j;

	STR_ALLOC((*gsm7bit), ascii->len);

	if (!gsm7bit->s)
	{
		LOG(L_ERROR, "%s: no more memory trying to allocate %d bytes\n", __FUNCTION__,  ascii->len);
		return;
	}

	for(i = 0; i < ascii->len; i++)
	{
		unsigned char ch = ascii->s[i];
		switch(ch)
		{
		case 12:
			gsm7bit->s[i]	= 10;
			continue;
		case 94:
			gsm7bit->s[i]	= 20;
			continue;
		case 123:
			gsm7bit->s[i]	= 40;
			continue;
		case 125:
			gsm7bit->s[i]	= 41;
			continue;
		case 92:
			gsm7bit->s[i]	= 47;
			continue;
		case 91:
			gsm7bit->s[i]	= 60;
			continue;
		case 126:
			gsm7bit->s[i]	= 61;
			continue;
		case 93:
			gsm7bit->s[i]	= 62;
			continue;
		case 124:
			gsm7bit->s[i]	= 64;
			continue;
		case 164:
			gsm7bit->s[i]	= 101;
			continue;
		}

		gsm7bit->s[i]	= ascii2gsm7bit[ascii->s[i]];
	}

//	printf("ascii-> %.*s\n", ascii->len, ascii->s);
//	printf("gsm7bit-> %.*s\n", gsm7bit->len, gsm7bit->s);
}

void dwg_ascii2unicode(str_t *ascii, str_t* unicode)
{
	int i;

	STR_ALLOC((*unicode), ascii->len * 2);
	if (!unicode->s)
	{
		LOG(L_ERROR, "%s: no more memory trying to allocate %d bytes\n", __FUNCTION__,  ascii->len * 2);
		return;
	}

	for(i = 0; i < ascii->len; i++)
	{
		unicode->s[i * 2]		= 0;
		unicode->s[i * 2 + 1]	= ascii->s[i];

	}
}

void dwg_unicode2ascii(str_t *unicode, str_t* ascii)
{
	short uni_array[unicode->len / 2];
	int i;

	memcpy(&uni_array, unicode->s, unicode->len);
	STR_ALLOC((*ascii), (unicode->len / 2));
	if (!ascii->s)
	{
		LOG(L_ERROR, "%s: no more memory trying to allocate %d bytes\n", __FUNCTION__,  (unicode->len / 2));
		return;
	}

	for(i = 0; i < (unicode->len / 2); i++)
		ascii->s[i]	= swap_bytes_16(uni_array[i]);

}
