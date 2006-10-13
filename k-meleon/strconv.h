/*
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

#ifndef __STRCONV_H_
#define __STRCONV_H_

#include <string>
#include "utf.h"

template<class T>
class base_convert
{
public:

	base_convert() : mBuffer(NULL)
	{
	}

	~base_convert()
	{
		if( mFBuffer != mBuffer )
			free(mBuffer);
	}

	inline operator const T* () const {return mBuffer;}

protected:
	bool Init(unsigned len)
	{
		if(len > STACK_BUFFER_SIZE)
		{
			mBuffer = static_cast<T*>(malloc(len*sizeof(T)));
			if (mBuffer == NULL)
				return false;
		}
		else
			mBuffer = mFBuffer;
		return true;
	}

	T* mBuffer;
	T  mFBuffer[STACK_BUFFER_SIZE];

};


class CUTF16_to_ANSI : public base_convert<char>
{
public:
	CUTF16_to_ANSI(const WCHAR* src)
	{
		if (!src) return;

		unsigned lengthDst = lstrlenW(src)*2+1;
		if (!Init(lengthDst))
			return;

		utf16_to_ansi(src, mBuffer, lengthDst);
	}
};

class CANSI_to_UTF16 : public base_convert<WCHAR>
{
public:
	CANSI_to_UTF16(const char* src)
	{
		if (!src) return;

		size_t lengthDst = strlen(src)+1;
		if (!Init(lengthDst))
			return;

		ansi_to_utf16(src, mBuffer, lengthDst);
	}
};

class CUTF8_to_UTF16 : public base_convert<WCHAR>
{
public:
	CUTF8_to_UTF16(const char* src)
	{
		if (!src) return;

		size_t lengthDst = strlen(src)+1;
		if (!Init(lengthDst))
			return;

		utf8_to_utf16(src, mBuffer, lengthDst);
	}
};

class CUTF16_to_UTF8 : public base_convert<char>
{
public:
	CUTF16_to_UTF8(const WCHAR* src)
	{
		if (!src) return;

		unsigned lengthDst = lstrlenW(src)*3+1;
		if (!Init(lengthDst))
			return;

		utf16_to_utf8(src, mBuffer, lengthDst);
	}
};

class CUTF8_to_ANSI : public base_convert<char>
{
public:
	CUTF8_to_ANSI(const char* src)
	{
		if (!src) return;

		size_t lengthDst = strlen(src)+1;
		if (!Init(lengthDst))
			return;

		utf8_to_ansi(src, mBuffer, lengthDst);
	}
};

class CANSI_to_UTF8 : public base_convert<char>
{
public:
	CANSI_to_UTF8(const char* src)
	{
		if (!src) return;

		size_t lengthDst = strlen(src)*3 + 1;
		if (!Init(lengthDst))
			return;

		ansi_to_utf8(src, mBuffer, lengthDst);
	}
};

#ifdef _UNICODE
#define CANSI_to_T(x) CANSI_to_UTF16(x)
#define CT_to_ANSI(x) CT_to_UTF16(x)
#define CUTF16_to_T(x) (x)
#define CT_to_UTF8(x) CUTF16_to_UTF8(x)
#define CUTF8_to_T(x) CUTF8_to_UTF16(x)
#else // Blarg
#define CANSI_to_T(x) (x)
#define CT_to_ANSI(x) (x)
#define CUTF16_to_T(x) CUTF16_to_ANSI(x)
#define CT_to_UTF8(x) CANSI_to_UTF8(x)
#define CUTF8_to_T(x) CUTF8_to_ANSI(x)
#endif

#endif // __STRCONV_H_