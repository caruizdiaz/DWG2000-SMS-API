/*
 * dwg_charset.h
 *
 *  Created on: Jan 5, 2013
 *      Author: carlos
 */

#ifndef DWG_CHARSET_H_
#define DWG_CHARSET_H_

#include "../util.h"

void dwg_initialize_translation_table();
void dwg_ascii2unicode(str_t *ascii, str_t* unicode);
void dwg_unicode2ascii(str_t *unicode, str_t* ascii);
void dwg_ascii2gsm7bit(str_t *ascii, str_t* gsm7bit);


#endif /* DWG_CHARSET_H_ */
