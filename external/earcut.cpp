#include <array>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <datoviz/common.h>

#include <earcut.hpp>



/*************************************************************************************************/
/*  Earcut polygon triangulation                                                                 */
/*************************************************************************************************/

// C wrapper for the ear-clip algorithm implementation in earcut.hpp
void dvz_triangulate_polygon(
    uint32_t point_count, const dvec3* polygon, uint32_t* index_count, uint32_t** out_indices)
{
    std::vector<std::vector<std::array<double, 3>>> polygon_v;
    polygon_v.push_back({});
    for (uint32_t i = 0; i < point_count; i++)
        polygon_v[0].push_back({{polygon[i][0], polygon[i][1], polygon[i][2]}});
    std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(polygon_v);
    ASSERT(indices.size() > 0);
    size_t size = indices.size() * sizeof(uint32_t);
    uint32_t* out = (uint32_t*)malloc(size);
    memcpy(out, indices.data(), size);
    *index_count = indices.size();
    *out_indices = out;
}
