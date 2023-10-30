/*************************************************************************************************/
/* Glyph                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GLYPH
#define DVZ_HEADER_GLYPH



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../graphics.h"
#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGlyphVertex DvzGlyphVertex;
typedef struct DvzGlyphParams DvzGlyphParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_glyph(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_normal(DvzVisual* visual, uint32_t first, uint32_t count, vec3* normals, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_glyph_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec2* coords, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_glyph_tex(DvzVisual* visual, DvzId tex);



/**
 *
 */
DVZ_EXPORT void dvz_glyph_size(DvzVisual* visual, vec2 size);



/**
 *
 */
DVZ_EXPORT void dvz_glyph_alloc(DvzVisual* visual, uint32_t item_count);



EXTERN_C_OFF

#endif
