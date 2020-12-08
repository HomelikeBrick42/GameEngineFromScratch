#pragma once

#include "brickpch.hpp"

#define STRINGIFICATE_(x) #x
#define STRINGIFICATE(x) STRINGIFICATE_(x)
#define LINE_STRING STRINGIFICATE(__LINE__)

#define CONCAT(x, y) x ## y

#if defined(BRICKENGINE_PLATFORM_WINDOWS)
	#define BRICKENGINE_FORCE_INLINE __forceinline
	#define BRICKENGINE_FORCE_NO_INLINE _declspec(noinline)

	#if defined(BRICKENGINE_DLL)
		#if defined(BRICKENGINE_BUILD_DLL)
			#define BRICKENGINE_API _declspec(dllexport)
		#else
			#define BRICKENGINE_API _declspec(dllimport)
		#endif
	#else
		#define BRICKENGINE_API
	#endif
#else
	#define FORCEINLINE
	#define FORCENOINLINE
	#define BRICKENGINE_API
#endif

#define BRICKENGINE_ENBALE_ASSERTS
#if defined(BRICKENGINE_ENBALE_ASSERTS)
	#if _MSC_VER
		#include <intrin.h>
		#define BRICKENGINE_DEBUG_BREAK() __debugbreak()
	#else
		#define BRICKENGINE_DEBUG_BREAK()
	#endif

	#define BRICKENGINE_ASSERT(x) {\
		if (!(x)) { \
			::BrickEngine::Log::Fatal("Assertion Failure : '" #x "' in file: " __FILE__ ":" LINE_STRING); \
			BRICKENGINE_DEBUG_BREAK(); \
		} \
	}
#else
	#define ASSERT(x)
#endif
