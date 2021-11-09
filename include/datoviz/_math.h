/*************************************************************************************************/
/*  Common mathematical macros                                                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MATH
#define DVZ_HEADER_MATH



/*************************************************************************************************/
/*  Standard includes                                                                            */
/*************************************************************************************************/

#include <inttypes.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_2PI 6.28318530717958647692



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define MIN(a, b)     (((a) < (b)) ? (a) : (b))
#define MAX(a, b)     (((a) > (b)) ? (a) : (b))
#define CLIP(x, a, b) MAX(MIN((x), (b)), (a))



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// Smallest power of 2 larger or equal than a positive integer.
static inline uint64_t dvz_next_pow2(uint64_t x)
{
    uint64_t p = 1;
    while (p < x)
        p *= 2;
    return p;
}



/*************************************************************************************************/
/*  Random number generation                                                                     */
/*************************************************************************************************/

/**
 * Return a random integer number between 0 and 255.
 *
 * @returns random number
 */
static inline uint8_t dvz_rand_byte() { return (uint8_t)(rand() % 256); }



/**
 * Return a random integer number.
 *
 * @returns random number
 */
static inline int dvz_rand_int() { return rand(); }



/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
static inline float dvz_rand_float() { return (float)rand() / (float)(RAND_MAX); }


/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
static inline float dvz_rand_double() { return (double)rand() / (double)(RAND_MAX); }



/**
 * Return a random normal floating-point number.
 *
 * @returns random number
 */
static inline float dvz_rand_normal()
{
    return sqrt(-2.0 * log(dvz_rand_float())) * cos(2 * M_PI * dvz_rand_float());
}



#endif
