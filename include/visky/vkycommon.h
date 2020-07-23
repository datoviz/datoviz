// #ifndef VKY_CONSTANTS_HEADER
// #define VKY_CONSTANTS_HEADER

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Export macros                                                                                */
/*************************************************************************************************/

#if MSVC
#ifdef VKY_SHARED
#define VKY_EXPORT __declspec(dllexport)
#else
#define VKY_EXPORT __declspec(dllimport)
#endif
#define VKY_INLINE __forceinline
#else
#define VKY_EXPORT __attribute__((visibility("default")))
#define VKY_INLINE static inline __attribute((always_inline))
#endif



#ifdef __cplusplus
}
#endif

// #endif
