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
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAtlas DvzAtlas;
typedef struct DvzAtlasFont DvzAtlasFont;

// Forward declarations.
typedef struct DvzFont DvzFont;
typedef struct DvzBatch DvzBatch;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 */
DvzAtlas* dvz_atlas(unsigned long ttf_size, unsigned char* ttf_bytes);



/**
 */
void dvz_atlas_clear(DvzAtlas* atlas);



/**
 */
void dvz_atlas_codepoints(DvzAtlas* atlas, uint32_t count, uint32_t* codepoints);



/**
 */
void dvz_atlas_string(DvzAtlas* atlas, const char* string);



/**
 */
int dvz_atlas_glyph(DvzAtlas* atlas, uint32_t codepoint, vec4 out_coords);



/**
 */
vec4* dvz_atlas_glyphs(DvzAtlas* atlas, uint32_t count, uint32_t* codepoints);



/**
 */
void dvz_atlas_load(DvzAtlas* atlas);



/**
 */
int dvz_atlas_generate(DvzAtlas* atlas);


/**
 */
void dvz_atlas_shape(DvzAtlas* atlas, uvec3 shape);



/**
 */
bool dvz_atlas_valid(DvzAtlas* atlas);



/**
 */
uint8_t* dvz_atlas_rgb(DvzAtlas* atlas);



/**
 */
DvzSize dvz_atlas_size(DvzAtlas* atlas);



/**
 */
void dvz_atlas_png(DvzAtlas* atlas, const char* png_filename);



/**
 */
DvzId dvz_atlas_texture(DvzAtlas* atlas, DvzBatch* batch);



/**
 */
void dvz_atlas_destroy(DvzAtlas* atlas);



/*************************************************************************************************/
/*  File util functions                                                                          */
/*************************************************************************************************/

/**
 */
DvzAtlasFont dvz_atlas_export(const char* font_name, const char* output_file);



/**
 */
DVZ_EXPORT
DvzAtlasFont dvz_atlas_import(const char* font_name, const char* atlas_name);



EXTERN_C_OFF

#endif
