/*************************************************************************************************/
/*  Collection of builtin graphics pipelines                                                     */
/*************************************************************************************************/

#ifndef VKL_GRAPHICS_HEADER
#define VKL_GRAPHICS_HEADER

#include "array.h"
#include "canvas.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

// Number of common bindings
// NOTE: must correspond to the same constant in common.glsl
#define VKL_USER_BINDING 2

#define VKL_MAX_GLYPHS_PER_TEXT 256



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Graphics flags.
typedef enum
{
    VKL_GRAPHICS_FLAGS_DEPTH_TEST_DISABLE = 0x0000,
    VKL_GRAPHICS_FLAGS_DEPTH_TEST_ENABLE = 0x0100,
} VklGraphicsFlags;



// Marker type.
// NOTE: the numbers need to correspond to markers.glsl at the bottom.
typedef enum
{
    VKL_MARKER_DISC = 0,
    VKL_MARKER_ASTERISK = 1,
    VKL_MARKER_CHEVRON = 2,
    VKL_MARKER_CLOVER = 3,
    VKL_MARKER_CLUB = 4,
    VKL_MARKER_CROSS = 5,
    VKL_MARKER_DIAMOND = 6,
    VKL_MARKER_ARROW = 7,
    VKL_MARKER_ELLIPSE = 8,
    VKL_MARKER_HBAR = 9,
    VKL_MARKER_HEART = 10,
    VKL_MARKER_INFINITY = 11,
    VKL_MARKER_PIN = 12,
    VKL_MARKER_RING = 13,
    VKL_MARKER_SPADE = 14,
    VKL_MARKER_SQUARE = 15,
    VKL_MARKER_TAG = 16,
    VKL_MARKER_TRIANGLE = 17,
    VKL_MARKER_VBAR = 18,
} VklMarkerType;



// Cap type.
typedef enum
{
    VKL_CAP_TYPE_NONE = 0,
    VKL_CAP_ROUND = 1,
    VKL_CAP_TRIANGLE_IN = 2,
    VKL_CAP_TRIANGLE_OUT = 3,
    VKL_CAP_SQUARE = 4,
    VKL_CAP_BUTT = 5,
    VKL_CAP_COUNT,
} VklCapType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVertex VklVertex;

typedef struct VklGraphicsPointParams VklGraphicsPointParams;

typedef struct VklGraphicsMarkerVertex VklGraphicsMarkerVertex;
typedef struct VklGraphicsMarkerParams VklGraphicsMarkerParams;

typedef struct VklGraphicsSegmentVertex VklGraphicsSegmentVertex;

typedef struct VklGraphicsImageVertex VklGraphicsImageVertex;
typedef struct VklGraphicsImageParams VklGraphicsImageParams;

typedef struct VklGraphicsVolumeSliceItem VklGraphicsVolumeSliceItem;
typedef struct VklGraphicsVolumeSliceVertex VklGraphicsVolumeSliceVertex;
typedef struct VklGraphicsVolumeSliceParams VklGraphicsVolumeSliceParams;

typedef struct VklGraphicsVolumeItem VklGraphicsVolumeItem;
typedef struct VklGraphicsVolumeVertex VklGraphicsVolumeVertex;
typedef struct VklGraphicsVolumeParams VklGraphicsVolumeParams;

typedef struct VklGraphicsMeshVertex VklGraphicsMeshVertex;
typedef struct VklGraphicsMeshParams VklGraphicsMeshParams;

typedef struct VklGraphicsTextParams VklGraphicsTextParams;
typedef struct VklGraphicsTextVertex VklGraphicsTextVertex;
typedef struct VklGraphicsTextItem VklGraphicsTextItem;

typedef struct VklGraphicsData VklGraphicsData;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
};



struct VklGraphicsData
{
    VklGraphics* graphics;
    VklArray* vertices;
    VklArray* indices;
    uint32_t item_count;
    uint32_t current_idx;
    uint32_t current_group;
    void* user_data;
};



/*************************************************************************************************/
/*  Graphics points                                                                              */
/*************************************************************************************************/

struct VklGraphicsPointParams
{
    float point_size; /* point size, in pixels */
};



/*************************************************************************************************/
/*  Graphics marker                                                                              */
/*************************************************************************************************/

struct VklGraphicsMarkerVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
    float size;  /* marker size, in pixels */
    // in fact a VklMarkerType but we should control the exact data type for the GPU
    uint8_t marker;    /* marker type enum */
    uint8_t angle;     /* angle, between 0 (0) included and 256 (M_2PI) excluded */
    uint8_t transform; /* transform enum */
};

struct VklGraphicsMarkerParams
{
    vec4 edge_color;  /* edge color RGBA */
    float edge_width; /* line width, in pixels */
};



/*************************************************************************************************/
/*  Graphics segment                                                                             */
/*************************************************************************************************/

struct VklGraphicsSegmentVertex
{
    vec3 P0;           /* start position */
    vec3 P1;           /* end position */
    vec4 shift;        /* shift of start (xy) and end (zw) positions, in pixels */
    cvec4 color;       /* color */
    float linewidth;   /* line width, in pixels */
    VklCapType cap0;   /* start cap enum */
    VklCapType cap1;   /* end cap enum */
    uint8_t transform; /* transform enum */
};



/*************************************************************************************************/
/*  Graphics text                                                                                */
/*************************************************************************************************/

struct VklGraphicsTextVertex
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

struct VklGraphicsTextItem
{
    VklGraphicsTextVertex vertex; /* text vertex */
    cvec4* glyph_colors;          /* glyph colors */
    float font_size;              /* font size */
    const char* string;           /* text string */
};

