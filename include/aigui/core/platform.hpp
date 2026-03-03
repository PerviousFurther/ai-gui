#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define AIGUI_PLATFORM_WINDOWS 1
    #define AIGUI_RENDERER_DX12    1
#elif defined(__linux__)
    #define AIGUI_PLATFORM_LINUX   1
    #define AIGUI_RENDERER_VULKAN  1
#elif defined(__APPLE__)
    #define AIGUI_PLATFORM_MACOS   1
    // macOS backend can be extended later
#else
    #error "Unsupported platform"
#endif

// Compiler helpers
#if defined(_MSC_VER)
    #define AIGUI_INLINE __forceinline
    #define AIGUI_EXPORT __declspec(dllexport)
    #define AIGUI_IMPORT __declspec(dllimport)
#else
    #define AIGUI_INLINE __attribute__((always_inline)) inline
    #define AIGUI_EXPORT __attribute__((visibility("default")))
    #define AIGUI_IMPORT
#endif

#ifdef AIGUI_BUILD_DLL
    #define AIGUI_API AIGUI_EXPORT
#else
    #define AIGUI_API AIGUI_IMPORT
#endif

// C++ standard check
#if __cplusplus < 201703L
    #error "ai-gui requires C++17 or later"
#endif
