/*************************************************************************************************/
/*  Collection of builtin graphics pipelines                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GRAPHICS
#define DVZ_HEADER_GRAPHICS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_enums.h"
#include "_macros.h"
#include "array.h"
// #include "fileio.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

// Number of common bindings
// NOTE: must correspond to the same constant in common.glsl
#define DVZ_USER_BINDING 2

#define DVZ_MAX_GLYPHS_PER_TEXT 256



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzVertex DvzVertex;
typedef struct DvzMVP DvzMVP;
typedef struct DvzViewport DvzViewport;

// typedef struct DvzGraphicsPointParams DvzGraphicsPointParams;

typedef struct DvzGraphicsPointVertex DvzGraphicsPointVertex;

typedef struct DvzGraphicsRasterVertex DvzGraphicsRasterVertex;
typedef struct DvzGraphicsRasterParams DvzGraphicsRasterParams;

typedef struct DvzGraphicsMarkerVertex DvzGraphicsMarkerVertex;
typedef struct DvzGraphicsMarkerParams DvzGraphicsMarkerParams;

typedef struct DvzGraphicsSegmentVertex DvzGraphicsSegmentVertex;

typedef struct DvzGraphicsPathVertex DvzGraphicsPathVertex;
typedef struct DvzGraphicsPathParams DvzGraphicsPathParams;

typedef struct DvzGraphicsImageVertex DvzGraphicsImageVertex;
typedef struct DvzGraphicsImageParams DvzGraphicsImageParams;
typedef struct DvzGraphicsImageCmapParams DvzGraphicsImageCmapParams;

typedef struct DvzGraphicsVolumeSliceVertex DvzGraphicsVolumeSliceVertex;
typedef struct DvzGraphicsVolumeSliceParams DvzGraphicsVolumeSliceParams;

typedef struct DvzGraphicsVolumeVertex DvzGraphicsVolumeVertex;
typedef struct DvzGraphicsVolumeParams DvzGraphicsVolumeParams;

typedef struct DvzGraphicsMeshVertex DvzGraphicsMeshVertex;
typedef struct DvzGraphicsMeshParams DvzGraphicsMeshParams;

typedef struct DvzGraphicsTextParams DvzGraphicsTextParams;
typedef struct DvzGraphicsTextVertex DvzGraphicsTextVertex;


// Forward declarations.
typedef struct DvzRenderpass DvzRenderpass;
typedef struct DvzGraphics DvzGraphics;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
};



struct DvzMVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
};



// NOTE: must correspond to the shader structure in common.glsl
struct DvzViewport
{
    VkViewport viewport; // Vulkan viewport
    vec4 margins;

    // Position and size of the viewport in screen coordinates.
    uvec2 offset_screen;
    uvec2 size_screen;

    // Position and size of the viewport in framebuffer coordinates.
    uvec2 offset_framebuffer;
    uvec2 size_framebuffer;

    // Options
    // Viewport clipping.
    DvzViewportClip clip; // used by the GPU for viewport clipping

    // Used to discard transform on one axis
    int32_t interact_axis;

    // TODO: aspect ratio
};



/*************************************************************************************************/
/*  Graphics point                                                                               */
/*************************************************************************************************/

struct DvzGraphicsPointVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
    float size;  /* marker size, in pixels */
};



/*************************************************************************************************/
/*  Graphics raster                                                                              */
/*************************************************************************************************/

struct DvzGraphicsRasterVertex
{
    vec2 pos;         /* position */
    uint8_t depth;    /* depth */
    uint8_t cmap_val; /* colormap value */
    uint8_t alpha;    /* rescaled alpha value (255 = alpha_max) */
    uint8_t size;     /* rescale marker size (255 = size_max) */
};

struct DvzGraphicsRasterParams
{
    vec2 alpha_range; /* alpha range for rescaling of vertex.alpha */
    vec2 size_range;  /* marker size (px) range for rescaling of vertex.size */
    uint32_t cmap_id; /* id of the colormap */
};



/*************************************************************************************************/
/*  Graphics marker                                                                              */
/*************************************************************************************************/

struct DvzGraphicsMarkerVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
    float size;  /* marker size, in pixels */
    // in fact a DvzMarkerType but we should control the exact data type for the GPU
    uint8_t marker;    /* marker type enum */
    uint8_t angle;     /* angle, between 0 (0) included and 256 (M_2PI) excluded */
    uint8_t transform; /* transform enum */
};

struct DvzGraphicsMarkerParams
{
    vec4 edge_color;  /* edge color RGBA */
    float edge_width; /* line width, in pixels */
};



/*************************************************************************************************/
/*  Graphics segment                                                                             */
/*************************************************************************************************/

struct DvzGraphicsSegmentVertex
{
    vec3 P0;           /* start position */
    vec3 P1;           /* end position */
    vec4 shift;        /* shift of start (xy) and end (zw) positions, in pixels */
    cvec4 color;       /* color */
    float linewidth;   /* line width, in pixels */
    DvzCapType cap0;   /* start cap enum */
    DvzCapType cap1;   /* end cap enum */
    uint8_t transform; /* transform enum */
};



