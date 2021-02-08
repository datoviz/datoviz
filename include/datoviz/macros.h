#ifndef DVZ_MACROS_HEADER
#define DVZ_MACROS_HEADER

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

#if GCC

#define BEGIN_INCL_NO_WARN                                                                        \
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

#define END_INCL_NO_WARN _Pragma("GCC diagnostic pop")

#elif CLANG

#define BEGIN_INCL_NO_WARN                                                                        \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wsign-conversion\"")    \
        _Pragma("clang diagnostic ignored \"-Wcast-qual\"")                                       \
            _Pragma("clang diagnostic ignored \"-Wredundant-decls\"")                             \
                _Pragma("clang diagnostic ignored \"-Wcast-qual\"")                               \
                    _Pragma("clang diagnostic ignored \"-Wstrict-overflow\"")                     \
                        _Pragma("clang diagnostic ignored \"-Wswitch-default\"")                  \
                            _Pragma("clang diagnostic ignored \"-Wcast-align\"")                  \
                                _Pragma("clang diagnostic ignored \"-Wundef\"")                   \
                                    _Pragma("clang diagnostic ignored \"-Wmissing-braces\"")

#define END_INCL_NO_WARN _Pragma("clang diagnostic pop")

#else

#define BEGIN_INCL_NO_WARN
#define END_INCL_NO_WARN

#endif
#endif
