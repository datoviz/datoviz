/*************************************************************************************************/
/*  Collection of builtin graphics pipelines                                                     */
/*************************************************************************************************/

#ifndef DVZ_GRAPHICS_HEADER
#define DVZ_GRAPHICS_HEADER

#include "array.h"
#include "canvas.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

// Number of common bindings
// NOTE: must correspond to the same constant in common.glsl
#define DVZ_USER_BINDING 2

#define DVZ_MAX_GLYPHS_PER_TEXT 256



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Marker type.
// NOTE: the numbers need to correspond to markers.glsl at the bottom.
typedef enum
{
    DVZ_MARKER_DISC = 0,
    DVZ_MARKER_ASTERISK = 1,
    DVZ_MARKER_CHEVRON = 2,
    DVZ_MARKER_CLOVER = 3,
    DVZ_MARKER_CLUB = 4,
    DVZ_MARKER_CROSS = 5,
    DVZ_MARKER_DIAMOND = 6,
    DVZ_MARKER_ARROW = 7,
    DVZ_MARKER_ELLIPSE = 8,
    DVZ_MARKER_HBAR = 9,
    DVZ_MARKER_HEART = 10,
    DVZ_MARKER_INFINITY = 11,
    DVZ_MARKER_PIN = 12,
    DVZ_MARKER_RING = 13,
    DVZ_MARKER_SPADE = 14,
    DVZ_MARKER_SQUARE = 15,
    DVZ_MARKER_TAG = 16,
    DVZ_MARKER_TRIANGLE = 17,
    DVZ_MARKER_VBAR = 18,
    DVZ_MARKER_COUNT,
} DvzMarkerType;



// Cap type.
typedef enum
{
    DVZ_CAP_TYPE_NONE = 0,
    DVZ_CAP_ROUND = 1,
    DVZ_CAP_TRIANGLE_IN = 2,
    DVZ_CAP_TRIANGLE_OUT = 3,
    DVZ_CAP_SQUARE = 4,
    DVZ_CAP_BUTT = 5,
    DVZ_CAP_COUNT,
} DvzCapType;



// Joint type.
typedef enum
{
    DVZ_JOIN_SQUARE = false,
    DVZ_JOIN_ROUND = true,
} DvzJoinType;



// Path topology.
typedef enum
{
    DVZ_PATH_OPEN,
    DVZ_PATH_CLOSED,
} DvzPathTopology;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzVertex DvzVertex;

typedef struct DvzGraphicsPointParams DvzGraphicsPointParams;

typedef struct DvzGraphicsMarkerVertex DvzGraphicsMarkerVertex;
typedef struct DvzGraphicsMarkerParams DvzGraphicsMarkerParams;

typedef struct DvzGraphicsSegmentVertex DvzGraphicsSegmentVertex;

typedef struct DvzGraphicsPathVertex DvzGraphicsPathVertex;
typedef struct DvzGraphicsPathParams DvzGraphicsPathParams;
// typedef struct DvzGraphicsPathItem DvzGraphicsPathItem;

typedef struct DvzGraphicsImageItem DvzGraphicsImageItem;
typedef struct DvzGraphicsImageVertex DvzGraphicsImageVertex;
typedef struct DvzGraphicsImageParams DvzGraphicsImageParams;
typedef struct DvzGraphicsImageCmapParams DvzGraphicsImageCmapParams;

typedef struct DvzGraphicsVolumeSliceItem DvzGraphicsVolumeSliceItem;
typedef struct DvzGraphicsVolumeSliceVertex DvzGraphicsVolumeSliceVertex;
typedef struct DvzGraphicsVolumeSliceParams DvzGraphicsVolumeSliceParams;

typedef struct DvzGraphicsVolumeItem DvzGraphicsVolumeItem;
typedef struct DvzGraphicsVolumeVertex DvzGraphicsVolumeVertex;
typedef struct DvzGraphicsVolumeParams DvzGraphicsVolumeParams;

typedef struct DvzGraphicsMeshVertex DvzGraphicsMeshVertex;
typedef struct DvzGraphicsMeshParams DvzGraphicsMeshParams;

typedef struct DvzGraphicsTextParams DvzGraphicsTextParams;
typedef struct DvzGraphicsTextVertex DvzGraphicsTextVertex;
typedef struct DvzGraphicsTextItem DvzGraphicsTextItem;

typedef struct DvzGraphicsData DvzGraphicsData;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
};



struct DvzGraphicsData
{
    DvzGraphics* graphics;
    DvzArray* vertices;
    DvzArray* indices;
    uint32_t item_count;
    uint32_t current_idx;
    uint32_t current_group;
    void* user_data;
};



