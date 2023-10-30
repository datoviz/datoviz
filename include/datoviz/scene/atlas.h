/*************************************************************************************************/
/* Atlas                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ATLAS
#define DVZ_HEADER_ATLAS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "_map.h"
#include "_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAtlas DvzAtlas;

// Forward declarations.
typedef struct DvzBatch DvzBatch;



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
 */
DVZ_EXPORT DvzAtlas* dvz_atlas(unsigned long ttf_size, unsigned char* ttf_bytes);



/**
 */
DVZ_EXPORT void dvz_atlas_clear(DvzAtlas* atlas);



/**
 */
DVZ_EXPORT void dvz_atlas_codepoints(DvzAtlas* atlas, uint32_t count, uint32_t* codepoints);



/**
 */
DVZ_EXPORT void dvz_atlas_string(DvzAtlas* atlas, const char* string);



/**
 */
DVZ_EXPORT int dvz_atlas_glyph(DvzAtlas* atlas, uint32_t codepoint, vec4 out_coords);



/**
 */
DVZ_EXPORT vec4* dvz_atlas_glyphs(DvzAtlas* atlas, uint32_t count, uint32_t* codepoints);



/**
 */
DVZ_EXPORT int dvz_atlas_generate(DvzAtlas* atlas);


/**
 */
DVZ_EXPORT void dvz_atlas_shape(DvzAtlas* atlas, uvec3 shape);



/**
 */
DVZ_EXPORT bool dvz_atlas_valid(DvzAtlas* atlas);



/**
 */
DVZ_EXPORT void dvz_atlas_png(DvzAtlas* atlas, const char* png_filename);



/**
 */
DVZ_EXPORT DvzId dvz_atlas_texture(DvzAtlas* atlas, DvzBatch* batch);



/**
 */
DVZ_EXPORT void dvz_atlas_destroy(DvzAtlas* atlas);



EXTERN_C_OFF

#endif
