#pragma once

// Standard base includes, defines that indicate our current platform, etc.

#include <stddef.h>

// Which platform we are on?
// UNITY_WIN - Windows (regular win32)
// UNITY_OSX - Mac OS X
// UNITY_LINUX - Linux
// UNITY_IOS - iOS
// UNITY_TVOS - tvOS
// UNITY_ANDROID - Android
// UNITY_METRO - WSA or UWP
// UNITY_WEBGL - WebGL
// UNITY_EMBEDDED_LINUX - EmbeddedLinux OpenGLES
// UNITY_EMBEDDED_LINUX_GL - EmbeddedLinux OpenGLCore
#if _MSC_VER
#define UNITY_WIN 1
#elif defined(__APPLE__)
#if TARGET_OS_TV
#define UNITY_TVOS 1
#elif TARGET_OS_IOS
#define UNITY_IOS 1
#else
#define UNITY_OSX 1
#endif
#elif defined(__ANDROID__)
#define UNITY_ANDROID 1
#elif defined(UNITY_METRO) || defined(UNITY_LINUX) || defined(UNITY_WEBGL) || \
    defined(UNITY_EMBEDDED_LINUX) || defined(UNITY_EMBEDDED_LINUX_GL) ||      \
    defined(UNITY_QNX)
// these are defined externally
#elif defined(__EMSCRIPTEN__)
// this is already defined in Unity 5.6
#define UNITY_WEBGL 1
#else
#error "Unknown platform!"
#endif
