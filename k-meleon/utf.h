/*
 * Copyright (C) 2004 Dorian Boissonnade
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __UTF_H_
#define __UTF_H_

#include <stdlib.h>

#ifndef STACK_BUFFER_SIZE
#define STACK_BUFFER_SIZE 128
#endif

inline size_t utf16_to_utf8(const wchar_t* src, char* dst, unsigned len)
{
#ifdef _UNICODE
	return WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, len, NULL, NULL);
#else
	if (!src) return 0;

	unsigned index = 0, windex = 0;
    unsigned short c = src[index++];

	if (!dst)
	{
		while(c)
		{
			if (c < 0x080) windex++;
			else if(c < 0x0800) windex+=2;
			else if (c < 0xD800 || c > 0xDFFF) windex+=3;
			else if (c  >= 0xD800 && c <= 0xDBFF) windex+=4;
			else windex+=1;
			c = src[index++];
		}
		return windex+1;
	}

    while(c)
    {
        if(c < 0x080)
		{
			if (len<2) break;
			len--;
            dst[windex++] = (unsigned char)c;
        }
		else if(c < 0x800)
		{
			if (len<3) break;
			len -=2;
            dst[windex++] = 0xc0 | (c >> 6);
            dst[windex++] = 0x80 | (c & 0x3f);
        }
		else if (c < 0xD800 || c > 0xDFFF)
		{ 
			if (len<4) break;
			len -=3;
            dst[windex++] = 0xe0 | (c >> 12);
            dst[windex++] = 0x80 | ((c >> 6) & 0x3f);
            dst[windex++] = 0x80 | (c & 0x3f);
        }
		else if (c  >= 0xD800 && c <= 0xDBFF)
		{
			if (len<5) break;
			len -=4;

			unsigned long c_utf32 = 0;
			unsigned short c2 = src[index++];

			if (c2 >= 0xDC00 && c2 <= 0xDFFF)
			{
				c_utf32 = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x0010000UL;

				dst[windex++] = 0xf0 | (char)(c_utf32 >> 18);
				dst[windex++] = 0x80 | (char)((c_utf32 >> 12) & 0x3f);
				dst[windex++] = 0x80 | (char)((c_utf32 >> 6) & 0x3f);
				dst[windex++] = 0x80 | (char)(c_utf32 & 0x3f);
			}
			else  
				dst[windex++] = '_'; // Bad UTF16 string
		}
		else 
			dst[windex++] = '_'; // Bad UTF16 string

        c = src[index++];
    }
    dst[windex] = 0x00;
	return windex + 1;
#endif
}

inline size_t utf8_to_utf16(const char* src, wchar_t* dst, unsigned len)
{
#ifdef _UNICODE
	return MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, len);
#else

	// Should make better invalid string handling 
	if (!src) return 0;

	unsigned index = 0, windex = 0;
	unsigned char c = src[index++];

	if (!dst)
	{
		while(c)
		{
			if((c & 0x80) == 0) index += 1;
			else if((c & 0xe0) == 0xc0) index += 2;
			else if((c & 0xf0) == 0xe0) index += 3;
			else if((c & 0xf8) == 0xf0) {index += 4;windex++;}
			else if((c & 0xfc) == 0xf8) index += 5;
			else if((c & 0xfe) == 0xfc) index += 6;
			else index += 1;
			windex++;
			c = src[index];
		}
		return windex+1;
    }	

    while(c)
    {
		if (!--len) break;
        if((c & 0x80) == 0)
		{
            dst[windex++] = c;
        } 
		else if((c & 0xe0) == 0xc0)
		{
            dst[windex] = (c & 0x1F) << 6;
	        c = src[index++];
            dst[windex++] |= (c & 0x3F);
        }
		else if((c & 0xf0) == 0xe0)
		{
            dst[windex] = (c & 0x0F) << 12;
	        c = src[index++];
            dst[windex] |= (c & 0x3F) << 6;
	        c = src[index++];
            dst[windex++] |= (c & 0x3F);
        }
		else if((c & 0xf8) == 0xf0)
		{
			unsigned long c_utf32;
			c_utf32 = (c & 0x07) << 18;
			c = src[index++];
			c_utf32 = (c & 0x3F) << 12;
	        c = src[index++];
            c_utf32 |= (c & 0x3F) << 6;
	        c = src[index++];
            c_utf32 |= (c & 0x3F);

			c_utf32 -= 0x0010000UL;
			dst[windex++] = (wchar_t)((c_utf32 >> 10) + 0xD800);
			dst[windex++] = (wchar_t)((c_utf32 & 0x3ff) + 0xDC00);
		}
		// Invalid UTF8
		else if((c & 0xfc) == 0xf8)
		{ 
			index +=4;
			dst[windex++] = L'_';
		} 
		else if((c & 0xfe) == 0xfc)
		{
			index +=5;
			dst[windex++] = L'_';
		}

        c = src[index++];
    }
    dst[windex] = 0;
	return windex + 1;
#endif
}


inline size_t ansi_to_utf16(const char* src, wchar_t* dst, size_t len)
{
	return MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, dst, len);
}

inline size_t utf16_to_ansi(const wchar_t* src, char* dst, size_t len)
{
	return WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, src, -1, dst, len, "_", NULL);
}

inline size_t ansi_to_utf8(const char* src, char* dst, size_t len)
{
	wchar_t s_unicode[STACK_BUFFER_SIZE];

	size_t lengthDst = strlen(src) + 1;
	wchar_t* unicode = lengthDst < STACK_BUFFER_SIZE ? s_unicode : (wchar_t*)calloc(lengthDst, sizeof(wchar_t));

    if (!unicode) return 0;

	size_t result = 0;
	if (ansi_to_utf16(src, unicode, lengthDst))
		result = utf16_to_utf8(unicode, dst, len);	
	
	if (unicode != s_unicode) free(unicode);
	return result;
}

inline size_t utf8_to_ansi(const char* src, char* dst, size_t len)
{
	wchar_t s_unicode[STACK_BUFFER_SIZE];

	size_t lengthDst = strlen(src) + 1;
	wchar_t* unicode = lengthDst < STACK_BUFFER_SIZE ? s_unicode : (wchar_t*)calloc(lengthDst, sizeof(wchar_t));

	if (!unicode) return 0;

	size_t result = 0;
	if (utf8_to_utf16(src, unicode, lengthDst))
		result = utf16_to_ansi(unicode, dst, len);
    
	if (unicode != s_unicode) free(unicode);
	return result;
}

inline char* ansi_from_utf16(const wchar_t* utf16)
{
	unsigned lengthDst = lstrlenW(utf16) + 1;
	char* ansi = (char*)malloc(sizeof(char) * lengthDst);
	utf16_to_ansi(utf16, ansi, lengthDst);
	return ansi;
}

inline wchar_t* utf16_from_ansi(const char* ansi)
{
	size_t lengthDst = strlen(ansi)*2 + 1;
	wchar_t* utf16 = (wchar_t*)malloc(sizeof(wchar_t) * lengthDst);
	ansi_to_utf16(ansi, utf16, lengthDst);
	return utf16;
}

inline char* utf8_from_utf16(const wchar_t* utf16)
{
	unsigned lengthDst = lstrlenW(utf16)*3 + 1;
	char* utf8 = (char*)malloc(sizeof(char) * lengthDst);
	utf16_to_utf8(utf16, utf8, lengthDst);
	return utf8;
}

inline wchar_t* utf16_from_utf8(const char* utf8)
{
	size_t lengthDst = strlen(utf8) + 1;
	wchar_t* utf16 = (wchar_t*)malloc(sizeof(wchar_t) * lengthDst);
	utf8_to_utf16(utf8, utf16, lengthDst);
	return utf16;
}

inline char* utf8_from_ansi(const char* ansi)
{
	size_t lengthDst = strlen(ansi)*3 + 1;
	char* utf8 = (char*)malloc(sizeof(char) * lengthDst);
	ansi_to_utf8(ansi, utf8, lengthDst);
	return utf8;
}
	
inline char* ansi_from_utf8(const char* utf8)
{
	size_t lengthDst = strlen(utf8) + 1;
	char* ansi = (char*)malloc(sizeof(char) * lengthDst);
	utf8_to_ansi(utf8, ansi, lengthDst);
	return ansi;
}

inline wchar_t* _ansi_to_utf16(const char* src, wchar_t* dst, unsigned len)
{
	if (!ansi_to_utf16(src, dst, len))
		return NULL;
	return dst;
}

inline wchar_t* _utf8_to_utf16(const char* src, wchar_t* dst, unsigned len)
{
	if (!utf8_to_utf16(src, dst, len))
		return NULL;
	return dst;
}

inline char* _utf8_to_ansi(const char* src, char* dst, unsigned len)
{
	if (!utf8_to_ansi(src, dst, len))
		return NULL;
	return dst;
}



inline char* _utf16_to_ansi(const wchar_t* src, char* dst, unsigned len)
{
	if (!utf16_to_ansi(src, dst, len))
		return NULL;
	return dst;
}

#define s_utf16_to_ansi(str) (strlen(str)*2+1)
#define s_ansi_to_utf16(str) (strlen(str)+1)

#ifdef _UNICODE

#define s_utf16_to_t(str) 0
#define s_t_to_utf16(str) 0
#define s_t_to_ansi(str) (strlen(str)*2+1)
#define s_ansi_to_t(str) (strlen(str)+1)

#define utf8_from_t(str) utf8_from_utf16(str);
#define t_from_utf8(str) utf16_from_utf8(str);
#define ansi_to_t(str, size) _ansi_to_utf16(str, (wchar_t*)alloca(size*sizeof(wchar_t)), size)
#define t_to_ansi(str, size) _utf16_to_ansi(str, (char*)alloca(size*sizeof(char)), size)
#define utf8_to_t(str, size) _utf8_to_utf16(str, (wchar_t*)alloca(size*sizeof(wchar_t)), size)
#define utf16_to_t(str, size) (str)
#define t_to_utf16(str, size) (str)

#else

#define s_utf16_to_t(str) (strlen(str)*2+1)
#define s_t_to_utf16(str) (strlen(str)+1)
#define s_t_to_ansi(str) 0
#define s_ansi_to_t(str) 0

#define utf8_from_t(str) utf8_from_ansi(str);
#define t_from_utf8(str) ansi_from_utf8(str);
#define ansi_to_t(str, size) (str)
#define t_to_ansi(str, size) (str)
#define utf16_to_t(str, size) _utf16_to_ansi(str, (char*)alloca(size*sizeof(char)), size)
#define t_to_utf16(str, size) _ansi_to_utf16(str, (wchar_t*)alloca(size*sizeof(wchar_t)), size)
#define utf8_to_t(str, size) _utf8_to_ansi(str, (char*)alloca(size*sizeof(char)), size)

#endif

#endif /* __UTF_H_ */