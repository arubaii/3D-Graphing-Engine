#pragma once

#if defined(__APPLE__) && defined(__MACH__)
	#define PLATFORM_MACOS 1
#else
	#define PLATFORM_MACOS 0
#endif