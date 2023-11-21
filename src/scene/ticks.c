/*************************************************************************************************/
/*  Ticks                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/ticks.h"
#include "_macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Ticks functions                                                                              */
/*************************************************************************************************/

DvzTicks* dvz_ticks(int flags)
{
    //
    return NULL;
}



// range_size is in the same unit as glyph size, it's the size between dmin
// and dmax (below)
void dvz_ticks_size(DvzTicks* ticks, double range_size, double glyph_size)
{
    ANN(ticks);
    //
}



// return if update needed?
bool dvz_ticks_compute(DvzTicks* ticks, double dmin, double dmax)
{
    ANN(ticks);
    //
    return false;
}



double dvz_ticks_score(DvzTicks* ticks)
{
    ANN(ticks);
    //
    return 0;
}



// lmin, lmax, lstep, returns the number of ticks
uint32_t dvz_ticks_range(DvzTicks* ticks, dvec3 range)
{
    ANN(ticks);
    return 0;
}



DvzTicksFormat dvz_ticks_format(DvzTicks* ticks)
{
    // decimal or scientific notation
    ANN(ticks);
    return 0;
}


uint32_t dvz_ticks_precision(DvzTicks* ticks)
{
    // number of digits after the dot
    ANN(ticks);
    return 0;
}



// return true if the score with that range is significantly lower than the current score
bool dvz_ticks_dirty(DvzTicks* ticks, double dmin, double dmax)
{
    ANN(ticks);
    return false;
}



void dvz_ticks_destroy(DvzTicks* ticks)
{
    ANN(ticks);
    //
}
