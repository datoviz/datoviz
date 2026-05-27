/*************************************************************************************************/
/*  stb_image implementation                                                                     */
/*************************************************************************************************/

#include "_alloc.h"

#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MALLOC(size) dvz_malloc((DvzSize)(size))
#define STBI_REALLOC(pointer, size) dvz_realloc((pointer), (DvzSize)(size))
#define STBI_REALLOC_SIZED(pointer, old_size, new_size)                                           \
    dvz_realloc((pointer), (DvzSize)(new_size))
#define STBI_FREE(pointer) dvz_free((pointer))
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