struct VklGraphicsTextParams
{
    ivec2 grid_size; /* font atlas grid size (rows, columns) */
    ivec2 tex_size;  /* font atlas texture size, in pixels */
};



/*************************************************************************************************/
/*  Graphics image                                                                               */
/*************************************************************************************************/

struct VklGraphicsImageVertex
{
    vec3 pos; /* position */
    vec2 uv;  /* tex coordinates */
};

struct VklGraphicsImageParams
{
    vec4 tex_coefs; /* blending coefficients for the four images */
};



/*************************************************************************************************/
/*  Graphics volume slice                                                                        */
/*************************************************************************************************/

struct VklGraphicsVolumeSliceItem
{
    vec3 pos0; /* top left corner */
    vec3 pos1; /* top right corner */
    vec3 pos2; /* bottom right corner */
    vec3 pos3; /* bottom left corner */

    vec3 uvw0; /* tex coords of the top left corner */
    vec3 uvw1; /* tex coords of the top right corner */
    vec3 uvw2; /* tex coords of the bottom right corner */
    vec3 uvw3; /* tex coords of the bottom left corner */
};

struct VklGraphicsVolumeSliceVertex
{
    vec3 pos; /* position */
    vec3 uvw; /* 3D tex coords */
};

struct VklGraphicsVolumeSliceParams
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

struct VklGraphicsVolumeItem
{
    // top left front, bottom right back
    vec3 pos0; /* top left front position */
    vec3 pos1; /* bottom right back position */
    vec3 uvw0; /* tex coordinates of the top left front corner */
    vec3 uvw1; /* tex coordinates of the bottom right back position */
};

struct VklGraphicsVolumeVertex
{
    vec3 pos; /* position */
    vec3 uvw; /* tex coords */
};

struct VklGraphicsVolumeParams
{
    vec4 view_pos; /* camera position */
    vec4 box_size; /* size of the box containing the volume, in NDC */
    int32_t cmap;  /* colormap */
};



/*************************************************************************************************/
/*  Graphics mesh                                                                                */
/*************************************************************************************************/

struct VklGraphicsMeshVertex
{
    vec3 pos;      /* position */
    vec3 normal;   /* normal vector */
    vec2 uv;       /* tex coords */
    uint8_t alpha; /* transparency value */
};

struct VklGraphicsMeshParams
{
    mat4 lights_pos_0;    /* positions of each of the maximum four lights */
    mat4 lights_params_0; /* ambient, diffuse, specular coefs for each light */
    vec4 view_pos;        /* camera position */
    vec4 tex_coefs;       /* blending coefficients for the four textures */
    vec4 clip_coefs;      /* clip coefficients */
};

static VklGraphicsMeshParams default_graphics_mesh_params(vec3 eye)
{
    VklGraphicsMeshParams params = {0};
    params.lights_params_0[0][0] = 0.2;  // ambient coefficient
    params.lights_params_0[0][1] = 0.5;  // diffuse coefficient
    params.lights_params_0[0][2] = 0.3;  // specular coefficient
    params.lights_params_0[0][3] = 32.0; // specular exponent
    params.lights_pos_0[0][0] = -1;      // light position
    params.lights_pos_0[0][1] = 1;       //
    params.lights_pos_0[0][2] = +10;     //
    params.tex_coefs[0] = 1;             // texture blending coefficients
    glm_vec3_copy(eye, params.view_pos); // camera position
    return params;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Set a graphics data callback.
 *
 * The callback function is called when one calls `vkl_graphics_append()` on that visual. It allows
 * one to easily add graphical elements, letting the graphics handle low-level GPU implementation
 * details (tesselation with vertices).
 *
 * Callback function signature: `void(VklGraphicsData*, uint32_t, const void*)`
 *
 * @param graphics the graphics pipeline
 * @param callback the callback function
 */
VKY_EXPORT void vkl_graphics_callback(VklGraphics* graphics, VklGraphicsCallback callback);

/**
 * Start a data collection for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param vertices pointer to an existing array containing vertices of the right type
 * @param indices pointer to an existing array containing the indices
 * @param user_data arbitrary user-provided pointer
 * @returns the graphics data object
 */
VKY_EXPORT VklGraphicsData
vkl_graphics_data(VklGraphics* graphics, VklArray* vertices, VklArray* indices, void* user_data);

/**
 * Allocate the graphics data object with the appropriate number of elements.
 *
 * @param data the graphics data object
 * @param item_count the number of graphical items
 */
VKY_EXPORT void vkl_graphics_alloc(VklGraphicsData* data, uint32_t item_count);

/**
 * Add one graphical element after the graphics data object has been properly allocated.
 *
 * @param data the graphics data object
 * @param item a pointer to an object of the appropriate graphics item type
 */
VKY_EXPORT void vkl_graphics_append(VklGraphicsData* data, const void* item);

/**
 * Create a new graphics pipeline of a given builtin type.
 *
 * @param canvas the canvas holding the grahpics pipeline
 * @param type the graphics type
 * @param flags the creation flags for the graphics
 */
VKY_EXPORT VklGraphics* vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsType type, int flags);



/**
 * Set up a 3D camera on a Model-View-Projection (MVP) object.
 *
 * @param viewport the viewport
 * @param eye the camera position in scene coordinates
 * @param center the position the camera points to
 * @param near_far the near and far values for the perspective matrix
 * @param[out] mvp a pointer to an MVP object
 */
VKY_EXPORT void
vkl_mvp_camera(VklViewport viewport, vec3 eye, vec3 center, vec2 near_far, VklMVP* mvp);


#endif