/*************************************************************************************************/
/*  Graphics segment                                                                             */
/*************************************************************************************************/

struct DvzGraphicsPathVertex
{
    vec3 p0;     /* previous position */
    vec3 p1;     /* current position */
    vec3 p2;     /* next position */
    vec3 p3;     /* next next position */
    cvec4 color; /* point color */
};

struct DvzGraphicsPathParams
{
    float linewidth;    /* line width in pixels */
    float miter_limit;  /* miter limit for joins */
    int32_t cap_type;   /* type of the ends of the path */
    int32_t round_join; /* whether to use round joins */
};



/*************************************************************************************************/
/*  Graphics text                                                                                */
/*************************************************************************************************/

struct DvzGraphicsTextVertex
{
    vec3 pos;          /* position */
    vec2 shift;        /* shift, in pixels */
    cvec4 color;       /* color */
    vec2 glyph_size;   /* glyph size, in pixels */
    vec2 anchor;       /* character anchor, in normalized coordinates */
    float angle;       /* string angle */
    usvec4 glyph;      /* glyph: char code, char index, string length, string index */
    uint8_t transform; /* transform enum */
};

struct DvzGraphicsTextParams
{
    ivec2 grid_size; /* font atlas grid size (rows, columns) */
    ivec2 tex_size;  /* font atlas texture size, in pixels */
};



/*************************************************************************************************/
/*  Graphics image                                                                               */
/*************************************************************************************************/

struct DvzGraphicsImageVertex
{
    vec3 pos; /* position */
    vec2 uv;  /* tex coordinates */
};

struct DvzGraphicsImageParams
{
    vec4 tex_coefs; /* blending coefficients for the four images */
};

struct DvzGraphicsImageCmapParams
{
    vec2 vrange; /* value range */
    int cmap;    /* colormap number */
};



/*************************************************************************************************/
/*  Graphics volume slice                                                                        */
/*************************************************************************************************/

struct DvzGraphicsVolumeSliceVertex
{
    vec3 pos; /* position */
    vec3 uvw; /* 3D tex coords */
};

struct DvzGraphicsVolumeSliceParams
{
    vec4 x_cmap; /* x values of the color transfer function */
    vec4 y_cmap; /* y values of the color transfer function */

    vec4 x_alpha; /* x values of the alpha transfer function */
    vec4 y_alpha; /* y values of the alpha transfer function */

    int32_t cmap; /* colormap */
    float scale;  /* scaling factor for the fetched volume values */
};



/*************************************************************************************************/
/*  Graphics volume                                                                              */
/*************************************************************************************************/

struct DvzGraphicsVolumeVertex
{
    vec3 pos; /* position */
};

struct DvzGraphicsVolumeParams
{
    vec4 box_size;        /* size of the box containing the volume, in NDC */
    vec4 uvw0;            /* texture coordinates of the 2 corner points */
    vec4 uvw1;            /* texture coordinates of the 2 corner points */
    vec4 clip;            /* plane normal vector for volume slicing */
    vec2 transfer_xrange; /* x coords of the endpoints of the transfer function */
    float color_coef;     /* scaling coefficient when fetching voxel color */
    // int32_t cmap;         /* colormap */
};



/*************************************************************************************************/
/*  Graphics mesh                                                                                */
/*************************************************************************************************/

struct DvzGraphicsMeshVertex
{
    // HACK: use vec4 for alignment when accessing from compute shader (need std140 on GPU)
    vec4 pos;      /* position */
    vec4 normal;   /* normal vector */
    vec2 uv;       /* tex coords */
    uint8_t alpha; /* transparency value */
};

struct DvzGraphicsMeshParams
{
    mat4 lights_pos_0;    /* positions of each of the maximum four lights */
    mat4 lights_params_0; /* ambient, diffuse, specular coefs for each light */
    vec4 tex_coefs;       /* blending coefficients for the four textures */
    vec4 clip_coefs;      /* clip coefficients */
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a new graphics pipeline of a given builtin type.
 *
 * @param renderpass the renderpass
 * @param graphics the graphics to create
 * @param type the graphics type
 * @param flags the creation flags for the graphics
 */
DVZ_EXPORT void dvz_graphics_builtin(
    DvzRenderpass* renderpass, DvzGraphics* graphics, DvzGraphicsType type, int flags);



/**
 * Return a default DvzMVP struct
 *
 * @returns the DvzMVP struct
 */
DVZ_EXPORT DvzMVP dvz_mvp_default(void);



/**
 * Return a default viewport
 *
 * @param width the viewport width, in framebuffer pixels
 * @param height the viewport height, in framebuffer pixels
 * @returns the viewport
 */
DVZ_EXPORT DvzViewport dvz_viewport_default(uint32_t width, uint32_t height);



EXTERN_C_OFF

#endif