/*************************************************************************************************/
/*  Graphics point                                                                               */
/*************************************************************************************************/

struct DvzGraphicsPointParams
{
    float point_size; /* point size, in pixels */
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

struct DvzGraphicsTextItem
{
    DvzGraphicsTextVertex vertex; /* text vertex */
    cvec4* glyph_colors;          /* glyph colors */
    float font_size;              /* font size */
    const char* string;           /* text string */
};

struct DvzGraphicsTextParams
{
    ivec2 grid_size; /* font atlas grid size (rows, columns) */
    ivec2 tex_size;  /* font atlas texture size, in pixels */
};



/*************************************************************************************************/
/*  Graphics image                                                                               */
/*************************************************************************************************/

struct DvzGraphicsImageItem
{
    vec3 pos0; /* top left corner */
    vec3 pos1; /* top right corner */
    vec3 pos2; /* bottom right corner */
    vec3 pos3; /* bottom left corner */

    vec2 uv0; /* tex coords of the top left corner */
    vec2 uv1; /* tex coords of the top right corner */
    vec2 uv2; /* tex coords of the bottom right corner */
    vec2 uv3; /* tex coords of the bottom left corner */
};

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

struct DvzGraphicsVolumeSliceItem
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

struct DvzGraphicsVolumeItem
{
    // top left front, bottom right back
    vec3 pos0; /* top left front position */
    vec3 pos1; /* bottom right back position */
};

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
    vec3 pos;      /* position */
    vec3 normal;   /* normal vector */
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

static DvzGraphicsMeshParams default_graphics_mesh_params(vec3 eye)
{
    DvzGraphicsMeshParams params = {0};
    params.lights_params_0[0][0] = 0.2;  // ambient coefficient
    params.lights_params_0[0][1] = 0.5;  // diffuse coefficient
    params.lights_params_0[0][2] = 0.3;  // specular coefficient
    params.lights_params_0[0][3] = 32.0; // specular exponent
    params.lights_pos_0[0][0] = -1;      // light position
    params.lights_pos_0[0][1] = 1;       //
    params.lights_pos_0[0][2] = +10;     //
    params.tex_coefs[0] = 1;             // texture blending coefficients
    return params;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Set a graphics data callback.
 *
 * The callback function is called when one calls `dvz_graphics_append()` on that visual. It allows
 * one to easily add graphical elements, letting the graphics handle low-level GPU implementation
 * details (tesselation with vertices).
 *
 * Callback function signature: `void(DvzGraphicsData*, uint32_t, const void*)`
 *
 * @param graphics the graphics pipeline
 * @param callback the callback function
 */
DVZ_EXPORT void dvz_graphics_callback(DvzGraphics* graphics, DvzGraphicsCallback callback);

/**
 * Start a data collection for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param vertices pointer to an existing array containing vertices of the right type
 * @param indices pointer to an existing array containing the indices
 * @param user_data arbitrary user-provided pointer
 * @returns the graphics data object
 */
DVZ_EXPORT DvzGraphicsData
dvz_graphics_data(DvzGraphics* graphics, DvzArray* vertices, DvzArray* indices, void* user_data);

/**
 * Allocate the graphics data object with the appropriate number of elements.
 *
 * @param data the graphics data object
 * @param item_count the number of graphical items
 */
DVZ_EXPORT void dvz_graphics_alloc(DvzGraphicsData* data, uint32_t item_count);

/**
 * Add one graphical element after the graphics data object has been properly allocated.
 *
 * @param data the graphics data object
 * @param item a pointer to an object of the appropriate graphics item type
 */
DVZ_EXPORT void dvz_graphics_append(DvzGraphicsData* data, const void* item);

/**
 * Create a new graphics pipeline of a given builtin type.
 *
 * @param canvas the canvas holding the grahpics pipeline
 * @param type the graphics type
 * @param flags the creation flags for the graphics
 */
DVZ_EXPORT DvzGraphics* dvz_graphics_builtin(DvzCanvas* canvas, DvzGraphicsType type, int flags);



/**
 * Set up a 3D camera on a Model-View-Projection (MVP) object.
 *
 * @param viewport the viewport
 * @param eye the camera position in scene coordinates
 * @param center the position the camera points to
 * @param near_far the near and far values for the perspective matrix
 * @param[out] mvp a pointer to an MVP object
 */
DVZ_EXPORT void
dvz_mvp_camera(DvzViewport viewport, vec3 eye, vec3 center, vec2 near_far, DvzMVP* mvp);


#endif
