#pragma once

//! Irrlicht SDK Version
#define IRRLICHT_VERSION_MAJOR 1
#define IRRLICHT_VERSION_MINOR 9
#define IRRLICHT_VERSION_REVISION 0

// This flag will be defined only in SVN, the official release code will have
// it undefined
#define IRRLICHT_VERSION_SVN alpha
#define IRRLICHT_SDK_VERSION "1.9.0"

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(_XBOX)
	#define _IRR_WINDOWS_API_
#endif

#ifdef  _IRR_WINDOWS_API_

	// To build Irrlicht as a static library, you must define IRRLICHT_STATIC in both the
	// Irrlicht build, *and* in the user application, before #including <irrlicht.h>
	#ifndef IRRLICHT_STATIC
		#ifdef IRRLICHT_EXPORTS
			#define IRRLICHT_API __declspec(dllexport)
		#else
			#define IRRLICHT_API __declspec(dllimport)
		#endif // IRRLICHT_EXPORT
	#else
		#define IRRLICHT_API
	#endif // IRRLICHT_STATIC

	// Declare the calling convention.
	#if defined(_STDCALL_SUPPORTED)
		#define IRRCALLCONV __stdcall
	#else
		#define IRRCALLCONV __cdecl
	#endif // STDCALL_SUPPORTED

#else // _IRR_WINDOWS_API_

	// Force symbol export in shared libraries built with gcc.
	#if defined(IRRLICHT_EXPORTS) && !defined(IRRLICHT_STATIC)
		#define IRRLICHT_API __attribute__ ((visibility("default")))
	#else
		#define IRRLICHT_API
	#endif
	#define IRRCALLCONV

#endif // not _IRR_WINDOWS_API_

//! define a break macro for debugging.
#if defined(_DEBUG)
	#if defined(_IRR_WINDOWS_API_) && defined(_MSC_VER) && !defined (_WIN32_WCE)
		#if defined(WIN64) || defined(_WIN64) // using portable common solution for x64 configuration
			#include <crtdbg.h>
			#define _IRR_DEBUG_BREAK_IF( _CONDITION_ ) if (_CONDITION_) {_CrtDbgBreak();}
		#else
			#define _IRR_DEBUG_BREAK_IF( _CONDITION_ ) if (_CONDITION_) {_asm int 3}
		#endif
	#else
		#include "assert.h"
		#define _IRR_DEBUG_BREAK_IF( _CONDITION_ ) assert( !(_CONDITION_) );
	#endif
#else
	#define _IRR_DEBUG_BREAK_IF( _CONDITION_ )
#endif

//! Defines a deprecated macro which generates a warning at compile time
/** The usage is simple
For typedef:		typedef _IRR_DEPRECATED_ int test1;
For classes/structs:	class _IRR_DEPRECATED_ test2 { ... };
For methods:		class test3 { _IRR_DEPRECATED_ virtual void foo() {} };
For functions:		template<class T> _IRR_DEPRECATED_ void test4(void) {}
**/
#if defined(IGNORE_DEPRECATED_WARNING)
	#define _IRR_DEPRECATED_
#elif _MSC_VER >= 1310 //vs 2003 or higher
	#define _IRR_DEPRECATED_ __declspec(deprecated)
#elif (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)) // all versions above 3.0 should support this feature
	#define _IRR_DEPRECATED_  __attribute__ ((deprecated))
#else
	#define _IRR_DEPRECATED_
#endif

//! Defines an override macro, to protect virtual functions from typos and other mismatches
/** Usage in a derived class:
virtual void somefunc() _IRR_OVERRIDE_;
*/
#define _IRR_OVERRIDE_ override

// memory debugging
#if defined(_DEBUG) && defined(IRRLICHT_EXPORTS) && defined(_MSC_VER) && \
	(_MSC_VER > 1299) && !defined(_IRR_DONT_DO_MEMORY_DEBUGGING_HERE) && !defined(_WIN32_WCE)

	#define CRTDBG_MAP_ALLOC
	#define _CRTDBG_MAP_ALLOC
	#define DEBUG_CLIENTBLOCK new( _CLIENT_BLOCK, __FILE__, __LINE__)
	#include <stdlib.h>
	#include <crtdbg.h>
	#define new DEBUG_CLIENTBLOCK
#endif


//! creates four CC codes used in Irrlicht for simple ids
/** some compilers can create those by directly writing the
code like 'code', but some generate warnings so we use this macro here */
#define MAKE_IRR_ID(c0, c1, c2, c3) \
		((irr::u32)(irr::u8)(c0) | ((irr::u32)(irr::u8)(c1) << 8) | \
		((irr::u32)(irr::u8)(c2) << 16) | ((irr::u32)(irr::u8)(c3) << 24 ))
