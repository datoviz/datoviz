#ifndef VKY_RAW_HEADER
#define VKY_RAW_HEADER

#include "constants.h"
#include "scene.h"



/*************************************************************************************************/
/*  Multi raw path visual                                                                        */
/*************************************************************************************************/

typedef struct VkyMultiRawPathParams VkyMultiRawPathParams;
struct VkyMultiRawPathParams
{
    vec4 info;                                  // path_count, vertex_count_per_path, scaling
    vec4 y_offsets[VKY_RAW_PATH_MAX_PATHS / 4]; // NOTE: 16 bytes alignment enforced
    vec4 colors[VKY_RAW_PATH_MAX_PATHS];        // 16 bytes per path
};


VKY_EXPORT VkyVisual* vky_visual_path_raw_multi(VkyScene* scene, VkyMultiRawPathParams params);



/*************************************************************************************************/
/*  Image visual                                                                                 */
/*************************************************************************************************/
typedef struct VkyImageData VkyImageData;
struct VkyImageData
{
    vec3 p0, p1;
    vec2 uv0, uv1;
};

VKY_EXPORT VkyVisual* vky_visual_image(VkyScene* scene, VkyTextureParams params);

VKY_EXPORT void vky_visual_image_upload(VkyVisual*, const void*);


/*************************************************************************************************/
/*  Raw marker                                                                                   */
/*************************************************************************************************/

typedef enum
{
    VKY_SCALING_OFF,
    VKY_SCALING_ON
} VkyScalingMode;

typedef enum
{
    VKY_ALPHA_SCALING_OFF,
    VKY_ALPHA_SCALING_ON
} VkyAlphaScalingMode;

typedef struct VkyMarkersRawParams VkyMarkersRawParams;
struct VkyMarkersRawParams
{
    vec2 marker_size;
    int32_t scaling_mode;
    int32_t alpha_scaling_mode;
};

VKY_EXPORT VkyVisual* vky_visual_marker_raw(VkyScene* scene, const VkyMarkersRawParams* params);



/*************************************************************************************************/
/*  Raw path                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_visual_path_raw(VkyScene* scene);



#endif
