
#ifndef Compiler_h
#define Compiler_h

/* http://gcc.gnu.org/wiki/Visibility */
#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllexport))
    #else
      #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllimport))
    #else
      #define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DLL_PUBLIC
    #define DLL_LOCAL
  #endif
#endif

// TODO: Other compilers like MSVC are not considered yet.

#ifndef __GNUC_PREREQ
  #if defined(__GNUC__) && defined(__GNUC_MINOR__)
    #define __GNUC_PREREQ(maj, min) \
      ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
  #else
    #define __GNUC_PREREQ(maj, min) 0
  #endif
#endif

// std::atomic
#if __GNUC_PREREQ(4, 5)
  #include <atomic>
#else
  #include <cstdatomic>
#endif

// nullptr
#if __GNUC_PREREQ(4, 6)
  #define HAVE_NULLPTR 1
#endif

#ifndef HAVE_NULLPTR
  #ifndef __cplusplus
    #define nullptr ((void*)0)
  #elif defined(__GNUC__)
    #define nullptr __null
  #elif defined(_WIN64)
    #define nullptr 0LL
  #else
    #define nullptr 0L
  #endif
#endif /* defined(HAVE_NULLPTR) */

#endif // Compiler_h
