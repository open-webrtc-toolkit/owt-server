/* <COPYRIGHT_TAG> */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_


/** OS **/
#if defined(__WINDOWS__) || defined(_WIN32) || defined(WIN32) || \
    defined(_WIN64) || defined(WIN64) || \
    defined(__WIN32__) || defined(__TOS_WIN__)
#define PLATFORM_OS_NAME "Windows"
#define PLATFORM_OS_WINDOWS
#elif defined(__linux__) || defined(linux) || defined(__linux) || \
    defined(__LINUX__) || defined(LINUX) || defined(_LINUX)
#define PLATFORM_OS_NAME "Linux"
#define PLATFORM_OS_LINUX
#else
#define PLATFORM_OS_NAME "Unknown"
#error Unknown OS
#endif


/** Bits **/
#if defined(_WIN64) || defined(WIN64) || defined(__amd64__) || \
    defined(__amd64) || defined(__LP64__) || defined(_LP64) || \
    defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || \
    defined(__ia64__) || defined(_IA64) || defined(__IA64__) || \
    defined(__ia64) || defined(_M_IA64)
#define PLATFORM_BITS_NAME "64"
#define PLATFORM_BITS_64
#elif defined(_WIN32) || defined(WIN32) || defined(__32BIT__) || \
    defined(__ILP32__) || defined(_ILP32) || defined(i386) || \
    defined(__i386__) || defined(__i486__) || defined(__i586__) || \
    defined(__i686__) || defined(__i386) || defined(_M_IX86) || \
    defined(__X86__) || defined(_X86_) || defined(__I86__)
#define PLATFORM_BITS_NAME "32"
#define PLATFORM_BITS_32
#else
#define PLATFORM_BITS_NAME "Unknown"
#error Unknown Bits
#endif


/** Compiler **/
#if defined(_MSC_VER)
#define PLATFORM_CC_NAME "VC"
#define PLATFORM_CC_VC
#elif defined(__GNUG__) || defined(__GNUC__)
#define PLATFORM_CC_NAME "GCC"
#define PLATFORM_CC_GCC
#else
#define PLATFORM_CC_NAME "Unknown"
#error Unknown Compiler
#endif


/*Module API definitions*/
#if defined(PLATFORM_OS_WINDOWS)
#define DLL_API extern "C" __declspec(dllexport)
#else
#define DLL_API extern "C"
#endif


/*String*/
#define PLATFORM_STR "OS: " PLATFORM_OS_NAME \
                     ", Bits: " PLATFORM_BITS_NAME \
                     ", Compiler: " PLATFORM_CC_NAME


#endif

