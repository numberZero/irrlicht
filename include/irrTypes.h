// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_TYPES_H_INCLUDED__
#define __IRR_TYPES_H_INCLUDED__

#include <cstdint>
#include "irr.h"

namespace irr
{

	using u8 = std::uint8_t;
	using s8 = std::int8_t;
	using c8 = char;

	using u16 = std::uint16_t;
	using s16 = std::int16_t;

	using u32 = std::uint32_t;
	using s32 = std::int32_t;

	using u64 = std::uint64_t;
	using s64 = std::int64_t;

	using f32 = float;
	using f64 = double;

	using fschar_t = char;
	#define _IRR_TEXT(X) X

} // end namespace irr

#include <wchar.h>
#ifdef _IRR_WINDOWS_API_
	//! Defines for s{w,n}printf_irr because s{w,n}printf methods do not match the ISO C
	//! standard on Windows platforms.
	//! We want int snprintf_irr(char *str, size_t size, const char *format, ...);
	//! and int swprintf_irr(wchar_t *wcs, size_t maxlen, const wchar_t *format, ...);
	#if defined(_MSC_VER) && _MSC_VER > 1310 && !defined (_WIN32_WCE)
		#define swprintf_irr swprintf_s
		#define snprintf_irr sprintf_s
	#elif !defined(__CYGWIN__)
		#define swprintf_irr _snwprintf
		#define snprintf_irr _snprintf
	#endif
#else
	#define swprintf_irr swprintf
	#define snprintf_irr snprintf
#endif // _IRR_WINDOWS_API_

#endif // __IRR_TYPES_H_INCLUDED__
