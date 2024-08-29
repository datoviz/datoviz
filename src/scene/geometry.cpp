/*************************************************************************************************/
/*  Geometry                                                                                     */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <array>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <earcut.hpp>

#include "_cglm.h"
#include "_log.h"
#include "_macros.h"
#include "datoviz.h"



/*************************************************************************************************/
/*  Earcut                                                                                       */
/*************************************************************************************************/

// NOTE: the caller must FREE the output.
DvzIndex* dvz_earcut(uint32_t point_count, const dvec2* polygon, uint32_t* out_index_count)
{
    // Prepare the data.
    std::vector<std::vector<std::array<double, 2>>> polygon_v;
    polygon_v.push_back({});
    for (uint32_t i = 0; i < point_count; i++)
        polygon_v[0].push_back({{polygon[i][0], polygon[i][1]}});

    // Run earcut.
    log_debug("running earcut polygon triangulation on %d points", point_count);
    std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(polygon_v);
    ASSERT(indices.size() > 0);
    log_debug("earcut found %d indices", indices.size());

    // Return the data as a C array.
    size_t size = indices.size() * sizeof(DvzIndex);
    DvzIndex* out = (DvzIndex*)malloc(size);
    memcpy(out, indices.data(), size);
    *out_index_count = indices.size();
    return out;
}
