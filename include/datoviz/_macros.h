/*************************************************************************************************/
/*  Common macros                                                                                */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MACROS
#define DVZ_HEADER_MACROS



/*************************************************************************************************/
/*  Symbol macros                                                                                */
/*************************************************************************************************/

#if MSVC
#ifdef DVZ_SHARED
#define DVZ_EXPORT __declspec(dllexport)
#else
#define DVZ_EXPORT __declspec(dllimport)
#endif
#define DVZ_INLINE __forceinline
#else
#define DVZ_EXPORT __attribute__((visibility("default")))
#define DVZ_INLINE static inline __attribute((always_inline))
#endif


#ifndef __STDC_VERSION__
#define __STDC_VERSION__ 0
#endif



/*************************************************************************************************/
/*  Mute macros                                                                                  */
/*************************************************************************************************/

#if GCC
#define MUTE_ON                                                                                   \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")        \
        _Pragma("GCC diagnostic ignored \"-Wundef\"")                                             \
            _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")                                     \
                _Pragma("GCC diagnostic ignored \"-Wredundant-decls\"")                           \
                    _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")                             \
                        _Pragma("GCC diagnostic ignored \"-Wunused\"")                            \
                            _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")              \
                                _Pragma("GCC diagnostic ignored \"-Wstrict-overflow\"")           \
                                    _Pragma("GCC diagnostic ignored \"-Wswitch-default\"")        \
                                        _Pragma("GCC diagnostic ignored \"-Wmissing-braces\"")

#define MUTE_OFF _Pragma("GCC diagnostic pop")
#elif CLANG
#define MUTE_ON                                                                                   \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wsign-conversion\"")    \
        _Pragma("clang diagnostic ignored \"-Wcast-qual\"")                                       \
            _Pragma("clang diagnostic ignored \"-Wredundant-decls\"")                             \
                _Pragma("clang diagnostic ignored \"-Wcast-qual\"")                               \
                    _Pragma("clang diagnostic ignored \"-Wstrict-overflow\"")                     \
                        _Pragma("clang diagnostic ignored \"-Wswitch-default\"")                  \
                            _Pragma("clang diagnostic ignored \"-Wcast-align\"") _Pragma(         \
                                "clang diagnostic ignored \"-Wundef\"")                           \
                                _Pragma("clang diagnostic ignored \"-Wmissing-braces\"") _Pragma( \
                                    "clang diagnostic ignored \"-Wnullability-extension\"")

#define MUTE_OFF _Pragma("clang diagnostic pop")
#else
#define MUTE_ON
#define MUTE_OFF



/*************************************************************************************************/
/*  Atomic macros                                                                                */
/*************************************************************************************************/

// Atomic macro, for both C++ and C
#ifndef __cplusplus
#include <stdatomic.h>
#define atomic(t, x) _Atomic t x
#else
#include <atomic>
#define atomic(t, x) std::atomic<t> x
#endif



/*************************************************************************************************/
/*  Memory management                                                                            */
/*************************************************************************************************/

#define FREE(x)                                                                                   \
    if ((x) != NULL)                                                                              \
    {                                                                                             \
        free((x));                                                                                \
        (x) = NULL;                                                                               \
    }

#define ALIGNED_FREE(x)                                                                           \
    if (x.aligned)                                                                                \
        aligned_free(x.pointer);                                                                  \
    else                                                                                          \
        FREE(x.pointer)

#define REALLOC(x, s)                                                                             \
    {                                                                                             \
        void* _new = realloc((x), (s));                                                           \
        if (_new == NULL)                                                                         \
            log_error("error reallocating %s to %d bytes", #x, (s));                              \
        else                                                                                      \
            x = _new;                                                                             \
    }



/*************************************************************************************************/
/*  Misc                                                                                         */
/*************************************************************************************************/

#define ARRAY_COUNT(arr) sizeof((arr)) / sizeof((arr)[0])
#define ASSERT(x)        assert((x))



#ifdef __cplusplus
extern "C" {
#endif

#endif
#endif
