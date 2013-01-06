/*
 * util.c
 *
 *  Created on: Mar 29, 2012
 *      Author: caruizdiaz
 */

#include <stdio.h>

/*
 * From: http://stackoverflow.com/questions/29242/off-the-shelf-c-hex-dump-code
 */
void hexdump(void *ptr, int buflen)
{
  unsigned char *buf = (unsigned char*)ptr;
  int i, j;
  for (i=0; i<buflen; i+=16) {
    printf("%06x: ", i);
    for (j=0; j<16; j++)
      if (i+j < buflen)
        printf("%02x ", buf[i+j]);
      else
        printf("   ");
    printf(" ");
    for (j=0; j<16; j++)
      if (i+j < buflen)
        printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
    printf("\n");
  }
}

short swap_bytes_16(short input)
{
	return (input>>8) | (input<<8);
}

int swap_bytes_32(int input)
{
	return ((input>>24)&0xff) 		| 		// move byte 3 to byte 0
            ((input<<8)&0xff0000) 	| 		// move byte 1 to byte 2
            ((input>>8)&0xff00) 	| 		// move byte 2 to byte 1
            ((input<<24)&0xff000000); 		// byte 0 to byte 3
}
