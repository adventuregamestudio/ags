//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

// borrowed and modified from [Stack Overflow](http://stackoverflow.com/q/34912145/1136311)
//
// Visual Studio's _snprintf is not C99-compliant as it does not
// null-terminate the buffer, which may result in unexpected bad memory
// access. This provides a C99-compliant alternative. From VS2015 this is
// no longer needed as a compliant snprintf function is implemented.

#ifndef __AGS_CN_UTIL__C99_SNPRINTF_H
#define __AGS_CN_UTIL__C99_SNPRINTF_H

#if defined(_MSC_VER) && (_MSC_VER < 1900)

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
	int count = -1;

	if (size != 0)
		count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
	if (count == -1)
		count = _vscprintf(format, ap);

	return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
	int count;
	va_list ap;

	va_start(ap, format);
	count = c99_vsnprintf(outBuf, size, format, ap);
	va_end(ap);

	return count;
}

#endif // _MSC_VER

#endif // __AGS_CN_UTIL__C99_SNPRINTF_H
