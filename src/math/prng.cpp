/*************************************************************************************************/
/*  Random 64-bit integer                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <random>

#include "_assertions.h"
#include "_log.h"
#include "_mutex.h"
#include "datoviz/math/prng.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

extern "C" struct DvzPrng
{
    std::mt19937_64 prng;
    std::uniform_int_distribution<uint64_t> dis;
    DvzMutex mutex;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzPrng* dvz_prng(void)
{
    log_trace("create prng");
    DvzPrng* prng = new DvzPrng();
    auto seed = std::random_device{}();
    prng->prng.seed(seed);
    prng->mutex = dvz_mutex();
    return prng;
}



uint64_t dvz_prng_uuid(DvzPrng* prng)
{
    ANN(prng);
    dvz_mutex_lock(&prng->mutex);
    auto out = prng->dis(prng->prng);
    dvz_mutex_unlock(&prng->mutex);
    return out;
}



void dvz_prng_destroy(DvzPrng* prng)
{
    ANN(prng);
    log_trace("delete prng");
    dvz_mutex_destroy(&prng->mutex);
    delete prng;
}
