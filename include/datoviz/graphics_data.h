/*************************************************************************************************/
/*  Data graphics creation helpers                                                               */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GRAPHICS_DATA
#define DVZ_HEADER_GRAPHICS_DATA



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "array.h"
#include "graphics.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGraphicsImageItem DvzGraphicsImageItem;
typedef struct DvzGraphicsVolumeSliceItem DvzGraphicsVolumeSliceItem;
typedef struct DvzGraphicsVolumeItem DvzGraphicsVolumeItem;
typedef struct DvzGraphicsTextItem DvzGraphicsTextItem;
typedef struct DvzGraphicsData DvzGraphicsData;

typedef void (*DvzGraphicsCallback)(DvzGraphicsData* data, uint32_t item_count, const void* item);

// Forward declarations.
typedef struct DvzGraphics DvzGraphics;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/


struct DvzGraphicsData
{
    DvzGraphicsCallback callback;
    DvzArray* vertices;
    DvzArray* indices;
    uint32_t item_count;
    uint32_t current_idx;
    uint32_t current_group;
    void* user_data;
};



struct DvzGraphicsTextItem
{
    DvzGraphicsTextVertex vertex; /* text vertex */

    // vec3 pos;          /* position */
    // vec2 shift;        /* shift, in pixels */
    // cvec4 color;       /* color */
    // vec2 glyph_size;   /* glyph size, in pixels */
    // vec2 anchor;       /* character anchor, in normalized coordinates */
    // float angle;       /* string angle */
    // usvec4 glyph;      /* glyph: char code, char index, string length, string index */
    // uint8_t transform; /* transform enum */

    cvec4* glyph_colors; /* glyph colors */
    float font_size;     /* font size */
    const char* string;  /* text string */
};



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



struct DvzGraphicsVolumeItem
{
    // top left front, bottom right back
    vec3 pos0; /* top left front position */
    vec3 pos1; /* bottom right back position */
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Start a data collection for a graphics pipeline.
 *
 * The callback function is called when one calls `dvz_graphics_append()` on that visual. It allows
 * one to easily add graphical elements, letting the graphics handle low-level GPU implementation
 * details (tesselation with vertices).
 *
 * Callback function signature: `void(DvzGraphicsData*, uint32_t, const void*)`
 *
 * @param callback the graphics data callback function
 * @param vertices pointer to an existing array containing vertices of the right type
 * @param indices pointer to an existing array containing the indices
 * @param user_data arbitrary user-provided pointer
 * @returns the graphics data object
 */
DVZ_EXPORT DvzGraphicsData dvz_graphics_data(
    DvzGraphicsCallback callback, DvzArray* vertices, DvzArray* indices, void* user_data);

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



EXTERN_C_OFF

#endif
