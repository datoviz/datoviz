#ifndef VKY_AXES_3D_HEADER
#define VKY_AXES_3D_HEADER

#include "../include/visky/visky.h"



/*************************************************************************************************/
/*  Axes 3D visual                                                                               */
/*************************************************************************************************/

typedef struct VkyAxes3DVertex VkyAxes3DVertex;
struct VkyAxes3DVertex
{
    float tick;
    VkyColor color;
    float linewidth;
    int32_t cap0;
    int32_t cap1;
    uint8_t coord_side; // H: 0x,1y,2z,(3). V: 4x, 5y, 6z, (7)
};

// VkyVisual* vky_visual_axes_3D(VkyScene* scene);



/*************************************************************************************************/
/*  Axes 3D text visual                                                                          */
/*************************************************************************************************/

typedef struct VkyAxes3DTextParams VkyAxes3DTextParams;
struct VkyAxes3DTextParams
{
    ivec2 grid_size;
    ivec2 tex_size;
    vec2 glyph_size;
    vec4 color;
};

typedef struct VkyAxes3DTextData VkyAxes3DTextData;
struct VkyAxes3DTextData
{
    float tick;
    uint8_t coord_side; // H: 0x,1y,2z,(3). V: 4x, 5y, 6z, (7)
    uint32_t string_len;
    char string[16];
};

typedef struct VkyAxes3DTextVertex VkyAxes3DTextVertex;
struct VkyAxes3DTextVertex
{
    float tick;
    uint8_t coord_side; // H: 0x,1y,2z,(3). V: 4x, 5y, 6z, (7)
    usvec4 glyph;       // char, char_index, str_len, str_index
};

// VkyVisual* vky_visual_axes_3D_text(VkyScene* scene);



/*************************************************************************************************/
/*  Axes 3D                                                                                      */
/*************************************************************************************************/

VkyVisual* vky_visual_axes_3D(VkyScene* scene);

void vky_axes_3D_init(VkyPanel* panel);



#endif
